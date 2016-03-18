#include "stdafx.h"
#include "ARM7TDMI.h"

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

uint32_t* ARM7TDMI::GetRegisterById(int id)
{
	if (id <= 7 || id == 15 || mGlobalState.mode == ARM7TDMI_CSPR_MODE_USR || mGlobalState.mode == ARM7TDMI_CSPR_MODE_SYS)
		return &mGlobalState.registers[id];
	if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_FIQ)
		return &mGlobalState.registers_fiq[id - 8];
	else if (id < 13)
		return &mGlobalState.registers[id];
	else if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_SVC)
		return &mGlobalState.registers_svc[id - 13];
	else if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_ABT)
		return &mGlobalState.registers_abt[id - 13];
	else if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_IRQ)
		return &mGlobalState.registers_irq[id - 13];
	else if (mGlobalState.mode == ARM7TDMI_CSPR_MODE_UND)
		return &mGlobalState.registers_und[id - 13];
	return NULL;
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

	//execute
	if (mExecutionState.instructionProc)
	{
		if ((this->*mExecutionState.instructionProc)())//returns true if the instruction is fully executed
			mExecutionState.instructionProc = NULL;
	}
	//fetch
	if (mDecodingState.instructionPC == mExecutionState.instructionPC && !mMemoryBus->IsBusy())//We can't fetch if the memory bus is busy
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
					mExecutionState.instructionProc = &ARM7TDMI::Instruction_DataProc;
					break;
				case 0x2:
				case 0x3:
					mExecutionState.instructionProc = &ARM7TDMI::Instruction_SingleDataTrans;
					break;
				case 0x4:
					//BlockDataTrans(c, inst);
					//c.Registers[15] += 4;
					break;
				case 0x5://101
					mExecutionState.instructionProc = &ARM7TDMI::Instruction_Branch;
					break;
				case 0x7:
					break;//ignore mcr, mrc and that kind of crap
				default:
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
			else if((mDecodingState.instruction >> 10) == 16)
				mExecutionState.instructionProc = &ARM7TDMI::Instruction_Thumb_4;

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
				uint32_t dst = Rn - 4;
				mGlobalState.thumb = dst & 1;
				mGlobalState.registers[15] = dst;
			}
			else if (op == 3)
			{
				*GetRegisterById(14) = mExecutionState.instructionPC + 4;
				uint32_t dst = Rn - 4;
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
				Op2 = Shift(ShiftType, *Rm, *Rs & 0xFF, Shift_C);
				additionalCycle = true;
			}
		}

		switch (Opcode)
		{
		case 0x2://sub
			result = *Rd = Rn - Op2;
			break;
		case 0x4://add
			result = *Rd = Rn + Op2;
			break;
		case 0x8:
			if (S == false)//MRS
			{
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
		case 0xC://OR logical 
			result = *Rd = Rn | Op2;
			break;
		case 0xD://move
			result = *Rd = Op2;
			break;
		case 0xE://bit clear 
			result = *Rd = Rn & ~Op2;
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
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 11:
					C = Op2 > (0xFFFFFFFFU - Rn);
					V = ((Rn >= 0 && Op2 >= 0) || (Rn < 0 && Op2 < 0)) && ((Rn < 0 && result >= 0) || (Rn >= 0 && result < 0));
					break;
				case 0x2:
				case 0xA:
					C = !(Op2 > Rn);
					V = ((Rn < 0 && Op2 >= 0) || (Rn >= 0 && Op2 < 0)) && ((Rn < 0 && result >= 0) || (Rn >= 0 && result < 0));
					break;
				}
				mGlobalState.c_flag = C;
				mGlobalState.n_flag = N;
				mGlobalState.v_flag = V;
				mGlobalState.z_flag = Z;
			}
			else FlushPipeline();
		}
		if (additionalCycle)
		{
			mExecutionState.instructionState++;
			return false;
		}
	}
	return true;
}

