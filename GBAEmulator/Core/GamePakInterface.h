#pragma once
#include "MemoryBus.h"
#include "MemoryDevice.h"

#define BACKUP_TYPE_NONE	0
#define BACKUP_TYPE_SRAM	1
#define BACKUP_TYPE_FLASH	2

#define FLASH_BACKUP_MODE_NORMAL	0
#define FLASH_BACKUP_MODE_ID		1
#define FLASH_BACKUP_MODE_ERASE		2
#define FLASH_BACKUP_MODE_WRITE		3
#define FLASH_BACKUP_MODE_BANK		4

class GamePakInterface : public MemoryDevice
{
private:
	void* mRomData;
	int mRomDataSize;

	int mBackupType;
	void* mBackupData;
	int mBackupDataSize;

	uint32_t mFlashBackupCommand;
	int mFlashBackupCommandState;
	int mFlashBackupMode;
	int mFlashBackupDataOffset;
public:
	GamePakInterface(void* romData, int size);
	~GamePakInterface()
	{
		if (mBackupData != NULL) free(mBackupData);
	}
	void HandleRequest(MemoryBus* memoryBus);
};