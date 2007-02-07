/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>
#include <kernel.h>
#include <lock.h>
#include <boot_item.h>
#include <vm.h>
#include <fs/devfs.h>
#include <boot/kernel_args.h>

#include <frame_buffer_console.h>
#include "font.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>


//#define TRACE_FB_CONSOLE
#ifdef TRACE_FB_CONSOLE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


struct console_info {
	mutex	lock;
	area_id	area;

	addr_t	frame_buffer;
	int32	width;
	int32	height;
	int32	depth;
	int32	bytes_per_pixel;
	int32	bytes_per_row;

	int32	columns;
	int32	rows;
	int32	cursor_x;
	int32	cursor_y;
};

// Palette is (white and black are exchanged):
//	0 - white, 1 - blue, 2 - green, 3 - cyan, 4 - red, 5 - magenta, 6 - yellow, 7 - black
//  8-15 - same but bright (we're ignoring those)

static uint8 sPalette8[] = {
	63, 32, 52, 70, 42, 88, 67, 0,
};
static uint16 sPalette15[] = {
	// 0bbbbbgggggrrrrr (5-5-5)
	0x7fff, 0x001f, 0x03e0, 0x03ff, 0x7c00, 0x7c1f, 0x7fe0, 0x0000, 
};
static uint16 sPalette16[] = {
	// bbbbbggggggrrrrr (5-6-5)
	0xffff, 0x001f, 0x07e0, 0x07ff, 0xf800, 0xf81f, 0xffe0, 0x0000, 
};
static uint32 sPalette32[] = {
	// is also used by 24 bit modes
	0xffffff, 0x0000ff, 0x00ff00, 0x00ffff, 0xff0000, 0xff00ff, 0xffff00, 0x000000,
};

static struct console_info sConsole;
static struct frame_buffer_boot_info sBootInfo;


static inline uint8
foreground_color(uint8 attr)
{
	return attr & 0x7;
}


static inline uint8
background_color(uint8 attr)
{
	return (attr >> 4) & 0x7;
}


static uint8 *
get_palette_entry(uint8 index)
{
	switch (sConsole.depth) {
		case 8:
			return &sPalette8[index];
		case 15:
			return (uint8 *)&sPalette15[index];
		case 16:
			return (uint8 *)&sPalette16[index];
		default:
			return (uint8 *)&sPalette32[index];
	}
}


static void
render_glyph(int32 x, int32 y, uint8 glyph, uint8 attr)
{
	// we're ASCII only
	if (glyph > 127)
		glyph = 127;

	if (sConsole.depth >= 8) {
		uint8 *base = (uint8 *)(sConsole.frame_buffer + sConsole.bytes_per_row * y * CHAR_HEIGHT
			+ x * CHAR_WIDTH * sConsole.bytes_per_pixel);
		uint8 *color = get_palette_entry(foreground_color(attr));
		uint8 *backgroundColor = get_palette_entry(background_color(attr));

		for (y = 0; y < CHAR_HEIGHT; y++) {
			uint8 bits = FONT[CHAR_HEIGHT * glyph + y];
			for (x = 0; x < CHAR_WIDTH; x++) {
				for (int32 i = 0; i < sConsole.bytes_per_pixel; i++) {
					if (bits & 1) 
						base[x * sConsole.bytes_per_pixel + i] = color[i];
					else
						base[x * sConsole.bytes_per_pixel + i] = backgroundColor[i];
				}
				bits >>= 1;
			}

			base += sConsole.bytes_per_row;
		}
	} else {
		// monochrome mode

		uint8 *base = (uint8 *)(sConsole.frame_buffer + sConsole.bytes_per_row * y * CHAR_HEIGHT
			+ x * CHAR_WIDTH / 8);
		uint8 baseOffset =  (x * CHAR_WIDTH) & 0x7;

		for (y = 0; y < CHAR_HEIGHT; y++) {
			uint8 bits = FONT[CHAR_HEIGHT * glyph + y];
			uint8 offset = baseOffset;
			uint8 mask = 1 << (7 - baseOffset);

			for (x = 0; x < CHAR_WIDTH; x++) {
				if (mask == 0)
					mask = 128;

				// black on white
				if (bits & 1)
					base[offset / 8] &= ~mask;
				else
					base[offset / 8] |= mask;

				bits >>= 1;
				mask >>= 1;
				offset += 1;
			}

			base += sConsole.bytes_per_row;
		}
	}
}


