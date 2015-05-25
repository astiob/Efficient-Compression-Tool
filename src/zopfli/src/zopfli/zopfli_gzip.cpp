/*
Copyright 2013 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Author: lode.vandevenne@gmail.com (Lode Vandevenne)
Author: jyrki.alakuijala@gmail.com (Jyrki Alakuijala)
*/

#include <stdio.h>

#include "util.h"
#include "zopfli.h"
#include "../zopflipng/lodepng/lodepng.h"
#include "deflate.h"
#include "zopfli.h"
#include "deflate.h"
#include "zlib_container.h"
#include "../../../main.h"

/*
Compresses the data according to the gzip specification.
*/
static void ZopfliGzipCompress(const ZopfliOptions* options,
                        const unsigned char* in, size_t insize,
                        unsigned char** out, size_t* outsize) {
  unsigned long crcvalue = lodepng_crc32(in, insize);
  unsigned char bp = 0;

  ZOPFLI_APPEND_DATA(31, out, outsize);  /* ID1 */
  ZOPFLI_APPEND_DATA(139, out, outsize);  /* ID2 */
  ZOPFLI_APPEND_DATA(8, out, outsize);  /* CM */
  ZOPFLI_APPEND_DATA(0, out, outsize);  /* FLG */
  /* MTIME */
  ZOPFLI_APPEND_DATA(0, out, outsize);
  ZOPFLI_APPEND_DATA(0, out, outsize);
  ZOPFLI_APPEND_DATA(0, out, outsize);
  ZOPFLI_APPEND_DATA(0, out, outsize);

  ZOPFLI_APPEND_DATA(2, out, outsize);  /* XFL, 2 indicates best compression. */
  ZOPFLI_APPEND_DATA(3, out, outsize);  /* OS follows Unix conventions. */

  ZopfliDeflate(options, 1, in, insize, &bp, out, outsize);

  /* CRC */
  ZOPFLI_APPEND_DATA(crcvalue % 256, out, outsize);
  ZOPFLI_APPEND_DATA((crcvalue >> 8) % 256, out, outsize);
  ZOPFLI_APPEND_DATA((crcvalue >> 16) % 256, out, outsize);
  ZOPFLI_APPEND_DATA((crcvalue >> 24) % 256, out, outsize);

  /* ISIZE */
  ZOPFLI_APPEND_DATA(insize % 256, out, outsize);
  ZOPFLI_APPEND_DATA((insize >> 8) % 256, out, outsize);
  ZOPFLI_APPEND_DATA((insize >> 16) % 256, out, outsize);
  ZOPFLI_APPEND_DATA((insize >> 24) % 256, out, outsize);
}

static void ZopfliCompress(const ZopfliOptions* options, ZopfliFormat output_type,
                    const unsigned char* in, size_t insize,
                    unsigned char** out, size_t* outsize) {
  if (output_type == ZOPFLI_FORMAT_GZIP) {
    ZopfliGzipCompress(options, in, insize, out, outsize);
  } else if (output_type == ZOPFLI_FORMAT_ZLIB) {
    //ZopfliZlibCompress(options, in, insize, out, outsize);
  } else if (output_type == ZOPFLI_FORMAT_DEFLATE) {
    unsigned char bp = 0;
    ZopfliDeflate(options, 1, in, insize, &bp, out, outsize);
  }
}

/*
 Loads a file into a memory array.
 */
static void LoadFile(const char* filename,
                     unsigned char** out, size_t* outsize) {
  FILE* file;

  *out = 0;
  *outsize = 0;
  file = fopen(filename, "rb");
  if (!file) return;

  fseek(file , 0 , SEEK_END);
  *outsize = ftell(file);
  rewind(file);

  *out = (unsigned char*)malloc(*outsize);

  if (*outsize && (*out)) {
    size_t testsize = fread(*out, 1, *outsize, file);
    if (testsize != *outsize) {
      /* It could be a directory */
      free(*out);
      *out = 0;
      *outsize = 0;
    }
  }
  if (!(*outsize) && !out){ /* If size is not zero, out must be allocated. */
    printf ("Memory error");
    exit(1);
  }
  fclose(file);
}

/*
 Saves a file from a memory array, overwriting the file if it existed.
 */
static void SaveFile(const char* filename,
                     const unsigned char* in, size_t insize) {
  FILE* file = fopen(filename, "wb" );
  if (file == NULL){
    printf ("Can't write to file");
  }
  else {
  fwrite((char*)in, 1, insize, file);
  fclose(file);
  }
}

/*
 outfilename: filename to write output to, or 0 to write to stdout instead
 */
static void CompressFile(const ZopfliOptions* options,
                         ZopfliFormat output_type,
                         const char* infilename,
                         const char* outfilename) {
  unsigned char* in;
  size_t insize;
  unsigned char* out = 0;
  size_t outsize = 0;
  LoadFile(infilename, &in, &insize);
  if (insize == 0) {
    fprintf(stderr, "Invalid filename: %s\n", infilename);
    return;
  }

  ZopfliCompress(options, output_type, in, insize, &out, &outsize);

  SaveFile(outfilename, out, outsize);

  free(out);
  free(in);
}

int ZopfliGzip(const char* filename, const char* outname, int mode) {
  ZopfliOptions options;
  //ZopfliFormat output_type = ZOPFLI_FORMAT_GZIP;
  //output_type = ZOPFLI_FORMAT_ZLIB;
  //output_type = ZOPFLI_FORMAT_DEFLATE;
  //options.blocksplittinglast = 1;

  ZopfliInitOptions(&options, mode);
  //Append ".gz" ".zlib" ".deflate"
  CompressFile(&options, ZOPFLI_FORMAT_GZIP, filename, outname != NULL ? outname : ((std::string)filename).append(".gz").c_str());

  return 0;
}