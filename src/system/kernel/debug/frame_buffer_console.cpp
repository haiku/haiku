/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <frame_buffer_console.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <KernelExport.h>
#include <kernel.h>
#include <lock.h>
#include <boot_item.h>
#include <vm/vm.h>
#include <fs/devfs.h>
#include <boot/kernel_args.h>

#ifndef _BOOT_MODE
#include <vesa_info.h>

#include <edid.h>
#endif

#include "font.h"


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
	FramebufferFont* font;
};

// Palette is (white and black are exchanged):
//	0 - white, 1 - blue, 2 - green, 3 - cyan, 4 - red, 5 - magenta, 6 - yellow,
//  7 - black
//  8-15 - same but bright (we're ignoring those)

static uint8 sPalette8[] = {
	63, 32, 52, 70, 42, 88, 67, 0,
};
static uint16 sPalette15[] = {
	// 0bbbbbgggggrrrrr (5-5-5)
	0x7fff, 0x1993, 0x2660, 0x0273, 0x6400, 0x390f, 0x6ea0, 0x0000,
};
static uint16 sPalette16[] = {
	// bbbbbggggggrrrrr (5-6-5)
	0xffff, 0x3333, 0x4cc0, 0x04d3, 0xc800, 0x722f, 0xdd40, 0x0000,
};
static uint32 sPalette32[] = {
	// is also used by 24 bit modes
	0xffffff,	// white
	0x336698,	// blue
	0x4e9a00,	// green
	0x06989a,	// cyan
	0xcc0000,	// red
	0x73447b,	// magenta
	0xdaa800,	// yellow
	0x000000,	// black
};

static struct console_info sConsole;

#ifndef _BOOT_MODE
static struct frame_buffer_boot_info sBootInfo;
static struct vesa_mode* sVesaModes;
#endif


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


static uint8*
get_palette_entry(uint8 index)
{
	switch (sConsole.depth) {
		case 8:
			return &sPalette8[index];
		case 15:
			return (uint8*)&sPalette15[index];
		case 16:
			return (uint8*)&sPalette16[index];
		default:
			return (uint8*)&sPalette32[index];
	}
}


static uint16
get_font_data(uint8 glyph, int y)
{
	if (sConsole.font->glyphWidth > 8) {
		uint16* data = (uint16*)sConsole.font->data;
		return data[sConsole.font->glyphHeight * glyph + y];
	} else
		return sConsole.font->data[sConsole.font->glyphHeight * glyph + y];
}