static void
draw_cursor(int32 x, int32 y)
{
	if (x < 0 || y < 0)
		return;

	x *= CHAR_WIDTH * sConsole.bytes_per_pixel;
	y *= CHAR_HEIGHT;
	int32 endX = x + CHAR_WIDTH * sConsole.bytes_per_pixel;
	int32 endY = y + CHAR_HEIGHT;
	uint8 *base = (uint8 *)(sConsole.frame_buffer + y * sConsole.bytes_per_row);

	if (sConsole.depth < 8) {
		x /= 8;
		endY /= 8;
	}

	for (; y < endY; y++) {
		for (int32 x2 = x; x2 < endX; x2++)
			base[x2] = ~base[x2];

		base += sConsole.bytes_per_row;
	}
}


static status_t
console_get_size(int32 *_width, int32 *_height)
{
	*_width = sConsole.columns;
	*_height = sConsole.rows;

	return B_OK;
}


static void
console_move_cursor(int32 x, int32 y)
{
	if (!frame_buffer_console_available())
		return;

	draw_cursor(sConsole.cursor_x, sConsole.cursor_y);
	draw_cursor(x, y);
	
	sConsole.cursor_x = x;
	sConsole.cursor_y = y;
}


static void
console_put_glyph(int32 x, int32 y, uint8 glyph, uint8 attr)
{
	if (x >= sConsole.columns || y >= sConsole.rows
		|| !frame_buffer_console_available())
		return;

	render_glyph(x, y, glyph, attr);
}


static void
console_fill_glyph(int32 x, int32 y, int32 width, int32 height, uint8 glyph, uint8 attr)
{
	if (x >= sConsole.columns || y >= sConsole.rows
		|| !frame_buffer_console_available())
		return;

	int32 left = x + width;
	if (left > sConsole.columns)
		left = sConsole.columns;

	int32 bottom = y + height;
	if (bottom > sConsole.rows)
		bottom = sConsole.rows;

	for (; y < bottom; y++) {
		for (int32 x2 = x; x2 < left; x2++) {
			render_glyph(x2, y, glyph, attr);
		}
	}
}


static void
console_blit(int32 srcx, int32 srcy, int32 width, int32 height, int32 destx, int32 desty)
{
	if (!frame_buffer_console_available())
		return;

	height *= CHAR_HEIGHT;
	srcy *= CHAR_HEIGHT;
	desty *= CHAR_HEIGHT;

	if (sConsole.depth >= 8) {
		width *= CHAR_WIDTH * sConsole.bytes_per_pixel;
		srcx *= CHAR_WIDTH * sConsole.bytes_per_pixel;
		destx *= CHAR_WIDTH * sConsole.bytes_per_pixel;
	} else {
		// monochrome mode
		width = width * CHAR_WIDTH / 8;
		srcx = srcx * CHAR_WIDTH / 8;
		destx = destx * CHAR_WIDTH / 8;
	}

	for (int32 y = 0; y < height; y++) {
		memmove((void *)(sConsole.frame_buffer + (desty + y) * sConsole.bytes_per_row + destx),
			(void *)(sConsole.frame_buffer + (srcy + y) * sConsole.bytes_per_row + srcx), width);
	}
}


static void
console_clear(uint8 attr)
{
	if (!frame_buffer_console_available())
		return;

	switch (sConsole.bytes_per_pixel) {
		case 1:
			if (sConsole.depth >= 8) {
				memset((void *)sConsole.frame_buffer, sPalette8[background_color(attr)],
					sConsole.height * sConsole.bytes_per_row);
			} else {
				// special case for VGA mode
				memset((void *)sConsole.frame_buffer, 0xff,
					sConsole.height * sConsole.bytes_per_row);
			}
			break;
		default:
		{
			uint8 *base = (uint8 *)sConsole.frame_buffer;
			uint8 *color = get_palette_entry(background_color(attr));

			for (int32 y = 0; y < sConsole.height; y++) {
				for (int32 x = 0; x < sConsole.width; x++) {
					for (int32 i = 0; i < sConsole.bytes_per_pixel; i++) {
						base[x * sConsole.bytes_per_pixel + i] = color[i];
					}
				}
				base += sConsole.bytes_per_row;
			}
			break;
		}
	}

	sConsole.cursor_x = -1;
	sConsole.cursor_y = -1;
}


