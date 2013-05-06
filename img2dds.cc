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

#include <algorithm>
#include <cstdio>
#include <endian.h>
#include <FreeImagePlus.h>
#include <squish.h>

using namespace std;

static const int DDSD_CAPS        = 0x00000001;
static const int DDSD_HEIGHT      = 0x00000002;
static const int DDSD_WIDTH       = 0x00000004;
static const int DDSD_PITCH       = 0x00000008;
static const int DDSD_PIXELFORMAT = 0x00001000;
static const int DDSD_MIPMAPCOUNT = 0x00020000;
static const int DDSD_LINEARSIZE  = 0x00080000;

static const int DDSDCAPS_COMPLEX = 0x00000008;
static const int DDSDCAPS_TEXTURE = 0x00001000;
static const int DDSDCAPS_MIPMAP  = 0x00400000;

static const int DDPF_ALPHAPIXELS = 0x00000001;
static const int DDPF_FOURCC      = 0x00000004;
static const int DDPF_RGB         = 0x00000040;
static const int DDPF_LUMINANCE   = 0x00020000;

// True iff `i` is a power of two.
static inline bool isPow2( int v )
{
  return ( v & ( v - 1 ) ) == 0;
}

// Index of the first 1 bit (counting from the least significant bit) or -1 if `v` is 0.
static inline int index1( int v )
{
  return v == 0 ? -1 : 31 - __builtin_clz( v );
}

static inline void writeInt( int i, FILE* f )
{
#if defined( __BIG_ENDIAN__ ) || ( defined( __BYTE_ORDER__ ) && __BYTE_ORDER__ == 4321 )
  i = __builtin_bswap32( i );
#endif

  fwrite( &i, sizeof( i ), 1, f );
}

bool buildDDS( const char* inPath, int options, const char* outPath )
{
  fipImage image;
  if( !image.load( inPath ) ) {
    fprintf( stderr, "img2dds: Failed to load image `%s'\n", inPath );
    return false;
  }

  image.flipVertical();

  int width  = int( image.getWidth() );
  int height = int( image.getHeight() );
  int bpp    = int( image.getBitsPerPixel() );
  int pitch  = int( image.getScanWidth() );

  if( ( options & COMPRESSION_BIT ) && ( !isPow2( width ) || !isPow2( height ) ) ) {
    fprintf( stderr, "img2dds: Image dimensions must be powers of 2 to use compression\n" );
    return false;
  }

  int pitchOrLinSize = pitch;
  int nMipmaps       = options & MIPMAPS_BIT ? index1( max( width, height ) ) + 1 : 1;

  int flags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
  flags |= options & MIPMAPS_BIT ? DDSD_MIPMAPCOUNT : 0;
  flags |= options & COMPRESSION_BIT ? DDSD_LINEARSIZE : DDSD_PITCH;

  int caps = DDSDCAPS_TEXTURE;
  caps |= options & MIPMAPS_BIT ? DDSDCAPS_COMPLEX | DDSDCAPS_MIPMAP : 0;

  int pixelFlags = 0;
  pixelFlags |= bpp == 32 ? DDPF_ALPHAPIXELS : 0;
  pixelFlags |= options & COMPRESSION_BIT ? DDPF_FOURCC :
                bpp == 8 ? DDPF_LUMINANCE : DDPF_RGB;

  int squishFlags = bpp == 32 ? squish::kDxt5 : squish::kDxt1;
  squishFlags |= options & QUALITY_BIT ?
                 squish::kColourIterativeClusterFit | squish::kWeightColourByAlpha :
                 squish::kColourRangeFit;

  if( options & COMPRESSION_BIT ) {
    pitchOrLinSize = squish::GetStorageRequirements( width, height, squishFlags );
  }

  FILE* out = fopen( outPath, "wb" );
  if( out == NULL ) {
    fprintf( stderr, "img2dds: Failed to open `%s' for writing\n", outPath );
    return false;
  }

  // Header beginning.
  fwrite( "DDS ", 1, 4, out );
  writeInt( 124, out );
  writeInt( flags, out );
  writeInt( height, out );
  writeInt( width, out );
  writeInt( pitchOrLinSize, out );
  writeInt( 0, out );
  writeInt( nMipmaps, out );

  // Reserved int[11].
  writeInt( 0, out );
  writeInt( 0, out );
  writeInt( 0, out );
  writeInt( 0, out );
  writeInt( 0, out );
  writeInt( 0, out );
  writeInt( 0, out );
  writeInt( 0, out );
  writeInt( 0, out );
  writeInt( 0, out );
  writeInt( 0, out );

  // Pixel format.
  writeInt( 32, out );
  writeInt( pixelFlags, out );
  fwrite( bpp == 32 ? "DXT5" : "DXT1", 1, 4, out );
  writeInt( bpp, out );
  writeInt( 0x00ff0000, out );
  writeInt( 0x0000ff00, out );
  writeInt( 0x000000ff, out );
  writeInt( 0xff000000, out );

  writeInt( caps, out );
  writeInt( 0, out );
  writeInt( 0, out );
  writeInt( 0, out );
  writeInt( 0, out );

  for( int i = 0; i < nMipmaps; ++i ) {
    fipImage level = image;

    if( i != 0 ) {
      level.rescale( uint( width ), uint( height ),
                     options & QUALITY_BIT ? FILTER_CATMULLROM : FILTER_BOX );
      pitch = int( level.getScanWidth() );
    }

    if( options & COMPRESSION_BIT ) {
      level.convertTo32Bits();

      int   size   = squish::GetStorageRequirements( width, height, squishFlags );
      BYTE* pixels = level.accessPixels();

      // Swap red and blue channels.
      for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
          swap( pixels[x * 4 + 0], pixels[x * 4 + 2] );
        }
        pixels += pitch;
      }

      void* blocks = malloc( size );
      squish::CompressImage( level.accessPixels(), width, height, blocks, squishFlags );

      fwrite( blocks, 1, size, out );
      free( blocks );
    }
    else {
      BYTE* pixels = level.accessPixels();

      for( int i = 0; i < height; ++i ) {
        fwrite( pixels, 1, width * ( bpp / 8 ), out );
        pixels += pitch;
      }
    }

    width  = max( width / 2, 1 );
    height = max( height / 2, 1 );
  }

  fclose( out );
  return true;
}
