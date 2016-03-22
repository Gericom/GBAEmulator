#include "stdafx.h"
#include "IORegisters.h"
#include "ARM7TDMI.h"

#define ADD_VFLAG(a,b,r)	((((a) >= 0 && (b) >= 0) || ((a) < 0 && (b) < 0)) && (((a) < 0 && (r) >= 0) || ((a) >= 0 && (r) < 0)))
#define SUB_VFLAG(a,b,r)	((((a) < 0 && (b) >= 0) || ((a) >= 0 && (b) < 0)) && (((a) < 0 && (r) >= 0) || ((a) >= 0 && (r) < 0)))

#define ADD_CFLAG(a,b)		((b) > (0xFFFFFFFFU - (a)))
#define SUB_CFLAG(a,b)		(!((b) > (a)))

static uint8_t count_ones_8(uint8_t byte)
{
	static const uint8_t NIBBLE_LOOKUP[16] =
	{
		0, 1, 1, 2, 1, 2, 2, 3,
		1, 2, 2, 3, 2, 3, 3, 4
	};


	return NIBBLE_LOOKUP[byte & 0x0F] + NIBBLE_LOOKUP[byte >> 4];
}

static uint8_t count_ones_16(uint16_t byte)
{
	static const uint8_t NIBBLE_LOOKUP[16] =
	{
		0, 1, 1, 2, 1, 2, 2, 3,
		1, 2, 2, 3, 2, 3, 3, 4
	};


	return NIBBLE_LOOKUP[byte & 0x0F] + NIBBLE_LOOKUP[(byte >> 4) & 0x0F] + NIBBLE_LOOKUP[(byte >> 8) & 0x0F] + NIBBLE_LOOKUP[(byte >> 12) & 0x0F];
}

bool ARM7TDMI::EvaluateCondition(uint32_t condition)
{
	switch (condition)
	{
	case 0x0: return mGlobalState.z_flag == 1;
	case 0x1: return mGlobalState.z_flag == 0;
	case 0x2: return mGlobalState.c_flag == 1;
	case 0x3: return mGlobalState.c_flag == 0;
	case 0x4: return mGlobalState.n_flag == 1;
	case 0x5: return mGlobalState.n_flag == 0;
	case 0x6: return mGlobalState.v_flag == 1;
	case 0x7: return mGlobalState.v_flag == 0;
	case 0x8: return mGlobalState.c_flag == 1 && mGlobalState.z_flag == 0;
	case 0x9: return mGlobalState.c_flag == 0 || mGlobalState.z_flag == 1;
	case 0xA: return mGlobalState.n_flag == mGlobalState.v_flag;
	case 0xB: return mGlobalState.n_flag != mGlobalState.v_flag;
	case 0xC: return mGlobalState.z_flag == 0 && mGlobalState.n_flag == mGlobalState.v_flag;
	case 0xD: return mGlobalState.z_flag == 1 || mGlobalState.n_flag != mGlobalState.v_flag;
	case 0xE: return true;
	case 0xF: return false;
	}
	//shouldn't happen!
	return false;
}

uint32_t ARM7TDMI::Shift(uint32_t ShiftType, uint32_t Value, uint32_t NrBits, bool &Carry)
{
	switch (ShiftType)
	{
	case 0:
		if (NrBits > 0) Carry = ((Value >> (32 - (int)NrBits)) & 1) == 1;
		return Value << (int)NrBits;
	case 1:
		if (NrBits > 0) Carry = ((Value >> ((int)NrBits - 1)) & 1) == 1;
		else Carry = ((Value >> 31) & 1) == 1;
		return Value >> (int)NrBits;
	case 2:
		if (NrBits > 0)
		{
			Carry = ((Value >> ((int)NrBits - 1)) & 1) == 1;
			return (uint32_t)(((int)Value) >> (int)NrBits);
		}
		else
		{
			Carry = ((Value >> 31) & 1) == 1;
			return ((Value >> 31) & 1) * 0xFFFFFFFF;
		}
	case 3:
		if (NrBits > 0)
		{
			Carry = ((Value >> ((int)NrBits - 1)) & 1) == 1;
			return (Value >> (int)NrBits) | (Value << (32 - (int)NrBits));
		}
		else
		{
			uint32_t tmp = ((Carry ? 1u : 0u) << 31) | (Value >> 1);
			Carry = (Value & 1) == 1;
			return tmp;
		}
	}
	return 0xFFFFFFFF;
}