static status_t
console_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return frame_buffer_console_available() ? B_OK : B_ERROR;
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


console_module_info gFrameBufferConsoleModule = {
	{
		FRAME_BUFFER_CONSOLE_MODULE_NAME,
		0,
		console_std_ops
	},
	&console_get_size,
	&console_move_cursor,
	&console_put_glyph,
	&console_fill_glyph,
	&console_blit,
	&console_clear,
};


static status_t
frame_buffer_update(addr_t baseAddress, int32 width, int32 height, int32 depth, int32 bytesPerRow)
{
	TRACE(("frame_buffer_update(buffer = %p, width = %ld, height = %ld, depth = %ld, bytesPerRow = %ld)\n",
		(void *)baseAddress, width, height, depth, bytesPerRow));

	mutex_lock(&sConsole.lock);

	sConsole.frame_buffer = baseAddress;
	sConsole.width = width;
	sConsole.height = height;
	sConsole.depth = depth;
	sConsole.bytes_per_pixel = (depth + 7) / 8;
	sConsole.bytes_per_row = bytesPerRow;
	sConsole.columns = sConsole.width / CHAR_WIDTH;
	sConsole.rows = sConsole.height / CHAR_HEIGHT;

	// initially, the cursor is hidden
	sConsole.cursor_x = -1;
	sConsole.cursor_y = -1;

	TRACE(("framebuffer mapped at %p, %ld columns, %ld rows\n",
		(void *)sConsole.frame_buffer, sConsole.columns, sConsole.rows));

	mutex_unlock(&sConsole.lock);
	return B_OK;
}


//	#pragma mark -


bool
frame_buffer_console_available(void)
{
	return sConsole.frame_buffer != NULL;
}


status_t
frame_buffer_console_init(kernel_args *args)
{
	if (!args->frame_buffer.enabled)
		return B_OK;

	void *frameBuffer;
	sConsole.area = map_physical_memory("vesa_fb",
		(void *)args->frame_buffer.physical_buffer.start,
		args->frame_buffer.physical_buffer.size, B_ANY_KERNEL_ADDRESS,
		B_READ_AREA | B_WRITE_AREA | B_USER_CLONEABLE_AREA, &frameBuffer);
	if (sConsole.area < B_OK)
		return sConsole.area;

	int32 bytesPerRow = args->frame_buffer.width;
	switch (args->frame_buffer.depth) {
		case 1:
		case 4:
			// special VGA mode (will always be treated as monochrome)
			bytesPerRow /= 8;
			break;
		case 15:
		case 16:
			bytesPerRow *= 2;
			break;
		case 24:
			bytesPerRow *= 3;
			break;
		case 32:
			bytesPerRow *= 4;
			break;
	}
	frame_buffer_update((addr_t)frameBuffer, args->frame_buffer.width, args->frame_buffer.height,
		args->frame_buffer.depth, bytesPerRow);

	sBootInfo.frame_buffer = (addr_t)frameBuffer;
	sBootInfo.width = args->frame_buffer.width;
	sBootInfo.height = args->frame_buffer.height;
	sBootInfo.depth = args->frame_buffer.depth;
	sBootInfo.bytes_per_row = bytesPerRow;
	add_boot_item(FRAME_BUFFER_BOOT_INFO, &sBootInfo, sizeof(frame_buffer_boot_info));

	return B_OK;
}


status_t
frame_buffer_console_init_post_modules(kernel_args *args)
{
	mutex_init(&sConsole.lock, "console_lock");

	// TODO: enable MTRR in VESA mode!
//	if (sConsole.frame_buffer == NULL)
		return B_OK;

	// try to set frame buffer memory to write combined

//	return vm_set_area_memory_type(sConsole.area,
//		args->frame_buffer.physical_buffer.start, B_MTR_WC);
}


//	#pragma mark -


status_t
_user_frame_buffer_update(addr_t baseAddress, int32 width, int32 height,
	int32 depth, int32 bytesPerRow)
{
	debug_stop_screen_debug_output();

	if (geteuid() != 0)
		return B_NOT_ALLOWED;
	if (IS_USER_ADDRESS(baseAddress) && baseAddress != NULL)
		return B_BAD_ADDRESS;

	return frame_buffer_update(baseAddress, width, height, depth, bytesPerRow);
}

