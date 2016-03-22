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