#include "stdafx.h"
#include "GamePakInterface.h"

void GamePakInterface::HandleRequest(MemoryBus* memoryBus)
{
	//TODO: Implement waitstates and alignment later on
	if (memoryBus->mRequestAddress < 0x0E000000)
	{
		switch (memoryBus->mRequest)
		{
		case MEMORYBUS_REQUEST_READ_8:
			memoryBus->mRequestData = ((uint8_t*)mRomData)[memoryBus->mRequestAddress - 0x08000000];
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_READ_16:
			memoryBus->mRequestData = ((uint8_t*)mRomData)[memoryBus->mRequestAddress - 0x08000000] | (((uint8_t*)mRomData)[memoryBus->mRequestAddress - 0x08000000 + 1] << 8);
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_READ_32:
			memoryBus->mRequestData = ((uint8_t*)mRomData)[memoryBus->mRequestAddress - 0x08000000] | (((uint8_t*)mRomData)[memoryBus->mRequestAddress - 0x08000000 + 1] << 8) | (((uint8_t*)mRomData)[memoryBus->mRequestAddress - 0x08000000 + 2] << 16) | (((uint8_t*)mRomData)[memoryBus->mRequestAddress - 0x08000000 + 3] << 24);
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_WRITE_8://not possible
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_WRITE_16:
			((uint8_t*)mRomData)[memoryBus->mRequestAddress - 0x08000000] = memoryBus->mRequestData & 0xFF;
			((uint8_t*)mRomData)[memoryBus->mRequestAddress - 0x08000000 + 1] = (memoryBus->mRequestData >> 8) & 0xFF;
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_WRITE_32:
			((uint8_t*)mRomData)[memoryBus->mRequestAddress - 0x08000000] = memoryBus->mRequestData & 0xFF;
			((uint8_t*)mRomData)[memoryBus->mRequestAddress - 0x08000000 + 1] = (memoryBus->mRequestData >> 8) & 0xFF;
			((uint8_t*)mRomData)[memoryBus->mRequestAddress - 0x08000000 + 2] = (memoryBus->mRequestData >> 16) & 0xFF;
			((uint8_t*)mRomData)[memoryBus->mRequestAddress - 0x08000000 + 3] = (memoryBus->mRequestData >> 24) & 0xFF;
			memoryBus->mRequestComplete = true;
			break;
		default:
			break;
		}
	}
	else
	{
		//TODO
	}
}