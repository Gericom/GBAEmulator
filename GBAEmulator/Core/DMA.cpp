#include "stdafx.h"
#include "IORegisters.h"
#include "DMA.h"

void DMA::RunCycle()
{
	uint16_t dma0 = *((uint16_t*)&mIORegisters->mMemory[0xBA]);
	uint16_t dma1 = *((uint16_t*)&mIORegisters->mMemory[0xC6]);
	uint16_t dma2 = *((uint16_t*)&mIORegisters->mMemory[0xD2]);
	uint16_t dma3 = *((uint16_t*)&mIORegisters->mMemory[0xDE]);

	if (dma0 >> 15 && mDMAChannels[0].busy && mDMAChannels[0].waitingForData && mMemoryBus->IsDataReady())
	{
		mDMAChannels[0].waitingForData = false;
		uint32_t data = mMemoryBus->GetReadResult();
		if (mDMAChannels[0].bitMode) mMemoryBus->Write32(mDMAChannels[0].dst, data);
		else mMemoryBus->Write16(mDMAChannels[0].dst, data & 0xFFFF);
		if (!mDMAChannels[0].dstMode || mDMAChannels[0].dstMode == 3) mDMAChannels[0].dst += (mDMAChannels[0].bitMode ? 4 : 2);
		else if (mDMAChannels[0].dstMode == 1) mDMAChannels[0].dst -= (mDMAChannels[0].bitMode ? 4 : 2);
		mDMAChannels[0].nrLeft--;
		if (mDMAChannels[0].nrLeft == 0)
		{
			mDMAChannels[0].busy = false;
			if (!((dma0 >> 9) & 1))
				*((uint16_t*)&mIORegisters->mMemory[0xBA]) &= ~0x8000;
		}
	}
	if (dma1 >> 15 && mDMAChannels[1].busy && mDMAChannels[1].waitingForData && mMemoryBus->IsDataReady())
	{
		mDMAChannels[1].waitingForData = false;
		uint32_t data = mMemoryBus->GetReadResult();
		if (mDMAChannels[1].bitMode) mMemoryBus->Write32(mDMAChannels[1].dst, data);
		else mMemoryBus->Write16(mDMAChannels[1].dst, data & 0xFFFF);
		if (!mDMAChannels[1].dstMode || mDMAChannels[1].dstMode == 3) mDMAChannels[1].dst += (mDMAChannels[1].bitMode ? 4 : 2);
		else if (mDMAChannels[1].dstMode == 1) mDMAChannels[1].dst -= (mDMAChannels[1].bitMode ? 4 : 2);
		mDMAChannels[1].nrLeft--;
		if (mDMAChannels[1].nrLeft == 0)
		{
			mDMAChannels[1].busy = false;
			if (!((dma1 >> 9) & 1))
				*((uint16_t*)&mIORegisters->mMemory[0xC6]) &= ~0x8000;
		}
	}
	if (dma2 >> 15 && mDMAChannels[2].busy && mDMAChannels[2].waitingForData && mMemoryBus->IsDataReady())
	{
		mDMAChannels[2].waitingForData = false;
		uint32_t data = mMemoryBus->GetReadResult();
		if (mDMAChannels[2].bitMode) mMemoryBus->Write32(mDMAChannels[2].dst, data);
		else mMemoryBus->Write16(mDMAChannels[2].dst, data & 0xFFFF);
		if (!mDMAChannels[2].dstMode || mDMAChannels[2].dstMode == 3) mDMAChannels[2].dst += (mDMAChannels[2].bitMode ? 4 : 2);
		else if (mDMAChannels[2].dstMode == 1) mDMAChannels[2].dst -= (mDMAChannels[2].bitMode ? 4 : 2);
		mDMAChannels[2].nrLeft--;
		if (mDMAChannels[2].nrLeft == 0)
		{
			mDMAChannels[2].busy = false;
			if (!((dma2 >> 9) & 1))
				*((uint16_t*)&mIORegisters->mMemory[0xD2]) &= ~0x8000;
		}
	}
	if (dma3 >> 15 && mDMAChannels[3].busy && mDMAChannels[3].waitingForData && mMemoryBus->IsDataReady())
	{
		mDMAChannels[3].waitingForData = false;
		uint32_t data = mMemoryBus->GetReadResult();
		if (mDMAChannels[3].bitMode) mMemoryBus->Write32(mDMAChannels[3].dst, data);
		else mMemoryBus->Write16(mDMAChannels[3].dst, data & 0xFFFF);
		if (!mDMAChannels[3].dstMode || mDMAChannels[3].dstMode == 3) mDMAChannels[3].dst += (mDMAChannels[3].bitMode ? 4 : 2);
		else if (mDMAChannels[3].dstMode == 1) mDMAChannels[3].dst -= (mDMAChannels[3].bitMode ? 4 : 2);
		mDMAChannels[3].nrLeft--;
		if (mDMAChannels[3].nrLeft == 0)
		{
			mDMAChannels[3].busy = false;
			if (!((dma3 >> 9) & 1))
				*((uint16_t*)&mIORegisters->mMemory[0xDE]) &= ~0x8000;
		}
	}

	dma0 = *((uint16_t*)&mIORegisters->mMemory[0xBA]);
	dma1 = *((uint16_t*)&mIORegisters->mMemory[0xC6]);
	dma2 = *((uint16_t*)&mIORegisters->mMemory[0xD2]);
	dma3 = *((uint16_t*)&mIORegisters->mMemory[0xDE]);

	if (dma0 >> 15)
	{
		if (!mDMAChannels[0].busy &&
			(
				((dma0 >> 12) & 3) == 0 ||
				(((dma0 >> 12) & 3) == 1 && !lastVBlankFlag && (*((uint16_t*)&mIORegisters->mMemory[4]) & 1)) ||
				(((dma0 >> 12) & 3) == 2 && !lastHBlankFlag && (*((uint16_t*)&mIORegisters->mMemory[4]) & 2))))
		{
			mDMAChannels[0].src = *((uint32_t*)&mIORegisters->mMemory[0xB0]);
			mDMAChannels[0].srcMode = (dma0 >> 7) & 0;
			mDMAChannels[0].dst = *((uint32_t*)&mIORegisters->mMemory[0xB4]);
			mDMAChannels[0].dstMode = (dma0 >> 5) & 0;
			mDMAChannels[0].bitMode = (dma0 >> 10) & 1;
			mDMAChannels[0].nrLeft = *((uint16_t*)&mIORegisters->mMemory[0xB8]);
			if (!mDMAChannels[0].nrLeft) mDMAChannels[0].nrLeft = 0x4000;
			mDMAChannels[0].busy = true;
		}
		if (mDMAChannels[0].busy && mDMAChannels[0].nrLeft > 0 && !mMemoryBus->IsBusy())
		{
			if (mDMAChannels[0].bitMode) mMemoryBus->Read32(mDMAChannels[0].src);
			else  mMemoryBus->Read16(mDMAChannels[0].src);
			if (!mDMAChannels[0].srcMode) mDMAChannels[0].src += (mDMAChannels[0].bitMode ? 4 : 2);
			else if (mDMAChannels[0].srcMode == 1) mDMAChannels[0].src -= (mDMAChannels[0].bitMode ? 4 : 2);
			mDMAChannels[0].waitingForData = true;
		}
	}
	if (dma1 >> 15)
	{
		if (!mDMAChannels[1].busy &&
			(
				((dma1 >> 12) & 3) == 0 ||
				(((dma1 >> 12) & 3) == 1 && !lastVBlankFlag && (*((uint16_t*)&mIORegisters->mMemory[4]) & 1)) ||
				(((dma1 >> 12) & 3) == 2 && !lastHBlankFlag && (*((uint16_t*)&mIORegisters->mMemory[4]) & 2))))
		{
			mDMAChannels[1].src = *((uint32_t*)&mIORegisters->mMemory[0xBC]);
			mDMAChannels[1].srcMode = (dma1 >> 7) & 0;
			mDMAChannels[1].dst = *((uint32_t*)&mIORegisters->mMemory[0xC0]);
			mDMAChannels[1].dstMode = (dma1 >> 5) & 0;
			mDMAChannels[1].bitMode = (dma1 >> 10) & 1;
			mDMAChannels[1].nrLeft = *((uint16_t*)&mIORegisters->mMemory[0xC4]);
			if (!mDMAChannels[1].nrLeft) mDMAChannels[1].nrLeft = 0x4000;
			mDMAChannels[1].busy = true;
		}
		if (mDMAChannels[1].busy && mDMAChannels[1].nrLeft > 0 && !mMemoryBus->IsBusy())
		{
			if (mDMAChannels[1].bitMode) mMemoryBus->Read32(mDMAChannels[1].src);
			else  mMemoryBus->Read16(mDMAChannels[1].src);
			if (!mDMAChannels[1].srcMode) mDMAChannels[1].src += (mDMAChannels[1].bitMode ? 4 : 2);
			else if (mDMAChannels[1].srcMode == 1) mDMAChannels[1].src -= (mDMAChannels[1].bitMode ? 4 : 2);
			mDMAChannels[1].waitingForData = true;
		}
	}
	if (dma2 >> 15)
	{
		if (!mDMAChannels[2].busy &&
			(
				((dma2 >> 12) & 3) == 0 ||
				(((dma2 >> 12) & 3) == 1 && !lastVBlankFlag && (*((uint16_t*)&mIORegisters->mMemory[4]) & 1)) ||
				(((dma2 >> 12) & 3) == 2 && !lastHBlankFlag && (*((uint16_t*)&mIORegisters->mMemory[4]) & 2))))
		{
			mDMAChannels[2].src = *((uint32_t*)&mIORegisters->mMemory[0xC8]);
			mDMAChannels[2].srcMode = (dma2 >> 7) & 0;
			mDMAChannels[2].dst = *((uint32_t*)&mIORegisters->mMemory[0xCC]);
			mDMAChannels[2].dstMode = (dma2 >> 5) & 0;
			mDMAChannels[2].bitMode = (dma2 >> 10) & 1;
			mDMAChannels[2].nrLeft = *((uint16_t*)&mIORegisters->mMemory[0xD0]);
			if (!mDMAChannels[2].nrLeft) mDMAChannels[2].nrLeft = 0x4000;
			mDMAChannels[2].busy = true;
		}
		if (mDMAChannels[2].busy && mDMAChannels[2].nrLeft > 0 && !mMemoryBus->IsBusy())
		{
			if (mDMAChannels[2].bitMode) mMemoryBus->Read32(mDMAChannels[2].src);
			else  mMemoryBus->Read16(mDMAChannels[2].src);
			if (!mDMAChannels[2].srcMode || mDMAChannels[2].dstMode == 3) mDMAChannels[2].src += (mDMAChannels[2].bitMode ? 4 : 2);
			else if (mDMAChannels[2].srcMode == 1) mDMAChannels[2].src -= (mDMAChannels[2].bitMode ? 4 : 2);
			mDMAChannels[2].waitingForData = true;
		}
	}
	if (dma3 >> 15)
	{
		if (!mDMAChannels[3].busy &&
			(
				((dma3 >> 12) & 3) == 0 ||
				(((dma3 >> 12) & 3) == 1 && !lastVBlankFlag && (*((uint16_t*)&mIORegisters->mMemory[4]) & 1)) ||
				(((dma3 >> 12) & 3) == 2 && !lastHBlankFlag && (*((uint16_t*)&mIORegisters->mMemory[4]) & 2))))
		{
			mDMAChannels[3].src = *((uint32_t*)&mIORegisters->mMemory[0xD4]);		
			mDMAChannels[3].srcMode = (dma3 >> 7) & 3;
			mDMAChannels[3].dst = *((uint32_t*)&mIORegisters->mMemory[0xD8]);
			mDMAChannels[3].dstMode = (dma3 >> 5) & 3;
			mDMAChannels[3].bitMode = (dma3 >> 10) & 1;
			mDMAChannels[3].nrLeft = *((uint16_t*)&mIORegisters->mMemory[0xDC]);
			if (!mDMAChannels[3].nrLeft) mDMAChannels[3].nrLeft = 0x10000;
			mDMAChannels[3].busy = true;
		}
		if (mDMAChannels[3].busy && mDMAChannels[3].nrLeft > 0 && !mMemoryBus->IsBusy())
		{
			if (mDMAChannels[3].bitMode) mMemoryBus->Read32(mDMAChannels[3].src);
			else  mMemoryBus->Read16(mDMAChannels[3].src);
			if (!mDMAChannels[3].srcMode) mDMAChannels[3].src += (mDMAChannels[3].bitMode ? 4 : 2);
			else if (mDMAChannels[3].srcMode == 1) mDMAChannels[3].src -= (mDMAChannels[3].bitMode ? 4 : 2);
			mDMAChannels[3].waitingForData = true;
		}
	}
	lastVBlankFlag = *((uint16_t*)&mIORegisters->mMemory[4]) & 1;
	lastHBlankFlag = (*((uint16_t*)&mIORegisters->mMemory[4]) >> 1) & 1;
}