bool ARM7TDMI::Instruction_SingleDataTrans()
{
	if (mExecutionState.instructionState == 0)
	{
		bool I = ((mExecutionState.instruction >> 25) & 1) == 1;
		bool P = ((mExecutionState.instruction >> 24) & 1) == 1;
		bool U = ((mExecutionState.instruction >> 23) & 1) == 1;
		bool B = ((mExecutionState.instruction >> 22) & 1) == 1;

		bool T = false;
		bool W = false;

		if (P) W = ((mExecutionState.instruction >> 21) & 1) == 1;
		else T = ((mExecutionState.instruction >> 21) & 1) == 1;

		bool L = ((mExecutionState.instruction >> 20) & 1) == 1;

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
			return true;
		}
	}
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
		mGlobalState.v_flag = ((mGlobalState.registers[Rs] >= 0 && mGlobalState.registers[(mExecutionState.instruction >> 6) & 7] >= 0) || (mGlobalState.registers[Rs] < 0 && mGlobalState.registers[(mExecutionState.instruction >> 6) & 7] < 0)) && ((mGlobalState.registers[Rs] < 0 && result >= 0) || (mGlobalState.registers[Rs] >= 0 && result < 0));
		mGlobalState.c_flag = mGlobalState.registers[(mExecutionState.instruction >> 6) & 7] > (0xFFFFFFFFU - mGlobalState.registers[Rs]);
		break;
	case 1://sub reg
		result = mGlobalState.registers[Rd] = mGlobalState.registers[Rs] - mGlobalState.registers[(mExecutionState.instruction >> 6) & 7];
		mGlobalState.v_flag = ((mGlobalState.registers[Rs] < 0 && mGlobalState.registers[(mExecutionState.instruction >> 6) & 7] >= 0) || (mGlobalState.registers[Rs] >= 0 && mGlobalState.registers[(mExecutionState.instruction >> 6) & 7] < 0)) && ((mGlobalState.registers[Rs] < 0 && result >= 0) || (mGlobalState.registers[Rs] >= 0 && result < 0));
		mGlobalState.c_flag = !(mGlobalState.registers[(mExecutionState.instruction >> 6) & 7] > mGlobalState.registers[Rs]);
		break;
	case 2://add imm
		result = mGlobalState.registers[Rd] = mGlobalState.registers[Rs] + ((mExecutionState.instruction >> 6) & 7);
		mGlobalState.v_flag = ((mGlobalState.registers[Rs] >= 0 && ((mExecutionState.instruction >> 6) & 7) >= 0) || (mGlobalState.registers[Rs] < 0 && ((mExecutionState.instruction >> 6) & 7) < 0)) && ((mGlobalState.registers[Rs] < 0 && result >= 0) || (mGlobalState.registers[Rs] >= 0 && result < 0));
		mGlobalState.c_flag = mGlobalState.registers[(mExecutionState.instruction >> 6) & 7] > (0xFFFFFFFFU - ((mExecutionState.instruction >> 6) & 7));
		break;
	case 3://sub imm
		result = mGlobalState.registers[Rd] = mGlobalState.registers[Rs] - ((mExecutionState.instruction >> 6) & 7);
		mGlobalState.v_flag = ((mGlobalState.registers[Rs] < 0 && ((mExecutionState.instruction >> 6) & 7) >= 0) || (mGlobalState.registers[Rs] >= 0 && ((mExecutionState.instruction >> 6) & 7) < 0)) && ((mGlobalState.registers[Rs] < 0 && result >= 0) || (mGlobalState.registers[Rs] >= 0 && result < 0));
		mGlobalState.c_flag = !(((mExecutionState.instruction >> 6) & 7) > mGlobalState.registers[Rs]);
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
		mGlobalState.v_flag = ((mGlobalState.registers[Rd] < 0 && imm >= 0) || (mGlobalState.registers[Rd] >= 0 && imm < 0)) && ((mGlobalState.registers[Rd] < 0 && result >= 0) || (mGlobalState.registers[Rd] >= 0 && result < 0));
		mGlobalState.c_flag = !(imm > mGlobalState.registers[Rd]);
		break;
	case 2://add imm
		result = mGlobalState.registers[Rd] + imm;
		mGlobalState.v_flag = ((mGlobalState.registers[Rd] >= 0 && imm >= 0) || (mGlobalState.registers[Rd] < 0 && imm < 0)) && ((mGlobalState.registers[Rd] < 0 && result >= 0) || (mGlobalState.registers[Rd] >= 0 && result < 0));
		mGlobalState.c_flag = mGlobalState.registers[(mExecutionState.instruction >> 6) & 7] >(0xFFFFFFFFU - ((mExecutionState.instruction >> 6) & 7));
		mGlobalState.registers[Rd] = result;
		break;
	case 3://sub imm
		result = mGlobalState.registers[Rd] - imm;
		mGlobalState.v_flag = ((mGlobalState.registers[Rd] < 0 && imm >= 0) || (mGlobalState.registers[Rd] >= 0 && imm < 0)) && ((mGlobalState.registers[Rd] < 0 && result >= 0) || (mGlobalState.registers[Rd] >= 0 && result < 0));
		mGlobalState.c_flag = !(imm > mGlobalState.registers[Rd]);
		mGlobalState.registers[Rd] = result;
		break;
	}
	mGlobalState.z_flag = result == 0;
	mGlobalState.n_flag = (result >> 31) == 1;
	return true;
}

bool ARM7TDMI::Instruction_Thumb_4()
{
	uint32_t Opcode = (mExecutionState.instruction >> 6) & 0xF;
	uint32_t Rs = (mExecutionState.instruction >> 3) & 7;
	uint32_t Rd = mExecutionState.instruction & 7;
	return true;
}