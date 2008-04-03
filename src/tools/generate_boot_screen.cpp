/*
 *	Copyright (c) 2008, Haiku, Inc. All rights reserved.
 *	Distributed under the terms of the MIT license. 
 *
 *	Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

//! Haiku boot splash image generator/converter

#include <iostream>
#include <png.h>
#include <string>

// TODO: Generate the single optimal palette for all three images,
// store palette versions of these images as well, so that they are
// ready to be used during boot in case we need to run in palette
// VGA mode.


FILE* sOutput = NULL;


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


static void
readPNG(const char* filename, int& width, int& height, png_bytep*& rowPtrs,
	png_structp& pngPtr, png_infop& infoPtr)
{
	char header[8];
	FILE* input = fopen(filename, "rb");
	if (!input)
		error("[readPNG] File %s could not be opened for reading", filename);

	AutoFileCloser _(input);

	fread(header, 1, 8, input);
	if (png_sig_cmp((png_byte *)header, 0, 8 ))
		error("[readPNG] File %s is not recognized as a PNG file", filename);
	
	pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL);
	if (!pngPtr)
		error("[readPNG] png_create_read_struct failed");
	
	infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr)
		error("[readPNG] png_create_info_struct failed");

// TODO: I don't know which version of libpng introduced this feature:
#if PNG_LIBPNG_VER > 10005
	if (setjmp(png_jmpbuf(pngPtr)))
		error("[readPNG] Error during init_io");
#endif

	png_init_io(pngPtr, input);
	png_set_sig_bytes(pngPtr, 8);

	// make sure we automatically get RGB data with 8 bits per channel
	// also make sure the alpha channel is stripped, in the end, we
	// expect 24 bits BGR data
	png_set_expand(pngPtr);
	png_set_gray_1_2_4_to_8(pngPtr);
	png_set_palette_to_rgb(pngPtr);
	png_set_gray_to_rgb(pngPtr);
	png_set_strip_alpha(pngPtr);
	png_set_bgr(pngPtr);

	png_read_info(pngPtr, infoPtr);
	width = infoPtr->width;
	height = infoPtr->height;
	if (infoPtr->bit_depth != 8)
		error("[readPNG] File %s has wrong bit depth\n", filename);
	if ((int)infoPtr->rowbytes < width * 3) {
		error("[readPNG] File %s has wrong color type (RGB required)\n",
			filename);
	}


	png_set_interlace_handling(pngPtr);
	png_read_update_info(pngPtr, infoPtr);

#if PNG_LIBPNG_VER > 10005
	if (setjmp(png_jmpbuf(pngPtr)))
		error("[readPNG] Error during read_image");
#endif

	rowPtrs = (png_bytep*)malloc(sizeof(png_bytep) * height);
	for (int y = 0; y < height; y++)
		rowPtrs[y] = (png_byte*)malloc(infoPtr->rowbytes);

	png_read_image(pngPtr, rowPtrs);
}


static void
writeHeader(const char* baseName, int width, int height, png_bytep* rowPtrs)
{
	fprintf(sOutput, "static const uint16 %sWidth = %d;\n", baseName, width);
	fprintf(sOutput, "static const uint16 %sHeight = %d;\n", baseName, height);
	fprintf(sOutput, "static const uint8 %sImage[] = {\n\t", baseName);

	int offset = 0;
	for (int y = 0; y < height; y++) {
		png_byte* row = rowPtrs[y];
		for (int x = 0; x < width * 3; x++) {
			offset++;
			if (x == width * 3 - 1 && y == height - 1) {
				fprintf(sOutput, "0x%02x\n};\n\n", row[x]);
				break;
			} else if ((offset % 12) == 0)
				fprintf(sOutput, "0x%02x,\n\t", row[x]);
			else
				fprintf(sOutput, "0x%02x, ", row[x]);
		}
	}
}


static void
parseImage(const char* filename, const char* baseName)
{
	int width;
	int height;
	png_bytep* rowPtrs = NULL;
	png_structp pngPtr;
	png_infop infoPtr;
	readPNG(filename, width, height, rowPtrs, pngPtr, infoPtr);
	writeHeader(baseName, width, height, rowPtrs);

	// free resources
	png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
	for (int y = 0; y < height; y++)
		free(rowPtrs[y]);
	free(rowPtrs);
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

	parseImage(argv[1], "kSplashLogo");
	parseImage(argv[4], "kSplashIcons");

	fclose(sOutput);
	return 0;
}

