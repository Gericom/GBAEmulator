#include "stdafx.h"
#include "GamePakInterface.h"

GamePakInterface::GamePakInterface(void* romData, int size)
	: mRomData(romData), mRomDataSize(size), mFlashBackupCommand(0), mFlashBackupCommandState(0), mFlashBackupMode(FLASH_BACKUP_MODE_NORMAL), mFlashBackupDataOffset(0)
{
	mBackupType = BACKUP_TYPE_FLASH;
	mBackupDataSize = 128 * 1024;
	//mBackupType = BACKUP_TYPE_SRAM;
	//mBackupDataSize = 32 * 1024;
	mBackupData = malloc(mBackupDataSize);
	ZeroMemory(mBackupData, mBackupDataSize);
}

void GamePakInterface::HandleRequest(MemoryBus* memoryBus)
{
	//TODO: Implement waitstates and alignment later on
	if (memoryBus->mRequestAddress < 0x0E000000)
	{
		if (memoryBus->mRequestAddress >= 0x0C000000)
			memoryBus->mRequestAddress -= 0x04000000;
		else if (memoryBus->mRequestAddress >= 0x0A000000)
			memoryBus->mRequestAddress -= 0x02000000;
		if (//(memoryBus->mRequest == MEMORYBUS_REQUEST_READ_8 ||
			//	memoryBus->mRequest == MEMORYBUS_REQUEST_READ_16 ||
			//	memoryBus->mRequest == MEMORYBUS_REQUEST_READ_32) &&
			(memoryBus->mRequestAddress - 0x08000000) >= mRomDataSize)
		{
			memoryBus->mRequestData = 0;
			memoryBus->mRequestComplete = true;
			return;
		}
		switch (memoryBus->mRequest)
		{
		case MEMORYBUS_REQUEST_READ_8:
			memoryBus->mRequestData = ((uint8_t*)mRomData)[memoryBus->mRequestAddress - 0x08000000];
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_READ_16:
			memoryBus->mRequestData = *((uint16_t*)&((uint8_t*)mRomData)[memoryBus->mRequestAddress - 0x08000000]);
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_READ_32:
			memoryBus->mRequestData = *((uint32_t*)&((uint8_t*)mRomData)[memoryBus->mRequestAddress - 0x08000000]);
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_WRITE_8://not possible
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_WRITE_16:
			*((uint16_t*)&((uint8_t*)mRomData)[memoryBus->mRequestAddress - 0x08000000]) = memoryBus->mRequestData & 0xFFFF;
			memoryBus->mRequestComplete = true;
			break;
		case MEMORYBUS_REQUEST_WRITE_32:
			*((uint32_t*)&((uint8_t*)mRomData)[memoryBus->mRequestAddress - 0x08000000]) = memoryBus->mRequestData & 0xFFFFFFFF;
			memoryBus->mRequestComplete = true;
			break;
		default:
			break;
		}
	}
	else
	{
		if (mBackupType == BACKUP_TYPE_FLASH)
		{
			if (memoryBus->mRequest == MEMORYBUS_REQUEST_WRITE_8)
			{
				if (mFlashBackupMode == FLASH_BACKUP_MODE_WRITE)
				{
					((uint8_t*)mBackupData)[(memoryBus->mRequestAddress & 0xFFFF) + mFlashBackupDataOffset] = memoryBus->mRequestData & 0xFF;
					mFlashBackupMode = FLASH_BACKUP_MODE_NORMAL;
				}
				if (mFlashBackupMode == FLASH_BACKUP_MODE_BANK && memoryBus->mRequestAddress == 0xE000000)
				{
					if (memoryBus->mRequestData == 0xF0)
					{
						wprintf(L"LOL!");
					}
					mFlashBackupDataOffset = (memoryBus->mRequestData & 0xFF) * 64 * 1024;
					mFlashBackupMode = FLASH_BACKUP_MODE_NORMAL;
				}
				else if (mFlashBackupCommandState == 0 && memoryBus->mRequestAddress == 0xE005555)
				{
					mFlashBackupCommand = (memoryBus->mRequestData & 0xFF) << 16;
					mFlashBackupCommandState = 1;
				}
				else if (mFlashBackupCommandState == 1 && memoryBus->mRequestAddress == 0xE002AAA)
				{
					mFlashBackupCommand |= (memoryBus->mRequestData & 0xFF) << 8;
					mFlashBackupCommandState = 2;
				}
				else if (mFlashBackupCommandState == 2 && memoryBus->mRequestAddress == 0xE005555)
				{
					mFlashBackupCommand |= memoryBus->mRequestData & 0xFF;
					if (mFlashBackupCommand == 0xAA5590)
						mFlashBackupMode = FLASH_BACKUP_MODE_ID;
					else if (mFlashBackupCommand == 0xAA55F0)
						mFlashBackupMode = FLASH_BACKUP_MODE_NORMAL;
					else if (mFlashBackupCommand == 0xAA5580)
						mFlashBackupMode = FLASH_BACKUP_MODE_ERASE;
					else if (mFlashBackupCommand == 0xAA55A0)
						mFlashBackupMode = FLASH_BACKUP_MODE_WRITE;
					else if (mFlashBackupCommand == 0xAA55B0)
						mFlashBackupMode = FLASH_BACKUP_MODE_BANK;
					else if (mFlashBackupMode == FLASH_BACKUP_MODE_ERASE && mFlashBackupCommand == 0xAA5510)
					{
						FillMemory(mBackupData, mBackupDataSize, 0xFF);
						mFlashBackupMode = FLASH_BACKUP_MODE_NORMAL;
					}
					mFlashBackupCommand = 0;
					mFlashBackupCommandState = 0;
				}
				else if (mFlashBackupMode == FLASH_BACKUP_MODE_ERASE && mFlashBackupCommandState == 2 && (memoryBus->mRequestData & 0xFF) == 0x30)
				{
					FillMemory(((uint8_t*)mBackupData) + (memoryBus->mRequestAddress & 0xFFFF) + mFlashBackupDataOffset, 4 * 1024, 0xFF);
					//mFlashBackupMode = FLASH_BACKUP_MODE_NORMAL;
					mFlashBackupCommand = 0;
					mFlashBackupCommandState = 0;
				}
			}
			else if (memoryBus->mRequest == MEMORYBUS_REQUEST_READ_8)
			{
				if (mFlashBackupMode == FLASH_BACKUP_MODE_ID && memoryBus->mRequestAddress == 0xE000000)
				{
					memoryBus->mRequestData = 0xC2;
				}
				else if (mFlashBackupMode == FLASH_BACKUP_MODE_ID && memoryBus->mRequestAddress == 0xE000001)
				{
					memoryBus->mRequestData = 0x09;
				}
				else if (mFlashBackupMode != FLASH_BACKUP_MODE_ID)
				{
					memoryBus->mRequestData = ((uint8_t*)mBackupData)[(memoryBus->mRequestAddress & 0xFFFF) + mFlashBackupDataOffset];
				}
			}
			memoryBus->mRequestComplete = true;
			return;
		}
		else if (mBackupType == BACKUP_TYPE_SRAM)
		{
			if (memoryBus->mRequest == MEMORYBUS_REQUEST_READ_8)
				memoryBus->mRequestData = ((uint8_t*)mBackupData)[memoryBus->mRequestAddress & 0xFFFF];
			else if (memoryBus->mRequest == MEMORYBUS_REQUEST_WRITE_8)
				((uint8_t*)mBackupData)[memoryBus->mRequestAddress & 0xFFFF] = memoryBus->mRequestData & 0xFF;
			memoryBus->mRequestComplete = true;
			return;
		}
		memoryBus->mRequestData = 0;
		memoryBus->mRequestComplete = true;
		return;
	}
}