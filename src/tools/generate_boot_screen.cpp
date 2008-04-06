/*
 *	Copyright (c) 2008, Haiku, Inc. All rights reserved.
 *	Distributed under the terms of the MIT license. 
 *
 *	Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Philippe Saint-Pierre <stpere@gmail.com>
 */

//! Haiku boot splash image generator/converter

#include <iostream>
#include <png.h>
#include <string>
#include <stdarg.h>
#include <stdint.h>

// TODO: Generate the single optimal palette for all three images,
// store palette versions of these images as well, so that they are
// ready to be used during boot in case we need to run in palette
// VGA mode.


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
	png_set_gray_1_2_4_to_8(pngPtr);
	png_set_palette_to_rgb(pngPtr);
	png_set_gray_to_rgb(pngPtr);
	png_set_strip_alpha(pngPtr);
	png_set_bgr(pngPtr);

	png_read_info(pngPtr, infoPtr);
	width = infoPtr->width;
	height = infoPtr->height;
	if (infoPtr->bit_depth != 8)
		error("[read_png] File %s has wrong bit depth\n", filename);
	if ((int)infoPtr->rowbytes < width * 3) {
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
		rowPtrs[y] = (png_byte*)malloc(infoPtr->rowbytes);

	png_read_image(pngPtr, rowPtrs);
}


static void
new_line_if_required()
{
	sOffset++;
	if (sOffset % 12 == 0)
		fprintf(sOutput, "\n\t");
}


static void
write_image(const char* baseName, int width, int height, png_bytep* rowPtrs)
{
	fprintf(sOutput, "static const uint16 %sWidth = %d;\n", baseName, width);
	fprintf(sOutput, "static const uint16 %sHeight = %d;\n", baseName, height);
	fprintf(sOutput, "#ifndef __BOOTSPLASH_KERNEL__\n");
	
	int buffer[128];
	// buffer[0] stores count, buffer[1..127] holds the actual values

	fprintf(sOutput, "static uint8 %sCompressedImage[] = {\n\t",
		baseName);

	for (int c = 0; c < 3; c++) {
		// for each component i.e. R, G, B ...
		// NOTE : I don't care much about performance at this step,
		// decoding however...
		int currentValue = rowPtrs[0][c];
		int count = 0;

		// When bufferActive == true, we store the number rather than writing 
		// them directly; we use this to store numbers until we find a pair..
		bool bufferActive = false;

		sOffset = 0;

		for (int y = 0; y < height; y++) {
			png_byte* row = rowPtrs[y];
			for (int x = c; x < width * 3; x += 3) {
				if (row[x] == currentValue) {
					if (bufferActive) {
						bufferActive = false;
						count = 2;
						if (buffer[0] > 1) {
							fprintf (sOutput, "%d, ", 
								128 + buffer[0] - 1);
							new_line_if_required();
							for (int i = 1; i < buffer[0] ; i++) {
								fprintf( sOutput, "%d, ", 
									buffer[i] );
								new_line_if_required();
							}
						}
					} else {
						count++;
						if (count == 127) {
							fprintf(sOutput, "127, ");
							new_line_if_required();
							fprintf(sOutput, "%d, ", currentValue);
							new_line_if_required();
							count = 0;
						}
					}
				} else {
					if (bufferActive) {
						if (buffer[0] == 127) {
							// we don't have enough room, 
							// flush the buffer
							fprintf(sOutput, "%d, ", 
								128 + buffer[0] - 1);
							new_line_if_required();
							for (int i = 1; i < buffer[0]; i++) {
								fprintf(sOutput, "%d, ", buffer[i]);
								new_line_if_required();
							}
							buffer[0] = 0;
						}
						buffer[0]++;
						buffer[buffer[0]] = row[x];
					} else if (count > 0) {
						buffer[0] = 1;
						buffer[1] = row[x];
						bufferActive = true;
						if (count > 1) {
							fprintf(sOutput, "%d, ", count);
							new_line_if_required();
							fprintf(sOutput, "%d, ", currentValue);
							new_line_if_required();
						}
					}
					currentValue = row[x];
				}
			}
		}
		if (bufferActive) {
			// I could have written 127 + buffer[0],
			// but I think this is more readable...
			fprintf(sOutput, "%d, ", 128 + buffer[0] - 1);
			new_line_if_required();
			for (int i = 1; i < buffer[0] ; i++) {
				fprintf(sOutput, "%d, ", buffer[i]);
				new_line_if_required();
			}
		} else {
			fprintf(sOutput, "%d, %d, ", count, currentValue);
			new_line_if_required();
		}		
		// we put a terminating zero for the next byte that indicates
		// a "count", just to indicate the end of the channel
		fprintf(sOutput, "0");
		if (c != 2)
			fprintf(sOutput, ",");

		fprintf(sOutput, "\n\t");
	}
	fprintf(sOutput, "};\n");
	fprintf(sOutput, "#endif\n\n");
}


static void
parse_image(const char* filename, const char* baseName)
{
	int width;
	int height;
	png_bytep* rowPtrs = NULL;
	png_structp pngPtr;
	png_infop infoPtr;
	read_png(filename, width, height, rowPtrs, pngPtr, infoPtr);
	write_image(baseName, width, height, rowPtrs);

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

	parse_image(argv[1], "kSplashLogo");
	parse_image(argv[4], "kSplashIcons");

	fclose(sOutput);
	return 0;
}

