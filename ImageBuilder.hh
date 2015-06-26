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
 * @file ozFactory/ImageBuilder.hh
 *
 * `ImageBuilder` class.
 */

#pragma once

/**
 * %Image pixel data with basic metadata (dimensions and transparency).
 */
struct ImageData
{
  /// Alpha flag.
  static const int ALPHA_BIT = 0x01;

  /// Normal map bit.
  static const int NORMAL_BIT = 0x02;

  int   width  = 0;       ///< Width.
  int   height = 0;       ///< Height.
  int   flags  = 0;       ///< Flags.
  char* pixels = nullptr; ///< Pixels data in RGBA format.

  /**
   * Create empty instance, no allocation is performed.
   */
  ImageData() = default;

  /**
   * Create an image an allocate memory for pixel data.
   */
  explicit ImageData(int width, int height);

  /**
   * Destructor.
   */
  ~ImageData();

  /**
   * Move constructor, moves pixel data.
   */
  ImageData(ImageData&& i);

  /**
   * Move operator, moves pixel data.
   */
  ImageData& operator = (ImageData&& i);

  /**
   * True iff it holds no image data.
   */
  bool isEmpty() const
  {
    return pixels == nullptr;
  }

  /**
   * Check if any non-opaque pixel is present and update alpha flag accordingly.
   */
  void determineAlpha();

  /**
   * Guess if the image is a normal map.
   *
   * The guess is based on average colour being close to #8080ff and whether pixel lengths roughly
   * one (\f$ (R - 0.5)^2 + (G - 0.5)^2 + (B - 0.5)^2 = 1 \f$).
   */
  bool isNormalMap() const;
};

/**
 * %ImageBuilder class converts generic image formats to DDS (DirectDraw Surface).
 *
 * FreeImage library is used to read source images and apply transformations to them (e.g. resizing
 * for mipmaps) and libsquish to apply S3 texture compression.
 */
class ImageBuilder
{
public:

  /// Image array is a cube map.
  static const int CUBE_MAP_BIT = 0x01;

  /// Image is a normal map (set DDPF_NORMAL bit).
  static const int NORMAL_MAP_BIT = 0x02;

  /// Generate mipmaps.
  static const int MIPMAPS_BIT = 0x04;

  /// Enable texture compression.
  static const int COMPRESSION_BIT = 0x08;

  /// Flip vertically.
  static const int FLIP_BIT = 0x10;

  /// Flip horizontally.
  static const int FLOP_BIT = 0x20;

  /// Perform RGB(A) -> GGGR swizzle (for DXT5nm normal map compression).
  static const int YYYX_BIT = 0x40;

  /// Perform RGB(A) -> BGBR swizzle (for DXT5nm+z normal map compression).
  static const int ZYZX_BIT = 0x80;

public:

  /**
   * Forbid instances.
   */
  ImageBuilder() = delete;

  /**
   * Print information about a DDS image.
   */
  static bool printInfo(const char* file);

  /**
   * Load an image.
   */
  static ImageData loadImage(const char* file);

  /**
   * Generate a DDS form a given image and optionally compress it and create mipmaps.
   *
   * An array texture is created if more than one image face is given. If an array of exactly 6
   * faces is given and `CUBE_MAP_BIT` option is set a cube map is generated. Cube map faces must
   * be given in the following order: +x, -x, +y, -y, +z, -z.
   *
   * @note
   * The highest possible quality settings are used for compression and mipmap scaling, so this
   * might take a long time for a large image.
   *
   * @param faces array of pointers to pixels of input images.
   * @param nFaces number of input images.
   * @param options bit-mask to control mipmap generation, compression and cube map.
   * @param destFile output file.
   */
  static bool createDDS(const ImageData* faces, int nFaces, int options, double scale,
                        const char* destFile);

  /**
   * Initialise underlaying FreeImage library.
   *
   * This function should be called before `ImageBuilder` class is used.
   */
  static void init();

  /**
   * Deinitialise underlaying FreeImage library.
   *
   * This function should be called after you finish using `ImageBuilder`.
   */
  static void destroy();

};
