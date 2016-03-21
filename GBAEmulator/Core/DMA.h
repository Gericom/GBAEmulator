#pragma once
#include "ClockBlock.h"

class MemoryBus;
class IORegisters;

class DMA : public ClockBlock
{
private:
	struct DMAState
	{
		bool busy;
		uint32_t src;
		int srcMode;
		uint32_t dst;
		int dstMode;
		int nrLeft;
		int bitMode;
		bool waitingForData;
	};

	DMAState mDMAChannels[4];

	MemoryBus* mMemoryBus;
	IORegisters* mIORegisters;
public:
	DMA(MemoryBus* memoryBus, IORegisters* ioRegisters)
		: mMemoryBus(memoryBus), mIORegisters(ioRegisters)
	{ 
		ZeroMemory(&mDMAChannels[0], sizeof(mDMAChannels));
	}
	void RunCycle();
};