#pragma once
#include "stdafx.h"
#include "MemoryBus.h"
#include "MemoryDevice.h"

class VRAM : public MemoryDevice
{
	friend class LCDVideoController;
private:
	uint8_t mMemory[96 * 1024];
public:
	VRAM()
	{
		ZeroMemory(&mMemory[0], sizeof(mMemory));
	}
	void HandleRequest(MemoryBus* memoryBus);
};