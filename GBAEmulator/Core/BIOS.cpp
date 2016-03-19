#include "stdafx.h"
#include "BIOS.h"

void BIOS::HandleRequest(MemoryBus* memoryBus)
{
	//TODO: Implement waitstates and alignment later on
	if (memoryBus->mRequestAddress >= 0 && memoryBus->mRequestAddress < 0x4000)
	{
		switch (memoryBus->mRequest)
		{
		case MEMORYBUS_REQUEST_READ_8:
			memoryBus->mRequestData = ((uint8_t*)mBIOSData)[memoryBus->mRequestAddress];
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_READ_16:
			memoryBus->mRequestData = ((uint8_t*)mBIOSData)[memoryBus->mRequestAddress] | (((uint8_t*)mBIOSData)[memoryBus->mRequestAddress + 1] << 8);
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_READ_32:
			memoryBus->mRequestData = ((uint8_t*)mBIOSData)[memoryBus->mRequestAddress] | (((uint8_t*)mBIOSData)[memoryBus->mRequestAddress + 1] << 8) |
				(((uint8_t*)mBIOSData)[memoryBus->mRequestAddress + 2] << 16) | (((uint8_t*)mBIOSData)[memoryBus->mRequestAddress + 3] << 24);
			memoryBus->mRequestComplete = true;
			break;
		default:
			break;
		}
	}
	else
	{
		//What do I do here, this shouldn't happen however
	}
}