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

/**
 * Enable generation of mipmaps for a texture.
 */
static const int MIPMAPS_BIT = 0x01;

/**
 * Enable texture compression.
 */
static const int COMPRESSION_BIT = 0x02;

/**
 * Use highest possible quality for texture compression and mipmap scaling.
 */
static const int QUALITY_BIT = 0x04;

/**
 * Convert given image to DDS format and optionally compress it and create mipmaps.
 *
 * Mipmap generation, S3 texture compression, and quality of compression and mipmap images scaling
 * can be controlled via `options` parameter.
 * @li `MIPMAPS_BIT` enables generation of mipmaps.
 * @li `COMPRESSION_BIT` enables S3 texture compression; DXT1 is used for images without an alpha
 *     channel and DXT5 for images with an alpha channel.
 * @li `QUALITY_BIT` enables highest quality but slow methods for scaling texture to smaller
 *     dimensions in mipmap generation and highest quality settings for S3 texture compression
 *     libsquish supports.
 *
 * Freeimage library is used for reading a file, so most of image file formats are supported.
 *
 * @param inPath input image path..
 * @param options bit-mask to control quality and compression.
 * @param outPath output DDS path..
 */
bool buildDDS( const char* inPath, int options, const char* outPath );
