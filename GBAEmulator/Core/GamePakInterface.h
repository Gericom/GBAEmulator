#pragma once
#include "MemoryBus.h"
#include "MemoryDevice.h"

class GamePakInterface : public MemoryDevice
{
private:
	void* mRomData;
public:
	GamePakInterface(void* romData)
		: mRomData(romData)
	{ }
	void HandleRequest(MemoryBus* memoryBus);
};