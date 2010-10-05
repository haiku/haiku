/*
 *	Copyright (c) 2008, Haiku, Inc. All rights reserved.
 *	Distributed under the terms of the MIT license.
 *
 *	Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Philippe Saint-Pierre <stpere@gmail.com>
 *		David Powell <david@mad.scientist.com>
 */

//! Haiku boot splash image generator/converter

#include <iostream>
#include <png.h>
#include <string>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

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
	png_set_expand_gray_1_2_4_to_8(pngPtr);
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
write_24bit_image(const char* baseName, int width, int height, png_bytep* rowPtrs)
{
	fprintf(sOutput, "static const uint16 %sWidth = %d;\n", baseName, width);
	fprintf(sOutput, "static const uint16 %sHeight = %d;\n", baseName, height);
	fprintf(sOutput, "#ifndef __BOOTSPLASH_KERNEL__\n");

	int buffer[128];
	// buffer[0] stores count, buffer[1..127] holds the actual values

	fprintf(sOutput, "static uint8 %s24BitCompressedImage[] = {\n\t",
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
							fprintf(sOutput, "%d, ",
								128 + buffer[0] - 1);
							new_line_if_required();
							for (int i = 1; i < buffer[0] ; i++) {
								fprintf(sOutput, "%d, ",
									buffer[i]);
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
					} else {
						if (count > 0) {
							fprintf(sOutput, "%d, ", count);
							new_line_if_required();
							fprintf(sOutput, "%d, ", currentValue);
							new_line_if_required();
						}
						buffer[0] = 0;
						bufferActive = true;
					}
					buffer[0]++;
					buffer[buffer[0]] = row[x];
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
write_8bit_image(const char* baseName, int width, int height, unsigned char** rowPtrs)
{
	int buffer[128];
	// buffer[0] stores count, buffer[1..127] holds the actual values

	fprintf(sOutput, "static uint8 %s8BitCompressedImage[] = {\n\t",
		baseName);

	// NOTE: I don't care much about performance at this step,
	// decoding however...
	unsigned char currentValue = rowPtrs[0][0];
	int count = 0;

	// When bufferActive == true, we store the number rather than writing
	// them directly; we use this to store numbers until we find a pair..
	bool bufferActive = false;

	sOffset = 0;
	for (int y = 0; y < height; y++) {
		unsigned char* row = rowPtrs[y];
		for (int x = 0; x < width; x++) {
			if (row[x] == currentValue) {
				if (bufferActive) {
					bufferActive = false;
					count = 2;
					if (buffer[0] > 1) {
						fprintf(sOutput, "%d, ",
							128 + buffer[0] - 1);
						new_line_if_required();
						for (int i = 1; i < buffer[0] ; i++) {
							fprintf(sOutput, "%d, ",
								buffer[i]);
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
				} else {
					if (count > 0) {
						fprintf(sOutput, "%d, ", count);
						new_line_if_required();
						fprintf(sOutput, "%d, ", currentValue);
						new_line_if_required();
					}
					buffer[0] = 0;
					bufferActive = true;
				}
				buffer[0]++;
				buffer[buffer[0]] = row[x];
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
	// a "count", to indicate the end
	fprintf(sOutput, "0");

	fprintf(sOutput, "\n\t");
	fprintf(sOutput, "};\n\n");
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
