#pragma once
#include "stdafx.h"
#include "MemoryBus.h"
#include "MemoryDevice.h"

class OAM : public MemoryDevice
{
	friend class LCDVideoController;
private:
	uint8_t mMemory[1024];
public:
	OAM()
	{
		ZeroMemory(&mMemory[0], sizeof(mMemory));
	}
	void HandleRequest(MemoryBus* memoryBus);
};