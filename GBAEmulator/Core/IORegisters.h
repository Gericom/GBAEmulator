#pragma once
#include "stdafx.h"
#include "MemoryBus.h"
#include "MemoryDevice.h"

class IORegisters : public MemoryDevice
{
/*	friend class LCDVideoController;
	friend class ARM7TDMI;
	friend class DMA;
private:*/
public:
	uint8_t mMemory[0x1000];
public:
	IORegisters()
	{
		ZeroMemory(&mMemory[0], sizeof(mMemory));
		*((uint16_t*)&mMemory[0x88]) = 0x200;
		*((uint16_t*)&mMemory[0x130]) = 0x3FF;
		mMemory[0x301] = 0xFF;
	}
	void HandleRequest(MemoryBus* memoryBus);
};