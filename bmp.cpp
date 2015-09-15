#include "li_bmp.h"
#include "fcntl.h"
#include "stdio.h"

extern "C" {
int GenBmpFile(uint8_t *pData, uint8_t bitCountPerPix, int width, int height, uint8_t * image_buffer)
{
//    FILE *fp = fopen(filename, "w+");
//    if(!fp)
//    {
//        printf("fopen failed : %s, %d\n", __FILE__, __LINE__);
//        return 0;
//    }

    uint32_t bmppitch = ((width*bitCountPerPix + 31) >> 5) << 2;
    uint32_t filesize = bmppitch*height;

    BITMAPFILE bmpfile;

fprintf(stderr, "file %s, line %d\n", __FILE__, __LINE__);
    bmpfile.bfHeader.bfType = 0x4D42;
    bmpfile.bfHeader.bfSize = filesize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmpfile.bfHeader.bfReserved1 = 0;
    bmpfile.bfHeader.bfReserved2 = 0;
    bmpfile.bfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    bmpfile.biInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpfile.biInfo.bmiHeader.biWidth = width;
    bmpfile.biInfo.bmiHeader.biHeight = height;
    bmpfile.biInfo.bmiHeader.biPlanes = 1;
    bmpfile.biInfo.bmiHeader.biBitCount = bitCountPerPix;
    bmpfile.biInfo.bmiHeader.biCompression = 0;
    bmpfile.biInfo.bmiHeader.biSizeImage = 0;
    bmpfile.biInfo.bmiHeader.biXPelsPerMeter = 0;
    bmpfile.biInfo.bmiHeader.biYPelsPerMeter = 0;
    bmpfile.biInfo.bmiHeader.biClrUsed = 0;
    bmpfile.biInfo.bmiHeader.biClrImportant = 0;

fprintf(stderr, "file %s, line %d\n", __FILE__, __LINE__);
    memcpy(image_buffer, &(bmpfile.bfHeader), sizeof(BITMAPFILEHEADER));
    memcpy(image_buffer + sizeof(BITMAPFILEHEADER), &(bmpfile.biInfo.bmiHeader), sizeof(BITMAPINFOHEADER));

fprintf(stderr, "file %s, line %d\n", __FILE__, __LINE__);
    uint8_t *pEachLinBuf = (uint8_t*) malloc(bmppitch);
    memset(pEachLinBuf, 0, bmppitch);
    uint8_t BytePerPix = bitCountPerPix >> 3;
    uint32_t pitch = width * BytePerPix;
    uint32_t memcount = 0;

    if(pEachLinBuf)
    {
        int h,w;
        for(h = height-1; h >= 0; h--)
        {
            for(w = 0; w < width; w++)
            {
                //copy by a pixel
                pEachLinBuf[w*BytePerPix+0] = pData[h*pitch + w*BytePerPix + 2];
                pEachLinBuf[w*BytePerPix+1] = pData[h*pitch + w*BytePerPix + 1];
                pEachLinBuf[w*BytePerPix+2] = pData[h*pitch + w*BytePerPix + 0];
            }
           memcpy(image_buffer + bmpfile.bfHeader.bfOffBits + memcount * bmppitch, pEachLinBuf, bmppitch);
		   memcount ++;
        }
        free(pEachLinBuf);
    }

fprintf(stderr, "file %s, line %d\n", __FILE__, __LINE__);
    return 1;
}

//获取BMP文件的位图数据(无颜色表的位图):丢掉BMP文件的文件信息头和位图信息头，获取其RGB(A)位图数据
uint8_t* GetBmpData(uint8_t *bitCountPerPix, int *width, int *height, const char* filename)
{
    FILE *pf = fopen(filename, "rb");
    if(!pf)
    {
        printf("fopen failed : %s, %d\n", __FILE__, __LINE__);
        return NULL;
    }

    BITMAPFILE bmpfile;
    fread(&(bmpfile.bfHeader), sizeof(BITMAPFILEHEADER), 1, pf);
    fread(&(bmpfile.biInfo.bmiHeader), sizeof(BITMAPINFOHEADER), 1, pf);

    // print_bmfh(bmpfile.bfHeader);
    // print_bmih(bmpfile.biInfo.bmiHeader);

    if(bitCountPerPix)
    {
        *bitCountPerPix = bmpfile.biInfo.bmiHeader.biBitCount;
    }
    if(width)
    {
        *width = bmpfile.biInfo.bmiHeader.biWidth;
    }
    if(height)
    {
        *height = bmpfile.biInfo.bmiHeader.biHeight;
    }

    uint32_t bmppicth = (((*width)*(*bitCountPerPix) + 31) >> 5) << 2;
    uint8_t *pdata = (uint8_t*)malloc((*height)*bmppicth);

    uint8_t *pEachLinBuf = (uint8_t*)malloc(bmppicth);
    memset(pEachLinBuf, 0, bmppicth);
    uint8_t BytePerPix = (*bitCountPerPix) >> 3;
    uint32_t pitch = (*width) * BytePerPix;

    if(pdata && pEachLinBuf)
    {
        int w, h;
        for(h = (*height) - 1; h >= 0; h--)
        {
            fread(pEachLinBuf, bmppicth, 1, pf);
            for(w = 0; w < (*width); w++)
            {
                pdata[h*pitch + w*BytePerPix + 0] = pEachLinBuf[w*BytePerPix+0];
                pdata[h*pitch + w*BytePerPix + 1] = pEachLinBuf[w*BytePerPix+1];
                pdata[h*pitch + w*BytePerPix + 2] = pEachLinBuf[w*BytePerPix+2];
            }
        }
        free(pEachLinBuf);
    }
    fclose(pf);

    return pdata;
}

//释放GetBmpData分配的空间
void FreeBmpData(uint8_t *pdata)
{
    if(pdata)
    {
        free(pdata);
        pdata = NULL;
    }
}

#if 0
typedef struct _LI_RGB
{
    uint8_t b;
    uint8_t g;
    uint8_t r;
}LI_RGB;

#define WIDTH   10
#define HEIGHT  10
int main(char argc, char *argv[])
{
    #if 1
    //test one
    LI_RGB pRGB[WIDTH][HEIGHT];  // 定义位图数据
    memset(pRGB, 0, sizeof(pRGB) ); // 设置背景为黑色
    // 在中间画一个10*10的矩形
    int i=0, j=0;
    for(i = 0; i < WIDTH; i++)
    {
        for( j = 0; j < HEIGHT; j++)
        {
            pRGB[i][j].b = 0xff;
            pRGB[i][j].g = 0x00;
            pRGB[i][j].r = 0x00;
        }
    }
    GenBmpFile((uint8_t*)pRGB, 24, WIDTH, HEIGHT, "out.bmp");//生成BMP文件
    #endif

    #if 1
    //test two
    uint8_t bitCountPerPix;
    uint32_t width, height;
    uint8_t *pdata = GetBmpData(&bitCountPerPix, &width, &height, "in.bmp");
    if(pdata)
    {
        GenBmpFile(pdata, bitCountPerPix, width, height, "out1.bmp");
        FreeBmpData(pdata);
    }
    #endif

    return 0;
}
#endif
}
