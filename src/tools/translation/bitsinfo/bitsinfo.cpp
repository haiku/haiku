/*****************************************************************************/
// bitsinfo
// Written by Michael Wilber, OBOS Translation Kit Team
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

void
PrintBitsInfo(const char *filepath)
{
	BFile file(filepath, B_READ_ONLY);
	
	if (file.InitCheck() == B_OK) {
		TranslatorBitmap header;
		memset(&header, 0, sizeof(TranslatorBitmap));
		
		// read in the rest of the header
		ssize_t size = sizeof(TranslatorBitmap);
		if (file.Read(reinterpret_cast<uint8 *> (&header), size) != size) {
			printf("Error reading from file\n");
			return;
		}
		// convert to host byte order
		if (swap_data(B_UINT32_TYPE, &header, sizeof(TranslatorBitmap),
			B_SWAP_BENDIAN_TO_HOST) != B_OK) {
			printf("Unable to swap byte order\n");
			return;
		}
		
		printf("bits header for: %s\n", filepath);
		
		printf("width: %d\n",
			static_cast<int>(header.bounds.Width() + 1));
		printf("height: %d\n",
			static_cast<int>(header.bounds.Height() + 1));
		
		printf("rowBytes: %u\n",
			static_cast<unsigned int>(header.rowBytes));
	
		// print out colorspace if it matches an item in the list	
		struct ColorSpaceName { 
			color_space id;
			const char *name;
		};
		ColorSpaceName colorspaces[] = {
			{ B_NO_COLOR_SPACE, "B_NO_COLOR_SPACE" },
			{ B_RGB32,		"B_RGB32" },
			{ B_RGBA32,		"B_RGBA32" },
			{ B_RGB24,		"B_RGB24" },
			{ B_RGB16,		"B_RGB16" },
			{ B_RGB15,		"B_RGB15" },
			{ B_RGBA15,		"B_RGBA15" },
			{ B_CMAP8,		"B_CMAP8" },
			{ B_GRAY8,		"B_GRAY8" },
			{ B_GRAY1,		"B_GRAY1" },
			{ B_RGB32_BIG,	"B_RGB32_BIG" },
			{ B_RGBA32_BIG,	"B_RGBA32_BIG" },
			{ B_RGB24_BIG,	"B_RGB24_BIG" },
			{ B_RGB16_BIG,	"B_RGB16_BIG" },
			{ B_RGB15_BIG,	"B_RGB15_BIG" },
			{ B_RGBA15_BIG,	"B_RGBA15_BIG" },
			{ B_YCbCr422,	"B_YCbCr422" },
			{ B_YCbCr411,	"B_YCbCr411" },
			{ B_YCbCr444,	"B_YCbCr444" },
			{ B_YCbCr420,	"B_YCbCr420" },
			{ B_YUV422,		"B_YUV422" },
			{ B_YUV411,		"B_YUV411" },
			{ B_YUV444,		"B_YUV444" },
			{ B_YUV420,		"B_YUV420" },
			{ B_YUV9,		"B_YUV9" },
			{ B_YUV12,		"B_YUV12" },
			{ B_UVL24,		"B_UVL24" },
			{ B_UVL32,		"B_UVL32" },
			{ B_UVLA32,		"B_UVLA32" },
			{ B_LAB24,		"B_LAB24" },
			{ B_LAB32,		"B_LAB32" },
			{ B_LABA32,		"B_LABA32" },
			{ B_HSI24,		"B_LABA32" },
			{ B_HSI32,		"B_HSI32" },
			{ B_HSIA32,		"B_HSIA32" },
			{ B_HSV24,		"B_HSV24" },
			{ B_HSV32,		"B_HSV32" },
			{ B_HSVA32,		"B_HSVA32" },
			{ B_HLS24,		"B_HLS24" },
			{ B_HLS32,		"B_HLS32" },
			{ B_HLSA32,		"B_HLSA32" },
			{ B_CMY24,		"B_CMY24" },
			{ B_CMY32,		"B_CMY32" },
			{ B_CMYA32,		"B_CMYA32" },
			{ B_CMYK32,		"B_CMYK32" }
		};
		const int32 kncolorspaces =  sizeof(colorspaces) /
			sizeof(ColorSpaceName);
		int32 i;
		for (i = 0; i < kncolorspaces; i++) {
			if (header.colors == colorspaces[i].id) {
				printf("colors: %s\n", colorspaces[i].name);
				break;
			}
		}
		if (i == kncolorspaces)
			printf("colors: Unknown (0x%.8lx)\n",
				static_cast<unsigned long>(header.colors));
			
		printf("dataSize: %u\n",
			static_cast<unsigned int>(header.dataSize));
	} else
		printf("Error opening %s\n", filepath);
}

int
main(int argc, char **argv)
{
	printf("\n");
	
	if (argc == 2)
		PrintBitsInfo(argv[1]);
	else {
		printf("bitsinfo - reports information about a Be bitmap (\"bits\") image\n");
		printf("\nUsage:\n");
		printf("bitsinfo filename.bits\n\n");	
	}

	return 0;
}

