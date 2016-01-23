/*
 * Copyright 2013-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include <Application.h>
#include <Screen.h>
#include <stdio.h>


#define M(x)	{ B_##x, #x }

struct ColorSpace {
	color_space space;
	const char* name;
};

ColorSpace table[] =
{
	M(RGB32),
	M(RGB32),
	M(RGBA32),
	M(RGB24),
	M(RGB16),
	M(RGB15),
	M(RGBA15),
	M(CMAP8),
	M(GRAY8),
	M(GRAY1),
	M(RGB32_BIG),
	M(RGBA32_BIG),
	M(RGB24_BIG),
	M(RGB16_BIG),
	M(RGB15_BIG),
	M(RGBA15_BIG),
	M(YCbCr422),
	M(YCbCr411),
	M(YCbCr444),
	M(YCbCr420),
	M(YUV422),
	M(YUV411),
	M(YUV444),
	M(YUV420),
	M(YUV9),
	M(YUV12),
	M(UVL24),
	M(UVL32),
	M(UVLA32),
	M(LAB24),
	M(LAB32),
	M(LABA32),
	M(HSI24),
	M(HSI32),
	M(HSIA32),
	M(HSV24),
	M(HSV32),
	M(HSVA32),
	M(HLS24),
	M(HLS32),
	M(HLSA32),
	M(CMY24),
	M(CMY32),
	M(CMYA32),
	M(CMYK32),
	M(CMYA32),
	M(CMYK32)
};


void
print_supported_overlays()
{
	for (int32 i = 0; i < sizeof(table) / sizeof(table[0]); i++)
	{
		uint32 supportFlags = 0;

		if (bitmaps_support_space(table[i].space, &supportFlags)
				&& (supportFlags & B_BITMAPS_SUPPORT_OVERLAY))
			printf("\t%s\n", table[i].name);
	}
}


int
main()
{
	// BScreen usage requires BApplication for AppServerLink
	BApplication app("application/x-vnd.Haiku-screen_info");

	BScreen screen(B_MAIN_SCREEN_ID);

	do {
		screen_id screenIndex = screen.ID();
		accelerant_device_info info;

		// At the moment, screen.ID() is always 0;
		printf("Screen %" B_PRId32 ":", screen.ID().id);
		if (screen.GetDeviceInfo(&info) != B_OK) {
			printf(" unavailable\n");
		} else {
			printf(" attached\n");
			printf("  version: %" B_PRId32 "\n", info.version);
			printf("  name:    %s\n", info.name);
			printf("  chipset: %s\n", info.chipset);
			printf("  serial:  %s\n", info.serial_no);
			printf("  bitmap overlay colorspaces supported:\n");
			print_supported_overlays();
		}
	} while (screen.SetToNext() == B_OK);

	return 0;
}
