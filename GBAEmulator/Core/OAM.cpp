#include "stdafx.h"
#include "OAM.h"

void OAM::HandleRequest(MemoryBus* memoryBus)
{
	//TODO: Implement waitstates and alignment later on
	if (memoryBus->mRequestAddress >= 0x07000000 && memoryBus->mRequestAddress < 0x07000400)
	{
		switch (memoryBus->mRequest)
		{
		case MEMORYBUS_REQUEST_READ_8:
			memoryBus->mRequestData = mMemory[memoryBus->mRequestAddress - 0x07000000];
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_READ_16:
			memoryBus->mRequestData = mMemory[memoryBus->mRequestAddress - 0x07000000] | (mMemory[(memoryBus->mRequestAddress - 0x07000000) + 1] << 8);
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_READ_32:
			memoryBus->mRequestData = mMemory[memoryBus->mRequestAddress - 0x07000000] | (mMemory[(memoryBus->mRequestAddress - 0x07000000) + 1] << 8) |
				(mMemory[(memoryBus->mRequestAddress - 0x07000000) + 2] << 16) | (mMemory[(memoryBus->mRequestAddress - 0x07000000) + 3] << 24);
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_WRITE_8:
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_WRITE_16:
			mMemory[memoryBus->mRequestAddress - 0x07000000] = memoryBus->mRequestData & 0xFF;
			mMemory[(memoryBus->mRequestAddress - 0x07000000) + 1] = (memoryBus->mRequestData >> 8) & 0xFF;
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_WRITE_32:
			mMemory[memoryBus->mRequestAddress - 0x07000000] = memoryBus->mRequestData & 0xFF;
			mMemory[(memoryBus->mRequestAddress - 0x07000000) + 1] = (memoryBus->mRequestData >> 8) & 0xFF;
			mMemory[(memoryBus->mRequestAddress - 0x07000000) + 2] = (memoryBus->mRequestData >> 16) & 0xFF;
			mMemory[(memoryBus->mRequestAddress - 0x07000000) + 3] = (memoryBus->mRequestData >> 24) & 0xFF;
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