void ARM7TDMI::RunCycle()
{
	//The pipeline consists out of 3 states: fetch, decode and execute
	if (mFetchingState.waitingForData && mMemoryBus->IsDataReady())
	{
		mFetchingState.waitingForData = false;
		uint32_t instruction = mMemoryBus->GetReadResult();
		if (!mFetchingState.cancelData)
		{
			mDecodingState.instruction = instruction;
			mDecodingState.instructionPC = mFetchingState.instructionPC;
			mDecodingState.done = false;
		}
		mFetchingState.cancelData = false;
	}

	if (mIORegisters->mMemory[0x301] == 0 && !(*((uint16_t*)&mIORegisters->mMemory[0x200]) & *((uint16_t*)&mIORegisters->mMemory[0x202])))
		return;
	else if (mIORegisters->mMemory[0x301] == 0)
	{
		if (((mGlobalState.registers[15] & ~1) == (mExecutionState.instructionPC & ~1)
			|| (mGlobalState.registers[15] & ~1) - 2 == (mExecutionState.instructionPC & ~1)
			|| (mGlobalState.registers[15] & ~1) - 4 == (mExecutionState.instructionPC & ~1)
			|| (mGlobalState.registers[15] & ~1) - 6 == (mExecutionState.instructionPC & ~1)
			|| (mGlobalState.registers[15] & ~1) - 8 == (mExecutionState.instructionPC & ~1)) &&
			((mGlobalState.registers[15] & ~1) == (mDecodingState.instructionPC & ~1)
				|| (mGlobalState.registers[15] & ~1) - 2 == (mDecodingState.instructionPC & ~1)
				|| (mGlobalState.registers[15] & ~1) - 4 == (mDecodingState.instructionPC & ~1)))
			mGlobalState.registers_irq[1] = (mExecutionState.instructionPC & ~1) + (mGlobalState.thumb ? 2 + 4 : 4 + 4);
		else
			mGlobalState.registers_irq[1] = (mGlobalState.registers[15] & ~1) + 4;
		mGlobalState.spsr_irq = mGlobalState.cspr;
		mGlobalState.irq_disable = true;
		mGlobalState.thumb = false;
		mGlobalState.mode = ARM7TDMI_CSPR_MODE_IRQ;
		mGlobalState.registers[15] = 0x00000018;
		FlushPipeline();
	}

	mIORegisters->mMemory[0x301] = 0xFF;

	//execute
	if (mExecutionState.instructionProc)
	{
		if (mExecutionState.instructionPC == 0x1238)
		{
			wprintf(L"LOL!");
		}
		if ((this->*mExecutionState.instructionProc)())//returns true if the instruction is fully executed
			mExecutionState.instructionProc = NULL;
	}

	if (!mExecutionState.instructionProc && !mGlobalState.irq_disable &&
		(*((uint16_t*)&mIORegisters->mMemory[0x200]) & *((uint16_t*)&mIORegisters->mMemory[0x202])) &&
		*((uint16_t*)&mIORegisters->mMemory[0x208]))
	{
		//irqPending = false;
		if (((mGlobalState.registers[15] & ~1) == (mExecutionState.instructionPC & ~1)
			|| (mGlobalState.registers[15] & ~1) - 2 == (mExecutionState.instructionPC & ~1)
			|| (mGlobalState.registers[15] & ~1) - 4 == (mExecutionState.instructionPC & ~1)
			|| (mGlobalState.registers[15] & ~1) - 6 == (mExecutionState.instructionPC & ~1)
			|| (mGlobalState.registers[15] & ~1) - 8 == (mExecutionState.instructionPC & ~1)) &&
			((mGlobalState.registers[15] & ~1) == (mDecodingState.instructionPC & ~1)
				|| (mGlobalState.registers[15] & ~1) - 2 == (mDecodingState.instructionPC & ~1)
				|| (mGlobalState.registers[15] & ~1) - 4 == (mDecodingState.instructionPC & ~1)))
			mGlobalState.registers_irq[1] = (mExecutionState.instructionPC & ~1) + (mGlobalState.thumb ? 2 + 4 : 4 + 4);
		else
			mGlobalState.registers_irq[1] = (mGlobalState.registers[15] & ~1) + 4;
		mGlobalState.spsr_irq = mGlobalState.cspr;
		mGlobalState.irq_disable = true;
		mGlobalState.thumb = false;
		mGlobalState.mode = ARM7TDMI_CSPR_MODE_IRQ;
		mGlobalState.registers[15] = 0x00000018;
		FlushPipeline();
	}

	//fetch
	if ((mDecodingState.instructionPC == mExecutionState.instructionPC || (mDecodingState.done && mDecodingState.instructionPC != mExecutionState.instructionPC))
		&& !mMemoryBus->IsBusy())//We can't fetch if the memory bus is busy
	{
		if (!mGlobalState.thumb)
		{
			mFetchingState.instructionPC = mGlobalState.registers[15];
			mMemoryBus->Read32(mGlobalState.registers[15]);
			mGlobalState.registers[15] += 4;
			mFetchingState.waitingForData = true;
		}
		else
		{
			mFetchingState.instructionPC = mGlobalState.registers[15];
			mMemoryBus->Read16(mGlobalState.registers[15] & ~1);
			mGlobalState.registers[15] += 2;
			mFetchingState.waitingForData = true;
		}
	}
	//decode
	if (!mExecutionState.instructionProc && !mDecodingState.done)
	{
		mExecutionState.instruction = mDecodingState.instruction;
		mExecutionState.instructionPC = mDecodingState.instructionPC;
		mExecutionState.instructionArgs[0] = 0;
		mExecutionState.instructionArgs[1] = 0;
		mExecutionState.instructionArgs[2] = 0;
		mExecutionState.instructionArgs[3] = 0;
		mExecutionState.instructionState = 0;
		if (!mGlobalState.thumb)
		{
			if (EvaluateCondition(mDecodingState.instruction >> 28))
			{
				switch ((mDecodingState.instruction >> 25) & 7)
				{
				case 0:
				case 1:
					if (((mDecodingState.instruction >> 25) & 7) == 0 && ((mDecodingState.instruction >> 7) & 1) == 1 && ((mDecodingState.instruction >> 4) & 1) == 1 && ((mDecodingState.instruction >> 5) & 3) != 0)
						mExecutionState.instructionProc = &ARM7TDMI::Instruction_HDSDataTrans;
					else if (((mDecodingState.instruction >> 25) & 7) == 0 && ((mDecodingState.instruction >> 4) & 0xF) == 9)
						mExecutionState.instructionProc = &ARM7TDMI::Instruction_Multiply;
					else mExecutionState.instructionProc = &ARM7TDMI::Instruction_DataProc;
					break;
				case 0x2:
				case 0x3:
					mExecutionState.instructionProc = &ARM7TDMI::Instruction_SingleDataTrans;
					break;
				case 0x4:
					mExecutionState.instructionProc = &ARM7TDMI::Instruction_BlockDataTrans;
					break;
				case 0x5://101
					mExecutionState.instructionProc = &ARM7TDMI::Instruction_Branch;
					break;
				case 0x7:
					if (((mDecodingState.instruction >> 24) & 0xF) == 0xF)
						mExecutionState.instructionProc = &ARM7TDMI::Instruction_SWI;
					else
						wprintf(L"Unknown instruction!");
					break;//ignore mcr, mrc and that kind of crap
				default:
					wprintf(L"Unknown instruction!");
					break;
				}

			}
			else mExecutionState.instructionProc = &ARM7TDMI::Instruction_Nop;
		}
		else
		{
			if ((mDecodingState.instruction >> 11) <= 2)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_1;
			else if ((mDecodingState.instruction >> 11) == 3)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_2;
			else if ((mDecodingState.instruction >> 11) <= 7)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_3;
			else if ((mDecodingState.instruction >> 10) == 16)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_4;
			else if ((mDecodingState.instruction >> 10) == 17)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_5;
			else if ((mDecodingState.instruction >> 11) == 9)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_6;
			else if ((mDecodingState.instruction >> 12) == 5 && ((mDecodingState.instruction >> 9) & 1) == 0)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_7;
			else if ((mDecodingState.instruction >> 12) == 5 && ((mDecodingState.instruction >> 9) & 1) == 1)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_8;
			else if ((mDecodingState.instruction >> 13) == 3)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_9;
			else if ((mDecodingState.instruction >> 12) == 8)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_10;
			else if ((mDecodingState.instruction >> 12) == 9)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_11;
			else if ((mDecodingState.instruction >> 12) == 10)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_12;
			else if ((mDecodingState.instruction >> 8) == 0xB0)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_13;
			else if ((mDecodingState.instruction >> 12) == 11 && ((mDecodingState.instruction >> 9) & 3) == 2)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_14;
			else if ((mDecodingState.instruction >> 12) == 12)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_15;
			else if ((mDecodingState.instruction >> 12) == 13 && ((mDecodingState.instruction >> 8) & 0xF) <= 0xD)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_16;
			else if ((mDecodingState.instruction >> 8) == 0xDF)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_17;
			else if ((mDecodingState.instruction >> 11) == 28)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_18;
			else if ((mDecodingState.instruction >> 11) == 30)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_19A;
			else if ((mDecodingState.instruction >> 11) == 31)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_19B;
			else
			{
				OutputDebugString(L"Unknown instruction!");
			}
		}
		mDecodingState.done = true;
	}
}

void ARM7TDMI::FlushPipeline()
{
	if (mFetchingState.waitingForData)
		mFetchingState.cancelData = true;
	mDecodingState.done = true;
}

bool ARM7TDMI::Instruction_Nop()
{
	return true;
}

