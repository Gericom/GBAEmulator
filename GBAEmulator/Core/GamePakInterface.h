#pragma once
#include "MemoryBus.h"
#include "MemoryDevice.h"

class GamePakInterface : public MemoryDevice
{
private:
	void* mRomData;
	int mRomDataSize;
public:
	GamePakInterface(void* romData, int size)
		: mRomData(romData), mRomDataSize(size)
	{ }
	void HandleRequest(MemoryBus* memoryBus);
};