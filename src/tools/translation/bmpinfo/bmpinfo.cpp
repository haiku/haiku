/*****************************************************************************/
// bmpinfo
// Written by Michael Wilber, Haiku Translation Kit Team
//
// Version:
//
// bmpinfo is a command line program for displaying information about
// BMP images.
//
//
// This application and all source files used in its construction, except
// where noted, are licensed under the MIT License, and have been written
// and are:
//
// Copyright (c) 2003 Haiku Project
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
#include <Catalog.h>
#include <File.h>
#include <TranslatorFormats.h>
#include <StorageDefs.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "bmpinfo"

#define BMP_NO_COMPRESS 0
#define BMP_RLE8_COMPRESS 1
#define BMP_RLE4_COMPRESS 2

struct BMPFileHeader {
	// for both MS and OS/2 BMP formats
	uint16 magic;			// = 'BM'
	uint32 fileSize;		// file size in bytes
	uint32 reserved;		// equals 0
	uint32 dataOffset;		// file offset to actual image
};

struct MSInfoHeader {
	uint32 size;			// size of this struct (40)
	uint32 width;			// bitmap width
	uint32 height;			// bitmap height
	uint16 planes;			// number of planes, always 1?
	uint16 bitsperpixel;	// bits per pixel, (1,4,8,16 or 24)
	uint32 compression;		// type of compression
	uint32 imagesize;		// size of image data if compressed
	uint32 xpixperm;		// horizontal pixels per meter
	uint32 ypixperm;		// vertical pixels per meter
	uint32 colorsused;		// number of actually used colors
	uint32 colorsimportant;	// number of important colors, zero = all
};

struct OS2InfoHeader {
	uint32 size;			// size of this struct (12)
	uint16 width;			// bitmap width
	uint16 height;			// bitmap height
	uint16 planes;			// number of planes, always 1?
	uint16 bitsperpixel;	// bits per pixel, (1,4,8,16 or 24)
};

