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
			memoryBus->mRequestData = *((uint16_t*)&((uint8_t*)mBIOSData)[memoryBus->mRequestAddress]);
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_READ_32:
			memoryBus->mRequestData = *((uint32_t*)&((uint8_t*)mBIOSData)[memoryBus->mRequestAddress]);
			memoryBus->mRequestComplete = true;
			break;
		default:
			memoryBus->mRequestData = 0;
			memoryBus->mRequestComplete = true;
			break;
		}
	}
	else
	{
		//What do I do here, this shouldn't happen however
		OutputDebugString(L"Unknown Memory!");
		memoryBus->mRequestData = 0;
		memoryBus->mRequestComplete = true;
	}
}