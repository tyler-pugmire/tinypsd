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
typedef struct tpsdPSD tpsdPSD;

int tpsdLoadPSD(tpsdPSD* psd, char const* file);

struct tpsdHeader
{
  int signature; //Signature: always equal to '8BPS' . Do not try to read the file if the signature does not match this value.
  short version; //Version: always equal to 1. Do not try to read the file if the version does not match this value. (**PSB** version is 2.)
  //6 bytes reserved here must be 0;
  short numChannels; //The number of channels in the image, including any alpha channels.Supported range is 1 to 56.
  unsigned height; //The height of the image in pixels. Supported range is 1 to 30,000. (**PSB** max of 300, 000.)
  unsigned width; //The width of the image in pixels. Supported range is 1 to 30,000. (*PSB** max of 300, 000)
  short depth; //Depth: the number of bits per channel. Supported values are 1, 8, 16 and 32.
  short mode; //The color mode of the file. Supported values are: Bitmap = 0; Grayscale = 1; Indexed = 2; RGB = 3; CMYK = 4; Multichannel = 7; Duotone = 8; Lab = 9.
};

struct tpsdPSD
{
  tpsdHeader header;
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

static char* tpsdReadFileToMemory(const char* path, int* size)
{
  char* data = 0;
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

unsigned short tpsdGetShort(char* data, int offset)
{
  return (short)(((short)data[offset]) << 8) | data[offset + 1];
}

unsigned tpsdGetInt(char* data, int offset)
{
  return (data[offset] << 24) | (data[offset + 1] << 16) | (data[offset + 2] << 8) | data[offset + 3];
}

unsigned short tpsdGetShortAndAdvance(char* data, int *offset)
{
  unsigned short num = (short)(((short)data[*offset]) << 8) | data[*offset + 1];
  *offset += sizeof(short);
  return num;
}

unsigned tpsdGetIntAndAdvance(char* data, int *offset)
{
  unsigned num = (data[*offset] << 24) | (data[*offset + 1] << 16) | (data[*offset + 2] << 8) | data[*offset + 3];
  *offset += sizeof(unsigned);
  return num;
}

tpsdHeader tpsdLoadPSDHeaderFromMemory(char* data, int* offset)
{
  tpsdHeader header = { 0 };
  header.signature = tpsdGetIntAndAdvance(data, offset);
  TPSD_ASSERT(header.signature == '8BPS');

  header.version = tpsdGetShortAndAdvance(data, offset);
  TPSD_ASSERT(header.version == 1);

  *offset += 6; //6 bytes reserved here and must be zero;

  header.numChannels = tpsdGetShortAndAdvance(data, offset);
  header.height = tpsdGetIntAndAdvance(data, offset);
  header.width = tpsdGetIntAndAdvance(data, offset);
  header.depth = tpsdGetShortAndAdvance(data, offset);
  header.mode = tpsdGetShortAndAdvance(data, offset);

  return header;
}

int tpsdLoadPSDFromMemory(tpsdPSD* psd, char* data, int len)
{
  unsigned offset = 0;
  psd->header = tpsdLoadPSDHeaderFromMemory(data, &offset);

  return TPSD_LOAD_OK;
}

int tpsdLoadPSD(tpsdPSD* psd, const char* file)
{
  int size = 0;
  char* data = tpsdReadFileToMemory(file, &size);
  if (!data) return TPSD_LOAD_ERROR;
  tpsdLoadPSDFromMemory(psd, data, size);
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