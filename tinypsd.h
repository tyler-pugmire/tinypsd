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
typedef struct tpsdRGBImage tpsdRGBImage;
typedef struct tpsdCMYKImage tpsdCMYKImage;

typedef struct tpsdCompositeImage tpsdCompositeImage;
typedef struct tpsdPSD tpsdPSD;

int tpsdLoadPSD(tpsdPSD* psd, char const* file);

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
  char* data;
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

////   end header file   /////////////////////////////////////////////////////
#endif /* TINYPSD_H */


#define TINYPSD_IMPLEMENTATION

#ifdef TINYPSD_IMPLEMENTATION

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
  *offset += cmd.length;
  return cmd;
}

tpsdImageResource tpsdParseImageResource(unsigned unsigned char* data, unsigned* offset)
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
  case 0: // 0 = Raw image data
  {
    imageData.data = TPSD_ALLOC(width * height * numChannels);
    memcpy(imageData.data, data + *offset, width * height * numChannels);
    *offset += width * height * numChannels;
  }
    break;
  case 1: // 1 = RLE compressed the image data starts with the byte counts for all the scan lines (rows * channels), with each count stored as a two-byte value.
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
  case 2: // 2 = ZIP without prediction(Unsupported)
    break;
  case 3: // 3 = ZIP with prediction (Unsupported)
    break;
  default:
    break;
  }

  return imageData;
}

void tpsdProcessBitmap(tpsdPSD* psd)
{
  unsigned totalPixels = psd->header.width * psd->header.height;
  psd->compositeImage.bitmap.data = TPSD_ALLOC(totalPixels / 8);
  memcpy(psd->compositeImage.bitmap.data, psd->imageData.data, totalPixels / 8);
}

void tpsdProcessGrayscale8(tpsdPSD* psd)
{
  unsigned totalPixels = psd->header.width * psd->header.height;
  psd->compositeImage.grayscale.data = TPSD_ALLOC(totalPixels);
  memcpy(psd->compositeImage.grayscale.data, psd->imageData.data, totalPixels);
}

void tpsdProcessRGB8(tpsdPSD* psd)
{
  unsigned totalPixels = psd->header.width * psd->header.height;
  psd->compositeImage.rgb.red = TPSD_ALLOC(totalPixels);
  memcpy(psd->compositeImage.rgb.red, psd->imageData.data, totalPixels);
  psd->compositeImage.rgb.green = TPSD_ALLOC(totalPixels);
  memcpy(psd->compositeImage.rgb.green, psd->imageData.data + totalPixels, totalPixels);
  psd->compositeImage.rgb.blue = TPSD_ALLOC(totalPixels);
  memcpy(psd->compositeImage.rgb.blue, psd->imageData.data + totalPixels * 2, totalPixels);
}

void tpsdProcessRGBA8(tpsdPSD* psd)
{
  unsigned totalPixels = psd->header.width * psd->header.height;
  psd->compositeImage.rgb.red = TPSD_ALLOC(totalPixels);
  memcpy(psd->compositeImage.rgb.red, psd->imageData.data, totalPixels);
  psd->compositeImage.rgb.green = TPSD_ALLOC(totalPixels);
  memcpy(psd->compositeImage.rgb.green, psd->imageData.data + totalPixels, totalPixels);
  psd->compositeImage.rgb.blue = TPSD_ALLOC(totalPixels);
  memcpy(psd->compositeImage.rgb.blue, psd->imageData.data + totalPixels * 2, totalPixels);
  psd->compositeImage.rgb.alpha = TPSD_ALLOC(totalPixels);
  memcpy(psd->compositeImage.rgb.alpha, psd->imageData.data + totalPixels * 3, totalPixels);
}

void tpsdProcessCMY8(tpsdPSD* psd)
{
  unsigned totalPixels = psd->header.width * psd->header.height;
  psd->compositeImage.cmyk.cyan = TPSD_ALLOC(totalPixels);
  memcpy(psd->compositeImage.cmyk.cyan, psd->imageData.data, totalPixels);
  psd->compositeImage.cmyk.magenta = TPSD_ALLOC(totalPixels);
  memcpy(psd->compositeImage.cmyk.magenta, psd->imageData.data + totalPixels, totalPixels);
  psd->compositeImage.cmyk.yellow = TPSD_ALLOC(totalPixels);
  memcpy(psd->compositeImage.cmyk.yellow, psd->imageData.data + totalPixels * 2, totalPixels);
}

void tpsdProcessCMYK8(tpsdPSD* psd)
{
  unsigned totalPixels = psd->header.width * psd->header.height;
  psd->compositeImage.cmyk.cyan = TPSD_ALLOC(totalPixels);
  memcpy(psd->compositeImage.cmyk.cyan, psd->imageData.data, totalPixels);
  psd->compositeImage.cmyk.magenta = TPSD_ALLOC(totalPixels);
  memcpy(psd->compositeImage.cmyk.magenta, psd->imageData.data + totalPixels, totalPixels);
  psd->compositeImage.cmyk.yellow = TPSD_ALLOC(totalPixels);
  memcpy(psd->compositeImage.cmyk.yellow, psd->imageData.data + totalPixels * 2, totalPixels);
  psd->compositeImage.cmyk.key = TPSD_ALLOC(totalPixels);
  memcpy(psd->compositeImage.cmyk.key, psd->imageData.data + totalPixels * 3, totalPixels);
}

void tpsdProcessCMYKA8(tpsdPSD* psd)
{
  unsigned totalPixels = psd->header.width * psd->header.height;
  psd->compositeImage.cmyk.cyan = TPSD_ALLOC(totalPixels);
  memcpy(psd->compositeImage.cmyk.cyan, psd->imageData.data, totalPixels);
  psd->compositeImage.cmyk.magenta = TPSD_ALLOC(totalPixels);
  memcpy(psd->compositeImage.cmyk.magenta, psd->imageData.data + totalPixels, totalPixels);
  psd->compositeImage.cmyk.yellow = TPSD_ALLOC(totalPixels);
  memcpy(psd->compositeImage.cmyk.yellow, psd->imageData.data + totalPixels * 2, totalPixels);
  psd->compositeImage.cmyk.key = TPSD_ALLOC(totalPixels);
  memcpy(psd->compositeImage.cmyk.key, psd->imageData.data + totalPixels * 3, totalPixels);
  psd->compositeImage.cmyk.alpha = TPSD_ALLOC(totalPixels);
  memcpy(psd->compositeImage.cmyk.alpha, psd->imageData.data + totalPixels * 4, totalPixels);
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
  case 0:
    tpsdProcessBitmap(psd);
    break;
  case 1:
    switch (psd->header.depth)
    {
    case 8:
      tpsdProcessGrayscale8(psd);
      break;
    case 16:
      break;
    default:
      break;
    }
    break;
  case 2:
    break;
  case 3:
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
  case 4:
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
  case 5:
    break;
  case 6:
    break;
  case 7:
    break;
  case 8:
    break;
  case 9:
    break;
  default:
    break;
  }

  return TPSD_LOAD_OK;
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