bool ARM7TDMI::Instruction_DataProc()
{
	if (mExecutionState.instructionState == 0)
	{
		bool additionalCycle = false;
		uint32_t result = 0;
		uint32_t Rn;
		if ((mExecutionState.instruction & 0x0FFFFF00) == 0x12FFF00)//bx, blx
		{
			uint32_t op = (mExecutionState.instruction >> 4) & 0xF;
			if ((mExecutionState.instruction & 0xF) == 15)
				Rn = mExecutionState.instructionPC + 8;
			else Rn = *GetRegisterById(mExecutionState.instruction & 0xF);
			if (op == 1)
			{
				uint32_t dst = Rn;
				mGlobalState.thumb = dst & 1;
				mGlobalState.registers[15] = dst;
			}
			else if (op == 3)
			{
				*GetRegisterById(14) = mExecutionState.instructionPC + 4;
				uint32_t dst = Rn;
				mGlobalState.thumb = dst & 1;
				mGlobalState.registers[15] = dst;
			}
			else
			{
				//shouldn't happen!
			}
			FlushPipeline();
			return true;
		}
		bool Shift_C = mGlobalState.c_flag;

		bool I = ((mExecutionState.instruction >> 25) & 1) == 1;
		uint32_t Opcode = (mExecutionState.instruction >> 21) & 0xF;
		bool S = ((mExecutionState.instruction >> 20) & 1) == 1;
		if (((mExecutionState.instruction >> 16) & 0xF) == 15)
			Rn = mExecutionState.instructionPC + 8;
		else Rn = *GetRegisterById((mExecutionState.instruction >> 16) & 0xF);
		uint32_t* Rd = GetRegisterById((mExecutionState.instruction >> 12) & 0xF);
		uint32_t Op2 = 0;
		if (I)
		{
			uint32_t Is = (mExecutionState.instruction >> 8) & 0xF;
			uint32_t nn = mExecutionState.instruction & 0xFF;

			Op2 = (nn >> (int)(Is * 2)) | (nn << (32 - (int)(Is * 2)));
		}
		else
		{
			uint32_t ShiftType = (mExecutionState.instruction >> 5) & 0x3;
			bool R = ((mExecutionState.instruction >> 4) & 1) == 1;
			uint32_t* Rm = GetRegisterById(mExecutionState.instruction & 0xF);

			if (!R)
			{
				uint32_t Is = (mExecutionState.instruction >> 7) & 0x1F;
				Op2 = Shift(ShiftType, *Rm, Is, Shift_C);
			}
			else
			{
				uint32_t* Rs = GetRegisterById((mExecutionState.instruction >> 8) & 0xF);
				uint32_t Reserved = (mExecutionState.instruction >> 7) & 1;
				if((*Rs & 0xFF) != 0)
					Op2 = Shift(ShiftType, *Rm, *Rs & 0xFF, Shift_C);
				else Op2 = *Rm;
				additionalCycle = true;
			}
		}

		switch (Opcode)
		{
		case 0x0://and
			result = *Rd = Rn & Op2;
			break;
		case 0x1://xor
			result = *Rd = Rn ^ Op2;
			break;
		case 0x6://sbc
			Rn += mGlobalState.c_flag - 1;
		case 0x2://sub
			result = *Rd = Rn - Op2;
			break;
		case 0x7://rsc
			Op2 += mGlobalState.c_flag - 1;
		case 0x3://rsb
			result = *Rd = Op2 - Rn;
			break;
		case 0x5://adc
			Op2 += mGlobalState.c_flag;
		case 0x4://add
			result = *Rd = Rn + Op2;
			break;
		case 0x8:
			if (S == false)//MRS
			{
				int src = (mExecutionState.instruction >> 22) & 1;
				if (src == 0) *Rd = mGlobalState.cspr;
				else
				{
					if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_FIQ)
						*Rd = mGlobalState.spsr_fiq;
					else if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_IRQ)
						*Rd = mGlobalState.spsr_irq;
					else if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_SVC)
						*Rd = mGlobalState.spsr_svc;
					else if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_ABT)
						*Rd = mGlobalState.spsr_abt;
					else if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_UND)
						*Rd = mGlobalState.spsr_und;
					else
					{
						//not possible!
					}
				}
				break;
			}
			else//TST
			{
				result = Rn & Op2;
			}
			break;
		case 0x9:
			if (S == false)//MSR
			{
				int f = (mExecutionState.instruction >> 19) & 1;
				int s = (mExecutionState.instruction >> 18) & 1;
				int x = (mExecutionState.instruction >> 17) & 1;
				int c = (mExecutionState.instruction >> 16) & 1;
				uint32_t mask = ((0xFF * f) << 24) | ((0xFF * s) << 16) | ((0xFF * x) << 8) | ((0xFF * c) << 0);
				int dst = (mExecutionState.instruction >> 22) & 1;
				if (dst == 0) mGlobalState.cspr = (mGlobalState.cspr & ~mask) | (Op2 & mask);
				else
				{
					if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_FIQ)
						mGlobalState.spsr_fiq = (mGlobalState.spsr_fiq & mask) | (Op2 & mask);
					else if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_IRQ)
						mGlobalState.spsr_irq = (mGlobalState.spsr_irq & mask) | (Op2 & mask);
					else if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_SVC)
						mGlobalState.spsr_svc = (mGlobalState.spsr_svc & mask) | (Op2 & mask);
					else if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_ABT)
						mGlobalState.spsr_abt = (mGlobalState.spsr_abt & mask) | (Op2 & mask);
					else if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_UND)
						mGlobalState.spsr_und = (mGlobalState.spsr_und & mask) | (Op2 & mask);
					else
					{
						//not possible!
					}
				}
				break;
			}
			else//TEQ
			{
				result = Rn ^ Op2;
			}
			break;
		case 0xA://compare
			result = Rn - Op2;
			break;
		case 0xB://compare negative
			result = Rn + Op2;
			break;
		case 0xC://OR logical 
			result = *Rd = Rn | Op2;
			break;
		case 0xD://move
			result = *Rd = Op2;
			break;
		case 0xE://bit clear 
			result = *Rd = Rn & ~Op2;
			break;
		case 0xF://move not
			result = *Rd = ~Op2;
			break;
		default:
			break;
		}

		if (S)
		{
			bool V = mGlobalState.v_flag;//tmp
			bool C = mGlobalState.c_flag;//tmp
			bool Z = result == 0;
			bool N = (result >> 31) == 1;
			if (Rd != &mGlobalState.registers[15])//15)
			{
				switch (Opcode)
				{
				case 0:
				case 1:
				case 8:
				case 9:
				case 12:
				case 13:
				case 14:
				case 15:
					C = Shift_C;
					break;
				case 4:
				case 5:
				case 6:
				case 11:
					C = ADD_CFLAG(Rn, Op2);
					V = ADD_VFLAG(Rn, Op2, result);
					break;
				case 0x2:
				case 7:
				case 0xA:
					C = SUB_CFLAG(Rn, Op2);
					V = SUB_VFLAG(Rn, Op2, result);
					break;
				case 0x3:
					C = SUB_CFLAG(Op2, Rn);
					V = SUB_VFLAG(Op2, Rn, result);
					break;
				}
				mGlobalState.c_flag = C;
				mGlobalState.n_flag = N;
				mGlobalState.v_flag = V;
				mGlobalState.z_flag = Z;
			}
			else
			{
				//Used for returning from a svc for example
				if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_FIQ)
					mGlobalState.cspr = mGlobalState.spsr_fiq;
				else if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_IRQ)
					mGlobalState.cspr = mGlobalState.spsr_irq;
				else if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_SVC)
					mGlobalState.cspr = mGlobalState.spsr_svc;
				else if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_ABT)
					mGlobalState.cspr = mGlobalState.spsr_abt;
				else if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_UND)
					mGlobalState.cspr = mGlobalState.spsr_und;
			}
		}
		if (Rd == &mGlobalState.registers[15])
			FlushPipeline();
		if (additionalCycle)
		{
			mExecutionState.instructionState++;
			return false;
		}
	}
	return true;
}

bool ARM7TDMI::Instruction_Multiply()
{
	if (mExecutionState.instructionState == 0)
	{
		int Opcode = (mExecutionState.instruction >> 21) & 15;
		bool S = (mExecutionState.instruction >> 20) & 1;
		uint32_t* Rd = GetRegisterById((mExecutionState.instruction >> 16) & 0xF);
		uint32_t* Rn = GetRegisterById((mExecutionState.instruction >> 12) & 0xF);
		uint32_t* Rs = GetRegisterById((mExecutionState.instruction >> 8) & 0xF);
		uint32_t* Rm = GetRegisterById(mExecutionState.instruction & 0xF);
		uint32_t result;
		uint64_t result_long;
		switch (Opcode)
		{
		case 0://mul
			result = *Rd = *Rm * *Rs;
			mExecutionState.instructionArgs[0] = 2;
			break;
		case 1://mla
			result = *Rd = *Rm * *Rs + *Rn;
			mExecutionState.instructionArgs[0] = 3;
			break;
		case 4://umull
			result_long = ((uint64_t)*Rm) *  ((uint64_t)*Rs);
			*Rd = result_long >> 32;
			*Rn = result_long & 0xFFFFFFFF;
			mExecutionState.instructionArgs[0] = 3;
			break;
		case 5://umlal
			result_long = ((uint64_t)*Rm) * ((uint64_t)*Rs) + ((((uint64_t)*Rd) << 32) | ((uint64_t)*Rn));
			*Rd = result_long >> 32;
			*Rn = result_long & 0xFFFFFFFF;
			mExecutionState.instructionArgs[0] = 3;
			break;
		case 6://smull
			result_long = ((int64_t)*Rm) *  ((int64_t)*Rs);
			*Rd = result_long >> 32;
			*Rn = result_long & 0xFFFFFFFF;
			mExecutionState.instructionArgs[0] = 3;
			break;
		case 7://smlal
			result_long = ((int64_t)*Rm) * ((int64_t)*Rs) + ((((int64_t)*Rd) << 32) | ((int64_t)*Rn));
			*Rd = result_long >> 32;
			*Rn = result_long & 0xFFFFFFFF;
			mExecutionState.instructionArgs[0] = 3;
			break;
		default:
			wprintf(L"Unknown opcode!");
			break;
		}
		if (S && Opcode <= 1)
		{
			mGlobalState.n_flag = (result >> 31) == 1;
			mGlobalState.z_flag = result == 0;
			mGlobalState.c_flag = 0;
		}
		else if (S)
		{
			mGlobalState.n_flag = (result_long >> 63) == 1;
			mGlobalState.z_flag = result_long == 0;
			mGlobalState.c_flag = 0;
			mGlobalState.v_flag = 0;
		}
		mExecutionState.instructionState++;
		return false;
	}
	else if (mExecutionState.instructionState < mExecutionState.instructionArgs[0])
	{
		mExecutionState.instructionState++;
		return false;
	}
	return true;
}

