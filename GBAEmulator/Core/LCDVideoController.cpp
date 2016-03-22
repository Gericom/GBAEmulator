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

		if (mX == 240 && *((uint16_t*)&mIORegisters->mMemory[4]) & 16)
			*((uint16_t*)&mIORegisters->mMemory[0x202]) |= 2;
		if (mY == (*((uint16_t*)&mIORegisters->mMemory[4]) >> 8) && mX == 0 && *((uint16_t*)&mIORegisters->mMemory[4]) & 32)
			*((uint16_t*)&mIORegisters->mMemory[0x202]) |= 4;
		if (mY == 160 && mX == 0 && *((uint16_t*)&mIORegisters->mMemory[4]) & 8)
		{
			*((uint16_t*)&mIORegisters->mMemory[0x202]) |= 1;
			//mProcessor->RequestIRQ();
			SetDIBitsToDevice(mHDC, 0, 0, 240, 160, 0, 0, 0, 160, mFrameBuffer, mDIB, DIB_RGB_COLORS);
		}

		*((uint16_t*)&mIORegisters->mMemory[6]) = mY;
		if (mX < 240 && mY < 160)//visible dots
		{
			int bgmode = *((uint16_t*)&mIORegisters->mMemory[0]) & 7;
			int coloreffect = (*((uint16_t*)&mIORegisters->mMemory[0x50]) >> 6) & 3;
			int color1sttarget = *((uint16_t*)&mIORegisters->mMemory[0x50]) & 31;
			GXRgb /*backdrop*/pixel = ((GXRgb*)&mPalRam->mMemory[0])[0];
			if (coloreffect == 2 && color1sttarget & 0x20)
			{
				pixel.r = pixel.r + (31 - pixel.r) * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
				pixel.g = pixel.g + (31 - pixel.g) * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
				pixel.b = pixel.b + (31 - pixel.b) * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
			}
			else if (coloreffect == 3 && color1sttarget & 0x20)
			{
				pixel.r = pixel.r - pixel.r * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
				pixel.g = pixel.g - pixel.g * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
				pixel.b = pixel.b - pixel.b * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
			}
			for (int prio = 3; prio >= 0; prio--)
			{
				if ((*((uint16_t*)&mIORegisters->mMemory[0]) >> 11) & 1 && (*((uint16_t*)&mIORegisters->mMemory[0xE]) & 3) == prio)//bg3
				{
					if (bgmode == 0)
					{
						int w = ((*((uint16_t*)&mIORegisters->mMemory[0xE]) >> 14) & 1) ? 512 : 256;
						int h = ((*((uint16_t*)&mIORegisters->mMemory[0xE]) >> 15) & 1) ? 512 : 256;
						int tilesize = ((*((uint16_t*)&mIORegisters->mMemory[0xE]) >> 7) & 1) ? 64 : 32;
						int tileoffset = ((*((uint16_t*)&mIORegisters->mMemory[0xE]) >> 2) & 3) * 1024 * 16;
						int mapoffset = ((*((uint16_t*)&mIORegisters->mMemory[0xE]) >> 8) & 31) * 1024 * 2;
						int x1 = 0;// -(*((uint16_t*)&mIORegisters->mMemory[0x1C]) & 0x1FF);
						int y1 = 0;// -(*((uint16_t*)&mIORegisters->mMemory[0x1E]) & 0x1FF);
						int x2 = 240;// x1 + w;
						int y2 = 160;// y1 + h;
						if (x1 <= mX && x2 > mX && y1 <= mY && y2 > mY)
						{
							int dx = (mX - x1 + (*((uint16_t*)&mIORegisters->mMemory[0x1C]) & 0x1FF)) % w;
							int dy = (mY - y1 + (*((uint16_t*)&mIORegisters->mMemory[0x1E]) & 0x1FF)) % h;
							int tilex = dx >> 3;
							int tiley = dy >> 3;
							uint16_t text = *((uint16_t*)&mVRAM->mMemory[mapoffset + (tiley * 32 + tilex) * 2]);
							int tilestart = tileoffset + (text & 0x3FF) * tilesize;
							int tsx = dx - (tilex << 3);
							if ((text >> 10) & 1) tsx = 7 - tsx;
							int tsy = dy - (tiley << 3);
							if ((text >> 11) & 1) tsy = 7 - tsy;
							tilestart += tsy * (tilesize >> 3);
							tilestart += (tsx * tilesize) >> 6;
							int color = 0;
							if (!((*((uint16_t*)&mIORegisters->mMemory[0xE]) >> 7) & 1))
							{
								uint8_t val = mVRAM->mMemory[tilestart];
								if (tsx & 1) val >>= 4;
								else val &= 0xF;
								color = val + (text >> 12) * 16;
							}
							else
							{
								color = mVRAM->mMemory[tilestart];
							}
							if ((!((*((uint16_t*)&mIORegisters->mMemory[0xE]) >> 7) & 1) && color % 16) ||
								(((*((uint16_t*)&mIORegisters->mMemory[0xE]) >> 7) & 1) && color % 256))
							{
								pixel = ((GXRgb*)&mPalRam->mMemory[0])[color];
								if (coloreffect == 2 && color1sttarget & 0x8)
								{
									pixel.r = pixel.r + (31 - pixel.r) * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
									pixel.g = pixel.g + (31 - pixel.g) * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
									pixel.b = pixel.b + (31 - pixel.b) * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
								}
								else if (coloreffect == 3 && color1sttarget & 0x8)
								{
									pixel.r = pixel.r - pixel.r * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
									pixel.g = pixel.g - pixel.g * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
									pixel.b = pixel.b - pixel.b * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
								}
							}
						}
					}
				}
				if ((*((uint16_t*)&mIORegisters->mMemory[0]) >> 10) & 1 && (*((uint16_t*)&mIORegisters->mMemory[0xC]) & 3) == prio)//bg2
				{
					//if (bgmode == 0)
					//{
					int w = ((*((uint16_t*)&mIORegisters->mMemory[0xC]) >> 14) & 1) ? 512 : 256;
					int h = ((*((uint16_t*)&mIORegisters->mMemory[0xC]) >> 15) & 1) ? 512 : 256;
					if (bgmode >= 1)
					{
						w = h = 128 << ((*((uint16_t*)&mIORegisters->mMemory[0xC]) >> 14) & 3);
					}
					int tilesize = ((*((uint16_t*)&mIORegisters->mMemory[0xC]) >> 7) & 1) ? 64 : 32;
					int tileoffset = ((*((uint16_t*)&mIORegisters->mMemory[0xC]) >> 2) & 3) * 1024 * 16;
					int mapoffset = ((*((uint16_t*)&mIORegisters->mMemory[0xC]) >> 8) & 31) * 1024 * 2;
					bool done = false;
					/*int x1 = -(*((uint16_t*)&mIORegisters->mMemory[0x18]) & 0x1FF) << 8;
					int y1 = -(*((uint16_t*)&mIORegisters->mMemory[0x1A]) & 0x1FF) << 8;
					int x2 = x1 + (w << 8);
					int y2 = y1 + (h << 8);
					if (bgmode >= 1)
					{
						x1 = 0;
						y1 = 0;
						x2 = 240 << 8;
						y2 = 160 << 8;
					}*/
					int x1 = 0;
					int y1 = 0;
					int x2 = 240 << 8;
					int y2 = 160 << 8;

					if (x1 <= (mX << 8) && x2 > (mX << 8) && y1 <= (mY << 8) && y2 > (mY << 8))
					{
						int dx = ((mX << 8) - x1 + (bgmode == 0 ? (*((uint16_t*)&mIORegisters->mMemory[0x18]) & 0x1FF) << 8 : 0)) % (w << 8);
						int dy = ((mY << 8) - y1 + (bgmode == 0 ? (*((uint16_t*)&mIORegisters->mMemory[0x1A]) & 0x1FF) << 8 : 0)) % (h << 8);
						if (bgmode >= 1)
						{
							int rotcenterx = 0;
							int rotcentery = 0;
							int dx2 = ((*((int16_t*)&mIORegisters->mMemory[0x20]) * (dx - rotcenterx) + *((int16_t*)&mIORegisters->mMemory[0x22]) * (dy - rotcentery)) >> 8) + rotcenterx;
							int dy2 = ((*((int16_t*)&mIORegisters->mMemory[0x24]) * (dx - rotcenterx) + *((int16_t*)&mIORegisters->mMemory[0x26]) * (dy - rotcentery)) >> 8) + rotcentery;
							dx = dx2;
							dy = dy2;
							dx += (((int)(*((uint32_t*)&mIORegisters->mMemory[0x28]) << 4)) >> 4);
							dy += (((int)(*((uint32_t*)&mIORegisters->mMemory[0x2C]) << 4)) >> 4);
						}
						if (dx >= 0 && dx < (w << 8) && dy >= 0 && dy < (h << 8))
						{
							dx >>= 8;
							dy >>= 8;
							int tilex = dx >> 3;
							int tiley = dy >> 3;
							int tsx = dx - (tilex << 3);
							int tsy = dy - (tiley << 3);
							int tilestart;
							uint16_t text;
							if (bgmode == 0)
							{
								text = *((uint16_t*)&mVRAM->mMemory[mapoffset + (tiley * 32 + tilex) * 2]);
								tilestart = tileoffset + (text & 0x3FF) * tilesize;
								if ((text >> 10) & 1) tsx = 7 - tsx;
								if ((text >> 11) & 1) tsy = 7 - tsy;
								tilestart += tsy * (tilesize >> 3);
								tilestart += (tsx * tilesize) >> 6;
							}
							else
							{
								tilestart = tileoffset + mVRAM->mMemory[mapoffset + tiley * (w / 8) + tilex] * 64;//tilesize;
								tilestart += tsy * (tilesize >> 3);
								tilestart += (tsx * tilesize) >> 6;
							}
							int color = 0;
							if (!((*((uint16_t*)&mIORegisters->mMemory[0xC]) >> 7) & 1) && bgmode == 0)
							{
								uint8_t val = mVRAM->mMemory[tilestart];
								if (tsx & 1) val >>= 4;
								else val &= 0xF;
								color = val + (text >> 12) * 16;
							}
							else
							{
								color = mVRAM->mMemory[tilestart];
							}
							if ((!((*((uint16_t*)&mIORegisters->mMemory[0xC]) >> 7) & 1) && bgmode == 0 && color % 16) ||
								((((*((uint16_t*)&mIORegisters->mMemory[0xC]) >> 7) & 1) || bgmode >= 1) && color % 256))
							{
								pixel = ((GXRgb*)&mPalRam->mMemory[0])[color];
								if (coloreffect == 2 && color1sttarget & 0x4)
								{
									pixel.r = pixel.r + (31 - pixel.r) * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
									pixel.g = pixel.g + (31 - pixel.g) * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
									pixel.b = pixel.b + (31 - pixel.b) * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
								}
								else if (coloreffect == 3 && color1sttarget & 0x4)
								{
									pixel.r = pixel.r - pixel.r * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
									pixel.g = pixel.g - pixel.g * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
									pixel.b = pixel.b - pixel.b * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
								}
							}
						}
					}
					//}
				}
				if ((*((uint16_t*)&mIORegisters->mMemory[0]) >> 9) & 1 && (*((uint16_t*)&mIORegisters->mMemory[0xA]) & 3) == prio)//bg1
				{
					if (bgmode == 0 || bgmode == 1)
					{
						int w = ((*((uint16_t*)&mIORegisters->mMemory[0xA]) >> 14) & 1) ? 512 : 256;
						int h = ((*((uint16_t*)&mIORegisters->mMemory[0xA]) >> 15) & 1) ? 512 : 256;
						int tilesize = ((*((uint16_t*)&mIORegisters->mMemory[0xA]) >> 7) & 1) ? 64 : 32;
						int tileoffset = ((*((uint16_t*)&mIORegisters->mMemory[0xA]) >> 2) & 3) * 1024 * 16;
						int mapoffset = ((*((uint16_t*)&mIORegisters->mMemory[0xA]) >> 8) & 31) * 1024 * 2;
						int x1 = 0;// -(*((uint16_t*)&mIORegisters->mMemory[0x14]) & 0x1FF);
						int y1 = 0;// -(*((uint16_t*)&mIORegisters->mMemory[0x16]) & 0x1FF);
						int x2 = 240;// x1 + w;
						int y2 = 160;// y1 + h;
						if (x1 <= mX && x2 > mX && y1 <= mY && y2 > mY)
						{
							int dx = (mX - x1 + (*((uint16_t*)&mIORegisters->mMemory[0x14]) & 0x1FF)) % w;
							int dy = (mY - y1 + (*((uint16_t*)&mIORegisters->mMemory[0x16]) & 0x1FF)) % h;
							int tilex = dx >> 3;
							int tiley = dy >> 3;
							uint16_t text = *((uint16_t*)&mVRAM->mMemory[mapoffset + (tiley * 32 + tilex) * 2]);
							int tilestart = tileoffset + (text & 0x3FF) * tilesize;
							int tsx = dx - (tilex << 3);
							if ((text >> 10) & 1) tsx = 7 - tsx;
							int tsy = dy - (tiley << 3);
							if ((text >> 11) & 1) tsy = 7 - tsy;
							tilestart += tsy * (tilesize >> 3);
							tilestart += (tsx * tilesize) >> 6;
							int color = 0;
							if (!((*((uint16_t*)&mIORegisters->mMemory[0xA]) >> 7) & 1))
							{
								uint8_t val = mVRAM->mMemory[tilestart];
								if (tsx & 1) val >>= 4;
								else val &= 0xF;
								color = val + (text >> 12) * 16;
							}
							else
							{
								color = mVRAM->mMemory[tilestart];
							}
							if ((!((*((uint16_t*)&mIORegisters->mMemory[0xA]) >> 7) & 1) && color % 16) ||
								(((*((uint16_t*)&mIORegisters->mMemory[0xA]) >> 7) & 1) && color % 256))
							{
								pixel = ((GXRgb*)&mPalRam->mMemory[0])[color];
								if (coloreffect == 2 && color1sttarget & 0x2)
								{
									pixel.r = pixel.r + (31 - pixel.r) * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
									pixel.g = pixel.g + (31 - pixel.g) * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
									pixel.b = pixel.b + (31 - pixel.b) * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
								}
								else if (coloreffect == 3 && color1sttarget & 0x2)
								{
									pixel.r = pixel.r - pixel.r * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
									pixel.g = pixel.g - pixel.g * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
									pixel.b = pixel.b - pixel.b * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
								}
							}
						}
					}
				}
				if ((*((uint16_t*)&mIORegisters->mMemory[0]) >> 8) & 1 && (*((uint16_t*)&mIORegisters->mMemory[0x8]) & 3) == prio)//bg0
				{
					if (bgmode == 0 || bgmode == 1)
					{
						int w = ((*((uint16_t*)&mIORegisters->mMemory[8]) >> 14) & 1) ? 512 : 256;
						int h = ((*((uint16_t*)&mIORegisters->mMemory[8]) >> 15) & 1) ? 512 : 256;
						int tilesize = ((*((uint16_t*)&mIORegisters->mMemory[8]) >> 7) & 1) ? 64 : 32;
						int tileoffset = ((*((uint16_t*)&mIORegisters->mMemory[8]) >> 2) & 3) * 1024 * 16;
						int mapoffset = ((*((uint16_t*)&mIORegisters->mMemory[8]) >> 8) & 31) * 1024 * 2;
						int x1 = 0;//-(*((uint16_t*)&mIORegisters->mMemory[0x10]) & 0x1FF);
						int y1 = 0;//-(*((uint16_t*)&mIORegisters->mMemory[0x12]) & 0x1FF);
						int x2 = 240;//x1 + w;
						int y2 = 160;//y1 + h;
						if (x1 <= mX && x2 > mX && y1 <= mY && y2 > mY)
						{
							int dx = (mX - x1 + (*((uint16_t*)&mIORegisters->mMemory[0x10]) & 0x1FF)) % w;
							int dy = (mY - y1 + (*((uint16_t*)&mIORegisters->mMemory[0x12]) & 0x1FF)) % h;
							int tilex = dx >> 3;
							int tiley = dy >> 3;
							uint16_t text = *((uint16_t*)&mVRAM->mMemory[mapoffset + (tiley * 32 + tilex) * 2]);
							int tilestart = tileoffset + (text & 0x3FF) * tilesize;
							int tsx = dx - (tilex << 3);
							if ((text >> 10) & 1) tsx = 7 - tsx;
							int tsy = dy - (tiley << 3);
							if ((text >> 11) & 1) tsy = 7 - tsy;
							tilestart += tsy * (tilesize >> 3);
							tilestart += (tsx * tilesize) >> 6;
							int color = 0;
							if (!((*((uint16_t*)&mIORegisters->mMemory[8]) >> 7) & 1))
							{
								uint8_t val = mVRAM->mMemory[tilestart];
								if (tsx & 1) val >>= 4;
								else val &= 0xF;
								color = val + (text >> 12) * 16;
							}
							else
							{
								color = mVRAM->mMemory[tilestart];
							}
							if ((!((*((uint16_t*)&mIORegisters->mMemory[8]) >> 7) & 1) && color % 16) ||
								(((*((uint16_t*)&mIORegisters->mMemory[8]) >> 7) & 1) && color % 256))
							{
								pixel = ((GXRgb*)&mPalRam->mMemory[0])[color];
								if (coloreffect == 2 && color1sttarget & 0x1)
								{
									pixel.r = pixel.r + (31 - pixel.r) * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
									pixel.g = pixel.g + (31 - pixel.g) * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
									pixel.b = pixel.b + (31 - pixel.b) * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
								}
								else if (coloreffect == 3 && color1sttarget & 0x1)
								{
									pixel.r = pixel.r - pixel.r * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
									pixel.g = pixel.g - pixel.g * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
									pixel.b = pixel.b - pixel.b * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
								}
							}
						}
					}
				}
				//oam
				if ((*((uint16_t*)&mIORegisters->mMemory[0]) >> 12) & 1)
				{
					int mappingmode = (*((uint16_t*)&mIORegisters->mMemory[0]) >> 6) & 1;
					int oamtileoffset = (bgmode <= 2 ? 0x10000 : 0x14000);

					for (int i = 127; i >= 0; i--)
					{
						GXOamAttr* pOAM = &((GXOamAttr*)&mOAM->mMemory[0])[i];
						if (pOAM->rsMode != 2 && pOAM->priority == prio)
						{
							/*if (pOAM->rsMode & 1)//affine
							{
								wprintf(L"LOL!");
							}
							else*/
							{
								int x1 = ((int)(pOAM->x << 23)) >> 23;
								int y1 = pOAM->y;// ((int)(pOAM->y << 24)) >> 24;
								int w = oamWidth[pOAM->size | (pOAM->shape << 2)];
								int h = oamHeight[pOAM->size | (pOAM->shape << 2)];
								int x2 = x1 + w;
								int y2 = y1 + h;
								if ((pOAM->rsMode & 1) && (pOAM->rsMode & 2))
								{
									//x1 += w / 2;
									//y1 += h / 2;
									x2 += w;// / 2;
									y2 += h;// / 2;
								}
								if (x1 <= mX && x2 > mX && y1 <= mY && y2 > mY)
								{
									if ((pOAM->rsMode & 1) && (pOAM->rsMode & 2))
									{
										x2 -= w;
										y2 -= h;
										x1 += w / 2;
										y1 += h / 2;
										x2 += w / 2;
										y2 += h / 2;
									}
									int dx = mX - x1;
									int dy = mY - y1;
									if (pOAM->rsMode & 1)
									{
										GXOamAffine* params = &((GXOamAffine*)&mOAM->mMemory[0])[pOAM->rsParam];
										int rotcenterx = w / 2;
										int rotcentery = h / 2;
										int dx2 = ((params->PA * (dx - rotcenterx) + params->PB * (dy - rotcentery)) >> 8) + rotcenterx;
										int dy2 = ((params->PC * (dx - rotcenterx) + params->PD * (dy - rotcentery)) >> 8) + rotcentery;
										dx = dx2;
										dy = dy2;
										if (dx < 0 || dx >= w || dy < 0 || dy >= h) continue;
									}
									if (!(pOAM->rsMode & 1))
									{
										if (pOAM->flipH) dx = (w - 1) - dx;
										if (pOAM->flipV) dy = (h - 1) - dy;
									}
									int tilex = dx >> 3;
									int tiley = dy >> 3;
									int tilesize = (pOAM->colorMode ? 64 : 32);
									int tilestart;
									if (!mappingmode)//2d mapping
									{
										tilestart = oamtileoffset + pOAM->charNo * 32 + tiley * 32 * 32 + tilex * tilesize;
									}
									else
									{
										tilestart = oamtileoffset + pOAM->charNo * 32 + tiley * (w / 8) * tilesize + tilex * tilesize;
									}
									int tsx = dx - (tilex << 3);
									int tsy = dy - (tiley << 3);
									tilestart += tsy * (tilesize >> 3);
									tilestart += (tsx * tilesize) >> 6;
									int color = 0;
									if (!pOAM->colorMode)
									{
										uint8_t val = mVRAM->mMemory[tilestart];
										if (tsx & 1) val >>= 4;
										else val &= 0xF;
										color = val + pOAM->cParam * 16;
									}
									else
									{
										color = mVRAM->mMemory[tilestart];
									}
									if ((!pOAM->colorMode && color % 16) || (pOAM->colorMode && color % 256))
									{
										pixel = ((GXRgb*)&mPalRam->mMemory[0x200])[color];
										if (coloreffect == 2 && color1sttarget & 0x10)
										{
											pixel.r = pixel.r + (31 - pixel.r) * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
											pixel.g = pixel.g + (31 - pixel.g) * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
											pixel.b = pixel.b + (31 - pixel.b) * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
										}
										else if (coloreffect == 3 && color1sttarget & 0x10)
										{
											pixel.r = pixel.r - pixel.r * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
											pixel.g = pixel.g - pixel.g * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
											pixel.b = pixel.b - pixel.b * min(16, *((uint32_t*)&mIORegisters->mMemory[0x54]) & 0x1F) / 16;
										}
									}
								}
							}
						}
					}
				}
			}
			mFrameBuffer[mY * 240 + mX] = RGB(pixel.b * 255 / 31, pixel.g * 255 / 31, pixel.r * 255 / 31);
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