#define TNYPSD_IMPLEMENTATION
#include "tinypsd.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int main(int argc, char** argv)
{
  tpsdPSD psd;
  if (tpsdLoadPSD(&psd, "imgs/medium_mech_color_00.psd") == TPSD_LOAD_ERROR)
  {

  }
  const unsigned totalPixels = psd.header.width * psd.header.height;
  unsigned char *pixelData = TPSD_ALLOC(4 * totalPixels);
  if (!pixelData)
    return 0;

  for (unsigned i = 0; i < totalPixels; ++i)
  {
    pixelData[i * 4] = psd.imageData.red[i];
    pixelData[i * 4 + 1] = psd.imageData.green[i];
    pixelData[i * 4 + 2] = psd.imageData.blue[i];
    pixelData[i * 4 + 3] = psd.imageData.alpha[i];
  }

  stbi_write_png("test.png", psd.header.width, psd.header.height, 4, pixelData, psd.header.width * 4);
  TPSD_FREE(pixelData);
}