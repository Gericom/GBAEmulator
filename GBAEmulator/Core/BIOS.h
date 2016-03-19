#pragma once
#include "MemoryBus.h"
#include "MemoryDevice.h"

class BIOS : public MemoryDevice
{
private:
	void* mBIOSData;
public:
	BIOS(void* biosData)
		: mBIOSData(biosData)
	{ }
	void HandleRequest(MemoryBus* memoryBus);
};