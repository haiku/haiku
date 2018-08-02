/*
 * Copyright 2009, Haiku Inc.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef GENERIC_VIDEO_H
#define GENERIC_VIDEO_H


#include <SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* blit helpers */

/* platform code is responsible for setting the palette correctly */
void video_blit_image(addr_t frameBuffer, const uint8 *data,
	uint16 width, uint16 height, uint16 imageWidth,
	uint16 left, uint16 top);

/* platform code must implement 4bit on its own */
void platform_blit4(addr_t frameBuffer, const uint8 *data,
	uint16 width, uint16 height, uint16 imageWidth,
	uint16 left, uint16 top);
void platform_set_palette(const uint8 *palette);


/* Run Length Encoding splash decompression */

void uncompress_24bit_RLE(const uint8 compressed[], uint8 *uncompressed);
void uncompress_8bit_RLE(const uint8 compressed[], uint8 *uncompressed);

/* default splash display */
status_t video_display_splash(addr_t frameBuffer);

#ifdef __cplusplus
}
#endif

#endif	/* GENERIC_VIDEO_H */
