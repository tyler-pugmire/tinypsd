#ifndef TINYPSD_H
#define TINYPSD_H

#ifdef _WIN32

#if !defined( _CRT_SECURE_NO_WARNINGS )
#define _CRT_SECURE_NO_WARNINGS
#endif

#endif

#define TPSD_LOAD_OK 1
#define TPSD_LOAD_ERROR 0

typedef struct tpsdHeader tpsdHeader;
typedef struct tpsdColorModeData tpsdColorModeData;
typedef struct tpsdImageResource tpsdImageResource;
typedef struct tpsdImageResourceBlock tpsdImageResourceBlock;
typedef struct tpsdImageData tpsdImageData;

typedef struct tpsdBitmapImage tpsdBitmapImage;
typedef struct tpsdGrayscaleImage tpsdGrayscaleImage;
typedef struct tpsdIndexedImage tpsdIndexedImage;
typedef struct tpsdRGBImage tpsdRGBImage;
typedef struct tpsdCMYKImage tpsdCMYKImage;

typedef struct tpsdCompositeImage tpsdCompositeImage;
typedef struct tpsdPSD tpsdPSD;
typedef struct tpsdImage tpsdImage;

int tpsdLoadPSD(tpsdPSD* psd, char const* file);
tpsdImage* tpsdGetImageFromPSD(tpsdPSD* psd);

struct tpsdHeader
{
  unsigned signature; //Signature: always equal to '8BPS' . Do not try to read the file if the signature does not match this value.
  short version; //Version: always equal to 1. Do not try to read the file if the version does not match this value. (**PSB** version is 2.)
  //6 bytes reserved here must be 0;
  short numChannels; //The number of channels in the image, including any alpha channels.Supported range is 1 to 56.
  unsigned height; //The height of the image in pixels. Supported range is 1 to 30,000. (**PSB** max of 300, 000.)
  unsigned width; //The width of the image in pixels. Supported range is 1 to 30,000. (*PSB** max of 300, 000)
  short depth; //Depth: the number of bits per channel. Supported values are 1, 8, 16 and 32.
  short mode; //The color mode of the file. Supported values are: Bitmap = 0; Grayscale = 1; Indexed = 2; RGB = 3; CMYK = 4; Multichannel = 7; Duotone = 8; Lab = 9.
};

struct tpsdColorModeData
{
  unsigned length;
  unsigned char* data;
};

struct tpsdImageResource
{
  unsigned length;
  tpsdImageResourceBlock* resourceBlocks;
};

struct tpsdImageResourceBlock
{
  unsigned signature; //Signature: '8BIM'
  short uniqueID; //Unique identifier for the resource. 
  char* name; //Name: Pascal string, padded to make the size even (a null name consists of two bytes of 0)
  unsigned size; //Actual size of resource data that follows
  char* data; //The resource data. It is padded to make the size even.
};

struct tpsdBitmapImage
{
  unsigned char* data;
};

struct tpsdGrayscaleImage
{
  unsigned char* data;
  unsigned char* alpha;
};

struct tpsdIndexedImage
{
  unsigned char* data;
};

struct tpsdRGBImage
{
  unsigned char* red;
  unsigned char* green;
  unsigned char* blue;
  unsigned char* alpha;
};

struct tpsdCMYKImage
{
  unsigned char* cyan;
  unsigned char* magenta;
  unsigned char* yellow;
  unsigned char* key;
  unsigned char* alpha;
};

struct tpsdCompositeImage
{
  union
  {
    tpsdBitmapImage bitmap;
    tpsdGrayscaleImage grayscale;
    tpsdIndexedImage indexed;
    tpsdRGBImage rgb;
    tpsdCMYKImage cmyk;
  };
};

struct tpsdImageData
{
  short compressionMethod;
  char* data;
};

struct tpsdPSD
{
  tpsdHeader header;
  tpsdColorModeData colorModeData;
  tpsdImageResource imageResource;
  tpsdImageData imageData;
  tpsdCompositeImage compositeImage;
};

struct tpsdImage
{
  unsigned width;
  unsigned height;
  unsigned char* pixels;
};

////   end header file   /////////////////////////////////////////////////////
#endif /* TINYPSD_H */


#define TINYPSD_IMPLEMENTATION

#ifdef TINYPSD_IMPLEMENTATION
#undef TINYPSD_IMPLEMENTATION