static void
render_glyph(int32 column, int32 row, uint8 glyph, uint8 attr)
{
	// we're ASCII only
	if (glyph > 127)
		glyph = 127;

	if (sConsole.depth >= 8) {
		uint8* base = (uint8*)(sConsole.frame_buffer
			+ sConsole.bytes_per_row * row * sConsole.font->glyphHeight
			+ column * sConsole.font->glyphWidth * sConsole.bytes_per_pixel);
		uint8* color = get_palette_entry(foreground_color(attr));
		uint8* backgroundColor = get_palette_entry(background_color(attr));

		set_ac();
		for (int y = 0; y < sConsole.font->glyphHeight; y++) {
			uint16_t bits = get_font_data(glyph, y);
			for (int x = 0; x < sConsole.font->glyphWidth; x++) {
				for (int32 i = 0; i < sConsole.bytes_per_pixel; i++) {
					if (bits & 1)
						base[x * sConsole.bytes_per_pixel + i] = color[i];
					else {
						base[x * sConsole.bytes_per_pixel + i]
							= backgroundColor[i];
					}
				}
				bits >>= 1;
			}

			base += sConsole.bytes_per_row;
		}
		clear_ac();

	} else {
		// VGA mode will be treated as monochrome
		// (ie. only the first plane will be used)

		uint8* base = (uint8*)(sConsole.frame_buffer
			+ sConsole.bytes_per_row * row * sConsole.font->glyphHeight
			+ column * sConsole.font->glyphWidth / 8);
		uint8 baseOffset =  (column * sConsole.font->glyphWidth) & 0x7;

		set_ac();
		for (int y = 0; y < sConsole.font->glyphHeight; y++) {
			uint16_t bits = get_font_data(glyph, y);
			uint8 offset = baseOffset;
			uint8 mask = 1 << (7 - baseOffset);

			for (int x = 0; x < sConsole.font->glyphWidth; x++) {
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
		clear_ac();
	}
}


static void
draw_cursor(int32 x, int32 y)
{
	if (x < 0 || y < 0)
		return;

	x *= sConsole.font->glyphWidth * sConsole.bytes_per_pixel;
	y *= sConsole.font->glyphHeight;
	int32 endX = x + sConsole.font->glyphWidth * sConsole.bytes_per_pixel;
	int32 endY = y + sConsole.font->glyphHeight;
	uint8* base = (uint8*)(sConsole.frame_buffer + y * sConsole.bytes_per_row);

	if (sConsole.depth < 8) {
		x /= 8;
		endY /= 8;
	}

	set_ac();
	for (; y < endY; y++) {
		for (int32 x2 = x; x2 < endX; x2++)
			base[x2] = ~base[x2];

		base += sConsole.bytes_per_row;
	}
	clear_ac();
}


static status_t
console_get_size(int32* _width, int32* _height)
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
console_fill_glyph(int32 x, int32 y, int32 width, int32 height, uint8 glyph,
	uint8 attr)
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
console_blit(int32 srcx, int32 srcy, int32 width, int32 height, int32 destx,
	int32 desty)
{
	if (!frame_buffer_console_available())
		return;

	height *= sConsole.font->glyphHeight;
	srcy *= sConsole.font->glyphHeight;
	desty *= sConsole.font->glyphHeight;

	if (sConsole.depth >= 8) {
		width *= sConsole.font->glyphWidth * sConsole.bytes_per_pixel;
		srcx *= sConsole.font->glyphWidth * sConsole.bytes_per_pixel;
		destx *= sConsole.font->glyphWidth * sConsole.bytes_per_pixel;
	} else {
		// monochrome mode
		width = width * sConsole.font->glyphWidth / 8;
		srcx = srcx * sConsole.font->glyphWidth / 8;
		destx = destx * sConsole.font->glyphWidth / 8;
	}

	set_ac();
	for (int32 y = 0; y < height; y++) {
		memmove((void*)(sConsole.frame_buffer + (desty + y)
				* sConsole.bytes_per_row + destx),
			(void*)(sConsole.frame_buffer + (srcy + y) * sConsole.bytes_per_row
				+ srcx), width);
	}
	clear_ac();
}


static void
console_clear(uint8 attr)
{
	if (!frame_buffer_console_available())
		return;

	set_ac();
	switch (sConsole.bytes_per_pixel) {
		case 1:
			if (sConsole.depth >= 8) {
				memset((void*)sConsole.frame_buffer,
					sPalette8[background_color(attr)],
					sConsole.height * sConsole.bytes_per_row);
			} else {
				// special case for VGA mode
				memset((void*)sConsole.frame_buffer, 0xff,
					sConsole.height * sConsole.bytes_per_row);
			}
			break;
		default:
		{
			uint8* base = (uint8*)sConsole.frame_buffer;
			uint8* color = get_palette_entry(background_color(attr));

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

	clear_ac();
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


//	#pragma mark -


bool
frame_buffer_console_available(void)
{
	return sConsole.frame_buffer != 0;
}


status_t
frame_buffer_update(addr_t baseAddress, int32 width, int32 height, int32 depth,
	int32 bytesPerRow)
{
	TRACE(("frame_buffer_update(buffer = %p, width = %ld, height = %ld, "
		"depth = %ld, bytesPerRow = %ld)\n", (void*)baseAddress, width, height,
		depth, bytesPerRow));

	mutex_lock(&sConsole.lock);

	if (width <= 1920 || height <= 1080) {
		sConsole.font = &smallFont;
	} else {
		sConsole.font = &spleen12Font;
	}

	sConsole.frame_buffer = baseAddress;
	sConsole.width = width;
	sConsole.height = height;
	sConsole.depth = depth;
	sConsole.bytes_per_pixel = (depth + 7) / 8;
	sConsole.bytes_per_row = bytesPerRow;
	sConsole.columns = sConsole.width / sConsole.font->glyphWidth;
	sConsole.rows = sConsole.height / sConsole.font->glyphHeight;
	// initially, the cursor is hidden
	sConsole.cursor_x = -1;
	sConsole.cursor_y = -1;

	TRACE(("framebuffer mapped at %p, %ld columns, %ld rows\n",
		(void*)sConsole.frame_buffer, sConsole.columns, sConsole.rows));

	mutex_unlock(&sConsole.lock);
	return B_OK;
}


#ifndef _BOOT_MODE
status_t
frame_buffer_console_init(kernel_args* args)
{
	mutex_init(&sConsole.lock, "console_lock");

	if (!args->frame_buffer.enabled)
		return B_OK;

	void* frameBuffer;
	sConsole.area = map_physical_memory("vesa frame buffer",
		args->frame_buffer.physical_buffer.start,
		args->frame_buffer.physical_buffer.size, B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_CLONEABLE_AREA,
		&frameBuffer);
	if (sConsole.area < 0)
		return sConsole.area;

	frame_buffer_update((addr_t)frameBuffer, args->frame_buffer.width,
		args->frame_buffer.height, args->frame_buffer.depth,
		args->frame_buffer.bytes_per_row);

	sBootInfo.area = sConsole.area;
	sBootInfo.physical_frame_buffer = args->frame_buffer.physical_buffer.start;
	sBootInfo.frame_buffer = (addr_t)frameBuffer;
	sBootInfo.width = args->frame_buffer.width;
	sBootInfo.height = args->frame_buffer.height;
	sBootInfo.depth = args->frame_buffer.depth;
	sBootInfo.bytes_per_row = args->frame_buffer.bytes_per_row;
	sBootInfo.vesa_capabilities = args->vesa_capabilities;

	add_boot_item(FRAME_BUFFER_BOOT_INFO, &sBootInfo,
		sizeof(frame_buffer_boot_info));

	sVesaModes = (vesa_mode*)malloc(args->vesa_modes_size);
	if (sVesaModes != NULL && args->vesa_modes_size > 0) {
		memcpy(sVesaModes, args->vesa_modes, args->vesa_modes_size);
		add_boot_item(VESA_MODES_BOOT_INFO, sVesaModes, args->vesa_modes_size);
	}

	if (args->edid_info != NULL) {
		edid1_info* info = (edid1_info*)malloc(sizeof(edid1_info));
		if (info != NULL) {
			memcpy(info, args->edid_info, sizeof(edid1_info));
			add_boot_item(VESA_EDID_BOOT_INFO, info, sizeof(edid1_info));
		}
	}

	return B_OK;
}


status_t
frame_buffer_console_init_post_modules(kernel_args* args)
{
	if (sConsole.frame_buffer == 0)
		return B_OK;

	// try to set frame buffer memory to write combined

	return vm_set_area_memory_type(sConsole.area,
		args->frame_buffer.physical_buffer.start, B_MTR_WC);
}


//	#pragma mark -


status_t
_user_frame_buffer_update(addr_t baseAddress, int32 width, int32 height,
	int32 depth, int32 bytesPerRow)
{
	debug_stop_screen_debug_output();

	if (geteuid() != 0)
		return B_NOT_ALLOWED;
	if (IS_USER_ADDRESS(baseAddress) && baseAddress != 0)
		return B_BAD_ADDRESS;

	return frame_buffer_update(baseAddress, width, height, depth, bytesPerRow);
}
#endif

