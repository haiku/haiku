//------------------------------------------------------------------------------
//	Copyright (c) 2008, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		hbsg.cpp
//	Author:			Artur Wyszynski <harakash@gmail.com>
//	Description:	Haiku Boot Splash Generator
//  
//------------------------------------------------------------------------------

#include <string>
#include <iostream>
#include <png.h>

int x, y;
int width, height;
png_byte colorType;
png_byte bpp;
png_structp pngPtr;
png_infop infoPtr;
int noOfPasses;
png_bytep* rowPtrs;
FILE* finput = NULL;
FILE* foutput = NULL;
png_colorp palette;
int paletteColors = 0;

void
cleanup()
{
	fclose(finput);
	fclose(foutput);
}

void
error(const char *s, ...)
{
	va_list args;
	va_start(args, s);
	vfprintf(stderr, s, args);
	fprintf(stderr, "\n");
	va_end(args);
	cleanup();
	exit(-1);
}

void
readPNG(char *filename)
{
	printf("Reading file %s\n", filename);
	char header[8];
	finput = fopen(filename, "rb");
	if (!finput)
		error("[readPNG] File %s could not be opened for reading", filename);
	
	fread(header, 1, 8, finput);
	if (png_sig_cmp((png_byte *)header, 0, 8 ))
		error("[readPNG] File %s is not recognized as a PNG file", filename);
	
	pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!pngPtr)
		error("[readPNG] png_create_read_struct failed");
	
	infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr)
		error("[readPNG] png_create_info_struct failed");
	
	if (setjmp(png_jmpbuf(pngPtr)))
		error("[readPNG] Error during init_io");
	
	png_init_io(pngPtr, finput);
	png_set_sig_bytes(pngPtr, 8);
	png_read_info(pngPtr, infoPtr);
	width = infoPtr->width;
	height = infoPtr->height;
	colorType = infoPtr->color_type;
	bpp = infoPtr->bit_depth;
	noOfPasses = png_set_interlace_handling(pngPtr);
	png_read_update_info(pngPtr, infoPtr);
	if (setjmp(png_jmpbuf(pngPtr)))
		error("[readPNG] Error during read_image");

	rowPtrs = (png_bytep*)malloc(sizeof(png_bytep) * height);
	for ( y = 0; y < height; y++)
		rowPtrs[y] = (png_byte*)malloc(infoPtr->rowbytes);
	png_read_image(pngPtr, rowPtrs);
	png_get_PLTE(pngPtr, infoPtr, &palette, &paletteColors);
	fclose(finput);
	printf("[PNG] width:\t%d\n", width);
	printf("[PNG] height:\t%d\n", height);
	printf("[PNG] bpp:\t%d\n", bpp);
	printf("[PNG] palette:\t%d\n", paletteColors);
}

void
writeHeader(char *filename)
{
	int length = strlen(filename) - 4;
	char *varName = NULL;
	varName = new char[length];
	memset(varName, 0, sizeof(char) * length);
	strncat(varName, filename, length);
	printf("Exporting image %s\n", varName);

	// dumping palette
	char *line = new char[80];
	sprintf(line, "static const uint8 %sPalette[] = {\n", varName);
	fwrite(line, strlen(line), 1, foutput);
	for (int i = 0; i < paletteColors; i++) {
		if (i == paletteColors - 1)
			sprintf(line, "\t0x%x, 0x%x, 0x%x\n", palette[i].red, palette[i].green, palette[i].blue);
		else
			sprintf(line, "\t0x%x, 0x%x, 0x%x,\n", palette[i].red, palette[i].green, palette[i].blue);
		fwrite(line, strlen(line), 1, foutput);
	}
	fwrite("};\n\n", strlen("};\n\n"), 1, foutput);

	sprintf(line, "static const uint %sWidth=%d;\n", varName, width);
	fwrite(line, strlen(line), 1, foutput);
	sprintf(line, "static const uint %sHeight=%d;\n", varName, height);
	fwrite(line, strlen(line), 1, foutput);
	sprintf(line, "static const uint8 %sImage[] = {\n\t", varName);
	fwrite(line, strlen(line), 1, foutput);
	int offset = 0;
	unsigned char *data = new unsigned char[width * height];
	for (y = 0; y < height; y++) {
		png_byte* row = rowPtrs[y];
		for (x = 0; x < width; x++) {
			png_byte* ptr = &row[x];
			offset = (y * width) + x;
			data[offset] = ptr[0];
		}
	}
	
	bool end = false;
	offset = 0;
	while (!end) {
		if (offset == (width * height)) {
			sprintf(line, "0x%x", data[offset]);
			end = true;
		}
		else
			sprintf(line, "0x%x, ", data[offset]);
		fwrite(line, strlen(line), 1, foutput);
		if ((offset % 8) == 0) {
			fwrite("\n\t", strlen("\n\t"), 1, foutput);
		}
		offset++;
	}

	fwrite("};\n\n", strlen("};\n\n"), 1, foutput);
	delete varName;
}

void
parseImage(char *filename)
{
	readPNG(filename);
	writeHeader(filename);
}

int
main(int argc, char *argv[])
{
	printf("Haiku Boot Splash Generator\n");
	printf("Copyright 2008, Artur Wyszynski <harakash@gmail.com>\n");
	printf("Distributed under the terms of the Haiku License. All rights reserved.\n");

	if (argc < 4) {
		printf("Usage:\n");
		printf("\thbsg splash.png icons.png copyright.png\n");
		return 0;
	}
	
	foutput = fopen("images.h", "wb");
	if (!foutput)
		error("Could not open file images.h for writing");

	fwrite("// This file was generated by Haiku Boot Splash Generator\n\n",
		strlen("// This file was generated by Haiku Boot Splash Generator\n\n"), 1, foutput);

	parseImage(argv[1]);
	parseImage(argv[2]);
	parseImage(argv[3]);

	fclose(foutput);
	return 0;
}
