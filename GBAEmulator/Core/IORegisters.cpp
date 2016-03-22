#include "stdafx.h"
#include "IORegisters.h"

void IORegisters::HandleRequest(MemoryBus* memoryBus)
{
	//TODO: Implement waitstates and alignment later on
	if (memoryBus->mRequestAddress >= 0x04000000 && memoryBus->mRequestAddress < 0x04001000)//0x04000400)
	{
		switch (memoryBus->mRequest)
		{
		case MEMORYBUS_REQUEST_READ_8:
			memoryBus->mRequestData = mMemory[memoryBus->mRequestAddress - 0x04000000];
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_READ_16:
			memoryBus->mRequestData = *((uint16_t*)&mMemory[memoryBus->mRequestAddress - 0x04000000]);
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_READ_32:
			memoryBus->mRequestData = *((uint32_t*)&mMemory[memoryBus->mRequestAddress - 0x04000000]);
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_WRITE_8:
			if (memoryBus->mRequestAddress == 0x04000202)
			{
				mMemory[memoryBus->mRequestAddress - 0x04000000] &= (~memoryBus->mRequestData) & 0xFF;
			}
			else if (memoryBus->mRequestAddress == 0x04000301)
				mMemory[memoryBus->mRequestAddress - 0x04000000] = memoryBus->mRequestData & 0xFF;
			else mMemory[memoryBus->mRequestAddress - 0x04000000] = memoryBus->mRequestData & 0xFF;
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_WRITE_16:
			if (memoryBus->mRequestAddress == 0x04000202)
			{
				mMemory[memoryBus->mRequestAddress - 0x04000000] &= (~memoryBus->mRequestData) & 0xFF;
				mMemory[memoryBus->mRequestAddress - 0x04000000 + 1] &= ((~memoryBus->mRequestData) >> 8) & 0xFF;
			}
			else
			{
				*((uint16_t*)&mMemory[memoryBus->mRequestAddress - 0x04000000]) = memoryBus->mRequestData & 0xFFFF;
			}
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_WRITE_32:
			*((uint32_t*)&mMemory[memoryBus->mRequestAddress - 0x04000000]) = memoryBus->mRequestData & 0xFFFFFFFF;
			memoryBus->mRequestComplete = true;
			break;
		default:
			break;
		}
	}
	else
	{
		//What do I do here, this shouldn't happen however
		OutputDebugString(L"Unknown Memory!");
		memoryBus->mRequestData = 0;
		memoryBus->mRequestComplete = true;
		return;
	}
}