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

#include <zlib.h>


//#define TRACE_VIDEO
#ifdef TRACE_VIDEO
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static status_t
uncompress(const uint8 compressed[], unsigned int compressedSize,
	uint8* uncompressed, unsigned int uncompressedSize)
{
	if (compressedSize == 0 || uncompressedSize == 0)
		return B_BAD_VALUE;

	// prepare zlib stream
	z_stream zStream = {
		(Bytef*)compressed,		// next_in
		compressedSize,			// avail_in
		0,						// total_in
		(Bytef*)uncompressed,	// next_out
		uncompressedSize,		// avail_out
		0,						// total_out
		0,						// msg
		0,						// state
		Z_NULL,					// zalloc (kernel_args_malloc?)
		Z_NULL,					// zfree (kernel_args_free?)
		Z_NULL,					// opaque
		0,						// data_type
		0,						// adler
		0						// reserved
	};

	int zlibError = inflateInit(&zStream);
	if (zlibError != Z_OK)
		return B_ERROR;	// TODO: translate zlibError

	// inflate
	status_t status = B_OK;
	zlibError = inflate(&zStream, Z_FINISH);
	if (zlibError != Z_STREAM_END) {
		if (zlibError == Z_OK)
			status = B_BUFFER_OVERFLOW;
		else
			status = B_ERROR;	// TODO: translate zlibError
	}

	// clean up
	zlibError = inflateEnd(&zStream);
	if (zlibError != Z_OK && status == B_OK)
		status = B_ERROR;	// TODO: translate zlibError

	if (status == B_OK && zStream.total_out != uncompressedSize)
		status = B_ERROR;

	return status;
}


extern "C" status_t
video_display_splash(addr_t frameBuffer)
{
	if (!gKernelArgs.frame_buffer.enabled)
		return B_NO_INIT;

	// clear the video memory
	memset((void*)frameBuffer, 0,
		gKernelArgs.frame_buffer.physical_buffer.size);

	uint8* uncompressedLogo = NULL;
	unsigned int uncompressedSize = kSplashLogoWidth * kSplashLogoHeight;
	switch (gKernelArgs.frame_buffer.depth) {
		case 8:
			platform_set_palette(k8BitPalette);
			uncompressedLogo = (uint8*)kernel_args_malloc(uncompressedSize);
			if (uncompressedLogo == NULL)
				return B_NO_MEMORY;

			uncompress(kSplashLogo8BitCompressedImage,
				sizeof(kSplashLogo8BitCompressedImage), uncompressedLogo,
				uncompressedSize);
		break;
		default: // 24 bits is assumed here
			uncompressedSize *= 3;
			uncompressedLogo = (uint8*)kernel_args_malloc(uncompressedSize);
			if (uncompressedLogo == NULL)
				return B_NO_MEMORY;

			uncompress(kSplashLogo24BitCompressedImage,
				sizeof(kSplashLogo24BitCompressedImage), uncompressedLogo,
				uncompressedSize);
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
	uncompressedSize = kSplashIconsWidth * kSplashIconsHeight;
	switch (gKernelArgs.frame_buffer.depth) {
		case 8:
			// pointer into the lower half of the icons image data
			gKernelArgs.boot_splash
				= (uint8*)kernel_args_malloc(uncompressedSize);
			if (gKernelArgs.boot_splash == NULL)
				return B_NO_MEMORY;
			uncompress(kSplashIcons8BitCompressedImage,
				sizeof(kSplashIcons8BitCompressedImage),
				gKernelArgs.boot_splash, uncompressedSize);
			lowerHalfIconImage = (uint8 *)gKernelArgs.boot_splash
				+ (kSplashIconsWidth * iconsHalfHeight);
		break;
		default:	// 24bits is assumed here
			uncompressedSize *= 3;
			// pointer into the lower half of the icons image data
			gKernelArgs.boot_splash
				= (uint8*)kernel_args_malloc(uncompressedSize);
			if (gKernelArgs.boot_splash == NULL)
				return B_NO_MEMORY;
			uncompress(kSplashIcons24BitCompressedImage,
				sizeof(kSplashIcons24BitCompressedImage),
				gKernelArgs.boot_splash, uncompressedSize);
			lowerHalfIconImage = (uint8 *)gKernelArgs.boot_splash
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



