#pragma once
#include "ClockBlock.h"
#include "MemoryBus.h"

#define ARM7TDMI_CSPR_MODE_OLD_USR		0
#define ARM7TDMI_CSPR_MODE_OLD_FIQ		1
#define ARM7TDMI_CSPR_MODE_OLD_IRQ		2
#define ARM7TDMI_CSPR_MODE_OLD_SVC		3
#define ARM7TDMI_CSPR_MODE_USR			16
#define ARM7TDMI_CSPR_MODE_FIQ			17
#define ARM7TDMI_CSPR_MODE_IRQ			18
#define ARM7TDMI_CSPR_MODE_SVC			19
#define ARM7TDMI_CSPR_MODE_ABT			23
#define ARM7TDMI_CSPR_MODE_UND			27
#define ARM7TDMI_CSPR_MODE_SYS			31

typedef struct
{
	uint32_t registers[16];//r0-r15
	union
	{
		uint32_t cspr;
		struct 
		{
			uint32_t mode : 5;
			uint32_t thumb : 1;
			uint32_t fiq_disable : 1;
			uint32_t irq_disable : 1;
			uint32_t reserved : 19;
			uint32_t q_flag : 1;//actually only used on ARMv5TE and up
			uint32_t v_flag : 1;
			uint32_t c_flag : 1;
			uint32_t z_flag : 1;
			uint32_t n_flag : 1;
		};
	};
	uint32_t registers_fiq[7];//r8_fiq-r14_fiq
	uint32_t spsr_fiq;
	uint32_t registers_svc[2];//r13_svc-r14_svc
	uint32_t spsr_svc;
	uint32_t registers_abt[2];//r13_abt-r14_abt
	uint32_t spsr_abt;
	uint32_t registers_irq[2];//r13_irq-r14_irq
	uint32_t spsr_irq;
	uint32_t registers_und[2];//r13_und-r14_und
	uint32_t spsr_und;
} ARM7TDMIGlobalState;

class ARM7TDMI : public ClockBlock
{
private:
	struct FetchingState
	{
		uint32_t instructionPC;
		bool waitingForData;
		bool cancelData;
	};

	struct DecodingState
	{
		uint32_t instruction;
		uint32_t instructionPC;
		bool done;
	};

	typedef bool (ARM7TDMI::*InstructionProc)();
	struct ExecutionState
	{
		InstructionProc instructionProc;
		int instructionState;
		uint32_t instructionArgs[4];//user data
		uint32_t instruction;
		uint32_t instructionPC;
	};

	ARM7TDMIGlobalState mGlobalState;
	FetchingState mFetchingState;
	DecodingState mDecodingState;
	ExecutionState mExecutionState;
	MemoryBus* mMemoryBus;
	IORegisters* mIORegisters;
public:
	ARM7TDMI(MemoryBus* memoryBus, IORegisters* ioRegisters)
		: mMemoryBus(memoryBus), mIORegisters(ioRegisters)
	{
		ZeroMemory(&mGlobalState, sizeof(mGlobalState));
		ZeroMemory(&mFetchingState, sizeof(mFetchingState));
		ZeroMemory(&mDecodingState, sizeof(mDecodingState));
		ZeroMemory(&mExecutionState, sizeof(mExecutionState));
		mDecodingState.done = true;
		mGlobalState.registers[15] = 0;
		mGlobalState.mode = ARM7TDMI_CSPR_MODE_SVC;
		mGlobalState.fiq_disable = true;
		mGlobalState.irq_disable = true;
	}
	void RunCycle();
private:
	bool EvaluateCondition(uint32_t condition);
	uint32_t Shift(uint32_t ShiftType, uint32_t Value, uint32_t NrBits, bool &Carry);
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
	void FlushPipeline();
	bool Instruction_Nop();
	bool Instruction_DataProc();
	bool Instruction_Multiply();
	bool Instruction_SingleDataTrans();
	bool Instruction_HDSDataTrans();
	bool Instruction_BlockDataTrans();
	bool Instruction_Branch();
	bool Instruction_SWI();

	bool Instruction_Thumb_1();
	bool Instruction_Thumb_2();
	bool Instruction_Thumb_3();
	bool Instruction_Thumb_4();
	bool Instruction_Thumb_5();
	bool Instruction_Thumb_6();
	bool Instruction_Thumb_7();
	bool Instruction_Thumb_8();
	bool Instruction_Thumb_9();
	bool Instruction_Thumb_10();
	bool Instruction_Thumb_11();
	bool Instruction_Thumb_12();
	bool Instruction_Thumb_13();
	bool Instruction_Thumb_14();
	bool Instruction_Thumb_15();
	bool Instruction_Thumb_16();
	bool Instruction_Thumb_17();
	bool Instruction_Thumb_18();
	bool Instruction_Thumb_19A();
	bool Instruction_Thumb_19B();
};