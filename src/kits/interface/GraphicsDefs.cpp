/*
 * Copyright 2001-2015, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Caz <turok2@currantbun.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */

//!	Graphics functions and variables for the Interface Kit

#include <GraphicsDefs.h>

#include <AppServerLink.h>
#include <ServerProtocol.h>


// patterns
const pattern B_SOLID_HIGH = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
const pattern B_MIXED_COLORS = {{0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55}};
const pattern B_SOLID_LOW = {{0, 0, 0, 0, 0, 0, 0, 0}};

// colors
const rgb_color B_TRANSPARENT_COLOR = {0x77, 0x74, 0x77, 0x00};
const rgb_color B_TRANSPARENT_32_BIT = {0x77, 0x74, 0x77, 0x00};
const uint8 B_TRANSPARENT_8_BIT = 0xff;

const uint8 B_TRANSPARENT_MAGIC_CMAP8 = 0xff;
const uint16 B_TRANSPARENT_MAGIC_RGBA15 = 0x39ce;
const uint16 B_TRANSPARENT_MAGIC_RGBA15_BIG = 0xce39;
const uint32 B_TRANSPARENT_MAGIC_RGBA32 = 0x00777477;
const uint32 B_TRANSPARENT_MAGIC_RGBA32_BIG = 0x77747700;

// misc.
const struct screen_id B_MAIN_SCREEN_ID = {0};


// rgb_color
int32
rgb_color::Brightness() const
{
	return ((int32)red * 41 + (int32)green * 187 + (int32)blue * 28) >> 8;
}


// Mix two colors without respect for their alpha values
rgb_color
mix_color(rgb_color color1, rgb_color color2, uint8 amount)
{
	color1.red = (uint8)(((int16(color2.red) - int16(color1.red)) * amount)
		/ 255 + color1.red);
	color1.green = (uint8)(((int16(color2.green) - int16(color1.green))
		* amount) / 255 + color1.green);
	color1.blue = (uint8)(((int16(color2.blue) - int16(color1.blue)) * amount)
		/ 255 + color1.blue );
	color1.alpha = (uint8)(((int16(color2.alpha) - int16(color1.alpha))
		* amount) / 255 + color1.alpha );

	return color1;
}


// Mix two colors, respecting their alpha values.
rgb_color
blend_color(rgb_color color1, rgb_color color2, uint8 amount)
{
	const uint8 alphaMix = (uint8)(((int16(color2.alpha) - int16(255
		- color1.alpha)) * amount) / 255 + (255 - color1.alpha));

	color1.red = (uint8)(((int16(color2.red) - int16(color1.red)) * alphaMix)
		/ 255 + color1.red );
	color1.green = (uint8)(((int16(color2.green) - int16(color1.green))
		* alphaMix) / 255 + color1.green);
	color1.blue = (uint8)(((int16(color2.blue) - int16(color1.blue))
		* alphaMix) / 255 + color1.blue);
	color1.alpha = (uint8)(((int16(color2.alpha) - int16(color1.alpha))
		* amount) / 255 + color1.alpha);

	return color1;
}


rgb_color
disable_color(rgb_color color, rgb_color background)
{
	return mix_color(color, background, 185);
}


