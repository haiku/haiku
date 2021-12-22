/*****************************************************************************/
// tgainfo
// Written by Michael Wilber, Haiku Translation Kit Team
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
#define B_TRANSLATION_CONTEXT "tgainfo"

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
#define TGA_EXT_LEN 495
#define LINE_LEN 82

const char *
colormaptype(uint8 n)
{
	switch (n) {
		case 0: return B_TRANSLATE("No colormap");
		case 1: return B_TRANSLATE("colormap");
	}
	return "unknown";
}

const char *
imagetype(uint8 n)
{
	switch (n) {
		case 0:  return B_TRANSLATE("No Image Data");
		case 1:  return B_TRANSLATE("colormap");
		case 2:  return B_TRANSLATE("true color");
		case 3:  return B_TRANSLATE("grayscale");
		case 9:  return B_TRANSLATE("RLE colormap");
		case 10: return B_TRANSLATE("RLE true color");
		case 11: return B_TRANSLATE("RLE grayscale");
		default: break;
	}
	return B_TRANSLATE("unknown");
}

uint16
tga_uint16(char *buffer, int32 offset)
{
	return B_LENDIAN_TO_HOST_INT16(*(reinterpret_cast<uint16 *>(buffer + offset)));
}

uint32
tga_uint32(char *buffer, int32 offset)
{
	return B_LENDIAN_TO_HOST_INT32(*(reinterpret_cast<uint32 *>(buffer + offset)));
}

