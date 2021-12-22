/*****************************************************************************/
// pnginfo
// Written by Michael Wilber, Haiku Translation Kit Team
//
// Version:
//
// pnginfo is a command line program for displaying text information about
// PNG images.
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2004 Haiku Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>
#include <ByteOrder.h>
#include <File.h>
#include <TranslatorFormats.h>
#include <StorageDefs.h>

 /* The png_jmpbuf() macro, used in error handling, became available in
  * libpng version 1.0.6.  If you want to be able to run your code with older
  * versions of libpng, you must define the macro yourself (but only if it
  * is not already defined by libpng!).
  */

#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

//// libpng Callback functions!
BPositionIO *
get_pio(png_structp ppng)
{
	BPositionIO *pio = NULL;
	pio = static_cast<BPositionIO *>(png_get_io_ptr(ppng));
	return pio;
}

void
pngcb_read_data(png_structp ppng, png_bytep pdata, png_size_t length)
{
	BPositionIO *pio = get_pio(ppng);
	pio->Read(pdata, static_cast<size_t>(length));
}

void
pngcb_write_data(png_structp ppng, png_bytep pdata, png_size_t length)
{
	BPositionIO *pio = get_pio(ppng);
	pio->Write(pdata, static_cast<size_t>(length));
}

void
pngcb_flush_data(png_structp ppng)
{
	// I don't think I really need to do anything here
}
//// End: libpng Callback functions

void
PrintPNGInfo(const char *path)
{
	printf("\n--- %s ---\n", path);
	
	BFile *pfile;
	pfile = new BFile(path, B_READ_ONLY);
	if (!pfile || pfile->InitCheck() != B_OK) {
		printf("Error: unable to open the file\n");
		return;
	}
	BPositionIO *pio = static_cast<BPositionIO *>(pfile);

	const int32 kSigSize = 8;
	uint8 buf[kSigSize];
	if (pio->Read(buf, kSigSize) != kSigSize) {
		printf("Error: unable to read PNG signature\n");
		return;
	}
	if (!png_check_sig(buf, kSigSize)) {
		// if first 8 bytes of stream don't match PNG signature bail
		printf("Error: file doesn't begin with PNG signature\n");
		return;
	}

	// use libpng to get info about the file
	png_structp ppng = NULL;
	png_infop pinfo = NULL;
	while (ppng == NULL) {
		// create PNG read pointer with default error handling routines
		ppng = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!ppng)
			break;
		// alocate / init memory for image information
		pinfo = png_create_info_struct(ppng);
		if (!pinfo)
			break;
		// set error handling
		if (setjmp(png_jmpbuf(ppng))) {
			// When an error occurs in libpng, it uses
			// the longjmp function to continue execution
			// from this point
			printf("Error: error in libpng function\n");
			break;
		}
		
		// set read callback function
		png_set_read_fn(ppng, static_cast<void *>(pio), pngcb_read_data);
		
		// Read in PNG image info
		png_set_sig_bytes(ppng, 8);
		png_read_info(ppng, pinfo);

		png_uint_32 width, height;
		int bit_depth, color_type, interlace_type, compression_type, filter_type;
		png_get_IHDR(ppng, pinfo, &width, &height, &bit_depth, &color_type,
			&interlace_type, &compression_type, &filter_type);

		printf("                   width: %lu\n", width);
		printf("                  height: %lu\n", height);
		printf("               row bytes: %lu\n", pinfo->rowbytes);
		printf("bit depth (bits/channel): %d\n", bit_depth);
		printf("                channels: %d\n", pinfo->channels);
		printf("pixel depth (bits/pixel): %d\n", pinfo->pixel_depth);
		printf("              color type: ");
		const char *desc = NULL;
		switch (color_type) {
			case PNG_COLOR_TYPE_GRAY:
				desc = "Grayscale";
				break;
			case PNG_COLOR_TYPE_PALETTE:
				desc = "Palette";
				break;
			case PNG_COLOR_TYPE_RGB:
				desc = "RGB";
				break;
			case PNG_COLOR_TYPE_RGB_ALPHA:
				desc = "RGB + Alpha";
				break;
			case PNG_COLOR_TYPE_GRAY_ALPHA:
				desc = "Grayscale + Alpha";
				break;
			default:
				desc = "Unknown";
				break;
		}
		printf("%s (%d)\n", desc, color_type);
		
		printf("             interlacing: ");
		switch (interlace_type) {
			case PNG_INTERLACE_NONE:
				desc = "None";
				break;
			case PNG_INTERLACE_ADAM7:
				desc = "Adam7";
				break;
			default:
				desc = "Unknown";
				break;
		}
		printf("%s (%d)\n", desc, interlace_type);
		
		printf("        compression type: ");
		switch (compression_type) {
			case PNG_COMPRESSION_TYPE_DEFAULT:
				desc = "Default: Deflate method 8, 32K window";
				break;
			default:
				desc = "Unknown";
				break;
		}
		printf("%s (%d)\n", desc, compression_type);
		
		printf("             filter type: ");
		switch (filter_type) {
			case PNG_FILTER_TYPE_DEFAULT:
				desc = "Single row per-byte";
				break;
			case PNG_INTRAPIXEL_DIFFERENCING:
				desc = "Intrapixel Differencing [for MNG files]";
				break;
			default:
				desc = "Unknown";
				break;
		}
		printf("%s (%d)\n", desc, filter_type);
	}
	if (ppng) {
		// free PNG handle / info structures
		if (!pinfo)
			png_destroy_read_struct(&ppng, png_infopp_NULL, png_infopp_NULL);
		else
			png_destroy_read_struct(&ppng, &pinfo, png_infopp_NULL);
	}
	
	delete pfile;
	pfile = NULL;
}

int
main(int argc, char **argv)
{
	if (argc == 1) {
		printf("\npnginfo - reports information about PNG images\n");
		printf("\nUsage:\n");
		printf("pnginfo [options] filename.png\n\n");
	}
	else {
		int32 first = 1;
		for (int32 i = first; i < argc; i++)
			PrintPNGInfo(argv[i]);
	}
	
	printf("\n");
	
	return 0;
}

