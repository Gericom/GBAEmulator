#pragma once
#include "stdafx.h"
#include "MemoryBus.h"
#include "MemoryDevice.h"

class PalRam : public MemoryDevice
{
	friend class LCDVideoController;
private:
	uint8_t mMemory[1024];
public:
	PalRam()
	{
		ZeroMemory(&mMemory[0], sizeof(mMemory));
	}
	void HandleRequest(MemoryBus* memoryBus);
};