#ifdef _WIN32

#if !defined( _CRT_SECURE_NO_WARNINGS )
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <malloc.h> // alloca

#else

#include <alloca.h> // alloca

#endif

#include <stdio.h>  // fopen, fclose, etc.
#include <stdlib.h> // malloc, free, calloc
#include <string.h> // memcpy
#include <assert.h> // assert

#define TPSD_ASSERT assert
#define TPSD_ALLOC malloc
#define TPSD_ALLOCA alloca
#define TPSD_FREE free
#define TPSD_MEMCPY memcpy
#define TPSD_CALLOC calloc

#define TPSD_GET_BIT(v, b) ((v & ( 1 << b )) >> b)

unsigned char tpsdCMYKtoRGB(unsigned char cmy, unsigned char k)
{
  return (65535 - (cmy * (255 - k) + (k << 8))) >> 8;
}

enum
{
  TPSD_BITMAP,
  TPSD_GRAYSCALE,
  TPSD_INDEXED,
  TPSD_RGB,
  TPSD_CMYK,
  TPSD_MULTICHANNEL = 7,
  TPSD_DUOTONE,
  TPSD_LAB
};

enum
{
  TPSD_RAW,
  TPSD_RLE,
  TPSD_ZIP_WITHOUT_PREDITCION,
  TPSD_ZIP_WITH_PREDICTION,
};

static unsigned char* tpsdReadFileToMemory(const char* path, int* size)
{
  unsigned char* data = 0;
  FILE* fp = fopen(path, "rb");
  int sizeNum = 0;

  if (fp)
  {
    fseek(fp, 0, SEEK_END);
    sizeNum = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    data = (char*)TPSD_ALLOC(sizeNum + 1);
    fread(data, sizeNum, 1, fp);
    data[sizeNum] = 0;
    fclose(fp);
  }

  if (size) *size = sizeNum;
  return data;
}

char tpsdGetChar(unsigned char* data, unsigned *offset)
{
  char num = data[*offset];
  ++(*offset);
  return num;
}

unsigned char tpsdGetUnsignedChar(unsigned char* data, unsigned *offset)
{
  unsigned char num = (unsigned char)data[*offset];
  ++(*offset);
  return num;
}

unsigned short tpsdGetShort(unsigned char* data, unsigned *offset)
{
  unsigned short num = (short)(((short)data[*offset]) << 8) | data[*offset + 1];
  *offset += sizeof(short);
  return num;
}

unsigned tpsdGetInt(unsigned char* data, unsigned *offset)
{
  unsigned num = (data[*offset] << 24) | (data[*offset + 1] << 16) | (data[*offset + 2] << 8) | data[*offset + 3];
  *offset += sizeof(unsigned);
  return num;
}

tpsdHeader tpsdParsePSDHeader(unsigned char* data, unsigned* offset)
{
  tpsdHeader header = { 0 };
  header.signature = tpsdGetInt(data, offset);
//  TPSD_ASSERT(header.signature == '8BPS');

  header.version = tpsdGetShort(data, offset);
  //TPSD_ASSERT(header.version == 1);

  *offset += 6; //6 bytes reserved here and must be zero;

  header.numChannels = tpsdGetShort(data, offset);
  header.height = tpsdGetInt(data, offset);
  header.width = tpsdGetInt(data, offset);
  header.depth = tpsdGetShort(data, offset);
  header.mode = tpsdGetShort(data, offset);

  return header;
}

tpsdColorModeData tpsdParseColorModeData(unsigned char* data, unsigned* offset)
{
  tpsdColorModeData cmd = { 0 };

  cmd.length = tpsdGetInt(data, offset);
  if (cmd.length > 0)
  {
    cmd.data = TPSD_ALLOC(cmd.length);
    memcpy(cmd.data, data + *offset, cmd.length);
  }
  *offset += cmd.length;
  return cmd;
}

tpsdImageResource tpsdParseImageResource(unsigned char* data, unsigned* offset)
{
  tpsdImageResource imageResource = { 0 };

  imageResource.length = tpsdGetInt(data, offset);
  *offset += imageResource.length;

  return imageResource;
}

void tpsdParseLayerAndMaskInfo(unsigned char* data, unsigned* offset)
{
  unsigned layerAndMaskInfoSize = tpsdGetInt(data, offset);
  *offset += layerAndMaskInfoSize;
}

