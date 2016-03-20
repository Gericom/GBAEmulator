#pragma once
#include "stdafx.h"
#include "MemoryBus.h"
#include "MemoryDevice.h"

class ChipWRAM : public MemoryDevice
{
private:
	uint8_t mMemory[32 * 1024];
public:
	ChipWRAM()
	{ 
		ZeroMemory(&mMemory[0], sizeof(mMemory));
	}
	void HandleRequest(MemoryBus* memoryBus);
};