void
print_bmp_info(BFile &file)
{
	uint8 buf[40];
	BMPFileHeader fh;

	ssize_t size = 14;
	if (file.Read(buf, size) != size) {
		printf(B_TRANSLATE("Error: unable to read BMP file header\n"));
		return;
	}

	// convert fileHeader to host byte order
	memcpy(&fh.magic, buf, 2);
	memcpy(&fh.fileSize, buf + 2, 4);
	memcpy(&fh.reserved, buf + 6, 4);
	memcpy(&fh.dataOffset, buf + 10, 4);
	swap_data(B_UINT16_TYPE, &fh.magic, sizeof(uint16),
		B_SWAP_BENDIAN_TO_HOST);
	swap_data(B_UINT32_TYPE, (reinterpret_cast<uint8 *> (&fh)) + 2,
		12, B_SWAP_LENDIAN_TO_HOST);

	printf(B_TRANSLATE("\nFile Header:\n"));
	printf(B_TRANSLATE("      magic: 0x%.4x (should be: 0x424d)\n"), fh.magic);
	printf(B_TRANSLATE("  file size: 0x%.8lx (%lu)\n"), fh.fileSize,
		fh.fileSize);
	printf(B_TRANSLATE("   reserved: 0x%.8lx (should be: 0x%.8x)\n"),
		fh.reserved, 0);
	printf(B_TRANSLATE("data offset: 0x%.8lx (%lu) (should be: >= 54 for MS "
		"format and >= 26 for OS/2 format)\n"), fh.dataOffset, fh.dataOffset);

	uint32 headersize = 0;
	if (file.Read(&headersize, 4) != 4) {
		printf(B_TRANSLATE("Error: unable to read info header size\n"));
		return;
	}
	swap_data(B_UINT32_TYPE, &headersize, 4, B_SWAP_LENDIAN_TO_HOST);

	if (headersize == sizeof(MSInfoHeader)) {
		// MS format
		MSInfoHeader msh;
		msh.size = headersize;
		if (file.Read(reinterpret_cast<uint8 *> (&msh) + 4, 36) != 36) {
			printf(B_TRANSLATE("Error: unable to read entire MS info header\n"));
			return;
		}

		// convert msheader to host byte order
		swap_data(B_UINT32_TYPE, reinterpret_cast<uint8 *> (&msh) + 4, 36,
			B_SWAP_LENDIAN_TO_HOST);

		printf(B_TRANSLATE("\nMS Info Header:\n"));
		printf(B_TRANSLATE("     header size: 0x%.8lx (%lu) (should be: "
			"40)\n"), msh.size, msh.size);
		printf(B_TRANSLATE("           width: %lu\n"), msh.width);
		printf(B_TRANSLATE("          height: %lu\n"), msh.height);
		printf(B_TRANSLATE("          planes: %u (should be: 1)\n"),
			msh.planes);
		printf(B_TRANSLATE("  bits per pixel: %u (should be: 1,4,8,16,24 or "
			"32)\n"), msh.bitsperpixel);
		if (msh.compression == BMP_NO_COMPRESS)
			printf(B_TRANSLATE("     compression: none (%lu)\n"),
				msh.compression);
		else if (msh.compression == BMP_RLE8_COMPRESS)
			printf(B_TRANSLATE("     compression: RLE8 (%lu)\n"),
				msh.compression);
		else if (msh.compression == BMP_RLE4_COMPRESS)
			printf(B_TRANSLATE("     compression: RLE4 (%lu)\n"),
				msh.compression);
		else
			printf(B_TRANSLATE("     compression: unknown (%lu)\n"),
				msh.compression);
		printf(B_TRANSLATE("      image size: 0x%.8lx (%lu)\n"), msh.imagesize,
			msh.imagesize);
		printf(B_TRANSLATE("  x pixels/meter: %lu\n"), msh.xpixperm);
		printf(B_TRANSLATE("  y pixels/meter: %lu\n"), msh.ypixperm);
		printf(B_TRANSLATE("     colors used: %lu\n"), msh.colorsused);
		printf(B_TRANSLATE("colors important: %lu\n"), msh.colorsimportant);

	} else if (headersize == sizeof(OS2InfoHeader)) {
		// OS/2 format

		OS2InfoHeader os2;
		os2.size = headersize;
		if (file.Read(reinterpret_cast<uint8 *> (&os2) + 4, 8) != 8) {
			printf(B_TRANSLATE("Error: unable to read entire OS/2 info "
				"header\n"));
			return;
		}

		// convert os2header to host byte order
		swap_data(B_UINT32_TYPE, reinterpret_cast<uint8 *> (&os2) + 4, 8,
			B_SWAP_LENDIAN_TO_HOST);

		printf(B_TRANSLATE("\nOS/2 Info Header:\n"));
		printf(B_TRANSLATE("   header size: 0x%.8lx (%lu) (should be: 12)\n"),
			os2.size, os2.size);
		printf(B_TRANSLATE("         width: %u\n"), os2.width);
		printf(B_TRANSLATE("        height: %u\n"), os2.height);
		printf(B_TRANSLATE("        planes: %u (should be: 1)\n"), os2.planes);
		printf(B_TRANSLATE("bits per pixel: %u (should be: 1,4,8 or 24)\n"),
			os2.bitsperpixel);

	} else
		printf(B_TRANSLATE("Error: info header size (%lu) does not match MS "
			"or OS/2 info header size\n"), headersize);
}

int
main(int argc, char **argv)
{
	printf("\n");

	if (argc == 1) {
		printf(B_TRANSLATE("bmpinfo - reports information about a BMP image "
			"file\n"));
		printf(B_TRANSLATE("\nUsage:\n"));
		printf(B_TRANSLATE("bmpinfo filename.bmp\n"));
	}
	else {
		BFile file;

		for (int32 i = 1; i < argc; i++) {
			if (file.SetTo(argv[i], B_READ_ONLY) != B_OK)
				printf(B_TRANSLATE("\nError opening %s\n"), argv[i]);
			else {
				printf(B_TRANSLATE("\nBMP image information for: %s\n"),
					argv[i]);
				print_bmp_info(file);
			}
		}

	}

	printf("\n");

	return 0;
}

