/*****************************************************************************/
// bitsinfo
// Written by Michael Wilber, Haiku Translation Kit Team
//
// Version:
//
// bitsinfo is a command line program for displaying text information about
// Be bitmap format ("bits") images.  The Be bitmap format is used by
// the Translation Kit as the intermediate format for converting images.
// To make "bits" images, you can use the "translate" command line program or
// the BBitmapTranslator (available: http://www.bebits.com/app/647).
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
#define B_TRANSLATION_CONTEXT "bitsinfo"

struct ColorSpaceName {
	color_space id;
	const char *name;
};
#define COLORSPACENAME(id) {id, #id}

const char *kpixels = "-pixels";

void
PrintBitsInfo(const char *filepath, bool bdumppixels)
{
	BFile file(filepath, B_READ_ONLY);

	if (file.InitCheck() == B_OK) {
		TranslatorBitmap header;
		memset(&header, 0, sizeof(TranslatorBitmap));

		// read in the rest of the header
		ssize_t size = sizeof(TranslatorBitmap);
		if (file.Read(reinterpret_cast<uint8 *> (&header), size) != size) {
			printf(B_TRANSLATE("\nError: Unable to read the Be bitmap "
				"header.\n"));
			return;
		}
		if (!bdumppixels)
			// I don't need the file anymore
			file.Unset();

		// convert to host byte order
		if (swap_data(B_UINT32_TYPE, &header, sizeof(TranslatorBitmap),
			B_SWAP_BENDIAN_TO_HOST) != B_OK) {
			printf(B_TRANSLATE("\nError: Unable to swap byte order\n"));
			return;
		}

		printf(B_TRANSLATE("\nBe bitmap (\"bits\") header for: %s\n\n"),
			filepath);

		const uint32 kbitsmagic = 0x62697473UL;
			// in ASCII, this number looks like "bits"
		if (header.magic == kbitsmagic)
			printf(B_TRANSLATE("magic number: 0x%.8lx (valid)\n"),
				header.magic);
		else
			printf(B_TRANSLATE("magic number: 0x%.8lx (INVALID, should be: "
				"0x%.8lx)\n"), header.magic, kbitsmagic);
		printf(B_TRANSLATE("bounds: (%f, %f, %f, %f)\n"),
			header.bounds.left, header.bounds.top,
			header.bounds.right, header.bounds.bottom);
		printf(B_TRANSLATE("dimensions: %d x %d\n"),
			static_cast<int>(header.bounds.Width() + 1),
			static_cast<int>(header.bounds.Height() + 1));

		printf(B_TRANSLATE("bytes per row: %u\n"),
			static_cast<unsigned int>(header.rowBytes));

		// print out colorspace if it matches an item in the list
		ColorSpaceName colorspaces[] = {
			COLORSPACENAME(B_NO_COLOR_SPACE),
			COLORSPACENAME(B_RGB32),
			COLORSPACENAME(B_RGBA32),
			COLORSPACENAME(B_RGB24),
			COLORSPACENAME(B_RGB16),
			COLORSPACENAME(B_RGB15),
			COLORSPACENAME(B_RGBA15),
			COLORSPACENAME(B_CMAP8),
			COLORSPACENAME(B_GRAY8),
			COLORSPACENAME(B_GRAY1),
			COLORSPACENAME(B_RGB32_BIG),
			COLORSPACENAME(B_RGBA32_BIG),
			COLORSPACENAME(B_RGB24_BIG),
			COLORSPACENAME(B_RGB16_BIG),
			COLORSPACENAME(B_RGB15_BIG),
			COLORSPACENAME(B_RGBA15_BIG),
			COLORSPACENAME(B_YCbCr422),
			COLORSPACENAME(B_YCbCr411),
			COLORSPACENAME(B_YCbCr444),
			COLORSPACENAME(B_YCbCr420),
			COLORSPACENAME(B_YUV422),
			COLORSPACENAME(B_YUV411),
			COLORSPACENAME(B_YUV444),
			COLORSPACENAME(B_YUV420),
			COLORSPACENAME(B_YUV9),
			COLORSPACENAME(B_YUV12),
			COLORSPACENAME(B_UVL24),
			COLORSPACENAME(B_UVL32),
			COLORSPACENAME(B_UVLA32),
			COLORSPACENAME(B_LAB24),
			COLORSPACENAME(B_LAB32),
			COLORSPACENAME(B_LABA32),
			COLORSPACENAME(B_HSI24),
			COLORSPACENAME(B_HSI32),
			COLORSPACENAME(B_HSIA32),
			COLORSPACENAME(B_HSV24),
			COLORSPACENAME(B_HSV32),
			COLORSPACENAME(B_HSVA32),
			COLORSPACENAME(B_HLS24),
			COLORSPACENAME(B_HLS32),
			COLORSPACENAME(B_HLSA32),
			COLORSPACENAME(B_CMY24),
			COLORSPACENAME(B_CMY32),
			COLORSPACENAME(B_CMYA32),
			COLORSPACENAME(B_CMYK32)
		};
		const int32 kncolorspaces =  sizeof(colorspaces) /
			sizeof(ColorSpaceName);
		int32 i;
		for (i = 0; i < kncolorspaces; i++) {
			if (header.colors == colorspaces[i].id) {
				printf(B_TRANSLATE("color space: %s\n"), colorspaces[i].name);
				break;
			}
		}
		if (i == kncolorspaces)
			printf(B_TRANSLATE("color space: Unknown (0x%.8lx)\n"),
				static_cast<unsigned long>(header.colors));

		printf(B_TRANSLATE("data size: %u\n"),
			static_cast<unsigned int>(header.dataSize));

		if (bdumppixels) {
			const char *components = NULL;
			int32 ncomponents = 0;
			switch (header.colors) {
				case B_RGB24:
					components = "BGR";
					ncomponents = 3;
					break;
				case B_RGB32:
					components = "BGR-";
					ncomponents = 4;
					break;
				case B_RGBA32:
					components = "BGRA";
					ncomponents = 4;
					break;

				default:
					printf(B_TRANSLATE("Sorry, %s isn't supported yet"
						" for this color space\n"), kpixels);
					return;
			}
			uint8 *prow = new uint8[header.rowBytes];
			if (!prow) {
				printf(B_TRANSLATE("Error: Not enough memory for row "
					"buffer\n"));
				return;
			}
			ssize_t ret, n;
			uint32 totalbytes = 0;
			printf(B_TRANSLATE("pixel data (%s):\n"), components);
			while ((ret = file.Read(prow, header.rowBytes)) > 0) {
				n = 0;
				while (n < ret) {
					if (n && !(n % ncomponents))
						printf(" ");
					printf("%.2X", prow[n]);
					n++;
					totalbytes++;
					if (!(totalbytes % (uint32) ret))
						printf("\n\n");
				}
			}
		}

	} else
		printf(B_TRANSLATE_COMMENT("Error opening %s\n",
			"file path is opening"), filepath);
}

int
main(int argc, char **argv)
{
	if (argc == 1) {
		printf(B_TRANSLATE("\nbitsinfo - reports information about a Be "
			"bitmap (\"bits\") image\n"));
		printf(B_TRANSLATE("\nUsage:\n"));
		printf(B_TRANSLATE("bitsinfo [options] filename.bits\n\n"));
		printf(B_TRANSLATE("Options:\n\n"));
		printf(B_TRANSLATE("\t%s \t print RGB color for each pixel\n"),
			kpixels);
	}
	else {
		int32 first = 1;
		bool bdumppixels = false;
		if (strcmp(argv[1], kpixels) == 0) {
			bdumppixels = true;
			first = 2;
		}

		for (int32 i = first; i < argc; i++)
			PrintBitsInfo(argv[i], bdumppixels);
	}

	printf("\n");

	return 0;
}

