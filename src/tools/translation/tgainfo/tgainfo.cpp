/*****************************************************************************/
// tgainfo
// Written by Michael Wilber, OBOS Translation Kit Team
//
// Version:
//
// tgainfo is a command line program for displaying information about
// TGA images. 
//
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2003 OpenBeOS Project
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
#include <ByteOrder.h>
#include <File.h>
#include <TranslatorFormats.h>

#define max(x,y) ((x > y) ? x : y)
#define DATA_BUFFER_SIZE 64

struct TGAFileHeader {
	uint8 idlength;
		// Number of bytes in the Image ID field
	uint8 colormaptype;
	uint8 imagetype;
};

#define TGA_NO_COLORMAP			0
#define TGA_COLORMAP			1

#define TGA_NO_IMAGE_DATA		0

#define TGA_NOCOMP_COLORMAP		1
#define TGA_NOCOMP_TRUECOLOR	2
#define TGA_NOCOMP_BW			3
#define TGA_RLE_COLORMAP		9
#define TGA_RLE_TRUECOLOR		10
#define TGA_RLE_BW				11

// Information about the color map (palette). These bytes are
// always present, but are zero if no color map is present
struct TGAColorMapSpec {
	uint16 firstentry;		// first useful entry in the color map
	uint16 length;			// number of color map entries
	uint8 entrysize;		// number of bits per entry
};

struct TGAImageSpec {
	uint16 xorigin;
	uint16 yorigin;
	uint16 width;
	uint16 height;
	uint8 depth;
	uint8 descriptor;
};

#define TGA_ORIGIN_VERT_BIT	0x20
#define TGA_ORIGIN_BOTTOM	0
#define TGA_ORIGIN_TOP		1

#define TGA_ORIGIN_HORZ_BIT	0x10
#define TGA_ORIGIN_LEFT		0
#define TGA_ORIGIN_RIGHT	1

#define TGA_DESC_BITS76		0xc0
#define TGA_DESC_ALPHABITS	0x0f

#define TGA_HEADERS_SIZE 18
#define TGA_FTR_LEN 26

const char *
colormaptype(uint8 n)
{
	switch (n) {
		case 0: return "No colormap";
		case 1: return "colormap";
	}
	return "unknown";
}

const char *
imagetype(uint8 n)
{
	switch (n) {
		case 0:  return "No Image Data";
		case 1:  return "colormap";
		case 2:  return "true color";
		case 3:  return "grayscale";
		case 9:  return "RLE colormap";
		case 10: return "RLE true color";
		case 11: return "RLE grayscale";
	}
	return "unknown";
}