bool ARM7TDMI::Instruction_SingleDataTrans()
{
	if (mExecutionState.instructionState == 0)
	{
		bool I = (mExecutionState.instruction >> 25) & 1;
		bool P = (mExecutionState.instruction >> 24) & 1;
		bool U = (mExecutionState.instruction >> 23) & 1;
		bool B = (mExecutionState.instruction >> 22) & 1;

		bool T = false;
		bool W = false;

		if (P) W = (mExecutionState.instruction >> 21) & 1;
		else T = (mExecutionState.instruction >> 21) & 1;

		bool L = (mExecutionState.instruction >> 20) & 1;

		uint32_t* Rn = GetRegisterById((mExecutionState.instruction >> 16) & 0xF);
		uint32_t* Rd = GetRegisterById((mExecutionState.instruction >> 12) & 0xF);

		uint32_t Offset;
		if (I)
		{
			uint32_t Is = (mExecutionState.instruction >> 7) & 0x1F;
			uint32_t ShiftType = (mExecutionState.instruction >> 5) & 0x3;
			uint32_t Reserved = (mExecutionState.instruction >> 4) & 1;
			uint32_t* Rm = GetRegisterById(mExecutionState.instruction & 0xF);
			bool Shift_C = mGlobalState.c_flag;
			Offset = Shift(ShiftType, *Rm, Is, Shift_C);
		}
		else
		{
			Offset = mExecutionState.instruction & 0xFFF;
		}

		uint32_t MemoryOffset = *Rn;
		if (Rn == &mGlobalState.registers[15]) MemoryOffset = mExecutionState.instructionPC + 8;
		if (P)
		{
			if (U) MemoryOffset += Offset;
			else MemoryOffset -= Offset;
			if (W) *Rn = MemoryOffset;
		}
		mExecutionState.instructionArgs[0] = MemoryOffset;
		mExecutionState.instructionArgs[1] = (uint32_t)Rd;
		mExecutionState.instructionArgs[2] = (uint32_t)Rn;
		mExecutionState.instructionArgs[3] = Offset;
		mExecutionState.instructionState++;
		return false;
	}
	else if (mExecutionState.instructionState == 1)
	{
		if (!mMemoryBus->IsBusy())
		{
			uint32_t MemoryOffset = mExecutionState.instructionArgs[0];
			uint32_t* Rd = (uint32_t*)mExecutionState.instructionArgs[1];
			uint32_t* Rn = (uint32_t*)mExecutionState.instructionArgs[2];
			uint32_t Offset = mExecutionState.instructionArgs[3];
			bool P = ((mExecutionState.instruction >> 24) & 1) == 1;
			bool U = ((mExecutionState.instruction >> 23) & 1) == 1;
			bool B = ((mExecutionState.instruction >> 22) & 1) == 1;
			bool L = ((mExecutionState.instruction >> 20) & 1) == 1;

			if (L)
			{
				if (B) mMemoryBus->Read8(MemoryOffset);
				else mMemoryBus->Read32(MemoryOffset);
			}
			else
			{
				if (Rd == &mGlobalState.registers[15])
				{
					if (B) mMemoryBus->Write8(MemoryOffset, (mExecutionState.instructionPC + 12) & 0xFF);
					else mMemoryBus->Write32(MemoryOffset, mExecutionState.instructionPC + 12);
				}
				else
				{
					if (B)  mMemoryBus->Write8(MemoryOffset, *Rd & 0xFF);
					else mMemoryBus->Write32(MemoryOffset, *Rd);
				}
			}
			if (!P)
			{
				if (U) MemoryOffset += Offset;
				else MemoryOffset -= Offset;
				*Rn = MemoryOffset;
			}
			mExecutionState.instructionState++;
		}
		return false;
	}
	else if (mExecutionState.instructionState == 2)
	{
		bool L = ((mExecutionState.instruction >> 20) & 1) == 1;
		if (!L && !mMemoryBus->IsBusy())
			return true;
		if (L && mMemoryBus->IsDataReady())
		{
			uint32_t* Rd = (uint32_t*)mExecutionState.instructionArgs[1];
			*Rd = mMemoryBus->GetReadResult();
			if (((mExecutionState.instruction >> 12) & 0xF) == 15)
				FlushPipeline();
			return true;
		}
	}
	return false;
}

bool ARM7TDMI::Instruction_HDSDataTrans()
{
	if (mExecutionState.instructionState == 0)
	{
		bool I = ((mExecutionState.instruction >> 22) & 1) == 1;
		bool P = ((mExecutionState.instruction >> 24) & 1) == 1;
		bool U = ((mExecutionState.instruction >> 23) & 1) == 1;

		bool W = false;

		if (P) W = ((mExecutionState.instruction >> 21) & 1) == 1;

		bool L = ((mExecutionState.instruction >> 20) & 1) == 1;

		uint32_t* Rn = GetRegisterById((mExecutionState.instruction >> 16) & 0xF);
		uint32_t* Rd = GetRegisterById((mExecutionState.instruction >> 12) & 0xF);

		uint32_t Offset;
		if (I)
		{
			Offset = (((mExecutionState.instruction >> 8) & 0xF) << 4) | (mExecutionState.instruction & 0xF);
		}
		else
		{
			Offset = *GetRegisterById(mExecutionState.instruction & 0xF);
		}

		uint32_t MemoryOffset = *Rn;
		if (Rn == &mGlobalState.registers[15]) MemoryOffset = mExecutionState.instructionPC + 8;
		if (P)
		{
			if (U) MemoryOffset += Offset;
			else MemoryOffset -= Offset;
			if (W) *Rn = MemoryOffset;
		}
		mExecutionState.instructionArgs[0] = MemoryOffset;
		mExecutionState.instructionArgs[1] = (uint32_t)Rd;
		mExecutionState.instructionArgs[2] = (uint32_t)Rn;
		mExecutionState.instructionArgs[3] = Offset;
		mExecutionState.instructionState++;
		return false;
	}
	else if (mExecutionState.instructionState == 1)
	{
		if (!mMemoryBus->IsBusy())
		{
			uint32_t MemoryOffset = mExecutionState.instructionArgs[0];
			uint32_t* Rd = (uint32_t*)mExecutionState.instructionArgs[1];
			uint32_t* Rn = (uint32_t*)mExecutionState.instructionArgs[2];
			uint32_t Offset = mExecutionState.instructionArgs[3];
			bool P = ((mExecutionState.instruction >> 24) & 1) == 1;
			bool U = ((mExecutionState.instruction >> 23) & 1) == 1;
			bool L = ((mExecutionState.instruction >> 20) & 1) == 1;

			if (L)
			{
				switch ((mExecutionState.instruction >> 5) & 3)
				{
				case 1:
				case 3:
					mMemoryBus->Read16(MemoryOffset);
					break;
				case 2:
					mMemoryBus->Read8(MemoryOffset);
					break;
				}
			}
			else
			{
				switch ((mExecutionState.instruction >> 5) & 3)
				{
				case 1:
					mMemoryBus->Write16(MemoryOffset, (Rd == &mGlobalState.registers[15] ? ((mExecutionState.instructionPC + 12) & 0xFFFF) : (*Rd & 0xFFFF)));
					break;
				}
			}
			if (!P)
			{
				if (U) MemoryOffset += Offset;
				else MemoryOffset -= Offset;
				*Rn = MemoryOffset;
			}
			mExecutionState.instructionState++;
		}
		return false;
	}
	else if (mExecutionState.instructionState == 2)
	{
		bool L = ((mExecutionState.instruction >> 20) & 1) == 1;
		if (!L && !mMemoryBus->IsBusy())
			return true;
		if (L && mMemoryBus->IsDataReady())
		{
			uint32_t* Rd = (uint32_t*)mExecutionState.instructionArgs[1];
			switch ((mExecutionState.instruction >> 5) & 3)
			{
			case 1:
				*Rd = mMemoryBus->GetReadResult();
				break;
			case 2:
				*Rd = (uint32_t)(((int)(mMemoryBus->GetReadResult() << 24)) >> 24);
				break;
			case 3:
				*Rd = (uint32_t)(((int)(mMemoryBus->GetReadResult() << 16)) >> 16);
				break;
			}
			return true;
		}
	}
	return false;
}

