#pragma once
#include "stdafx.h"
#include "MemoryBus.h"
#include "MemoryDevice.h"

class BoardWRAM : public MemoryDevice
{
private:
	uint8_t mMemory[256 * 1024];
public:
	BoardWRAM()
	{
		ZeroMemory(&mMemory[0], sizeof(mMemory));
	}
	void HandleRequest(MemoryBus* memoryBus);
};