void
print_tga_info(BFile &file)
{
	uint8 buf[TGA_HEADERS_SIZE];

	// read in TGA headers
	ssize_t size = TGA_HEADERS_SIZE;
	if (size > 0 && file.Read(buf, size) != size) {
		printf(B_TRANSLATE("Error: unable to read all TGA headers\n"));
		return;
	}

	// TGA file header
	TGAFileHeader fh;
	fh.idlength = buf[0];
	fh.colormaptype = buf[1];
	fh.imagetype = buf[2];

	printf(B_TRANSLATE("\nFile Header:\n"));
	printf(B_TRANSLATE("    id length: %d\n"), static_cast<int>(fh.idlength));

	printf(B_TRANSLATE("colormap type: %d (%s)\n"),
		static_cast<int>(fh.colormaptype),
		static_cast<const char *>(colormaptype(fh.colormaptype)));
	printf(B_TRANSLATE("   image type: %d (%s)\n"),
		static_cast<int>(fh.imagetype),
		static_cast<const char *>(imagetype(fh.imagetype)));


	// TGA color map spec
	TGAColorMapSpec mapspec;
	mapspec.firstentry = tga_uint16(reinterpret_cast<char *>(buf), 3);
	mapspec.length = tga_uint16(reinterpret_cast<char *>(buf), 5);
	mapspec.entrysize = buf[7];

	printf(B_TRANSLATE("\nColormap Spec:\n"));
	printf(B_TRANSLATE("first entry: %d\n"),
		static_cast<int>(mapspec.firstentry));
	printf(B_TRANSLATE("     length: %d\n"),
		static_cast<int>(mapspec.length));
	printf(B_TRANSLATE(" entry size: %d\n"),
		static_cast<int>(mapspec.entrysize));


	// TGA image spec
	TGAImageSpec imagespec;
	imagespec.xorigin = tga_uint16(reinterpret_cast<char *>(buf), 8);
	imagespec.yorigin = tga_uint16(reinterpret_cast<char *>(buf), 10);
	imagespec.width = tga_uint16(reinterpret_cast<char *>(buf), 12);
	imagespec.height = tga_uint16(reinterpret_cast<char *>(buf), 14);
	imagespec.depth = buf[16];
	imagespec.descriptor = buf[17];

	printf(B_TRANSLATE("\nImage Spec:\n"));
	printf(B_TRANSLATE("  x origin: %d\n"),
		static_cast<int>(imagespec.xorigin));
	printf(B_TRANSLATE("  y origin: %d\n"),
		static_cast<int>(imagespec.yorigin));
	printf(B_TRANSLATE("     width: %d\n"),
		static_cast<int>(imagespec.width));
	printf(B_TRANSLATE("    height: %d\n"),
		static_cast<int>(imagespec.height));
	printf(B_TRANSLATE("     depth: %d\n"),
		static_cast<int>(imagespec.depth));
	printf(B_TRANSLATE("descriptor: 0x%.2x\n"),
		static_cast<int>(imagespec.descriptor));
	printf(B_TRANSLATE("\talpha (attr): %d\n"),
		static_cast<int>(imagespec.descriptor & TGA_DESC_ALPHABITS));
	if (imagespec.descriptor & TGA_ORIGIN_VERT_BIT)
		if (imagespec.descriptor & TGA_ORIGIN_HORZ_BIT)
			printf(B_TRANSLATE("\t      origin: %d (%s %s)\n"),
				static_cast<int>(imagespec.descriptor & (TGA_ORIGIN_VERT_BIT
				| TGA_ORIGIN_HORZ_BIT)), static_cast<const char *>("top"),
				static_cast<const char *>("right"));
		else
			printf(B_TRANSLATE("\t      origin: %d (%s %s)\n"),
			static_cast<int>(imagespec.descriptor & (TGA_ORIGIN_VERT_BIT
				| TGA_ORIGIN_HORZ_BIT)), static_cast<const char *>("top"),
				static_cast<const char *>("left"));
	else
		if (imagespec.descriptor & TGA_ORIGIN_HORZ_BIT)
			printf(B_TRANSLATE("\t      origin: %d (%s %s)\n"),
				static_cast<int>(imagespec.descriptor & (TGA_ORIGIN_VERT_BIT
				| TGA_ORIGIN_HORZ_BIT)), static_cast<const char *>("bottom"),
				static_cast<const char *>("right"));
		else
			printf(B_TRANSLATE("\t      origin: %d (%s %s)\n"),
			static_cast<int>(imagespec.descriptor & (TGA_ORIGIN_VERT_BIT
				| TGA_ORIGIN_HORZ_BIT)), static_cast<const char *>("bottom"),
				static_cast<const char *>("left"));


	printf(B_TRANSLATE("\t  bits 7 & 6: %d\n"),
		static_cast<int>(imagespec.descriptor & TGA_DESC_BITS76));


	// Optional TGA Footer
	off_t filesize = 0;
	if (file.GetSize(&filesize) == B_OK) {

		char tgafooter[TGA_FTR_LEN + 1] = { 0 };
		if (file.ReadAt(filesize - TGA_FTR_LEN, tgafooter, TGA_FTR_LEN) == TGA_FTR_LEN) {

			if (strcmp(tgafooter + 8, "TRUEVISION-XFILE.") == 0) {

				uint32 extoffset = 0, devoffset = 0;
				extoffset = tga_uint32(tgafooter, 0);
				devoffset = tga_uint32(tgafooter, 4);

				printf(B_TRANSLATE("\nTGA Footer:\n"));
				printf(B_TRANSLATE("extension offset: 0x%.8lx (%ld)\n"),
					static_cast<long int>(extoffset),
					static_cast<long int>(extoffset));
				printf(B_TRANSLATE("developer offset: 0x%.8lx (%ld)\n"),
					static_cast<long int>(devoffset),
					static_cast<long int>(devoffset));
				printf(B_TRANSLATE("signature: %s\n"), tgafooter + 8);

				if (extoffset) {
					char extbuf[TGA_EXT_LEN];
					if (file.ReadAt(extoffset, extbuf, TGA_EXT_LEN) == TGA_EXT_LEN) {

						printf(B_TRANSLATE("\nExtension Area:\n"));

						char strbuffer[LINE_LEN];

						uint16 extsize = tga_uint16(extbuf, 0);
						if (extsize < TGA_EXT_LEN) {
							printf(B_TRANSLATE("\nError: extension "
								"area is too small (%d)\n"), extsize);
							return;
						}
						printf(B_TRANSLATE("size: %d\n"), extsize);

						memset(strbuffer, 0, LINE_LEN);
						strncpy(strbuffer, extbuf + 2, 41);
						printf("author: \"%s\"\n", strbuffer);

						printf(B_TRANSLATE("comments:\n"));
						for (int32 i = 0; i < 4; i++) {
							memset(strbuffer, 0, LINE_LEN);
							strcpy(strbuffer, extbuf + 43 + (i * 81));
							printf(B_TRANSLATE("\tline %ld: \"%s\"\n"),
								static_cast<long int>(i + 1),
								static_cast<const char *>(strbuffer));
						}

						printf(B_TRANSLATE("date/time (yyyy-mm-dd hh:mm:ss): "
							"%.4d-%.2d-%.2d %.2d:%.2d:%.2d\n"),
							tga_uint16(extbuf, 367), tga_uint16(extbuf, 369),
							tga_uint16(extbuf, 371), tga_uint16(extbuf, 373),
							tga_uint16(extbuf, 375), tga_uint16(extbuf, 377));

						memset(strbuffer, 0, LINE_LEN);
						strncpy(strbuffer, extbuf + 379, 41);
						printf(B_TRANSLATE("job name: \"%s\"\n"), strbuffer);

						printf(B_TRANSLATE("job time (hh:mm:ss): "
							"%.2d:%.2d:%.2d\n"), tga_uint16(extbuf, 420),
							tga_uint16(extbuf, 422), tga_uint16(extbuf, 424));

						memset(strbuffer, 0, LINE_LEN);
						strncpy(strbuffer, extbuf + 426, 41);
						printf(B_TRANSLATE("software id: \"%s\"\n"),
							strbuffer);

						char strver[] = "[null]";
						if (extbuf[469] != '\0') {
							strver[0] = extbuf[469];
							strver[1] = '\0';
						}
						printf(B_TRANSLATE("software version, letter: %d, "
							"%s\n"), tga_uint16(extbuf, 467), strver);

						printf(B_TRANSLATE("key color (A,R,G,B): %d, %d, %d, "
							"%d\n"), extbuf[470], extbuf[471], extbuf[472],
							extbuf[473]);

						printf(B_TRANSLATE("pixel aspect ratio: %d / %d\n"),
							tga_uint16(extbuf, 474), tga_uint16(extbuf, 476));

						printf(B_TRANSLATE("gamma value: %d / %d\n"),
							tga_uint16(extbuf, 478), tga_uint16(extbuf, 480));

						printf(B_TRANSLATE("color correction offset: 0x%.8lx "
							"(%ld)\n"),	tga_uint32(extbuf, 482),
							tga_uint32(extbuf, 482));
						printf(B_TRANSLATE("postage stamp offset: 0x%.8lx "
							"(%ld)\n"),	tga_uint32(extbuf, 486),
							tga_uint32(extbuf, 486));
						printf(B_TRANSLATE("scan line offset: 0x%.8lx "
							"(%ld)\n"), tga_uint32(extbuf, 490),
							tga_uint32(extbuf, 490));

						const char *strattrtype = NULL;
						uint8 attrtype = extbuf[494];
						switch (attrtype) {
							case 0: strattrtype
								= B_TRANSLATE("no alpha"); break;
							case 1: strattrtype
								= B_TRANSLATE("undefined, ignore"); break;
							case 2: strattrtype
								= B_TRANSLATE("undefined, retain"); break;
							case 3: strattrtype
								= B_TRANSLATE("alpha"); break;
							case 4: strattrtype
								= B_TRANSLATE("pre-multiplied alpha"); break;
							default:
								if (attrtype > 4 && attrtype < 128)
									strattrtype = B_TRANSLATE("reserved");
								else
									strattrtype = B_TRANSLATE("unassigned");
								break;
						}
						printf(B_TRANSLATE("attributes type: %d (%s)\n"),
							attrtype, strattrtype);

					} else
						printf(B_TRANSLATE("\nError: Unable to read entire "
							"extension area\n"));
				}

			} else
				printf(B_TRANSLATE("\nTGA footer not found\n"));

		} else
			printf(B_TRANSLATE("\nError: Unable to read TGA footer "
				"section\n"));

	} else
		printf(B_TRANSLATE("\nError: Unable to get file size\n"));
}

int
main(int argc, char **argv)
{
	printf("\n");

	if (argc == 1) {
		printf(B_TRANSLATE("tgainfo - reports information about a TGA image file\n"));
		printf(B_TRANSLATE("\nUsage:\n"));
		printf(B_TRANSLATE("tgainfo filename.tga\n"));
	}
	else {
		BFile file;

		for (int32 i = 1; i < argc; i++) {
			if (file.SetTo(argv[i], B_READ_ONLY) != B_OK)
				printf(B_TRANSLATE("\nError opening %s\n"), argv[i]);
			else {
				printf(B_TRANSLATE("\nTGA image information for: %s\n"), argv[i]);
				print_tga_info(file);
			}
		}

	}

	printf("\n");

	return 0;
}

