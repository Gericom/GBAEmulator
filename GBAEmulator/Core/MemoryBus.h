#pragma once
#include "ClockBlock.h"

#define MEMORYBUS_REQUEST_NONE		0
#define MEMORYBUS_REQUEST_READ_8	1
#define MEMORYBUS_REQUEST_READ_16	2
#define MEMORYBUS_REQUEST_READ_32	3
#define MEMORYBUS_REQUEST_WRITE_8	4
#define MEMORYBUS_REQUEST_WRITE_16	5
#define MEMORYBUS_REQUEST_WRITE_32	6

class MemoryDevice;

class MemoryBus : ClockBlock
{
	friend class GamePakInterface;
	friend class WRAM;
	friend class IORegisters;
	friend class BIOS;
private:
	MemoryDevice* mDevices[256];

	bool mBusy;
	uint32_t mRequest;
	uint32_t mRequestAddress;
	uint32_t mRequestData;
	bool mRequestComplete;//Mainly for reading purposes
public:
	MemoryBus()
		: mBusy(false), mRequest(MEMORYBUS_REQUEST_NONE)
	{
		ZeroMemory(&mDevices[0], sizeof(mDevices));
	}

	void RunCycle();

	bool IsBusy() { return mBusy; }

	bool IsDataReady()
	{
		return mRequestComplete &&
			(mRequest == MEMORYBUS_REQUEST_READ_8 ||
				mRequest == MEMORYBUS_REQUEST_READ_16 ||
				mRequest == MEMORYBUS_REQUEST_READ_32);
	}

	bool Read8(uint32_t address);
	bool Read16(uint32_t address);
	bool Read32(uint32_t address);
	bool Write8(uint32_t address, uint8_t data);
	bool Write16(uint32_t address, uint16_t data);
	bool Write32(uint32_t address, uint32_t data);

	uint32_t GetReadResult();

	void SetDevice(MemoryDevice* device, int id)
	{
		mDevices[id] = device;
	}
};