status_t
get_pixel_size_for(color_space space, size_t *pixelChunk, size_t *rowAlignment,
	size_t *pixelsPerChunk)
{
	status_t status = B_OK;
	int32 bytesPerPixel = 0;
	int32 pixPerChunk = 0;
	switch (space) {
		// supported
		case B_RGBA64: case B_RGBA64_BIG:
			bytesPerPixel = 8;
			pixPerChunk = 2;
			break;
		case B_RGB48: case B_RGB48_BIG:
			bytesPerPixel = 6;
			pixPerChunk = 2;
			break;
		case B_RGB32: case B_RGBA32:
		case B_RGB32_BIG: case B_RGBA32_BIG:
		case B_UVL32: case B_UVLA32:
		case B_LAB32: case B_LABA32:
		case B_HSI32: case B_HSIA32:
		case B_HSV32: case B_HSVA32:
		case B_HLS32: case B_HLSA32:
		case B_CMY32: case B_CMYA32: case B_CMYK32:
			bytesPerPixel = 4;
			pixPerChunk = 1;
			break;
		case B_RGB24: case B_RGB24_BIG:
		case B_UVL24: case B_LAB24: case B_HSI24:
		case B_HSV24: case B_HLS24: case B_CMY24:
			bytesPerPixel = 3;
			pixPerChunk = 1;
			break;
		case B_RGB16:		case B_RGB15:		case B_RGBA15:
		case B_RGB16_BIG:	case B_RGB15_BIG:	case B_RGBA15_BIG:
			bytesPerPixel = 2;
			pixPerChunk = 1;
			break;
		case B_CMAP8: case B_GRAY8:
			bytesPerPixel = 1;
			pixPerChunk = 1;
			break;
		case B_GRAY1:
			bytesPerPixel = 1;
			pixPerChunk = 8;
			break;
		case B_YCbCr422: case B_YUV422:
			bytesPerPixel = 4;
			pixPerChunk = 2;
			break;
		case B_YCbCr411: case B_YUV411:
			bytesPerPixel = 12;
			pixPerChunk = 8;
			break;
		case B_YCbCr444: case B_YUV444:
			bytesPerPixel = 3;
			pixPerChunk = 1;
			break;
		// TODO: I don't know if it's correct,
		// but beos reports B_YUV420 to be
		// 6 bytes per pixel and 4 pixel per chunk	
		case B_YCbCr420: case B_YUV420:
			bytesPerPixel = 3;
			pixPerChunk = 2;
			break;
		case B_YUV9:
			bytesPerPixel = 5;
			pixPerChunk = 4;
			break;
		case B_YUV12:
			bytesPerPixel = 6;
			pixPerChunk = 4;
			break;
		// unsupported
		case B_NO_COLOR_SPACE:
		default:
			status = B_BAD_VALUE;
			break;
	}
		
	if (pixelChunk != NULL)
		*pixelChunk = bytesPerPixel;
	
	size_t alignment = 0;
	if (bytesPerPixel != 0) {
		alignment = (sizeof(int) % bytesPerPixel) * sizeof(int);
		if (alignment < sizeof(int))
			alignment = sizeof(int);
	}	
	
	if (rowAlignment!= NULL)
		*rowAlignment = alignment;
		
	if (pixelsPerChunk!= NULL)
		*pixelsPerChunk = pixPerChunk;
		
	return status;
}


static uint32
get_overlay_flags(color_space space)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_BITMAP_SUPPORT_FLAGS);
	link.Attach<uint32>((uint32)space);

	uint32 flags = 0;
	int32 code;
	if (link.FlushWithReply(code) == B_OK && code == B_OK) {
		if (link.Read<uint32>(&flags) < B_OK)
			flags = 0;
	}
	return flags;
}


bool
bitmaps_support_space(color_space space, uint32 *supportFlags)
{
	bool result = true;
	switch (space) {
		// supported, also for drawing and for attaching BViews
		case B_RGB32:		case B_RGBA32:		case B_RGB24:
		case B_RGB32_BIG:	case B_RGBA32_BIG:	case B_RGB24_BIG:
		case B_RGB16:		case B_RGB15:		case B_RGBA15:
		case B_RGB16_BIG:	case B_RGB15_BIG:	case B_RGBA15_BIG:
		case B_CMAP8:		case B_GRAY8:		case B_GRAY1:
			if (supportFlags != NULL) {
				*supportFlags = B_VIEWS_SUPPORT_DRAW_BITMAP
					| B_BITMAPS_SUPPORT_ATTACHED_VIEWS
					| get_overlay_flags(space);
			}
			break;

		// supported, but cannot draw
		case B_RGBA64: case B_RGBA64_BIG:
		case B_RGB48: case B_RGB48_BIG:
		case B_YCbCr422: case B_YCbCr411: case B_YCbCr444: case B_YCbCr420:
		case B_YUV422: case B_YUV411: case B_YUV444: case B_YUV420:
		case B_UVL24: case B_UVL32: case B_UVLA32:
		case B_LAB24: case B_LAB32: case B_LABA32:
		case B_HSI24: case B_HSI32: case B_HSIA32:
		case B_HSV24: case B_HSV32: case B_HSVA32:
		case B_HLS24: case B_HLS32: case B_HLSA32:
		case B_CMY24: case B_CMY32: case B_CMYA32: case B_CMYK32:
			if (supportFlags != NULL)
				*supportFlags = get_overlay_flags(space);
			break;
		// unsupported
		case B_NO_COLOR_SPACE:
		case B_YUV9: case B_YUV12:
			result = false;
			break;
	}
	return result;
}