tpsdImageData tpsdParseImageData(unsigned char* data, unsigned* offset, unsigned width, unsigned height, short numChannels)
{
  tpsdImageData imageData = { 0 };

  imageData.compressionMethod = tpsdGetShort(data, offset);

  switch (imageData.compressionMethod)
  {
  case TPSD_RAW: // 0 = Raw image data
  {
    imageData.data = TPSD_ALLOC(width * height * numChannels);
    memcpy(imageData.data, data + *offset, width * height * numChannels);
    *offset += width * height * numChannels;
  }
    break;
  case TPSD_RLE: // 1 = RLE compressed the image data starts with the byte counts for all the scan lines (rows * channels), with each count stored as a two-byte value.
    //The RLE compressed data follows, with each scan line compressed separately. The RLE compression is the same compression algorithm used by the Macintosh ROM routine PackBits , and the TIFF standard.
  {
    unsigned totalPixels = width * height;
    imageData.data = TPSD_ALLOC(numChannels * totalPixels);
    const char* currentBufferPos = data + *offset;
    *offset += height * numChannels * 2;
    currentBufferPos = data + *offset;
    for (int i = 0; i < numChannels; ++i)
    {
      unsigned currentPixel = 0;
      unsigned char *bufferPtr = imageData.data + (i * totalPixels);

      while (currentPixel < totalPixels)
      {
        int length = tpsdGetChar(data, offset);
        if (length == 128)
        {
          //noop
        }
        else if (length >= 0 && length < 128)
        {
          ++length;
          currentPixel += length;
          while (length)
          {
            unsigned char color = tpsdGetUnsignedChar(data, offset);
            *bufferPtr = color;
            ++bufferPtr;
            --length;
          }
        }
        else if (length < 0 && length > -128)
        {
          length = -length + 1;
          unsigned char color = tpsdGetUnsignedChar(data, offset);
          currentPixel += length;
          while (length)
          {
            *bufferPtr = color;
            ++bufferPtr;
            --length;
          }
        }
      }
    }
  }
    break;
  case TPSD_ZIP_WITHOUT_PREDITCION: // 2 = ZIP without prediction(Unsupported)
    break;
  case TPSD_ZIP_WITH_PREDICTION: // 3 = ZIP with prediction (Unsupported)
    break;
  default:
    break;
  }

  return imageData;
}

void tpsdProcessBitmap(tpsdPSD* psd)
{
  unsigned totalPixels = psd->header.width * psd->header.height;
  psd->compositeImage.bitmap.data = psd->imageData.data;
}

void tpsdProcessGrayscale8(tpsdPSD* psd)
{
  unsigned totalPixels = psd->header.width * psd->header.height;
  psd->compositeImage.grayscale.data = psd->imageData.data;
}

void tpsdProcessGrayscale8Alpha(tpsdPSD* psd)
{
  unsigned totalPixels = psd->header.width * psd->header.height;
  psd->compositeImage.grayscale.data = psd->imageData.data;
  psd->compositeImage.grayscale.alpha = psd->imageData.data + totalPixels;
}

void tpsdProcessIndexed(tpsdPSD* psd)
{
  unsigned totalPixels = psd->header.width * psd->header.height;
  psd->compositeImage.indexed.data = psd->imageData.data;
}

void tpsdProcessRGB8(tpsdPSD* psd)
{
  unsigned totalPixels = psd->header.width * psd->header.height;
  psd->compositeImage.rgb.red = psd->imageData.data;
  psd->compositeImage.rgb.green = psd->imageData.data + totalPixels;
  psd->compositeImage.rgb.blue = psd->imageData.data + totalPixels * 2;
}

void tpsdProcessRGBA8(tpsdPSD* psd)
{
  unsigned totalPixels = psd->header.width * psd->header.height;
  psd->compositeImage.rgb.red = psd->imageData.data;
  psd->compositeImage.rgb.green = psd->imageData.data + totalPixels;
  psd->compositeImage.rgb.blue = psd->imageData.data + totalPixels * 2;
  psd->compositeImage.rgb.alpha = psd->imageData.data + totalPixels * 3;
}

