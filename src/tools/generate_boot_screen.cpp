/*
 *	Copyright (c) 2008-2011, Haiku, Inc. All rights reserved.
 *	Distributed under the terms of the MIT license.
 *
 *	Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Philippe Saint-Pierre <stpere@gmail.com>
 *		David Powell <david@mad.scientist.com>
 *		Philippe Houdoin
 */

//! Haiku boot splash image generator/converter

#include <iostream>
#include <png.h>
#include <string>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#include <zlib.h>

#include <ColorQuantizer.h>

// TODO: Create 4 bit palette version of these
// images as well, so that they are ready to be
// used during boot in case we need to run in
// palette 4 bit VGA mode.


FILE* sOutput = NULL;
int sOffset = 0;

static void
error(const char *s, ...)
{
	va_list args;
	va_start(args, s);
	vfprintf(stderr, s, args);
	fprintf(stderr, "\n");
	va_end(args);
	exit(-1);
}


static void
new_line_if_required()
{
	sOffset++;
	if (sOffset % 12 == 0)
		fprintf(sOutput, "\n\t");
}


// #pragma mark -


class AutoFileCloser {
public:
	AutoFileCloser(FILE* file)
		: fFile(file)
	{}
	~AutoFileCloser()
	{
		fclose(fFile);
	}
private:
	FILE* fFile;
};


// #pragma mark -


class ZlibCompressor {
public:
	ZlibCompressor(FILE* output);

	int Compress(const void* data, int dataSize, int flush = Z_NO_FLUSH);
	int Finish();

private:
	FILE* 		fOutput;
	z_stream 	fStream;
};


ZlibCompressor::ZlibCompressor(FILE* output)
	: fOutput(output)
{
	// prepare zlib stream

	fStream.next_in = NULL;
	fStream.avail_in = 0;
	fStream.total_in = 0;
	fStream.next_out = NULL;
	fStream.avail_out = 0;
	fStream.total_out = 0;
	fStream.msg = 0;
	fStream.state = 0;
	fStream.zalloc = Z_NULL;
	fStream.zfree = Z_NULL;
	fStream.opaque = Z_NULL;
	fStream.data_type = 0;
	fStream.adler = 0;
	fStream.reserved = 0;

	int zlibError = deflateInit(&fStream, Z_BEST_COMPRESSION);
	if (zlibError != Z_OK)
		return;	// TODO: translate zlibError
}


int
ZlibCompressor::Compress(const void* data, int dataSize, int flush)
{
	uint8 buffer[1024];

	fStream.next_in = (Bytef*)data;
	fStream.avail_in = dataSize;

	int zlibError = Z_OK;

	do {
		fStream.next_out = (Bytef*)buffer;
		fStream.avail_out = sizeof(buffer);

		zlibError = deflate(&fStream, flush);
		if (zlibError != Z_OK &&
			(flush == Z_FINISH && zlibError != Z_STREAM_END))
			return zlibError;

		unsigned int outputSize = sizeof(buffer) - fStream.avail_out;
		for (unsigned int i = 0; i < outputSize; i++) {
			fprintf(fOutput, "%d, ", buffer[i]);
			new_line_if_required();
		}

		if (zlibError == Z_STREAM_END)
			break;

	} while (fStream.avail_in > 0 || flush == Z_FINISH);

	return zlibError;
}


int
ZlibCompressor::Finish()
{
	Compress(NULL, 0, Z_FINISH);
	return deflateEnd(&fStream);
}


// #pragma mark -


static void
read_png(const char* filename, int& width, int& height, png_bytep*& rowPtrs,
	png_structp& pngPtr, png_infop& infoPtr)
{
	char header[8];
	FILE* input = fopen(filename, "rb");
	if (!input)
		error("[read_png] File %s could not be opened for reading", filename);

	AutoFileCloser _(input);

	fread(header, 1, 8, input);
	if (png_sig_cmp((png_byte *)header, 0, 8 ))
		error("[read_png] File %s is not recognized as a PNG file", filename);

	pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL);
	if (!pngPtr)
		error("[read_png] png_create_read_struct failed");

	infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr)
		error("[read_png] png_create_info_struct failed");

// TODO: I don't know which version of libpng introduced this feature:
#if PNG_LIBPNG_VER > 10005
	if (setjmp(png_jmpbuf(pngPtr)))
		error("[read_png] Error during init_io");
