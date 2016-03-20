#include "stdafx.h"
#include "MemoryDevice.h"
#include "MemoryBus.h"

void MemoryBus::RunCycle()
{
	if (!IsBusy()) return;
	if (mDevices[mRequestAddress >> 24])
		mDevices[mRequestAddress >> 24]->HandleRequest(this);
	else
	{
		OutputDebugString(L"Unknown Memory!");
	}
	if (mRequestComplete &&
		(mRequest == MEMORYBUS_REQUEST_WRITE_8 || mRequest == MEMORYBUS_REQUEST_WRITE_16 || mRequest == MEMORYBUS_REQUEST_WRITE_32))
		mBusy = false;
}

bool MemoryBus::Read8(uint32_t address)
{
	if (IsBusy()) return false;
	mRequest = MEMORYBUS_REQUEST_READ_8;
	mRequestAddress = address;
	mRequestComplete = false;
	mBusy = true;
	return true;
}

bool MemoryBus::Read16(uint32_t address)
{
	if (IsBusy()) return false;
	mRequest = MEMORYBUS_REQUEST_READ_16;
	mRequestAddress = address;
	mRequestComplete = false;
	mBusy = true;
	return true;
}

bool MemoryBus::Read32(uint32_t address)
{
	if (IsBusy()) return false;
	mRequest = MEMORYBUS_REQUEST_READ_32;
	mRequestAddress = address;
	mRequestComplete = false;
	mBusy = true;
	return true;
}

bool MemoryBus::Write8(uint32_t address, uint8_t data)
{
	if (IsBusy()) return false;
	mRequest = MEMORYBUS_REQUEST_WRITE_8;
	mRequestAddress = address;
	mRequestData = data;
	mRequestComplete = false;
	mBusy = true;
	return true;
}

bool MemoryBus::Write16(uint32_t address, uint16_t data)
{
	if (IsBusy()) return false;
	mRequest = MEMORYBUS_REQUEST_WRITE_16;
	mRequestAddress = address;
	mRequestData = data;
	mRequestComplete = false;
	mBusy = true;
	return true;
}

bool MemoryBus::Write32(uint32_t address, uint32_t data)
{
	if (IsBusy()) return false;
	mRequest = MEMORYBUS_REQUEST_WRITE_32;
	mRequestAddress = address;
	mRequestData = data;
	mRequestComplete = false;
	mBusy = true;
	return true;
}

uint32_t MemoryBus::GetReadResult()
{
	if (!mRequestComplete ||
		(mRequest != MEMORYBUS_REQUEST_READ_8 &&
			mRequest != MEMORYBUS_REQUEST_READ_16 &&
			mRequest != MEMORYBUS_REQUEST_READ_32)) return 0xFFFFFFFF;
	mBusy = false;
	return mRequestData;
}