void tpsdProcessCMY8(tpsdPSD* psd)
{
  unsigned totalPixels = psd->header.width * psd->header.height;
  psd->compositeImage.cmyk.cyan = psd->imageData.data;
  psd->compositeImage.cmyk.magenta = psd->imageData.data + totalPixels;
  psd->compositeImage.cmyk.yellow = psd->imageData.data + totalPixels * 2;
}

void tpsdProcessCMYK8(tpsdPSD* psd)
{
  unsigned totalPixels = psd->header.width * psd->header.height;
  psd->compositeImage.cmyk.cyan = psd->imageData.data;
  psd->compositeImage.cmyk.magenta = psd->imageData.data + totalPixels;
  psd->compositeImage.cmyk.yellow = psd->imageData.data + totalPixels * 2;
  psd->compositeImage.cmyk.key = psd->imageData.data + totalPixels * 3;
}

void tpsdProcessCMYKA8(tpsdPSD* psd)
{
  unsigned totalPixels = psd->header.width * psd->header.height;
  psd->compositeImage.cmyk.cyan = psd->imageData.data;
  psd->compositeImage.cmyk.magenta = psd->imageData.data + totalPixels;
  psd->compositeImage.cmyk.yellow = psd->imageData.data + totalPixels * 2;
  psd->compositeImage.cmyk.key = psd->imageData.data + totalPixels * 3;
  psd->compositeImage.cmyk.alpha = psd->imageData.data + totalPixels * 4;
}

int tpsdLoadPSDFromMemory(tpsdPSD* psd, unsigned char* data, int len)
{
  unsigned offset = 0;
  psd->header = tpsdParsePSDHeader(data, &offset);

  psd->colorModeData = tpsdParseColorModeData(data, &offset);
  tpsdParseImageResource(data, &offset);
  tpsdParseLayerAndMaskInfo(data, &offset);

  psd->imageData = tpsdParseImageData(data, &offset, psd->header.width, psd->header.height, psd->header.numChannels);

  return TPSD_LOAD_OK;
}

