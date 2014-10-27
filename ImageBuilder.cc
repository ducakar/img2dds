/*
 * ozFactory - OpenZone Assets Builder Library.
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

/**
 * @file ozFactory/ImageBuilder.cc
 */

#include "ImageBuilder.hh"

#include <algorithm>
#include <assert.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <FreeImage.h>
#include <squish.h>

using namespace std;

namespace oz
{

static const unsigned DDSD_CAPS                          = 0x00000001;
static const unsigned DDSD_HEIGHT                        = 0x00000002;
static const unsigned DDSD_WIDTH                         = 0x00000004;
static const unsigned DDSD_PITCH                         = 0x00000008;
static const unsigned DDSD_PIXELFORMAT                   = 0x00001000;
static const unsigned DDSD_MIPMAPCOUNT                   = 0x00020000;
static const unsigned DDSD_LINEARSIZE                    = 0x00080000;

static const unsigned DDSCAPS_COMPLEX                    = 0x00000008;
static const unsigned DDSCAPS_MIPMAP                     = 0x00400000;
static const unsigned DDSCAPS_TEXTURE                    = 0x00001000;

static const unsigned DDSCAPS2_CUBEMAP                   = 0x00000200;
static const unsigned DDSCAPS2_CUBEMAP_POSITIVEX         = 0x00000400;
static const unsigned DDSCAPS2_CUBEMAP_NEGITIVEX         = 0x00000800;
static const unsigned DDSCAPS2_CUBEMAP_POSITIVEY         = 0x00001000;
static const unsigned DDSCAPS2_CUBEMAP_NEGITIVEY         = 0x00002000;
static const unsigned DDSCAPS2_CUBEMAP_POSITIVEZ         = 0x00004000;
static const unsigned DDSCAPS2_CUBEMAP_NEGITIVEZ         = 0x00008000;

static const unsigned DDPF_ALPHAPIXELS                   = 0x00000001;
static const unsigned DDPF_FOURCC                        = 0x00000004;
static const unsigned DDPF_RGB                           = 0x00000040;
static const unsigned DDPF_NORMAL                        = 0x80000000;

static const unsigned DXGI_FORMAT_R8G8B8A8_UNORM         = 28;
static const unsigned DXGI_FORMAT_BC1_UNORM              = 71;
static const unsigned DXGI_FORMAT_BC3_UNORM              = 77;

static const unsigned D3D10_RESOURCE_DIMENSION_TEXTURE2D = 3;

static inline int index1(int v)
{
  return v == 0 ? -1 : 31 - __builtin_clz(v);
}

static inline void writeInt(int i, FILE* f)
{
#if defined( __BIG_ENDIAN__ ) || ( defined( __BYTE_ORDER__ ) && __BYTE_ORDER__ == 4321 )
  i = __builtin_bswap32(i);
#endif

  fwrite(&i, sizeof(i), 1, f);
}

static inline void writeChars(const char* bytes, int count, FILE* f)
{
  fwrite(bytes, count, 1, f);
}

static void printError(FREE_IMAGE_FORMAT fif, const char* message)
{
  printf("FreeImage(%s): %s", FreeImage_GetFormatFromFIF(fif), message);
}

static FIBITMAP* createBitmap(const ImageData& image)
{
  FIBITMAP* dib = FreeImage_ConvertFromRawBits(reinterpret_cast<BYTE*>(image.pixels),
                                               image.width, image.height, image.width * 4, 32,
                                               FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK,
                                               FI_RGBA_BLUE_MASK);

  // Convert RGBA -> BGRA.
  int   size   = image.width * image.height * 4;
  BYTE* pixels = FreeImage_GetBits(dib);

  for (int i = 0; i < size; i += 4) {
    BYTE temp = pixels[i + 0];

    pixels[i + 0] = pixels[i + 2];
    pixels[i + 2] = temp;
  }

  if (dib == nullptr) {
    printf("FreeImage_ConvertFromRawBits failed to build image.");
  }

  FreeImage_SetTransparent(dib, image.flags & ImageData::ALPHA_BIT);
  return dib;
}

static FIBITMAP* loadBitmap(const char* file)
{
  FREE_IMAGE_FORMAT format = FreeImage_GetFileType(file);
  FIBITMAP*         dib    = FreeImage_Load(format < 0 ? FIF_TARGA : format, file);

  if (dib == nullptr) {
    printf("Failed to read '%s'.", file);
    return nullptr;
  }

  FIBITMAP* oldDib = dib;
  dib = FreeImage_ConvertTo32Bits(dib);
  FreeImage_Unload(oldDib);

  // Remove alpha if unused.
  int   width    = int(FreeImage_GetWidth(dib));
  int   height   = int(FreeImage_GetHeight(dib));
  int   size     = width * height * 4;
  bool  hasAlpha = false;
  BYTE* pixels   = FreeImage_GetBits(dib);

  for (int i = 0; i < size; i += 4) {
    if (pixels[i + 3] != 255) {
      hasAlpha = true;
      break;
    }
  }
  if (!hasAlpha) {
    FreeImage_SetTransparent(dib, false);
  }

  FreeImage_FlipVertical(dib);
  return dib;
}

static bool buildDDS(FIBITMAP** faces, int nFaces, int options, const char* destFile)
{
  assert(nFaces > 0);

  int width      = int(FreeImage_GetWidth(faces[0]));
  int height     = int(FreeImage_GetHeight(faces[0]));

  bool isCubeMap = options & ImageBuilder::CUBE_MAP_BIT;
  bool isNormal  = options & ImageBuilder::NORMAL_MAP_BIT;
  bool doMipmaps = options & ImageBuilder::MIPMAPS_BIT;
  bool compress  = options & ImageBuilder::COMPRESSION_BIT;
  bool doFlip    = options & ImageBuilder::FLIP_BIT;
  bool doFlop    = options & ImageBuilder::FLOP_BIT;
  bool doYYYX    = options & ImageBuilder::YYYX_BIT;
  bool doZYZX    = options & ImageBuilder::ZYZX_BIT;
  bool hasAlpha  = FreeImage_IsTransparent(faces[0]) || doYYYX || doZYZX;
  bool isArray   = !isCubeMap && nFaces > 1;

  for (int i = 1; i < nFaces; ++i) {
    if (int(FreeImage_GetWidth(faces[i])) != width ||
        int(FreeImage_GetHeight(faces[i])) != height)
    {
      printf("All faces must have the same dimensions.");
      return false;
    }
  }

  if (isCubeMap && nFaces != 6) {
    printf("Cube map requires exactly 6 faces.");
    return false;
  }

  int targetBPP      = hasAlpha || compress || isArray ? 32 : 24;
  int pitchOrLinSize = ((width * targetBPP / 8 + 3) / 4) * 4;
  int nMipmaps       = doMipmaps ? index1(max(width, height)) + 1 : 1;

  int flags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
  flags |= doMipmaps ? DDSD_MIPMAPCOUNT : 0;
  flags |= compress  ? DDSD_LINEARSIZE  : DDSD_PITCH;

  int caps = DDSCAPS_TEXTURE;
  caps |= doMipmaps ? DDSCAPS_COMPLEX | DDSCAPS_MIPMAP : 0;
  caps |= isCubeMap ? DDSCAPS_COMPLEX : 0;

  int caps2 = isCubeMap ? DDSCAPS2_CUBEMAP : 0;
  caps2 |= isCubeMap ? DDSCAPS2_CUBEMAP_POSITIVEX | DDSCAPS2_CUBEMAP_NEGITIVEX : 0;
  caps2 |= isCubeMap ? DDSCAPS2_CUBEMAP_POSITIVEY | DDSCAPS2_CUBEMAP_NEGITIVEY : 0;
  caps2 |= isCubeMap ? DDSCAPS2_CUBEMAP_POSITIVEZ | DDSCAPS2_CUBEMAP_NEGITIVEZ : 0;

  int pixelFlags = 0;
  pixelFlags |= hasAlpha ? DDPF_ALPHAPIXELS : 0;
  pixelFlags |= compress ? DDPF_FOURCC : DDPF_RGB;
  pixelFlags |= isNormal ? DDPF_NORMAL : 0;

  const char* fourCC = isArray ? "DX10" : "\0\0\0\0";
  int dx10Format = DXGI_FORMAT_R8G8B8A8_UNORM;

#ifdef OZ_NONFREE
  int squishFlags = squish::kColourIterativeClusterFit | squish::kWeightColourByAlpha;
  squishFlags    |= hasAlpha ? squish::kDxt5 : squish::kDxt1;

  if (compress) {
    pitchOrLinSize = squish::GetStorageRequirements(width, height, squishFlags);
    dx10Format     = hasAlpha ? DXGI_FORMAT_BC3_UNORM : DXGI_FORMAT_BC1_UNORM;
    fourCC         = isArray ? "DX10" : hasAlpha ? "DXT5" : "DXT1";
  }
#endif

  FILE* f = fopen(destFile, "w");

  if (f == nullptr) {
    printf("Failed to write '%s'.", destFile);
    return false;
  }

  // Header beginning.
  writeChars("DDS ", 4, f);
  writeInt(124, f);
  writeInt(flags, f);
  writeInt(height, f);
  writeInt(width, f);
  writeInt(pitchOrLinSize, f);
  writeInt(0, f);
  writeInt(nMipmaps, f);

  // Reserved int[11].
  writeInt(0, f);
  writeInt(0, f);
  writeInt(0, f);
  writeInt(0, f);
  writeInt(0, f);
  writeInt(0, f);
  writeInt(0, f);
  writeInt(0, f);
  writeInt(0, f);
  writeInt(0, f);
  writeInt(0, f);

  // Pixel format.
  writeInt(32, f);
  writeInt(pixelFlags, f);
  writeChars(fourCC, 4, f);

  if (compress) {
    writeInt(0, f);
    writeInt(0, f);
    writeInt(0, f);
    writeInt(0, f);
    writeInt(0, f);
  }
  else {
    writeInt(targetBPP, f);
    writeInt(0x00ff0000, f);
    writeInt(0x0000ff00, f);
    writeInt(0x000000ff, f);
    writeInt(0xff000000, f);
  }

  writeInt(caps, f);
  writeInt(caps2, f);
  writeInt(0, f);
  writeInt(0, f);
  writeInt(0, f);

  if (isArray) {
    writeInt(dx10Format, f);
    writeInt(D3D10_RESOURCE_DIMENSION_TEXTURE2D, f);
    writeInt(0, f);
    writeInt(nFaces, f);
    writeInt(0, f);
  }

  for (int i = 0; i < nFaces; ++i) {
    FIBITMAP* face = faces[i];

    if (doFlip) {
      FreeImage_FlipVertical(face);
    }
    if (doFlop) {
      FreeImage_FlipHorizontal(face);
    }

    if (doYYYX) {
      FreeImage_SetTransparent(face, true);

      BYTE* pixels = FreeImage_GetBits(face);
      int   size   = width * height * 4;

      for (int j = 0; j < size; j += 4) {
        pixels[j + 3] = pixels[j + 2];
        pixels[j + 0] = pixels[j + 1];
        pixels[j + 2] = pixels[j + 1];
      }
    }
    else if (doZYZX) {
      FreeImage_SetTransparent(face, true);

      BYTE* pixels = FreeImage_GetBits(face);
      int   size   = width * height * 4;

      for (int j = 0; j < size; j += 4) {
        pixels[j + 3] = pixels[j + 2];
        pixels[j + 2] = pixels[j + 0];
      }
    }
    else if (compress) {
      BYTE* pixels = FreeImage_GetBits(face);
      int   size   = width * height * 4;

      for (int j = 0; j < size; j += 4) {
        swap(pixels[j], pixels[j + 2]);
      }
    }

    if (targetBPP == 24) {
      face = FreeImage_ConvertTo24Bits(faces[i]);
    }

    int levelWidth  = width;
    int levelHeight = height;

    for (int j = 0; j < nMipmaps; ++j) {
      FIBITMAP* level = face;
      if (j != 0) {
        level = FreeImage_Rescale(face, levelWidth, levelHeight, FILTER_CATMULLROM);
      }

      if (compress) {
#ifdef OZ_NONFREE
        ubyte* pixels = FreeImage_GetBits(level);
        int    s3Size = squish::GetStorageRequirements(levelWidth, levelHeight, squishFlags);

        squish::CompressImage(pixels, levelWidth, levelHeight, os.forward(s3Size), squishFlags);
#endif
      }
      else {
        const char* pixels = reinterpret_cast<const char*>(FreeImage_GetBits(level));
        int         pitch  = int(FreeImage_GetPitch(level));

        for (int k = 0; k < levelHeight; ++k) {
          writeChars(pixels, levelWidth * targetBPP / 8, f);
          pixels += pitch;
        }
      }

      levelWidth  = max(1, levelWidth / 2);
      levelHeight = max(1, levelHeight / 2);

      if (j != 0) {
        FreeImage_Unload(level);
      }
    }

    if (face != faces[i]) {
      FreeImage_Unload(face);
    }
  }

  fclose(f);
  return true;
}

ImageData::ImageData() :
  width(0), height(0), flags(0), pixels(nullptr)
{}

ImageData::ImageData(int width_, int height_) :
  width(width_), height(height_), flags(0), pixels(new char[width* height * 4])
{}

ImageData::~ImageData()
{
  delete[] pixels;
}

ImageData::ImageData(ImageData&& i) :
  width(i.width), height(i.height), flags(i.flags), pixels(i.pixels)
{
  i.width  = 0;
  i.height = 0;
  i.flags  = 0;
  i.pixels = nullptr;
}

ImageData& ImageData::operator = (ImageData&& i)
{
  if (&i != this) {
    width  = i.width;
    height = i.height;
    flags  = i.flags;
    pixels = i.pixels;

    i.width  = 0;
    i.height = 0;
    i.flags  = 0;
    i.pixels = nullptr;
  }
  return *this;
}

void ImageData::determineAlpha()
{
  if (pixels == nullptr) {
    return;
  }

  int size = width * height * 4;

  flags &= ~ALPHA_BIT;

  for (int i = 0; i < size; i += 4) {
    if (pixels[i + 3] != char(255)) {
      flags |= ALPHA_BIT;
      return;
    }
  }
}

bool ImageData::isNormalMap() const
{
  if (pixels == nullptr) {
    return false;
  }

  int   size       = width * height * 4;
  float average[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

  for (int i = 0; i < size; i += 4) {
    float c[4] = {
      BYTE(pixels[i + 0]) / 255.0f - 0.5f,
      BYTE(pixels[i + 1]) / 255.0f - 0.5f,
      BYTE(pixels[i + 2]) / 255.0f - 0.5f,
      BYTE(pixels[i + 3]) / 255.0f
    };

    float cSq = c[0]*c[0] + c[1]*c[1] + c[2]*c[2];

    if (abs(1.0f - cSq) > 1.0f || c[3] < 0.9f) {
      return false;
    }

    average[0] += c[0];
    average[1] += c[1];
    average[2] += c[2];
    average[3] += c[3];
  }

  average[0] /= float(width * height);
  average[1] /= float(width * height);
  average[2] /= float(width * height);
  average[3] /= float(width * height);

  average[2] -= 0.5;
  average[3] -= 1.0f;

  return average[0]*average[0] + average[1]*average[1] + average[2]*average[2] < 0.1f;
}

ImageData ImageBuilder::loadImage(const char* file)
{
  ImageData image;

  FIBITMAP* dib = loadBitmap(file);
  if (dib == nullptr) {
    return image;
  }

  image = ImageData(int(FreeImage_GetWidth(dib)), int(FreeImage_GetHeight(dib)));

  // Copy and convert BGRA -> RGBA.
  int   size   = image.width * image.height * 4;
  BYTE* pixels = FreeImage_GetBits(dib);

  for (int i = 0; i < size; i += 4) {
    image.pixels[i + 0] = char(pixels[i + 2]);
    image.pixels[i + 1] = char(pixels[i + 1]);
    image.pixels[i + 2] = char(pixels[i + 0]);
    image.pixels[i + 3] = char(pixels[i + 3]);
  }

  FreeImage_Unload(dib);
  return image;
}

bool ImageBuilder::createDDS(const ImageData* faces, int nFaces, int options, const char* destFile)
{
  if (nFaces < 1) {
    printf("At least one face must be given.");
    return false;
  }

  bool       success = false;
  FIBITMAP** dibs    = new FIBITMAP*[nFaces];

  for (int i = 0; i < nFaces; ++i) {
    dibs[i] = createBitmap(faces[i]);

    if (dibs[i] == nullptr) {
      nFaces = i;
      goto cleanUp;
    }
  }

  success = buildDDS(dibs, nFaces, options, destFile);

cleanUp:
  for (int i = 0; i < nFaces; ++i) {
    FreeImage_Unload(dibs[i]);
  }
  delete[] dibs;

  return success;
}

bool ImageBuilder::convertToDDS(const char* file, int options, const char* destPath)
{
  string destFile;

  if (destPath != nullptr) {
    destFile = destPath;
  }
  else {
    const char* dot = strrchr(file, '.');

    if (dot == nullptr) {
      printf("File extensfion missing: %s", file);
      return false;
    }
    else {
      destFile = std::string(file, dot) + ".dds";
    }
  }

  FIBITMAP* dib = loadBitmap(file);
  if (dib == nullptr) {
    return false;
  }
  bool success = buildDDS(&dib, 1, options, destFile.c_str());

  FreeImage_Unload(dib);
  return success;
}

void ImageBuilder::init()
{
  FreeImage_Initialise();
  FreeImage_SetOutputMessage(printError);
}

void ImageBuilder::destroy()
{
  FreeImage_DeInitialise();
}

}
