/*
 * Copyright 2005-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <KernelExport.h>
#include <Drivers.h>
#include <ISA.h>

#include <console.h>

#include <string.h>


#define SCREEN_START 0xb8000
#define SCREEN_END   0xc0000
#define LINES 25
#define COLUMNS 80

#define TEXT_INDEX 0x3d4
#define TEXT_DATA  0x3d5

#define TEXT_CURSOR_LO 0x0f
#define TEXT_CURSOR_HI 0x0e

static uint16 *sOrigin;
static isa_module_info *sISA;


static int
text_init(void)
{
	addr_t i;

	if (get_module(B_ISA_MODULE_NAME, (module_info **)&sISA) < 0) {
		dprintf("text module_init: no isa bus found..\n");
		return -1;
	}

	map_physical_memory("video_mem", SCREEN_START, SCREEN_END - SCREEN_START,
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void *)&sOrigin);
	dprintf("console/text: mapped vid mem to virtual address %p\n", sOrigin);

	/* pre-touch all of the memory so that we dont fault while deep inside the kernel and displaying something */
	for (i = (addr_t)sOrigin; i < (addr_t)sOrigin + (SCREEN_END - SCREEN_START);
			i += B_PAGE_SIZE) {
		uint16 val = *(volatile uint16 *)i;
		*(volatile uint16 *)i = val;
	}
	return 0;
}


static int
text_uninit(void)
{
	put_module(B_ISA_MODULE_NAME);

	// ToDo: unmap video memory (someday)
	return 0;
}


static status_t
get_size(int32 *width, int32 *height)
{
	*width = COLUMNS;
	*height = LINES;
	return 0;
}


static void
move_cursor(int32 x, int32 y)
{
	short int pos;

	if (x < 0 || y < 0)
		pos = LINES * COLUMNS + 1;
	else
		pos = y * COLUMNS + x;

	sISA->write_io_8(TEXT_INDEX, TEXT_CURSOR_LO);
	sISA->write_io_8(TEXT_DATA, (char)pos);
	sISA->write_io_8(TEXT_INDEX, TEXT_CURSOR_HI);
	sISA->write_io_8(TEXT_DATA, (char)(pos >> 8));
}


static void
put_glyph(int32 x, int32 y, uint8 glyph, uint8 attr)
{
	uint16 pair = ((uint16)attr << 8) | (uint16)glyph;
	uint16 *p = sOrigin + (y * COLUMNS) + x;
	*p = pair;
}


static void
fill_glyph(int32 x, int32 y, int32 width, int32 height, uint8 glyph, uint8 attr)
{
	uint16 pair = ((uint16)attr << 8) | (uint16)glyph;
	int32 y_limit = y + height;

	while (y < y_limit) {
		uint16 *p = sOrigin + (y * COLUMNS) + x;
		uint16 *p_limit = p + width;
		while (p < p_limit) *p++ = pair;
		y++;
	}
}


static void
blit(int32 srcx, int32 srcy, int32 width, int32 height, int32 destx, int32 desty)
{
	if ((srcx == 0) && (width == COLUMNS)) {
		// whole lines
		memmove(sOrigin + (desty * COLUMNS), sOrigin + (srcy * COLUMNS), height * COLUMNS * 2);
	} else {
		// FIXME
	}
}


static void
clear(uint8 attr)
{
	uint16 *base = (uint16 *)sOrigin;
	uint32 i;

	for (i = 0; i < COLUMNS * LINES; i++)
		base[i] = (attr << 8) | 0x20;
}


static status_t
text_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return text_init();
		case B_MODULE_UNINIT:
			return text_uninit();

		default:
			return B_ERROR;
	}
}


static console_module_info sVGATextConsole = {
	{
		"console/vga_text/v1",
		0,
		text_std_ops
	},
	get_size,
	move_cursor,
	put_glyph,
	fill_glyph,
	blit,
	clear,
};

module_info *modules[] = {
	(module_info *)&sVGATextConsole,
	NULL
};
