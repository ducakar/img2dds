/*
 * img2dds - DDS image builder.
 *
 * Copyright © 2002-2014 Davorin Učakar
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software in
 *    a product, an acknowledgement in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "ImageBuilder.hh"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <string>

using namespace std;

static void printUsage()
{
  printf(
    "Usage: ozDDS [options] <inputImage> [<outputDirOrFile>]\n"
    "  -c  Use S3 texture compression (DXT1 or DXT5 if the image has transparent pixels)\n"
    "  -h  Flip horizontally\n"
    "  -m  Generate mipmaps\n"
    "  -n  Set normal map flag (DDPF_NORMAL)\n"
    "  -N  Set normal map flag if the image looks like a RGB = XYZ normal map\n"
    "      disable -n, -s and -S options otherwise\n"
    "  -r  Set readable flag for DDSLoader\n"
    "  -s  Do RGB -> GGGR swizzle (for DXT5nm), ignored for MBM normal maps\n"
    "  -S  Do RGB -> BGBR swizzle (for DXT5nm+z), ignored for MBM normal maps\n"
    "  -v  Flip vertically\n\n");
}

int main(int argc, char** argv)
{
  int  ddsOptions    = 0;
  bool detectNormals = false;

  int opt;
  while ((opt = getopt(argc, argv, "chmnNrsSv")) >= 0) {
    switch (opt) {
      case 'c': {
        ddsOptions |= ImageBuilder::COMPRESSION_BIT;
        break;
      }
      case 'h': {
        ddsOptions |= ImageBuilder::FLOP_BIT;
        break;
      }
      case 'm': {
        ddsOptions |= ImageBuilder::MIPMAPS_BIT;
        break;
      }
      case 'n': {
        ddsOptions |= ImageBuilder::NORMAL_MAP_BIT;
        break;
      }
      case 'N': {
        detectNormals = true;
        break;
      }
      case 'r': {
        ddsOptions |= ImageBuilder::READABLE_BIT;
        break;
      }
      case 's': {
        ddsOptions |= ImageBuilder::YYYX_BIT;
        break;
      }
      case 'S': {
        ddsOptions |= ImageBuilder::ZYZX_BIT;
        break;
      }
      case 'v': {
        ddsOptions |= ImageBuilder::FLIP_BIT;
        break;
      }
      default: {
        printUsage();
        return EXIT_FAILURE;
      }
    }
  }

  int nArgs = argc - optind;
  if (nArgs < 1 || nArgs > 2) {
    printUsage();
    return EXIT_FAILURE;
  }

  ImageData image = ImageBuilder::loadImage(argv[optind]);

  if (image.isEmpty()) {
    printf("Failed to open image '%s'.\n", argv[optind]);
    return EXIT_FAILURE;
  }

  if (image.flags & ImageData::NORMAL_BIT) {
    ddsOptions |= ImageBuilder::NORMAL_MAP_BIT;
    ddsOptions &= ~(ImageBuilder::YYYX_BIT | ImageBuilder::ZYZX_BIT);
  }
  else if (detectNormals) {
    if (image.isNormalMap()) {
      ddsOptions |= ImageBuilder::NORMAL_MAP_BIT;
    }
    else {
      ddsOptions &= ~ImageBuilder::NORMAL_MAP_BIT;
      ddsOptions &= ~(ImageBuilder::YYYX_BIT | ImageBuilder::ZYZX_BIT);
    }
  }

  string destFile;

  if (nArgs == 2) {
    destFile = argv[optind + 1];
  }
  else {
    const char* dot = strrchr(argv[optind], '.');

    if (dot == nullptr) {
      printf("File extensfion missing: '%s'.\n", argv[optind]);
      return false;
    }
    else {
      destFile = string(argv[optind], size_t(dot - argv[optind])) + ".dds";
    }
  }

  if (!ImageBuilder::createDDS(&image, 1, ddsOptions, destFile.c_str())) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
