#define TNYPSD_IMPLEMENTATION
#include "tinypsd.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

unsigned char CMYKtoRGB(unsigned char cmy, unsigned char k)
{
  return (65535 - (cmy * (255 - k) + (k << 8))) >> 8;
}

#define GET_BIT(v, b) (v & ( 1 << b )) >> b

int main(int argc, char** argv)
{
  tpsdPSD psd;
  if (tpsdLoadPSD(&psd, "imgs/checkerboard_bitmap.psd") == TPSD_LOAD_ERROR)
  {

  }
  const unsigned totalPixels = psd.header.width * psd.header.height;
  unsigned char *pixelData = TPSD_ALLOC(totalPixels * 8);
  if (!pixelData)
    return 0;

  for (unsigned i = 0; i < totalPixels; ++i)
  {
    int bit1 = GET_BIT(psd.compositeImage.bitmap.data[i], 0);
    int bit2 = GET_BIT(psd.compositeImage.bitmap.data[i], 1);
    int bit3 = GET_BIT(psd.compositeImage.bitmap.data[i], 2);
    int bit4 = GET_BIT(psd.compositeImage.bitmap.data[i], 3);
    int bit5 = GET_BIT(psd.compositeImage.bitmap.data[i], 4);
    int bit6 = GET_BIT(psd.compositeImage.bitmap.data[i], 5);
    int bit7 = GET_BIT(psd.compositeImage.bitmap.data[i], 6);
    int bit8 = GET_BIT(psd.compositeImage.bitmap.data[i], 7);
    pixelData[i * 8] = bit1 * 255;
    pixelData[i * 8 + 1] = bit2 * 255;
    pixelData[i * 8 + 2] = bit3 * 255;
    pixelData[i * 8 + 3] = bit4 * 255;
    pixelData[i * 8 + 4] = bit5 * 255;
    pixelData[i * 8 + 5] = bit6 * 255;
    pixelData[i * 8 + 6] = bit7 * 255;
    pixelData[i * 8 + 7] = bit8 * 255;
  }

  stbi_write_png("test.png", psd.header.width, psd.header.height, 1, pixelData, psd.header.width);
  TPSD_FREE(pixelData);
}