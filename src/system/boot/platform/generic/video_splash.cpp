/*
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2008, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2008, Philippe Saint-Pierre <stpere@gmail.com>
 * Distributed under the terms of the MIT License.
 */


#include <arch/cpu.h>
#include <boot/stage2.h>
#include <boot/platform.h>
#include <boot/menu.h>
#include <boot/kernel_args.h>
#include <boot/platform/generic/video.h>
#include <boot/images.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//#define TRACE_VIDEO
#ifdef TRACE_VIDEO
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


extern "C" status_t
video_display_splash(addr_t frameBuffer)
{
	if (!gKernelArgs.frame_buffer.enabled)
		return B_NO_INIT;

	// clear the video memory
	memset((void *)frameBuffer, 0,
		gKernelArgs.frame_buffer.physical_buffer.size);

	uint8 *uncompressedLogo = NULL;
	switch (gKernelArgs.frame_buffer.depth) {
		case 8:
			platform_set_palette(k8BitPalette);
			uncompressedLogo = (uint8 *)kernel_args_malloc(kSplashLogoWidth
				* kSplashLogoHeight);
			if (uncompressedLogo == NULL)
				return B_NO_MEMORY;
			uncompress_8bit_RLE(kSplashLogo8BitCompressedImage,
				uncompressedLogo);
		break;
		default:
			uncompressedLogo = (uint8 *)kernel_args_malloc(kSplashLogoWidth
				* kSplashLogoHeight * 3);
			if (uncompressedLogo == NULL)
				return B_NO_MEMORY;
			uncompress_24bit_RLE(kSplashLogo24BitCompressedImage,
				uncompressedLogo);
		break;
	}

	// TODO: support 4-bit indexed version of the images!

	// render splash logo
	uint16 iconsHalfHeight = kSplashIconsHeight / 2;

	int width = min_c(kSplashLogoWidth, gKernelArgs.frame_buffer.width);
	int height = min_c(kSplashLogoHeight + iconsHalfHeight,
		gKernelArgs.frame_buffer.height);
	int placementX = max_c(0, min_c(100, kSplashLogoPlacementX));
	int placementY = max_c(0, min_c(100, kSplashLogoPlacementY));

	int x = (gKernelArgs.frame_buffer.width - width) * placementX / 100;
	int y = (gKernelArgs.frame_buffer.height - height) * placementY / 100;

	height = min_c(kSplashLogoHeight, gKernelArgs.frame_buffer.height);
	switch (gKernelArgs.frame_buffer.depth) {
		case 8:
			break;
	}
	video_blit_image(frameBuffer, uncompressedLogo, width, height,
		kSplashLogoWidth, x, y);

	kernel_args_free(uncompressedLogo);

	const uint8* lowerHalfIconImage;

	switch (gKernelArgs.frame_buffer.depth) {
		case 8:
			// pointer into the lower half of the icons image data
			gKernelArgs.boot_splash
				= (uint8 *)kernel_args_malloc(kSplashIconsWidth
					* kSplashIconsHeight);
			if (gKernelArgs.boot_splash == NULL)
				return B_NO_MEMORY;
			uncompress_8bit_RLE(kSplashIcons8BitCompressedImage,
				gKernelArgs.boot_splash );
			lowerHalfIconImage = gKernelArgs.boot_splash
				+ (kSplashIconsWidth * iconsHalfHeight);
		break;
		default:
			// pointer into the lower half of the icons image data
			gKernelArgs.boot_splash
				= (uint8 *)kernel_args_malloc(kSplashIconsWidth
					* kSplashIconsHeight * 3);
			if (gKernelArgs.boot_splash == NULL)
				return B_NO_MEMORY;
			uncompress_24bit_RLE(kSplashIcons24BitCompressedImage,
				gKernelArgs.boot_splash );
			lowerHalfIconImage = gKernelArgs.boot_splash
				+ (kSplashIconsWidth * iconsHalfHeight) * 3;
		break;
	}

	// render initial (grayed out) icons
	// the grayed out version is the lower half of the icons image

	width = min_c(kSplashIconsWidth, gKernelArgs.frame_buffer.width);
	height = min_c(kSplashLogoHeight + iconsHalfHeight,
		gKernelArgs.frame_buffer.height);
	placementX = max_c(0, min_c(100, kSplashIconsPlacementX));
	placementY = max_c(0, min_c(100, kSplashIconsPlacementY));

	x = (gKernelArgs.frame_buffer.width - width) * placementX / 100;
	y = kSplashLogoHeight + (gKernelArgs.frame_buffer.height - height)
		* placementY / 100;

	height = min_c(iconsHalfHeight, gKernelArgs.frame_buffer.height);
	video_blit_image(frameBuffer, lowerHalfIconImage, width, height,
		kSplashIconsWidth, x, y);
	return B_OK;
}