int tpsdLoadPSD(tpsdPSD* psd, const char* file)
{
  int size = 0;
  unsigned char* data = tpsdReadFileToMemory(file, &size);
  if (!data) return TPSD_LOAD_ERROR;
  tpsdLoadPSDFromMemory(psd, data, size);

  switch (psd->header.mode)
  {
  case TPSD_BITMAP:
    tpsdProcessBitmap(psd);
    break;
  case TPSD_GRAYSCALE:
    switch (psd->header.depth)
    {
    case 8:
      if (psd->header.numChannels == 1)
        tpsdProcessGrayscale8(psd);
      else
        tpsdProcessGrayscale8Alpha(psd);
      break;
    case 16:
      break;
    default:
      break;
    }
    break;
  case TPSD_INDEXED:
    tpsdProcessIndexed(psd);
    break;
  case TPSD_RGB:
    switch (psd->header.depth)
    {
    case 8:
      if (psd->header.numChannels == 3)
        tpsdProcessRGB8(psd);
      else
        tpsdProcessRGBA8(psd);
      break;
    case 16:
      break;
    default:
      break;
    }
    break;
  case TPSD_CMYK:
  case TPSD_MULTICHANNEL:
    switch (psd->header.depth)
    {
    case 8:
      if (psd->header.numChannels == 3)
        tpsdProcessCMY8(psd);
      else if (psd->header.numChannels == 4)
        tpsdProcessCMYK8(psd);
      else
        tpsdProcessCMYKA8(psd);
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

  return TPSD_LOAD_OK;
}

unsigned char* tpsdGetBitmapFromPSD(tpsdPSD* psd)
{
  const unsigned totalPixels = psd->header.width * psd->header.height;
  unsigned char *pixelData = TPSD_ALLOC(totalPixels * 8);
  if (!pixelData)
    return 0;
  
  for (unsigned i = 0; i < totalPixels; ++i)
  {
    pixelData[i * 8] = TPSD_GET_BIT(psd->compositeImage.bitmap.data[i], 0) * 255;
    pixelData[i * 8 + 1] = TPSD_GET_BIT(psd->compositeImage.bitmap.data[i], 1) * 255;
    pixelData[i * 8 + 2] = TPSD_GET_BIT(psd->compositeImage.bitmap.data[i], 2) * 255;
    pixelData[i * 8 + 3] = TPSD_GET_BIT(psd->compositeImage.bitmap.data[i], 3) * 255;
    pixelData[i * 8 + 4] = TPSD_GET_BIT(psd->compositeImage.bitmap.data[i], 4) * 255;
    pixelData[i * 8 + 5] = TPSD_GET_BIT(psd->compositeImage.bitmap.data[i], 5) * 255;
    pixelData[i * 8 + 6] = TPSD_GET_BIT(psd->compositeImage.bitmap.data[i], 6) * 255;
    pixelData[i * 8 + 7] = TPSD_GET_BIT(psd->compositeImage.bitmap.data[i], 7) * 255;
  }
  return pixelData;
}

unsigned char* tpsdGetGrayscale8FromPSD(tpsdPSD* psd)
{
  const unsigned totalPixels = psd->header.width * psd->header.height;
  unsigned char *pixelData = TPSD_ALLOC(totalPixels);
  if (!pixelData)
    return 0;

  memcpy(pixelData, psd->compositeImage.grayscale.data, totalPixels);
  return pixelData;
}

unsigned char* tpsdGetGrayscale8AlphaFromPSD(tpsdPSD* psd)
{
  const unsigned totalPixels = psd->header.width * psd->header.height;
  unsigned char *pixelData = TPSD_ALLOC(totalPixels * 4);
  if (!pixelData)
    return 0;

  for (unsigned i = 0; i < totalPixels; ++i)
  {
    pixelData[i * 4] = psd->compositeImage.grayscale.data[i];
    pixelData[i * 4 + 1] = psd->compositeImage.grayscale.data[i];
    pixelData[i * 4 + 2] = psd->compositeImage.grayscale.data[i];
    pixelData[i * 4 + 3] = psd->compositeImage.grayscale.alpha[i];
  }
  return pixelData;
}

unsigned char* tpsdGetIndexedFromPSD(tpsdPSD* psd)
{
  const unsigned totalPixels = psd->header.width * psd->header.height;
  unsigned char *pixelData = TPSD_ALLOC(totalPixels * 3);
  if (!pixelData)
    return 0;

  unsigned colorCount = psd->colorModeData.length / 3;
  unsigned char* red = psd->colorModeData.data;
  unsigned char* green = psd->colorModeData.data + colorCount;
  unsigned char* blue = psd->colorModeData.data + colorCount * 2;

  for (unsigned i = 0; i < totalPixels; ++i)
  {
    pixelData[i * 3] = red[psd->compositeImage.indexed.data[i]];
    pixelData[i * 3 + 1] = green[psd->compositeImage.indexed.data[i]];
    pixelData[i * 3 + 2] = blue[psd->compositeImage.indexed.data[i]];
  }
  return pixelData;
}

unsigned char* tpsdGetRGB8FromPSD(tpsdPSD* psd)
{
  const unsigned totalPixels = psd->header.width * psd->header.height;
  unsigned char *pixelData = TPSD_ALLOC(totalPixels * 3);
  if (!pixelData)
    return 0;

  for (unsigned i = 0; i < totalPixels; ++i)
  {
    pixelData[i * 3] = psd->compositeImage.rgb.red[i];
    pixelData[i * 3 + 1] = psd->compositeImage.rgb.green[i];
    pixelData[i * 3 + 2] = psd->compositeImage.rgb.blue[i];
  }
  return pixelData;
}

unsigned char* tpsdGetRGBA8FromPSD(tpsdPSD* psd)
{
  const unsigned totalPixels = psd->header.width * psd->header.height;
  unsigned char *pixelData = TPSD_ALLOC(totalPixels * 4);
  if (!pixelData)
    return 0;

  for (unsigned i = 0; i < totalPixels; ++i)
  {
    pixelData[i * 4] = psd->compositeImage.rgb.red[i];
    pixelData[i * 4 + 1] = psd->compositeImage.rgb.green[i];
    pixelData[i * 4 + 2] = psd->compositeImage.rgb.blue[i];
    pixelData[i * 4 + 3] = psd->compositeImage.rgb.alpha[i];
  }
  return pixelData;
}

unsigned char* tpsdGetCMY8FromPSD(tpsdPSD* psd)
{
  const unsigned totalPixels = psd->header.width * psd->header.height;
  unsigned char *pixelData = TPSD_ALLOC(totalPixels * 3);
  if (!pixelData)
    return 0;

  for (unsigned i = 0; i < totalPixels; ++i)
  {
    pixelData[i * 3] = tpsdCMYKtoRGB(255 - psd->compositeImage.cmyk.cyan[i], 255);
    pixelData[i * 3 + 1] = tpsdCMYKtoRGB(255 - psd->compositeImage.cmyk.magenta[i], 255);
    pixelData[i * 3 + 2] = tpsdCMYKtoRGB(255 - psd->compositeImage.cmyk.yellow[i], 255);
  }
  return pixelData;
}

unsigned char* tpsdGetCMYK8FromPSD(tpsdPSD* psd)
{
  const unsigned totalPixels = psd->header.width * psd->header.height;
  unsigned char *pixelData = TPSD_ALLOC(totalPixels * 3);
  if (!pixelData)
    return 0;

  for (unsigned i = 0; i < totalPixels; ++i)
  {
    pixelData[i * 3] = tpsdCMYKtoRGB(255 - psd->compositeImage.cmyk.cyan[i], (255 - psd->compositeImage.cmyk.key[i]));
    pixelData[i * 3 + 1] = tpsdCMYKtoRGB(255 - psd->compositeImage.cmyk.magenta[i], 255 - psd->compositeImage.cmyk.key[i]);
    pixelData[i * 3 + 2] = tpsdCMYKtoRGB(255 - psd->compositeImage.cmyk.yellow[i], 255 - psd->compositeImage.cmyk.key[i]);
  }
  return pixelData;
}

unsigned char* tpsdGetCMYKA8FromPSD(tpsdPSD* psd)
{
  const unsigned totalPixels = psd->header.width * psd->header.height;
  unsigned char *pixelData = TPSD_ALLOC(totalPixels * 4);
  if (!pixelData)
    return 0;

  for (unsigned i = 0; i < totalPixels; ++i)
  {
    pixelData[i * 4] = tpsdCMYKtoRGB(255 - psd->compositeImage.cmyk.cyan[i], 255 - psd->compositeImage.cmyk.key[i]);
    pixelData[i * 4 + 1] = tpsdCMYKtoRGB(255 - psd->compositeImage.cmyk.magenta[i], 255 - psd->compositeImage.cmyk.key[i]);
    pixelData[i * 4 + 2] = tpsdCMYKtoRGB(255 - psd->compositeImage.cmyk.yellow[i], 255 - psd->compositeImage.cmyk.key[i]);
    pixelData[i * 4 + 3] = psd->compositeImage.cmyk.alpha[i];
  }
  return pixelData;
}

tpsdImage* tpsdGetImageFromPSD(tpsdPSD* psd) 
{
  tpsdImage* image = TPSD_ALLOC(sizeof(tpsdImage));
  image->width = psd->header.width;
  image->height = psd->header.height;

  switch (psd->header.mode)
  {
  case TPSD_BITMAP:
    image->pixels = tpsdGetBitmapFromPSD(psd);
    break;
  case TPSD_GRAYSCALE:
    switch (psd->header.depth)
    {
    case 8:
      if(psd->header.numChannels == 1)
        image->pixels = tpsdGetGrayscale8FromPSD(psd);
      else
        image->pixels = tpsdGetGrayscale8AlphaFromPSD(psd);
      break;
    case 16:
      break;
    default:
      break;
    }
    break;
  case TPSD_INDEXED:
    image->pixels = tpsdGetIndexedFromPSD(psd);
    break;
  case TPSD_RGB:
    switch (psd->header.depth)
    {
    case 8:
      if (psd->header.numChannels == 3)
        image->pixels = tpsdGetRGB8FromPSD(psd);
      else
        image->pixels = tpsdGetRGBA8FromPSD(psd);
      break;
    case 16:
      break;
    default:
      break;
    }
    break;
  case TPSD_CMYK:
  case TPSD_MULTICHANNEL:
    switch (psd->header.depth)
    {
    case 8:
      if (psd->header.numChannels == 3)
        image->pixels = tpsdGetCMY8FromPSD(psd);
      else if (psd->header.numChannels == 4)
        image->pixels = tpsdGetCMYK8FromPSD(psd);
      else
        image->pixels = tpsdGetCMYKA8FromPSD(psd);
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

  return image;
}

#endif /* TINYPSD_IMPLEMENTATION */

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2017 Tyler Pugmire
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/