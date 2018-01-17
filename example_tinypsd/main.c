#define TNYPSD_IMPLEMENTATION
#include "tinypsd.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int main(int argc, char** argv)
{
  tpsdPSD psd;
  if (tpsdLoadPSD(&psd, "imgs/drone.psd") == TPSD_LOAD_ERROR)
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

  unsigned totalPixels = psd.header.width * psd.header.height;
  tpsdLayer* layer = &psd.layers[2];
  unsigned char* a = layer->data;
  unsigned char* red = layer->data + totalPixels;
  unsigned char* green = layer->data + totalPixels * 2;
  unsigned char* blue = layer->data + totalPixels * 3;

  unsigned char *pixelData = TPSD_ALLOC(totalPixels * 3);
  if (!pixelData)
    return 0;

  for (unsigned i = 0; i < totalPixels; ++i)
  {
    pixelData[i * 3] = red[i];
    pixelData[i * 3 + 1] = green[i];
    pixelData[i * 3 + 2] = blue[i];
  }
  stbi_write_png("test_layer.png", layer->right - layer->left, layer->bottom - layer->top, 3, pixelData, (layer->bottom - layer->top) * 3);

  //TPSD_FREE(pixelData);
}