bool ARM7TDMI::Instruction_BlockDataTrans()
{
	uint32_t P = (mExecutionState.instruction >> 24) & 1;
	uint32_t U = (mExecutionState.instruction >> 23) & 1;
	uint32_t S = (mExecutionState.instruction >> 22) & 1;
	uint32_t L = (mExecutionState.instruction >> 20) & 1;
	uint32_t RList = mExecutionState.instruction & 0xFFFF;
	if (mExecutionState.instructionState == 0)
	{
		uint32_t W = (mExecutionState.instruction >> 21) & 1;
		uint32_t Rn = (mExecutionState.instruction >> 16) & 0xF;
		uint32_t base_address = *GetRegisterById(Rn);
		int nrregisters = count_ones_16(RList);
		uint32_t start_address;
		if (P && U) start_address = base_address + 4;
		else if (!P && U) start_address = base_address;
		else if (P && !U) start_address = base_address - 4 * nrregisters;
		else start_address = base_address - 4 * (nrregisters - 1);
		//uint32_t end_address = base_address + (U ? 4 * nrregisters : -4 * nrregisters);
		if (W) *GetRegisterById(Rn) = base_address + (U ? 4 * nrregisters : -4 * nrregisters);
		mExecutionState.instructionArgs[0] = base_address + (U ? 4 * nrregisters : 0) + ((U && P) ? 4 : 0);
		mExecutionState.instructionArgs[1] = start_address;
		mExecutionState.instructionArgs[2] = 0;
		mExecutionState.instructionState++;
		if (!L) goto store;
	}
	else
	{
		if (L)//load
		{
			if (mExecutionState.instructionArgs[3] && mMemoryBus->IsDataReady())
			{
				mExecutionState.instructionArgs[3] = 0;
				if (S && (RList >> 15) & 1 && mExecutionState.instructionArgs[2] == 15)
				{
					mGlobalState.registers[15] = mMemoryBus->GetReadResult();
					if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_FIQ)
						mGlobalState.cspr = mGlobalState.spsr_fiq;
					else if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_IRQ)
						mGlobalState.cspr = mGlobalState.spsr_irq;
					else if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_SVC)
						mGlobalState.cspr = mGlobalState.spsr_svc;
					else if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_ABT)
						mGlobalState.cspr = mGlobalState.spsr_abt;
					else if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_UND)
						mGlobalState.cspr = mGlobalState.spsr_und;
					FlushPipeline();
				}
				else if (S && !((RList >> 15) & 1))
					mGlobalState.registers[mExecutionState.instructionArgs[2]] = mMemoryBus->GetReadResult();
				else
				{
					*GetRegisterById(mExecutionState.instructionArgs[2]) = mMemoryBus->GetReadResult();
					if (mExecutionState.instructionArgs[2] == 15) FlushPipeline();
				}
				/*if (U) */mExecutionState.instructionArgs[1] += 4;
				//else mExecutionState.instructionArgs[1] -= 4;
				mExecutionState.instructionArgs[2]++;
				if (mExecutionState.instructionArgs[1] == mExecutionState.instructionArgs[0]) return true;
			}
			if (!mMemoryBus->IsBusy())
			{
				while (mExecutionState.instructionArgs[2] <= 15 && !((RList >> mExecutionState.instructionArgs[2]) & 1)) mExecutionState.instructionArgs[2]++;
				mMemoryBus->Read32(mExecutionState.instructionArgs[1]);
				mExecutionState.instructionArgs[3] = 1;
			}
		}
		else
		{
		store:
			if (!mMemoryBus->IsBusy())
			{
				while (mExecutionState.instructionArgs[2] <= 15 && !((RList >> mExecutionState.instructionArgs[2]) & 1)) mExecutionState.instructionArgs[2]++;
				if (mExecutionState.instructionArgs[2] == 15)
					mMemoryBus->Write32(mExecutionState.instructionArgs[1], mExecutionState.instructionPC);
				else if (S)
					mMemoryBus->Write32(mExecutionState.instructionArgs[1], mGlobalState.registers[mExecutionState.instructionArgs[2]]);
				else mMemoryBus->Write32(mExecutionState.instructionArgs[1], *GetRegisterById(mExecutionState.instructionArgs[2]));
				mExecutionState.instructionArgs[1] += 4;
				if (mExecutionState.instructionArgs[1] == mExecutionState.instructionArgs[0]) return true;
				mExecutionState.instructionArgs[2]++;
			}
		}
	}
	return false;
}

bool ARM7TDMI::Instruction_Branch()
{
	uint32_t Opcode = (mExecutionState.instruction >> 24) & 1;
	int nn = (int)((mExecutionState.instruction & 0xFFFFFF) << 8) >> 8;
	if (Opcode == 1)
		*GetRegisterById(14) = mExecutionState.instructionPC + 4;
	mGlobalState.registers[15] = (uint32_t)((int)mExecutionState.instructionPC + 8 + nn * 4);
	FlushPipeline();
	return true;
}

bool ARM7TDMI::Instruction_SWI()
{
	mGlobalState.registers_svc[1] = mExecutionState.instructionPC + 4;
	mGlobalState.spsr_svc = mGlobalState.cspr;
	mGlobalState.mode = ARM7TDMI_CSPR_MODE_SVC;
	mGlobalState.irq_disable = true;
	mGlobalState.registers[15] = 8;
	FlushPipeline();
	return true;
}

bool ARM7TDMI::Instruction_Thumb_1()
{
	uint32_t Opcode = (mExecutionState.instruction >> 11) & 3;
	bool Shift_C = mGlobalState.c_flag;
	uint32_t result = Shift(Opcode, mGlobalState.registers[(mExecutionState.instruction >> 3) & 7], (mExecutionState.instruction >> 6) & 31, Shift_C);
	mGlobalState.registers[mExecutionState.instruction & 7] = result;
	mGlobalState.z_flag = result == 0;
	mGlobalState.n_flag = (result >> 31) == 1;
	mGlobalState.c_flag = Shift_C;
	return true;
}

bool ARM7TDMI::Instruction_Thumb_2()
{
	uint32_t Opcode = (mExecutionState.instruction >> 9) & 3;
	uint32_t Rs = (mExecutionState.instruction >> 3) & 7;
	uint32_t Rd = mExecutionState.instruction & 7;
	uint32_t result;
	switch (Opcode)
	{
	case 0://add reg
		result = mGlobalState.registers[Rd] = mGlobalState.registers[Rs] + mGlobalState.registers[(mExecutionState.instruction >> 6) & 7];
		mGlobalState.v_flag = ADD_VFLAG(mGlobalState.registers[Rs], mGlobalState.registers[(mExecutionState.instruction >> 6) & 7], result);
		mGlobalState.c_flag = ADD_CFLAG(mGlobalState.registers[Rs], mGlobalState.registers[(mExecutionState.instruction >> 6) & 7]);
		break;
	case 1://sub reg
		result = mGlobalState.registers[Rd] = mGlobalState.registers[Rs] - mGlobalState.registers[(mExecutionState.instruction >> 6) & 7];
		mGlobalState.v_flag = SUB_VFLAG(mGlobalState.registers[Rs], mGlobalState.registers[(mExecutionState.instruction >> 6) & 7], result);
		mGlobalState.c_flag = SUB_CFLAG(mGlobalState.registers[Rs], mGlobalState.registers[(mExecutionState.instruction >> 6) & 7]);
		break;
	case 2://add imm
		result = mGlobalState.registers[Rd] = mGlobalState.registers[Rs] + ((mExecutionState.instruction >> 6) & 7);
		mGlobalState.v_flag = ADD_VFLAG(mGlobalState.registers[Rs], (mExecutionState.instruction >> 6) & 7, result);
		mGlobalState.c_flag = ADD_CFLAG(mGlobalState.registers[Rs], (mExecutionState.instruction >> 6) & 7);
		break;
	case 3://sub imm
		result = mGlobalState.registers[Rd] = mGlobalState.registers[Rs] - ((mExecutionState.instruction >> 6) & 7);
		mGlobalState.v_flag = SUB_VFLAG(mGlobalState.registers[Rs], (mExecutionState.instruction >> 6) & 7, result);
		mGlobalState.c_flag = SUB_CFLAG(mGlobalState.registers[Rs], (mExecutionState.instruction >> 6) & 7);
		break;
	}
	mGlobalState.z_flag = result == 0;
	mGlobalState.n_flag = (result >> 31) == 1;
	return true;
}

