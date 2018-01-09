#define TNYPSD_IMPLEMENTATION
#include "tinypsd.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

unsigned char CMYKtoRGB(unsigned char cmy, unsigned char k)
{
  return (65535 - (cmy * (255 - k) + (k << 8))) >> 8;
}

int main(int argc, char** argv)
{
  tpsdPSD psd;
  if (tpsdLoadPSD(&psd, "imgs/test_rgba.psd") == TPSD_LOAD_ERROR)
  {

  }
  const unsigned totalPixels = psd.header.width * psd.header.height;
  unsigned char *pixelData = TPSD_ALLOC(4 * totalPixels);
  if (!pixelData)
    return 0;

  for (unsigned i = 0; i < totalPixels; ++i)
  {
    pixelData[i * 4] = psd.compositeImage.rgb.red[i];
    pixelData[i * 4 + 1] = psd.compositeImage.rgb.green[i];
    pixelData[i * 4 + 2] = psd.compositeImage.rgb.blue[i];
    pixelData[i * 4 + 3] = psd.compositeImage.rgb.alpha[i];
  }

  stbi_write_png("test.png", psd.header.width, psd.header.height, 4, pixelData, psd.header.width * 4);
  TPSD_FREE(pixelData);
}