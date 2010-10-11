/*
 * Copyright 2008-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <KernelExport.h>

#define __BOOTSPLASH_KERNEL__
#include <boot/images.h>

#include <boot_item.h>
#include <debug.h>
#include <frame_buffer_console.h>

#include <boot_splash.h>


//#define TRACE_BOOT_SPLASH 1
#ifdef TRACE_BOOT_SPLASH
#	define TRACE(x...) dprintf(x);
#else
#	define TRACE(x...) ;
#endif


static struct frame_buffer_boot_info *sInfo;
static uint8 *sUncompressedIcons;


static void
blit8_cropped(const uint8 *data, uint16 imageLeft, uint16 imageTop,
	uint16 imageRight, uint16 imageBottom, uint16 imageWidth,
	uint16 left, uint16 top)
{
	data += (imageWidth * imageTop + imageLeft);
	uint8* start = (uint8*)(sInfo->frame_buffer
		+ sInfo->bytes_per_row * (top + imageTop) + 1 * (left + imageLeft));

	for (int32 y = imageTop; y < imageBottom; y++) {
		const uint8* src = data;
		uint8* dst = start;
		for (int32 x = imageLeft; x < imageRight; x++) {
			dst[0] = src[0];
			dst++;
			src++;
		}

		data += imageWidth;
		start = (uint8*)((addr_t)start + sInfo->bytes_per_row);
	}
}


static void
blit15_cropped(const uint8 *data, uint16 imageLeft, uint16 imageTop,
	uint16 imageRight, uint16 imageBottom, uint16 imageWidth,
	uint16 left, uint16 top)
{
	data += (imageWidth * imageTop + imageLeft) * 3;
	uint16* start = (uint16*)(sInfo->frame_buffer
		+ sInfo->bytes_per_row * (top + imageTop)
		+ 2 * (left + imageLeft));

	for (int32 y = imageTop; y < imageBottom; y++) {
		const uint8* src = data;
		uint16* dst = start;
		for (int32 x = imageLeft; x < imageRight; x++) {
			dst[0] = ((src[2] >> 3) << 10)
				| ((src[1] >> 3) << 5)
				| ((src[0] >> 3));

			dst++;
			src += 3;
		}

		data += imageWidth * 3;
		start = (uint16*)((addr_t)start + sInfo->bytes_per_row);
	}
}


static void
blit16_cropped(const uint8 *data, uint16 imageLeft, uint16 imageTop,
	uint16 imageRight, uint16 imageBottom, uint16 imageWidth,
	uint16 left, uint16 top)
{
	data += (imageWidth * imageTop + imageLeft) * 3;
	uint16* start = (uint16*)(sInfo->frame_buffer
		+ sInfo->bytes_per_row * (top + imageTop) + 2 * (left + imageLeft));

	for (int32 y = imageTop; y < imageBottom; y++) {
		const uint8* src = data;
		uint16* dst = start;
		for (int32 x = imageLeft; x < imageRight; x++) {
			dst[0] = ((src[2] >> 3) << 11)
				| ((src[1] >> 2) << 5)
				| ((src[0] >> 3));

			dst++;
			src += 3;
		}

		data += imageWidth * 3;
		start = (uint16*)((addr_t)start + sInfo->bytes_per_row);
	}
}


static void
blit24_cropped(const uint8 *data, uint16 imageLeft, uint16 imageTop,
	uint16 imageRight, uint16 imageBottom, uint16 imageWidth,
	uint16 left, uint16 top)
{
	data += (imageWidth * imageTop + imageLeft) * 3;
	uint8* start = (uint8*)(sInfo->frame_buffer
		+ sInfo->bytes_per_row * (top + imageTop) + 3 * (left + imageLeft));

	for (int32 y = imageTop; y < imageBottom; y++) {
		const uint8* src = data;
		uint8* dst = start;
		for (int32 x = imageLeft; x < imageRight; x++) {
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst += 3;
			src += 3;
		}

		data += imageWidth * 3;
		start = (uint8*)((addr_t)start + sInfo->bytes_per_row);
	}
}


static void
blit32_cropped(const uint8 *data, uint16 imageLeft, uint16 imageTop,
	uint16 imageRight, uint16 imageBottom, uint16 imageWidth,
	uint16 left, uint16 top)
{
	data += (imageWidth * imageTop + imageLeft) * 3;
	uint32* start = (uint32*)(sInfo->frame_buffer
		+ sInfo->bytes_per_row * (top + imageTop) + 4 * (left + imageLeft));

	for (int32 y = imageTop; y < imageBottom; y++) {
		const uint8* src = data;
		uint32* dst = start;
		for (int32 x = imageLeft; x < imageRight; x++) {
			dst[0] = (src[2] << 16) | (src[1] << 8) | (src[0]);
			dst++;
			src += 3;
		}

		data += imageWidth * 3;
		start = (uint32*)((addr_t)start + sInfo->bytes_per_row);
	}
}


static void
blit_cropped(const uint8* data, uint16 imageLeft, uint16 imageTop,
	uint16 imageRight, uint16 imageBottom, uint16 imageWidth,
	uint16 left, uint16 top)
{
	switch (sInfo->depth) {
		case 8:
			blit8_cropped(data, imageLeft, imageTop, imageRight, imageBottom,
				imageWidth, left, top);
			return;
		case 15:
			blit15_cropped(data, imageLeft, imageTop, imageRight, imageBottom,
				imageWidth, left, top);
			return;
		case 16:
			blit16_cropped(data, imageLeft, imageTop, imageRight, imageBottom,
				imageWidth, left, top);
			return;
		case 24:
			blit24_cropped(data, imageLeft, imageTop, imageRight, imageBottom,
				imageWidth, left, top);
			return;
		case 32:
			blit32_cropped(data, imageLeft, imageTop, imageRight, imageBottom,
				imageWidth, left, top);
			return;
	}
}


//	#pragma mark - exported functions


void
boot_splash_init(uint8 *bootSplash)
{
	TRACE("boot_splash_init: enter\n");

	if (debug_screen_output_enabled())
		return;

	sInfo = (frame_buffer_boot_info *)get_boot_item(FRAME_BUFFER_BOOT_INFO,
		NULL);

	sUncompressedIcons = bootSplash;
}


void
boot_splash_uninit(void)
{
	sInfo = NULL;
}


void
boot_splash_set_stage(int stage)
{
	TRACE("boot_splash_set_stage: stage=%d\n", stage);

	if (sInfo == NULL || stage < 0 || stage >= BOOT_SPLASH_STAGE_MAX)
		return;

	int iconsHalfHeight = kSplashIconsHeight / 2;
	int width = min_c(kSplashIconsWidth, sInfo->width);
	int height = min_c(kSplashLogoHeight + iconsHalfHeight, sInfo->height);
	int placementX = max_c(0, min_c(100, kSplashIconsPlacementX));
	int placementY = max_c(0, min_c(100, kSplashIconsPlacementY));

	int x = (sInfo->width - width) * placementX / 100;
	int y = kSplashLogoHeight + (sInfo->height - height) * placementY / 100;

	int stageLeftEdge = width * stage / BOOT_SPLASH_STAGE_MAX;
	int stageRightEdge = width * (stage + 1) / BOOT_SPLASH_STAGE_MAX;

	height = min_c(iconsHalfHeight, sInfo->height);
	blit_cropped(sUncompressedIcons, stageLeftEdge, 0, stageRightEdge,
		height, kSplashIconsWidth, x, y);
}