bool ARM7TDMI::Instruction_Thumb_3()
{
	uint32_t Opcode = (mExecutionState.instruction >> 11) & 3;
	uint32_t Rd = (mExecutionState.instruction >> 8) & 7;
	uint32_t imm = mExecutionState.instruction & 0xFF;
	uint32_t result;
	switch (Opcode)
	{
	case 0://mov imm
		result = mGlobalState.registers[Rd] = imm;
		break;
	case 1://cmp imm
		result = mGlobalState.registers[Rd] - imm;
		mGlobalState.v_flag = SUB_VFLAG(mGlobalState.registers[Rd], imm, result);
		mGlobalState.c_flag = SUB_CFLAG(mGlobalState.registers[Rd], imm);
		break;
	case 2://add imm
		result = mGlobalState.registers[Rd] + imm;
		mGlobalState.v_flag = ADD_VFLAG(mGlobalState.registers[Rd], imm, result);
		mGlobalState.c_flag = ADD_CFLAG(mGlobalState.registers[Rd], imm);
		mGlobalState.registers[Rd] = result;
		break;
	case 3://sub imm
		result = mGlobalState.registers[Rd] - imm;
		mGlobalState.v_flag = SUB_VFLAG(mGlobalState.registers[Rd], imm, result);
		mGlobalState.c_flag = SUB_CFLAG(mGlobalState.registers[Rd], imm);
		mGlobalState.registers[Rd] = result;
		break;
	}
	mGlobalState.z_flag = result == 0;
	mGlobalState.n_flag = (result >> 31) == 1;
	return true;
}

bool ARM7TDMI::Instruction_Thumb_4()
{
	if (mExecutionState.instructionState == 0)
	{
		uint32_t Opcode = (mExecutionState.instruction >> 6) & 0xF;
		uint32_t Rs = (mExecutionState.instruction >> 3) & 7;
		uint32_t Rs_val = mGlobalState.registers[Rs];
		uint32_t Rd = mExecutionState.instruction & 7;
		uint32_t Rd_val = mGlobalState.registers[Rd];
		bool C = mGlobalState.c_flag;
		uint32_t result;
		switch (Opcode)
		{
		case 0: result = mGlobalState.registers[Rd] = Rd_val & Rs_val; break;
		case 1: result = mGlobalState.registers[Rd] = Rd_val ^ Rs_val; break;
		case 2: result = mGlobalState.registers[Rd] = Shift(0, Rd_val, Rs_val & 0xFF, C); break;
		case 3: result = mGlobalState.registers[Rd] = Shift(1, Rd_val, Rs_val & 0xFF, C); break;
		case 4: result = mGlobalState.registers[Rd] = Shift(2, Rd_val, Rs_val & 0xFF, C); break;
		case 5:
			result = Rd_val + Rs_val + mGlobalState.c_flag;
			mGlobalState.v_flag = ADD_VFLAG(Rd_val, Rs_val + mGlobalState.c_flag, result);
			mGlobalState.c_flag = ADD_CFLAG(Rd_val, Rs_val + mGlobalState.c_flag);
			mGlobalState.registers[Rd] = result;
			break;
		case 6:
			result = Rd_val - (Rs_val + !mGlobalState.c_flag);
			mGlobalState.v_flag = SUB_VFLAG(Rd_val, Rs_val + !mGlobalState.c_flag, result);
			mGlobalState.c_flag = SUB_CFLAG(Rd_val, Rs_val + !mGlobalState.c_flag);
			mGlobalState.registers[Rd] = result;
			break;
		case 7: result = mGlobalState.registers[Rd] = Shift(3, Rd_val, Rs_val & 0xFF, C); break;
		case 8: result = Rd_val & Rs_val; break;
		case 9:
			result = 0 - Rs_val;
			mGlobalState.v_flag = SUB_VFLAG(0, Rs_val, result);
			mGlobalState.c_flag = SUB_CFLAG(0, Rs_val);
			mGlobalState.registers[Rd] = result;
			break;
		case 10:
			result = Rd_val - Rs_val;
			mGlobalState.v_flag = SUB_VFLAG(Rd_val, Rs_val, result);
			mGlobalState.c_flag = SUB_CFLAG(Rd_val, Rs_val);
			break;
		case 11:
			result = Rd_val + Rs_val;
			mGlobalState.v_flag = ADD_VFLAG(Rd_val, Rs_val, result);
			mGlobalState.c_flag = ADD_CFLAG(Rd_val, Rs_val);
			break;
		case 12: result = mGlobalState.registers[Rd] = Rd_val | Rs_val; break;
		case 13:
			result = mGlobalState.registers[Rd] = Rd_val * Rs_val;
			mGlobalState.c_flag = 0;//just destroy the c flag
			//This opcode should have additional I-cycles, but I don't know how much exactly
			//We'll use 2 cycles for now
			mExecutionState.instructionArgs[0] = 2;
			break;
		case 14: result = mGlobalState.registers[Rd] = Rd_val & ~Rs_val; break;
		case 15: result = mGlobalState.registers[Rd] = ~Rs_val; break;
		}
		mGlobalState.z_flag = result == 0;
		mGlobalState.n_flag = (result >> 31) == 1;
		if (Opcode == 13)
		{
			mExecutionState.instructionState++;
			return false;
		}
		return true;
	}
	else if (mExecutionState.instructionState < mExecutionState.instructionArgs[0])
	{
		mExecutionState.instructionState++;
		return false;
	}
	return true;
}

bool ARM7TDMI::Instruction_Thumb_5()
{
	uint32_t Opcode = (mExecutionState.instruction >> 8) & 0x3;
	uint32_t Rs = ((mExecutionState.instruction >> 3) & 0xF);
	uint32_t Rs_val = (Rs == 15 ? (mExecutionState.instructionPC & ~1) + 4 : *GetRegisterById(Rs));
	uint32_t Rd = (mExecutionState.instruction & 7) | (((mExecutionState.instruction >> 7) & 1) << 3);
	uint32_t Rd_val = (Rd == 15 ? (mExecutionState.instructionPC & ~1) + 4 : *GetRegisterById(Rd));
	uint32_t result;
	switch (Opcode)
	{
	case 0: *GetRegisterById(Rd) = Rd_val + Rs_val; break;
	case 1:
		result = Rd_val - Rs_val;
		mGlobalState.v_flag = SUB_VFLAG(Rd_val, Rs_val, result);
		mGlobalState.c_flag = SUB_CFLAG(Rd_val, Rs_val);
		mGlobalState.z_flag = result == 0;
		mGlobalState.n_flag = (result >> 31) == 1;
		break;
	case 2:
		*GetRegisterById(Rd) = Rs_val;
		break;
	case 3:
		mGlobalState.thumb = Rs_val & 1;
		if (!mGlobalState.thumb)
			mGlobalState.registers[15] = Rs_val & ~2;
		else mGlobalState.registers[15] = Rs_val;
		break;
	}
	if (Rd == 15 || Opcode == 3)
		FlushPipeline();
	return true;
}

bool ARM7TDMI::Instruction_Thumb_6()
{
	if (mExecutionState.instructionState == 0)
	{
		uint32_t offset = mExecutionState.instruction & 0xFF;
		mExecutionState.instructionArgs[0] = (((mExecutionState.instructionPC & ~1) + 4) & ~2) + offset * 4;
		mExecutionState.instructionState++;
	}
	else if (mExecutionState.instructionState == 1)
	{
		if (!mMemoryBus->IsBusy())
		{
			mMemoryBus->Read32(mExecutionState.instructionArgs[0]);
			mExecutionState.instructionState++;
		}
	}
	else
	{
		if (mMemoryBus->IsDataReady())
		{
			uint32_t Rd = (mExecutionState.instruction >> 8) & 7;
			mGlobalState.registers[Rd] = mMemoryBus->GetReadResult();
			return true;
		}
	}
	return false;
}

