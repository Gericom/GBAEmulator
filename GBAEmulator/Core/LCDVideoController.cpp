#include "stdafx.h"
#include "ARM7TDMI.h"
#include "IORegisters.h"
#include "PalRam.h"
#include "VRAM.h"
#include "OAM.h"
#include "LCDVideoController.h"

static int oamWidth[16] =
{
	8, 16, 32, 64,
	16, 32, 32, 64,
	8, 8, 16, 32,
	0, 0, 0, 0
};

static int oamHeight[16] =
{
	8, 16, 32, 64,
	8, 8, 16, 32,
	16, 32, 32, 64,
	0, 0, 0, 0
};


void LCDVideoController::RunCycle()
{
	if (!(mClockCounter % 4))//One dot takes 4 cycles
	{
		if (mY >= 160 && mY != 227) *((uint16_t*)&mIORegisters->mMemory[4]) |= 1;
		else *((uint16_t*)&mIORegisters->mMemory[4]) &= ~1;
		if (mX >= 240) *((uint16_t*)&mIORegisters->mMemory[4]) |= 2;
		else *((uint16_t*)&mIORegisters->mMemory[4]) &= ~2;

		if (mY == 160 && mX == 0 && *((uint16_t*)&mIORegisters->mMemory[4]) & 8 && *((uint16_t*)&mIORegisters->mMemory[0x200]) & 1 && *((uint16_t*)&mIORegisters->mMemory[0x208]) & 1)
		{
			*((uint16_t*)&mIORegisters->mMemory[0x202]) |= 1;
			mProcessor->RequestIRQ();
		}
		
		*((uint16_t*)&mIORegisters->mMemory[6]) = mY;
		if (mX < 240 && mY < 160)//visible dots
		{
			int bgmode = *((uint16_t*)&mIORegisters->mMemory[0]) & 7;
			GXRgb pixel = ((GXRgb*)&mPalRam->mMemory[0])[0];
			//oam
			if ((*((uint16_t*)&mIORegisters->mMemory[0]) >> 12) & 1)
			{
				int mappingmode = (*((uint16_t*)&mIORegisters->mMemory[0]) >> 6) & 1;
				int oamtileoffset = (bgmode <= 2 ? 0x10000 : 0x14000);
				GXOamAttr* pOAM = (GXOamAttr*)&mOAM->mMemory[0];
				for (int i = 0; i < 128; i++)
				{
					if (pOAM->rsMode != 2)
					{
						if (pOAM->rsMode & 1)//affine
						{
							wprintf(L"LOL!");
						}
						else
						{
							int x1 = pOAM->x;
							int y1 = pOAM->y;
							int w = oamWidth[pOAM->size | (pOAM->shape << 2)];
							int h = oamHeight[pOAM->size | (pOAM->shape << 2)];
							int x2 = x1 + w;
							int y2 = y1 + h;
							if (x1 <= mX && x2 > mX && y1 <= mY && y2 > mY)
							{
								int dx = mX - x1;
								int dy = mY - y1;
								int tilex = dx >> 3;
								int tiley = dy >> 3;
								int tilesize = (pOAM->colorMode ? 64 : 32);
								if (!mappingmode)//2d mapping
								{
									int tilestart = oamtileoffset + pOAM->charNo * 32 + tiley * 32 * 32 +tilex * tilesize;
									int tsx = dx - (tilex << 3);
									int tsy = dy - (tiley << 3);
									tilestart += tsy * (tilesize >> 3);
									tilestart += (tsx * tilesize) >> 6;
									int color = 0;
									if (!pOAM->colorMode)
									{
										uint8_t val = mVRAM->mMemory[tilestart];
										if (tsx & 1) val >>= 1;
										else val &= 0xF;
										color = val + pOAM->cParam * 16;
									}
									else
									{
										color = mVRAM->mMemory[tilestart];
									}
									if(color > 0)
										pixel = ((GXRgb*)&mPalRam->mMemory[0x200])[color];
								}
							}
						}
					}
					pOAM++;//rot/scale stuff
				}
			}
			//blit to screen
			SetPixel(mHDC, mX, mY, RGB(pixel.r * 255 / 31, pixel.g * 255 / 31, pixel.b * 255 / 31));
		}
		mX++;
		if (mX == 308)
		{
			mX = 0;
			mY++;
			if (mY == 228)
				mY = 0;
		}
	}
	mClockCounter = (mClockCounter + 1) % 4;
}