#endif

	png_init_io(pngPtr, input);
	png_set_sig_bytes(pngPtr, 8);

	// make sure we automatically get RGB data with 8 bits per channel
	// also make sure the alpha channel is stripped, in the end, we
	// expect 24 bits BGR data
	png_set_expand(pngPtr);
	png_set_expand_gray_1_2_4_to_8(pngPtr);
	png_set_palette_to_rgb(pngPtr);
	png_set_gray_to_rgb(pngPtr);
	png_set_strip_alpha(pngPtr);
	png_set_bgr(pngPtr);

	png_read_info(pngPtr, infoPtr);
	width = (int)png_get_image_width(pngPtr, infoPtr);
	height = (int)png_get_image_height(pngPtr, infoPtr);
	if (png_get_bit_depth(pngPtr, infoPtr) != 8)
		error("[read_png] File %s has wrong bit depth\n", filename);
	if ((int)png_get_rowbytes(pngPtr, infoPtr) < width * 3) {
		error("[read_png] File %s has wrong color type (RGB required)\n",
			filename);
	}


	png_set_interlace_handling(pngPtr);
	png_read_update_info(pngPtr, infoPtr);

#if PNG_LIBPNG_VER > 10005
	if (setjmp(png_jmpbuf(pngPtr)))
		error("[read_png] Error during read_image");
#endif

	rowPtrs = (png_bytep*)malloc(sizeof(png_bytep) * height);
	for (int y = 0; y < height; y++)
		rowPtrs[y] = (png_byte*)malloc(png_get_rowbytes(pngPtr, infoPtr));

	png_read_image(pngPtr, rowPtrs);
}


static void
write_24bit_image(const char* baseName, int width, int height, png_bytep* rowPtrs)
{
	fprintf(sOutput, "static const uint16 %sWidth = %d;\n", baseName, width);
	fprintf(sOutput, "static const uint16 %sHeight = %d;\n", baseName, height);
	fprintf(sOutput, "#ifndef __BOOTSPLASH_KERNEL__\n");

	fprintf(sOutput, "static uint8 %s24BitCompressedImage[] = {\n\t",
		baseName);

	ZlibCompressor zlib(sOutput);

	for (int y = 0; y < height; y++)
		zlib.Compress(rowPtrs[y], width * 3);

	zlib.Finish();

	fprintf(sOutput, "\n};\n");
	fprintf(sOutput, "#endif\n\n");
}


static void
write_8bit_image(const char* baseName, int width, int height, unsigned char** rowPtrs)
{
	//int buffer[128];
	// buffer[0] stores count, buffer[1..127] holds the actual values

	fprintf(sOutput, "static uint8 %s8BitCompressedImage[] = {\n\t",
		baseName);


	ZlibCompressor zlib(sOutput);

	for (int y = 0; y < height; y++)
		zlib.Compress(rowPtrs[y], width);

	zlib.Finish();

	fprintf(sOutput, "\n};\n\n");
}


unsigned char
nearest_color(unsigned char* color, RGBA palette[256])
{
	int i, dist, minDist, index = 0;
	minDist = 255 * 255 + 255 * 255 + 255 * 255 + 1;
	for (i = 0; i < 256; i++) {
		int dr = ((int)color[2]) - palette[i].r;
		int dg = ((int)color[1]) - palette[i].g;
		int db = ((int)color[0]) - palette[i].b;
		dist = dr * dr + dg * dg + db * db;
		if (dist < minDist) {
			minDist = dist;
			index = i;
		}
	}
	return index;
}


static void
create_8bit_images(const char* logoBaseName, int logoWidth, int logoHeight,
	png_bytep* logoRowPtrs, const char* iconsBaseName, int iconsWidth,
	int iconsHeight, png_bytep* iconsRowPtrs)
{
	// Generate 8-bit palette
	BColorQuantizer quantizer(256, 8);
	quantizer.ProcessImage(logoRowPtrs, logoWidth, logoHeight);
	quantizer.ProcessImage(iconsRowPtrs, iconsWidth, iconsHeight);

	RGBA palette[256];
	quantizer.GetColorTable(palette);

	// convert 24-bit logo image to 8-bit indexed color
	uint8* logoIndexedImageRows[logoHeight];
	for (int y = 0; y < logoHeight; y++) {
		logoIndexedImageRows[y] = new uint8[logoWidth];
		for (int x = 0; x < logoWidth; x++)	{
			logoIndexedImageRows[y][x] = nearest_color(&logoRowPtrs[y][x*3],
				palette);
		}
	}

	// convert 24-bit icons image to 8-bit indexed color
	uint8* iconsIndexedImageRows[iconsHeight];
	for (int y = 0; y < iconsHeight; y++) {
		iconsIndexedImageRows[y] = new uint8[iconsWidth];
		for (int x = 0; x < iconsWidth; x++)	{
			iconsIndexedImageRows[y][x] = nearest_color(&iconsRowPtrs[y][x*3],
				palette);
		}
	}


	fprintf(sOutput, "#ifndef __BOOTSPLASH_KERNEL__\n");

	// write the 8-bit color palette
	fprintf(sOutput, "static const uint8 k8BitPalette[] = {\n");
	for (int c = 0; c < 256; c++) {
		fprintf(sOutput, "\t0x%x, 0x%x, 0x%x,\n",
			palette[c].r, palette[c].g, palette[c].b);
	}
	fprintf(sOutput, "\t};\n\n");

	// write the 8-bit images
	write_8bit_image(logoBaseName, logoWidth, logoHeight, logoIndexedImageRows);
	write_8bit_image(iconsBaseName, iconsWidth, iconsHeight,
		iconsIndexedImageRows);

	fprintf(sOutput, "#endif\n\n");

	// free memory
	for (int y = 0; y < logoHeight; y++)
		delete[] logoIndexedImageRows[y];

	for (int y = 0; y < iconsHeight; y++)
		delete[] iconsIndexedImageRows[y];
}