bool ARM7TDMI::Instruction_Thumb_7()
{
	if (mExecutionState.instructionState == 0)
	{
		uint32_t Ro = (mExecutionState.instruction >> 6) & 7;
		uint32_t Rb = (mExecutionState.instruction >> 3) & 7;
		uint32_t address = mGlobalState.registers[Rb] + mGlobalState.registers[Ro];
		mExecutionState.instructionArgs[0] = address;
		mExecutionState.instructionState++;
		return false;
	}
	else if (mExecutionState.instructionState == 1)
	{
		if (!mMemoryBus->IsBusy())
		{
			uint32_t Opcode = (mExecutionState.instruction >> 10) & 3;
			uint32_t Rd = mExecutionState.instruction & 7;
			switch (Opcode)
			{
			case 0:
				mMemoryBus->Write32(mExecutionState.instructionArgs[0], mGlobalState.registers[Rd]);
				break;
			case 1:
				mMemoryBus->Write8(mExecutionState.instructionArgs[0], mGlobalState.registers[Rd] & 0xFF);
				break;
			case 2:
				mMemoryBus->Read32(mExecutionState.instructionArgs[0]);
				break;
			case 3:
				mMemoryBus->Read8(mExecutionState.instructionArgs[0]);
				break;
			}
			mExecutionState.instructionState++;
		}
		return false;
	}
	else if (mExecutionState.instructionState == 2)
	{
		uint32_t Opcode = (mExecutionState.instruction >> 10) & 3;
		uint32_t Rd = mExecutionState.instruction & 7;
		if (Opcode <= 1 && !mMemoryBus->IsBusy())
			return true;
		if (Opcode >= 2 && mMemoryBus->IsDataReady())
		{
			mGlobalState.registers[Rd] = mMemoryBus->GetReadResult();
			return true;
		}
	}
	return false;
}

bool ARM7TDMI::Instruction_Thumb_8()
{
	if (mExecutionState.instructionState == 0)
	{
		uint32_t Ro = (mExecutionState.instruction >> 6) & 7;
		uint32_t Rb = (mExecutionState.instruction >> 3) & 7;
		uint32_t address = mGlobalState.registers[Rb] + mGlobalState.registers[Ro];
		mExecutionState.instructionArgs[0] = address;
		mExecutionState.instructionState++;
		return false;
	}
	else if (mExecutionState.instructionState == 1)
	{
		if (!mMemoryBus->IsBusy())
		{
			uint32_t Opcode = (mExecutionState.instruction >> 10) & 3;
			uint32_t Rd = mExecutionState.instruction & 7;
			switch (Opcode)
			{
			case 0:
				mMemoryBus->Write16(mExecutionState.instructionArgs[0], mGlobalState.registers[Rd] & 0xFFFF);
				break;
			case 1:
				mMemoryBus->Read8(mExecutionState.instructionArgs[0]);
				break;
			case 2:
			case 3:
				mMemoryBus->Read16(mExecutionState.instructionArgs[0]);
				break;
			}
			mExecutionState.instructionState++;
		}
		return false;
	}
	else if (mExecutionState.instructionState == 2)
	{
		uint32_t Opcode = (mExecutionState.instruction >> 10) & 3;
		uint32_t Rd = mExecutionState.instruction & 7;
		if (Opcode == 0 && !mMemoryBus->IsBusy())
			return true;
		if (Opcode > 0 && mMemoryBus->IsDataReady())
		{
			switch (Opcode)
			{
			case 1:
				mGlobalState.registers[Rd] = (uint32_t)(((int)(mMemoryBus->GetReadResult() << 24)) >> 24);
				break;
			case 2:
				mGlobalState.registers[Rd] = mMemoryBus->GetReadResult();
				break;
			case 3:
				mGlobalState.registers[Rd] = (uint32_t)(((int)(mMemoryBus->GetReadResult() << 16)) >> 16);
				break;
			}
			return true;
		}
	}
	return false;
}


bool ARM7TDMI::Instruction_Thumb_9()
{
	if (mExecutionState.instructionState == 0)
	{
		uint32_t Opcode = (mExecutionState.instruction >> 11) & 3;
		uint32_t imm = (mExecutionState.instruction >> 6) & 31;
		uint32_t Rb = (mExecutionState.instruction >> 3) & 7;
		uint32_t address = mGlobalState.registers[Rb] + (Opcode <= 1 ? imm * 4 : imm);
		mExecutionState.instructionArgs[0] = address;
		mExecutionState.instructionState++;
		return false;
	}
	else if (mExecutionState.instructionState == 1)
	{
		if (!mMemoryBus->IsBusy())
		{
			uint32_t Opcode = (mExecutionState.instruction >> 11) & 3;
			uint32_t Rd = mExecutionState.instruction & 7;
			switch (Opcode)
			{
			case 0:
				mMemoryBus->Write32(mExecutionState.instructionArgs[0], mGlobalState.registers[Rd]);
				break;
			case 1:
				mMemoryBus->Read32(mExecutionState.instructionArgs[0]);
				break;
			case 2:
				mMemoryBus->Write8(mExecutionState.instructionArgs[0], mGlobalState.registers[Rd] & 0xFF);
				break;
			case 3:
				mMemoryBus->Read8(mExecutionState.instructionArgs[0]);
				break;
			}
			mExecutionState.instructionState++;
		}
		return false;
	}
	else if (mExecutionState.instructionState == 2)
	{
		uint32_t Opcode = (mExecutionState.instruction >> 11) & 3;
		uint32_t Rd = mExecutionState.instruction & 7;
		if (!(Opcode & 1) && !mMemoryBus->IsBusy())
			return true;
		if (Opcode & 1 && mMemoryBus->IsDataReady())
		{
			mGlobalState.registers[Rd] = mMemoryBus->GetReadResult();
			return true;
		}
	}
	return false;
}

bool ARM7TDMI::Instruction_Thumb_10()
{
	if (mExecutionState.instructionState == 0)
	{
		uint32_t imm = (mExecutionState.instruction >> 6) & 31;
		uint32_t Rb = (mExecutionState.instruction >> 3) & 7;
		uint32_t address = mGlobalState.registers[Rb] + imm * 2;
		mExecutionState.instructionArgs[0] = address;
		mExecutionState.instructionState++;
		return false;
	}
	else if (mExecutionState.instructionState == 1)
	{
		if (!mMemoryBus->IsBusy())
		{
			uint32_t Opcode = (mExecutionState.instruction >> 11) & 1;
			uint32_t Rd = mExecutionState.instruction & 7;
			switch (Opcode)
			{
			case 0:
				mMemoryBus->Write16(mExecutionState.instructionArgs[0], mGlobalState.registers[Rd] & 0xFFFF);
				break;
			case 1:
				mMemoryBus->Read16(mExecutionState.instructionArgs[0]);
				break;
			}
			mExecutionState.instructionState++;
		}
		return false;
	}
	else if (mExecutionState.instructionState == 2)
	{
		uint32_t Opcode = (mExecutionState.instruction >> 11) & 1;
		uint32_t Rd = mExecutionState.instruction & 7;
		if (!(Opcode & 1) && !mMemoryBus->IsBusy())
			return true;
		if (Opcode & 1 && mMemoryBus->IsDataReady())
		{
			mGlobalState.registers[Rd] = mMemoryBus->GetReadResult();
			return true;
		}
	}
	return false;
}

bool ARM7TDMI::Instruction_Thumb_11()
{
	if (mExecutionState.instructionState == 0)
	{
		uint32_t imm = mExecutionState.instruction & 0xFF;
		uint32_t address = *GetRegisterById(13) + imm * 4;
		mExecutionState.instructionArgs[0] = address;
		mExecutionState.instructionState++;
		return false;
	}
	else if (mExecutionState.instructionState == 1)
	{
		if (!mMemoryBus->IsBusy())
		{
			uint32_t Opcode = (mExecutionState.instruction >> 11) & 1;
			uint32_t Rd = (mExecutionState.instruction >> 8) & 7;
			switch (Opcode)
			{
			case 0:
				mMemoryBus->Write32(mExecutionState.instructionArgs[0], mGlobalState.registers[Rd]);
				break;
			case 1:
				mMemoryBus->Read32(mExecutionState.instructionArgs[0]);
				break;
			}
			mExecutionState.instructionState++;
		}
		return false;
	}
	else if (mExecutionState.instructionState == 2)
	{
		uint32_t Opcode = (mExecutionState.instruction >> 11) & 1;
		uint32_t Rd = (mExecutionState.instruction >> 8) & 7;
		if (!(Opcode & 1) && !mMemoryBus->IsBusy())
			return true;
		if (Opcode & 1 && mMemoryBus->IsDataReady())
		{
			mGlobalState.registers[Rd] = mMemoryBus->GetReadResult();
			return true;
		}
	}
	return false;
}

