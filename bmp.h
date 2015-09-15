#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <stdio.h>
#pragma pack(push, 1)

typedef struct tagBITMAPFILEHEADER
{
 uint16_t bfType;
 uint32_t bfSize;
 uint16_t bfReserved1;
 uint16_t bfReserved2;
 uint32_t bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
 uint32_t biSize;
 int biWidth;
 int biHeight;
 uint16_t biPlanes;
 uint16_t biBitCount;
 uint32_t biCompression;
 uint32_t biSizeImage;
 uint32_t biXPelsPerMeter;
 uint32_t biYPelsPerMeter;
 uint32_t biClrUsed;
 uint32_t biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagRGBQUAD
{
 uint8_t rgbBlue;
 uint8_t rgbGreen;
 uint8_t rgbRed;
 uint8_t rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFO
{
 BITMAPINFOHEADER bmiHeader;
 RGBQUAD bmiColors[1];
} BITMAPINFO;

typedef struct tagBITMAP
{
 BITMAPFILEHEADER bfHeader;
 BITMAPINFO biInfo;
} BITMAPFILE;

uint8_t* GetBmpData(uint8_t *bitCountPerPix, int *width, int *height, const char* filename);
int GenBmpFile(uint8_t *pData, uint8_t bitCountPerPix, int width, int height, void *image_buffer);
void FreeBmpData(uint8_t *pdata);

#pragma pack(pop)
#endif