void
print_tga_info(BFile &file)
{
	uint8 buf[TGA_HEADERS_SIZE];
	
	// read in TGA headers
	ssize_t size = TGA_HEADERS_SIZE;
	if (size > 0 && file.Read(buf, size) != size) {
		printf("Error: unable to read all TGA headers\n");
		return;
	}
	
	// TGA file header
	TGAFileHeader fh;
	fh.idlength = buf[0];
	fh.colormaptype = buf[1];
	fh.imagetype = buf[2];
	
	printf("\nFile Header:\n");
	printf("    id length: %d\n", fh.idlength);
	
	printf("colormap type: %d (%s)\n", fh.colormaptype,
		colormaptype(fh.colormaptype));
	printf("   image type: %d (%s)\n", fh.imagetype, imagetype(fh.imagetype));
	
	
	// TGA color map spec
	TGAColorMapSpec mapspec;
	memcpy(&mapspec.firstentry, buf + 3, 2);
	mapspec.firstentry = B_LENDIAN_TO_HOST_INT16(mapspec.firstentry);
	memcpy(&mapspec.length, buf + 5, 2);
	mapspec.length = B_LENDIAN_TO_HOST_INT16(mapspec.length);
	mapspec.entrysize = buf[7];
	
	printf("\nColormap Spec:\n");
	printf("first entry: %d\n", mapspec.firstentry);
	printf("     length: %d\n", mapspec.length);
	printf(" entry size: %d\n", mapspec.entrysize);
	
	
	// TGA image spec
	TGAImageSpec imagespec;
	memcpy(&imagespec.xorigin, buf + 8, 2);
	imagespec.xorigin = B_LENDIAN_TO_HOST_INT16(imagespec.xorigin);
	
	memcpy(&imagespec.yorigin, buf + 10, 2);
	imagespec.yorigin = B_LENDIAN_TO_HOST_INT16(imagespec.yorigin);
	
	memcpy(&imagespec.width, buf + 12, 2);
	imagespec.width = B_LENDIAN_TO_HOST_INT16(imagespec.width);

	memcpy(&imagespec.height, buf + 14, 2);
	imagespec.height = B_LENDIAN_TO_HOST_INT16(imagespec.height);
	
	imagespec.depth = buf[16];
	imagespec.descriptor = buf[17];
	
	printf("\nImage Spec:\n");
	printf("  x origin: %d\n", imagespec.xorigin);
	printf("  y origin: %d\n", imagespec.yorigin);
	printf("     width: %d\n", imagespec.width);
	printf("    height: %d\n", imagespec.height);
	printf("     depth: %d\n", imagespec.depth);
	printf("descriptor: 0x%.2lx\n", imagespec.descriptor);
		printf("\talpha (attr): %d\n",
			imagespec.descriptor & TGA_DESC_ALPHABITS);
		printf("\t      origin: %d (%s %s)\n",
			imagespec.descriptor & (TGA_ORIGIN_VERT_BIT | TGA_ORIGIN_HORZ_BIT),
			((imagespec.descriptor & TGA_ORIGIN_VERT_BIT) ? "top" : "bottom"),
			((imagespec.descriptor & TGA_ORIGIN_HORZ_BIT) ? "right" : "left"));
		printf("\t  bits 7 & 6: %d\n", imagespec.descriptor & TGA_DESC_BITS76);
		
		
	// Optional TGA Footer
	off_t filesize = 0;
	if (file.GetSize(&filesize) == B_OK) {
	
		char tgafooter[TGA_FTR_LEN + 1] = { 0 };
		if (file.ReadAt(filesize - TGA_FTR_LEN, tgafooter, TGA_FTR_LEN) == TGA_FTR_LEN) {
		
			if (strcmp(tgafooter + 8, "TRUEVISION-XFILE.") == 0) {
			
				uint32 extoffset = 0, devoffset = 0;
				memcpy(&extoffset, tgafooter, 4);
				memcpy(&devoffset, tgafooter + 4, 4);
				extoffset = B_LENDIAN_TO_HOST_INT32(extoffset);
				devoffset = B_LENDIAN_TO_HOST_INT32(devoffset);
			
				printf("\nTGA Footer:\n");
				printf("extension offset: 0x%.8lx (%d)\n", extoffset, extoffset);
				printf("developer offset: 0x%.8lx (%d)\n", devoffset, devoffset);
				printf("signature: %s\n", tgafooter + 8);
				
			} else
				printf("\nTGA footer not found\n");

		} else
			printf("\nError: Unable to read TGA footer section\n");
			
	} else
		printf("\nError: Unable to get file size\n");
}

int
main(int argc, char **argv)
{
	printf("\n");
	
	if (argc == 2) {
		BFile file(argv[1], B_READ_ONLY);
		if (file.InitCheck() != B_OK)
			printf("Error opening %s\n", argv[1]);
		else {
			printf("TGA image information for: %s\n", argv[1]);
			print_tga_info(file);
		}
	}
	else {
		printf("tgainfo - reports information about a TGA image file\n");
		printf("\nUsage:\n");
		printf("tgainfo filename.tga\n");	
	}
	
	printf("\n");

	return 0;
}

