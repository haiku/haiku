/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "blue_screen.h"

#include <frame_buffer_console.h>
#include <console.h>
#include <arch/debug_console.h>

#include <string.h>
#include <stdio.h>


#define USE_SCROLLING 0
#define NO_CLEAR 1

struct screen_info {
	int32	columns;
	int32	rows;
	int32	x, y;
	uint8	attr;
} sScreen;

console_module_info *sModule;


static inline void
hide_cursor(void)
{
	sModule->move_cursor(-1, -1);
}


static inline void
update_cursor(int32 x, int32 y)
{
	sModule->move_cursor(x, y);
}


#if USE_SCROLLING

/** scroll from the cursor line up to the top of the scroll region up one line */

static void
scroll_up(void)
{
	// move the screen up one
	sModule->blit(0, 1, sScreen.columns, sScreen.rows - 1, 0, 0);

	// clear the bottom line
	sModule->fill_glyph(0, 0, sScreen.columns, 1, ' ', sScreen.attr);
}
#endif


static void
next_line(void)
{
	if (sScreen.y == sScreen.rows - 1) {
#if USE_SCROLLING
 		scroll_up();
#else
		sScreen.y = 0;
#endif
	} else if (sScreen.y < sScreen.rows - 1) {
		sScreen.y++;
	}

#if NO_CLEAR
	sModule->fill_glyph(0, (sScreen.y + 2) % sScreen.rows, sScreen.columns, 1, ' ', sScreen.attr);
#endif
	sScreen.x = 0;
}


static void
back_space(void)
{
	if (sScreen.x <= 0)
		return;

	sScreen.x--;
	sModule->put_glyph(sScreen.x, sScreen.y, ' ', sScreen.attr);
}


static void
put_character(char c)
{
	if (++sScreen.x >= sScreen.columns) {
		next_line();
		sScreen.x++;
	}

	sModule->put_glyph(sScreen.x - 1, sScreen.y, c, sScreen.attr);
}


static void
parse_character(char c)
{
	// just output the stuff
	switch (c) {
		case '\n':
			next_line();
			break;
		case 0x8:
			back_space();
			break;
		case '\t':
			put_character(' ');
			break;

		case '\r':
		case '\0':
			break;

		default:
			put_character(c);
	}
}


//	#pragma mark -


status_t
blue_screen_init(void)
{
	extern console_module_info gFrameBufferConsoleModule;

	// we can't use get_module() here, since it's too early in the boot process

	if (!frame_buffer_console_available())
		return B_ERROR;

	sModule = &gFrameBufferConsoleModule;
	return B_OK;
}


void
blue_screen_enter(void)
{
	sScreen.attr = 0x0f;	// black on white
	sScreen.x = sScreen.y = 0;

	sModule->get_size(&sScreen.columns, &sScreen.rows);
#if !NO_CLEAR
	sModule->clear(sScreen.attr);
#else
	sModule->fill_glyph(0, sScreen.y, sScreen.columns, 3, ' ', sScreen.attr);
#endif
}


char
blue_screen_getchar(void)
{
	return arch_debug_blue_screen_getchar();
}


void
blue_screen_putchar(char c)
{
	hide_cursor();

	parse_character(c);

	update_cursor(sScreen.x, sScreen.y);
}


void
blue_screen_puts(const char *text)
{
	hide_cursor();

	while (text[0] != '\0') {
		parse_character(text[0]);
		text++;
	}

	update_cursor(sScreen.x, sScreen.y);
}