bool ARM7TDMI::Instruction_Thumb_12()
{
	uint32_t offset = mExecutionState.instruction & 0xFF;
	uint32_t Rd = (mExecutionState.instruction >> 8) & 7;
	if ((mExecutionState.instruction >> 11) & 1)
		mGlobalState.registers[Rd] = *GetRegisterById(13) + offset * 4;
	else mGlobalState.registers[Rd] = (((mExecutionState.instructionPC & ~1) + 4) & ~2) + offset * 4;
	return true;
}

bool ARM7TDMI::Instruction_Thumb_13()
{
	uint32_t offset = mExecutionState.instruction & 127;
	if (mExecutionState.instruction & 0x80)
		*GetRegisterById(13) = *GetRegisterById(13) - offset * 4;
	else *GetRegisterById(13) = *GetRegisterById(13) + offset * 4;
	return true;
}

bool ARM7TDMI::Instruction_Thumb_14()
{
	bool POP = (mExecutionState.instruction >> 11) & 1;
	bool PCLR = (mExecutionState.instruction >> 8) & 1;
	uint8_t rlist = mExecutionState.instruction & 0xFF;
	if (mExecutionState.instructionState == 0)
	{
		int nrregisters = count_ones_8(rlist) + PCLR;
		uint32_t sp = *GetRegisterById(13);
		mExecutionState.instructionState++;
		if (!POP)
		{
			mExecutionState.instructionArgs[0] = sp;
			mExecutionState.instructionArgs[1] = sp - nrregisters * 4;
			mExecutionState.instructionArgs[2] = 0;
			*GetRegisterById(13) = sp - nrregisters * 4;
			goto push;//try to push the first value
		}
		else
		{
			mExecutionState.instructionArgs[0] = sp + nrregisters * 4;
			mExecutionState.instructionArgs[1] = sp;
			mExecutionState.instructionArgs[2] = 0;
			*GetRegisterById(13) = sp + nrregisters * 4;
			return false;
			//goto pop;
		}
	}
	else
	{
		if (!POP)
		{
		push:
			if (!mMemoryBus->IsBusy())
			{
				while (mExecutionState.instructionArgs[2] <= 7 && !((rlist >> mExecutionState.instructionArgs[2]) & 1)) mExecutionState.instructionArgs[2]++;
				if (mExecutionState.instructionArgs[2] == 8)
				{
					if (PCLR)
					{
						mMemoryBus->Write32(mExecutionState.instructionArgs[1], *GetRegisterById(14));
						mExecutionState.instructionArgs[1] += 4;
						if (mExecutionState.instructionArgs[1] == mExecutionState.instructionArgs[0]) return true;
					}
				}
				else
				{
					mMemoryBus->Write32(mExecutionState.instructionArgs[1], *GetRegisterById(mExecutionState.instructionArgs[2]));
					mExecutionState.instructionArgs[1] += 4;
					if (mExecutionState.instructionArgs[1] == mExecutionState.instructionArgs[0]) return true;
				}
				mExecutionState.instructionArgs[2]++;
			}
		}
		else
		{
			if (mExecutionState.instructionArgs[3] && mMemoryBus->IsDataReady())
			{
				mExecutionState.instructionArgs[3] = 0;
				if (mExecutionState.instructionArgs[2] == 8)
				{
					mGlobalState.registers[15] = mMemoryBus->GetReadResult();
					FlushPipeline();
				}
				else *GetRegisterById(mExecutionState.instructionArgs[2]) = mMemoryBus->GetReadResult();
				mExecutionState.instructionArgs[1] += 4;
				mExecutionState.instructionArgs[2]++;
				if (mExecutionState.instructionArgs[1] == mExecutionState.instructionArgs[0]) return true;
			}
			if (!mMemoryBus->IsBusy())
			{
				while (mExecutionState.instructionArgs[2] <= 7 && !((rlist >> mExecutionState.instructionArgs[2]) & 1)) mExecutionState.instructionArgs[2]++;
				if (mExecutionState.instructionArgs[2] == 8)
				{
					if (PCLR)
					{
						mMemoryBus->Read32(mExecutionState.instructionArgs[1]);
						mExecutionState.instructionArgs[3] = 1;
					}
				}
				else
				{
					mMemoryBus->Read32(mExecutionState.instructionArgs[1]);
					mExecutionState.instructionArgs[3] = 1;
				}
			}
		}
	}
	return false;
}

bool ARM7TDMI::Instruction_Thumb_15()
{
	uint32_t L = (mExecutionState.instruction >> 11) & 1;
	uint32_t RList = mExecutionState.instruction & 0xFF;
	if (mExecutionState.instructionState == 0)
	{
		uint32_t Rb = (mExecutionState.instruction >> 8) & 7;
		uint32_t base_address = mGlobalState.registers[Rb];
		int nrregisters = count_ones_8(RList);
		uint32_t start_address = base_address;
		uint32_t end_address = base_address + 4 * nrregisters;
		mGlobalState.registers[Rb] = end_address;
		mExecutionState.instructionArgs[0] = end_address;
		mExecutionState.instructionArgs[1] = start_address;
		mExecutionState.instructionArgs[2] = 0;
		mExecutionState.instructionState++;
		if (!L) goto store;
	}
	else
	{
		if (L)//load
		{
			if (mExecutionState.instructionArgs[3] && mMemoryBus->IsDataReady())
			{
				mExecutionState.instructionArgs[3] = 0;
				mGlobalState.registers[mExecutionState.instructionArgs[2]] = mMemoryBus->GetReadResult();
				mExecutionState.instructionArgs[1] += 4;
				mExecutionState.instructionArgs[2]++;
				if (mExecutionState.instructionArgs[1] == mExecutionState.instructionArgs[0]) return true;
			}
			if (!mMemoryBus->IsBusy())
			{
				while (mExecutionState.instructionArgs[2] <= 7 && !((RList >> mExecutionState.instructionArgs[2]) & 1)) mExecutionState.instructionArgs[2]++;
				mMemoryBus->Read32(mExecutionState.instructionArgs[1]);
				mExecutionState.instructionArgs[3] = 1;
			}
		}
		else
		{
		store:
			if (!mMemoryBus->IsBusy())
			{
				while (mExecutionState.instructionArgs[2] <= 7 && !((RList >> mExecutionState.instructionArgs[2]) & 1)) mExecutionState.instructionArgs[2]++;
				mMemoryBus->Write32(mExecutionState.instructionArgs[1], mGlobalState.registers[mExecutionState.instructionArgs[2]]);
				mExecutionState.instructionArgs[1] += 4;
				if (mExecutionState.instructionArgs[1] == mExecutionState.instructionArgs[0]) return true;
				mExecutionState.instructionArgs[2]++;
			}
		}
	}
	return false;
}

bool ARM7TDMI::Instruction_Thumb_16()
{
	uint32_t cond = (mExecutionState.instruction >> 8) & 0xF;
	if (EvaluateCondition(cond))
	{
		mGlobalState.registers[15] = mExecutionState.instructionPC + 4 + ((int8_t)(mExecutionState.instruction & 0xFF)) * 2;
		FlushPipeline();
	}
	return true;
}

bool ARM7TDMI::Instruction_Thumb_17()
{
	mGlobalState.registers_svc[1] = (mExecutionState.instructionPC & ~1) + 2;
	mGlobalState.spsr_svc = mGlobalState.cspr;
	mGlobalState.irq_disable = true;
	mGlobalState.thumb = false;
	mGlobalState.mode = ARM7TDMI_CSPR_MODE_SVC;
	mGlobalState.registers[15] = 0x00000008;
	FlushPipeline();
	return true;
}

bool ARM7TDMI::Instruction_Thumb_18()
{
	int offset = ((int)(mExecutionState.instruction << 21)) >> 20;
	mGlobalState.registers[15] = mExecutionState.instructionPC + 4 + offset;
	FlushPipeline();
	return true;
}


bool ARM7TDMI::Instruction_Thumb_19A()
{
	*GetRegisterById(14) = mExecutionState.instructionPC + 4 + ((((int)((mExecutionState.instruction & 0x7FF) << 21)) >> 21) << 12);
	return true;
}

bool ARM7TDMI::Instruction_Thumb_19B()
{
	mGlobalState.registers[15] = *GetRegisterById(14) + ((mExecutionState.instruction & 0x7FF) << 1);
	*GetRegisterById(14) = (mExecutionState.instructionPC + 2) | 1;
	FlushPipeline();
	return true;
}