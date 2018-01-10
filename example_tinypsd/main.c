#define TNYPSD_IMPLEMENTATION
#include "tinypsd.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int main(int argc, char** argv)
{
  tpsdPSD psd;
  if (tpsdLoadPSD(&psd, "imgs/checkerboard_bitmap.psd") == TPSD_LOAD_ERROR)
  {

  }

  tpsdImage* image = tpsdGetImageFromPSD(&psd);

  switch (psd.header.mode)
  {
  case TPSD_BITMAP:
    stbi_write_png("test.png", psd.header.width, psd.header.height, 1, image->pixels, psd.header.width);
    break;
  case TPSD_GRAYSCALE:
    switch (psd.header.depth)
    {
    case 8:
      if(psd.header.numChannels == 1)
        stbi_write_png("test.png", psd.header.width, psd.header.height, 1, image->pixels, psd.header.width);
      else
        stbi_write_png("test.png", psd.header.width, psd.header.height, 4, image->pixels, psd.header.width * 4);
      break;
    case 16:
      break;
    default:
      break;
    }
    break;
  case TPSD_INDEXED:
    stbi_write_png("test.png", psd.header.width, psd.header.height, 3, image->pixels, psd.header.width * 3);
    break;
  case TPSD_RGB:
    switch (psd.header.depth)
    {
    case 8:
      if (psd.header.numChannels == 3)
        stbi_write_png("test.png", psd.header.width, psd.header.height, 3, image->pixels, psd.header.width * 3);
      else
        stbi_write_png("test.png", psd.header.width, psd.header.height, 4, image->pixels, psd.header.width * 4);
      break;
    case 16:
      break;
    default:
      break;
    }
    break;
  case TPSD_CMYK:
  case TPSD_MULTICHANNEL:
    switch (psd.header.depth)
    {
    case 8:
      if (psd.header.numChannels == 3 || psd.header.numChannels == 4)
        stbi_write_png("test.png", psd.header.width, psd.header.height, 3, image->pixels, psd.header.width * 3);
      else if (psd.header.numChannels == 5)
        stbi_write_png("test.png", psd.header.width, psd.header.height, 4, image->pixels, psd.header.width * 4);
      break;
    case 16:
      break;
    default:
      break;
    }
    break;
  case TPSD_DUOTONE:
    break;
  case TPSD_LAB:
    break;
  default:
    break;
  }

  //TPSD_FREE(pixelData);
}