static void
parse_images(const char* logoFilename, const char* logoBaseName,
	const char* iconsFilename, const char* iconsBaseName)
{
	int logoWidth;
	int logoHeight;
	png_bytep* logoRowPtrs = NULL;
	png_structp logoPngPtr;
	png_infop logoInfoPtr;

	int iconsWidth;
	int iconsHeight;
	png_bytep* iconsRowPtrs = NULL;
	png_structp iconsPngPtr;
	png_infop iconsInfoPtr;

	read_png(logoFilename, logoWidth, logoHeight, logoRowPtrs, logoPngPtr,
		logoInfoPtr);
	read_png(iconsFilename, iconsWidth, iconsHeight, iconsRowPtrs, iconsPngPtr,
		iconsInfoPtr);

	// write 24-bit images
	write_24bit_image(logoBaseName, logoWidth, logoHeight, logoRowPtrs);
	write_24bit_image(iconsBaseName, iconsWidth, iconsHeight, iconsRowPtrs);

	// write 8-bit index color images
	create_8bit_images(logoBaseName, logoWidth, logoHeight, logoRowPtrs,
		iconsBaseName, iconsWidth, iconsHeight, iconsRowPtrs);

	// free resources
	png_destroy_read_struct(&logoPngPtr, &logoInfoPtr, NULL);
	for (int y = 0; y < logoHeight; y++)
		free(logoRowPtrs[y]);
	free(logoRowPtrs);

	png_destroy_read_struct(&iconsPngPtr, &iconsInfoPtr, NULL);
	for (int y = 0; y < iconsHeight; y++)
		free(iconsRowPtrs[y]);
	free(iconsRowPtrs);
}


int
main(int argc, char* argv[])
{
	if (argc < 8) {
		printf("Usage:\n");
		printf("\t%s <splash.png> <x placement in %%> <y placement in %%> "
			"<icons.png> <x placement in %%> <y placement in %%> <images.h>\n",
			argv[0]);
		return 0;
	}

	int logoPlacementX = atoi(argv[2]);
	int logoPlacementY = atoi(argv[3]);
	int iconPlacementX = atoi(argv[5]);
	int iconPlacementY = atoi(argv[6]);
	if (logoPlacementX < 0 || logoPlacementX > 100) {
		printf("Bad X placement for logo: %d%%\n", logoPlacementX);
		return 1;
	}
	if (logoPlacementY < 0 || logoPlacementY > 100) {
		printf("Bad Y placement for logo: %d%%\n\n", logoPlacementY);
		return 1;
	}
	if (iconPlacementX < 0 || iconPlacementX > 100) {
		printf("Bad X placement for icons: %d%%\n", iconPlacementX);
		return 1;
	}
	if (iconPlacementY < 0 || iconPlacementY > 100) {
		printf("Bad Y placement for icons: %d%%\n", iconPlacementY);
		return 1;
	}

	const char* headerFileName = argv[7];
	sOutput = fopen(headerFileName, "wb");
	if (!sOutput)
		error("Could not open file \"%s\" for writing", headerFileName);

	fputs("// This file was generated by the generate_boot_screen Haiku build "
		"tool.\n\n", sOutput);

	fprintf(sOutput, "static const int32 kSplashLogoPlacementX = %d;\n",
		logoPlacementX);
	fprintf(sOutput, "static const int32 kSplashLogoPlacementY = %d;\n",
		logoPlacementY);
	fprintf(sOutput, "static const int32 kSplashIconsPlacementX = %d;\n",
		iconPlacementX);
	fprintf(sOutput, "static const int32 kSplashIconsPlacementY = %d;\n\n",
		iconPlacementY);

	parse_images(argv[1], "kSplashLogo", argv[4], "kSplashIcons");

	fclose(sOutput);
	return 0;
}
