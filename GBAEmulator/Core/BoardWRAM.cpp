#include "stdafx.h"
#include "BoardWRAM.h"

void BoardWRAM::HandleRequest(MemoryBus* memoryBus)
{
	//TODO: Implement waitstates and alignment later on
	//if (memoryBus->mRequestAddress >= 0x03000000 && memoryBus->mRequestAddress < 0x03008000)
	//{
	switch (memoryBus->mRequest)
	{
	case MEMORYBUS_REQUEST_READ_8:
		memoryBus->mRequestData = mMemory[memoryBus->mRequestAddress & 0x3FFFF];
		memoryBus->mRequestComplete = true;
		break;
	case MEMORYBUS_REQUEST_READ_16:
		memoryBus->mRequestData = *((uint16_t*)&mMemory[memoryBus->mRequestAddress & 0x3FFFF]);
		memoryBus->mRequestComplete = true;
		break;
	case MEMORYBUS_REQUEST_READ_32:
		memoryBus->mRequestData = *((uint32_t*)&mMemory[memoryBus->mRequestAddress & 0x3FFFF]);
		memoryBus->mRequestComplete = true;
		break;
	case MEMORYBUS_REQUEST_WRITE_8:
		mMemory[memoryBus->mRequestAddress & 0x3FFFF] = memoryBus->mRequestData & 0xFF;
		memoryBus->mRequestComplete = true;
		break;
	case MEMORYBUS_REQUEST_WRITE_16:
		*((uint16_t*)&mMemory[memoryBus->mRequestAddress & 0x3FFFF]) = memoryBus->mRequestData & 0xFFFF;
		memoryBus->mRequestComplete = true;
		break;
	case MEMORYBUS_REQUEST_WRITE_32:
		*((uint32_t*)&mMemory[memoryBus->mRequestAddress & 0x3FFFF]) = memoryBus->mRequestData & 0xFFFFFFFF;
		memoryBus->mRequestComplete = true;
		break;
	default:
		break;
	}
	//}
	//else
	//{
	//What do I do here, this shouldn't happen however
	//}
}