/*
 * img2dds - DDS image builder.
 *
 * Copyright © 2002-2013 Davorin Učakar
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

#include "img2dds.hh"

#include <cstdio>
#include <cstdlib>
#include <getopt.h>

static void usage()
{
  printf(
    "Usage: img2dds [-c] [-m] [-q] <inputImage> <outputDDS>\n"
    "\t-c\tUse S3 texture compression\n"
    "\t-m\tGenerate mipmaps\n"
    "\t-q\tUse top quality for texture compression and mipmap scaling.\n"
  );
  exit( 1 );
}

int main( int argc, char** argv )
{
  int ddsOptions = 0;

  int opt;
  while( ( opt = getopt( argc, argv, "cmq" ) ) >= 0 ) {
    switch( opt ) {
      case 'c': {
        ddsOptions |= COMPRESSION_BIT;
        break;
      }
      case 'm': {
        ddsOptions |= MIPMAPS_BIT;
        break;
      }
      case 'q': {
        ddsOptions |= QUALITY_BIT;
        break;
      }
      default: {
        usage();
      }
    }
  }

  if( argc - optind != 2 ) {
    usage();
  }

  return !buildDDS( argv[optind], ddsOptions, argv[optind + 1] );
}
