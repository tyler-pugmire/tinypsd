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
  if (tpsdLoadPSD(&psd, "imgs/test_indexed.psd") == TPSD_LOAD_ERROR)
  {

  }
  const unsigned totalPixels = psd.header.width * psd.header.height;
  unsigned char *pixelData = TPSD_ALLOC(totalPixels * 4);
  if (!pixelData)
    return 0;

  unsigned colorCount = psd.colorModeData.length / 3;
  unsigned char* red = psd.colorModeData.data;
  unsigned char* green = psd.colorModeData.data + colorCount;
  unsigned char* blue = psd.colorModeData.data + colorCount * 2;

  for (unsigned i = 0; i < totalPixels; ++i)
  {
    pixelData[i * 3] = red[psd.compositeImage.indexed.data[i]];
    pixelData[i * 3 + 1] = green[psd.compositeImage.indexed.data[i]];
    pixelData[i * 3 + 2] = blue[psd.compositeImage.indexed.data[i]];
  }

  stbi_write_png("test.png", psd.header.width, psd.header.height, 3, pixelData, psd.header.width * 3);
  TPSD_FREE(pixelData);
}