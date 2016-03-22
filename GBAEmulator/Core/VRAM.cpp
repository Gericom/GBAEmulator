#include "stdafx.h"
#include "VRAM.h"

void VRAM::HandleRequest(MemoryBus* memoryBus)
{
	//TODO: Implement waitstates and alignment later on
	if (memoryBus->mRequestAddress >= 0x06000000 && memoryBus->mRequestAddress < 0x06018000)
	{
		switch (memoryBus->mRequest)
		{
		case MEMORYBUS_REQUEST_READ_8:
			memoryBus->mRequestData = mMemory[memoryBus->mRequestAddress - 0x06000000];
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_READ_16:
			memoryBus->mRequestData = *((uint16_t*)&mMemory[memoryBus->mRequestAddress - 0x06000000]);
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_READ_32:
			memoryBus->mRequestData = *((uint32_t*)&mMemory[memoryBus->mRequestAddress - 0x06000000]);
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_WRITE_8:
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_WRITE_16:
			*((uint16_t*)&mMemory[memoryBus->mRequestAddress - 0x06000000]) = memoryBus->mRequestData & 0xFFFF;
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_WRITE_32:
			*((uint32_t*)&mMemory[memoryBus->mRequestAddress - 0x06000000]) = memoryBus->mRequestData & 0xFFFFFFFF;
			memoryBus->mRequestComplete = true;
			break;
		default:
			break;
		}
	}
	else
	{
		//What do I do here, this shouldn't happen however
		if ((memoryBus->mRequest == MEMORYBUS_REQUEST_READ_8 ||
			memoryBus->mRequest == MEMORYBUS_REQUEST_READ_16 ||
			memoryBus->mRequest == MEMORYBUS_REQUEST_READ_32))
		{
			memoryBus->mRequestData = 0;
			memoryBus->mRequestComplete = true;
			return;
		}
		memoryBus->mRequestComplete = true;
		return;
	}
}