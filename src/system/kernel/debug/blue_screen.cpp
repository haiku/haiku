/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "blue_screen.h"

#include <KernelExport.h>
#include <frame_buffer_console.h>
#include <console.h>
#include <arch/debug_console.h>

#include <string.h>
#include <stdio.h>


#define USE_SCROLLING 0
#define NO_CLEAR 1

#define MAX_ARGS 8

#define FMASK 0x0f
#define BMASK 0x70

typedef enum {
	CONSOLE_STATE_NORMAL = 0,
	CONSOLE_STATE_GOT_ESCAPE,
	CONSOLE_STATE_SEEN_BRACKET,
	CONSOLE_STATE_NEW_ARG,
	CONSOLE_STATE_PARSING_ARG,
} console_state;

typedef enum {
	LINE_ERASE_WHOLE,
	LINE_ERASE_LEFT,
	LINE_ERASE_RIGHT
} erase_line_mode;

struct screen_info {
	int32	columns;
	int32	rows;
	int32	x, y;
	uint8	attr;
	bool	bright_attr;
	bool	reverse_attr;

	// state machine
	console_state state;
	int32	arg_count;
	int32	args[MAX_ARGS];
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


static inline void
move_cursor(int32 x, int32 y)
{
	sScreen.x = x;
	sScreen.y = y;
	update_cursor(x, y);
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
erase_line(erase_line_mode mode)
{
	switch (mode) {
		case LINE_ERASE_WHOLE:
			sModule->fill_glyph(0, sScreen.y, sScreen.columns, 1, ' ', sScreen.attr);
			break;
		case LINE_ERASE_LEFT:
			sModule->fill_glyph(0, sScreen.y, sScreen.x + 1, 1, ' ', sScreen.attr);
			break;
		case LINE_ERASE_RIGHT:
			sModule->fill_glyph(sScreen.x, sScreen.y, sScreen.columns - sScreen.x,
				1, ' ', sScreen.attr);
			break;
//		default:
	}
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
set_vt100_attributes(int32 *args, int32 argCount)
{
	if (argCount == 0) {
		// that's the default (attributes off)
		argCount++;
		args[0] = 0;
	}

	for (int32 i = 0; i < argCount; i++) {
		switch (args[i]) {
			case 0: // reset
				sScreen.attr = 0x0f;
				sScreen.bright_attr = true;
				sScreen.reverse_attr = false;
				break;
			case 1: // bright
				sScreen.bright_attr = true;
				sScreen.attr |= 0x08; // set the bright bit
				break;
			case 2: // dim
				sScreen.bright_attr = false;
				sScreen.attr &= ~0x08; // unset the bright bit
				break;
			case 4: // underscore we can't do
				break;
			case 5: // blink
				sScreen.attr |= 0x80; // set the blink bit
				break;
			case 7: // reverse
				sScreen.reverse_attr = true;
				sScreen.attr = ((sScreen.attr & BMASK) >> 4) | ((sScreen.attr & FMASK) << 4);
				if (sScreen.bright_attr)
					sScreen.attr |= 0x08;
				break;
			case 8: // hidden?
				break;

			/* foreground colors */
			case 30: sScreen.attr = (sScreen.attr & ~FMASK) | 0 | (sScreen.bright_attr ? 0x08 : 0); break; // black
			case 31: sScreen.attr = (sScreen.attr & ~FMASK) | 4 | (sScreen.bright_attr ? 0x08 : 0); break; // red
			case 32: sScreen.attr = (sScreen.attr & ~FMASK) | 2 | (sScreen.bright_attr ? 0x08 : 0); break; // green
			case 33: sScreen.attr = (sScreen.attr & ~FMASK) | 6 | (sScreen.bright_attr ? 0x08 : 0); break; // yellow
			case 34: sScreen.attr = (sScreen.attr & ~FMASK) | 1 | (sScreen.bright_attr ? 0x08 : 0); break; // blue
			case 35: sScreen.attr = (sScreen.attr & ~FMASK) | 5 | (sScreen.bright_attr ? 0x08 : 0); break; // magenta
			case 36: sScreen.attr = (sScreen.attr & ~FMASK) | 3 | (sScreen.bright_attr ? 0x08 : 0); break; // cyan
			case 37: sScreen.attr = (sScreen.attr & ~FMASK) | 7 | (sScreen.bright_attr ? 0x08 : 0); break; // white

			/* background colors */
			case 40: sScreen.attr = (sScreen.attr & ~BMASK) | (0 << 4); break; // black
			case 41: sScreen.attr = (sScreen.attr & ~BMASK) | (4 << 4); break; // red
			case 42: sScreen.attr = (sScreen.attr & ~BMASK) | (2 << 4); break; // green
			case 43: sScreen.attr = (sScreen.attr & ~BMASK) | (6 << 4); break; // yellow
			case 44: sScreen.attr = (sScreen.attr & ~BMASK) | (1 << 4); break; // blue
			case 45: sScreen.attr = (sScreen.attr & ~BMASK) | (5 << 4); break; // magenta
			case 46: sScreen.attr = (sScreen.attr & ~BMASK) | (3 << 4); break; // cyan
			case 47: sScreen.attr = (sScreen.attr & ~BMASK) | (7 << 4); break; // white
		}
	}
}


static bool
process_vt100_command(const char c, bool seenBracket, int32 *args, int32 argCount)
{
	bool ret = true;

//	kprintf("process_vt100_command: c '%c', argCount %ld, arg[0] %ld, arg[1] %ld, seenBracket %d\n",
//		c, argCount, args[0], args[1], seenBracket);

	if (seenBracket) {
		switch (c) {
			case 'H': /* set cursor position */
			case 'f': {
				int32 row = argCount > 0 ? args[0] : 1;
				int32 col = argCount > 1 ? args[1] : 1;
				if (row > 0)
					row--;
				if (col > 0)
					col--;
				move_cursor(col, row);
				break;
			}
			case 'A': { /* move up */
				int32 deltay = argCount > 0 ? -args[0] : -1;
				if (deltay == 0)
					deltay = -1;
				move_cursor(sScreen.x, sScreen.y + deltay);
				break;
			}
			case 'e':
			case 'B': { /* move down */
				int32 deltay = argCount > 0 ? args[0] : 1;
				if (deltay == 0)
					deltay = 1;
				move_cursor(sScreen.x, sScreen.y + deltay);
				break;
			}
			case 'D': { /* move left */
				int32 deltax = argCount > 0 ? -args[0] : -1;
				if (deltax == 0)
					deltax = -1;
				move_cursor(sScreen.x + deltax, sScreen.y);
				break;
			}
			case 'a':
			case 'C': { /* move right */
				int32 deltax = argCount > 0 ? args[0] : 1;
				if (deltax == 0)
					deltax = 1;
				move_cursor(sScreen.x + deltax, sScreen.y);
				break;
			}
			case '`':
			case 'G': { /* set X position */
				int32 newx = argCount > 0 ? args[0] : 1;
				if (newx > 0)
					newx--;
				move_cursor(newx, sScreen.y);
				break;
			}
			case 'd': { /* set y position */
				int32 newy = argCount > 0 ? args[0] : 1;
				if (newy > 0)
					newy--;
				move_cursor(sScreen.x, newy);
				break;
			}
#if 0
			case 's': /* save current cursor */
				save_cur(console, false);
				break;
			case 'u': /* restore cursor */
				restore_cur(console, false);
				break;
			case 'r': { /* set scroll region */
				int32 low = argCount > 0 ? args[0] : 1;
				int32 high = argCount > 1 ? args[1] : sScreen.lines;
				if (low <= high)
					set_scroll_region(console, low - 1, high - 1);
				break;
			}
			case 'L': { /* scroll virtual down at cursor */
				int32 lines = argCount > 0 ? args[0] : 1;
				while (lines > 0) {
					scrdown(console);
					lines--;
				}
				break;
			}
			case 'M': { /* scroll virtual up at cursor */
				int32 lines = argCount > 0 ? args[0] : 1;
				while (lines > 0) {
					scrup(console);
					lines--;
				}
				break;
			}
#endif
			case 'K':
				if (argCount == 0 || args[0] == 0) {
					// erase to end of line
					erase_line(LINE_ERASE_RIGHT);
				} else if (argCount > 0) {
					if (args[0] == 1)
						erase_line(LINE_ERASE_LEFT);
					else if (args[0] == 2)
						erase_line(LINE_ERASE_WHOLE);
				}
				break;
#if 0
			case 'J':
				if (argCount == 0 || args[0] == 0) {
					// erase to end of screen
					erase_screen(console, SCREEN_ERASE_DOWN);
				} else {
					if (args[0] == 1)
						erase_screen(console, SCREEN_ERASE_UP);
					else if (args[0] == 2)
						erase_screen(console, SCREEN_ERASE_WHOLE);
				}
				break;
#endif
			case 'm':
				if (argCount >= 0)
					set_vt100_attributes(args, argCount);
				break;
			default:
				ret = false;
		}
	} else {
		switch (c) {
#if 0
			case 'c':
				reset_console(console);
				break;
			case 'D':
				rlf(console);
				break;
			case 'M':
				lf(console);
				break;
			case '7':
				save_cur(console, true);
				break;
			case '8':
				restore_cur(console, true);
				break;
#endif
			default:
				ret = false;
		}
	}

	return ret;
}


static void
parse_character(char c)
{
	switch (sScreen.state) {
		case CONSOLE_STATE_NORMAL:
			// just output the stuff
			switch (c) {
				case '\n':
					next_line();
					break;
				case 0x8:
					back_space();
					break;
				case '\t':
					// ToDo: real tab...
					sScreen.x = (sScreen.x + 8) & ~7;
					if (sScreen.x >= sScreen.columns)
						next_line();
					break;

				case '\r':
				case '\0':
				case '\a': // beep
					break;

				case 0x1b:
					// escape character
					sScreen.arg_count = -1;
					sScreen.state = CONSOLE_STATE_GOT_ESCAPE;
					break;
				default:
					put_character(c);
			}
			break;
		case CONSOLE_STATE_GOT_ESCAPE:
			// look for either commands with no argument, or the '[' character
			switch (c) {
				case '[':
					sScreen.state = CONSOLE_STATE_SEEN_BRACKET;
					break;
				default:
					sScreen.args[sScreen.arg_count] = 0;
					process_vt100_command(c, false, sScreen.args, sScreen.arg_count + 1);
					sScreen.state = CONSOLE_STATE_NORMAL;
			}
			break;
		case CONSOLE_STATE_SEEN_BRACKET:
			switch (c) {
				case '0'...'9':
					sScreen.arg_count = 0;
					sScreen.args[sScreen.arg_count] = c - '0';
					sScreen.state = CONSOLE_STATE_PARSING_ARG;
					break;
				case '?':
					// private DEC mode parameter follows - we ignore those anyway
					break;
				default:
					process_vt100_command(c, true, sScreen.args, sScreen.arg_count + 1);
					sScreen.state = CONSOLE_STATE_NORMAL;
			}
			break;
		case CONSOLE_STATE_NEW_ARG:
			switch (c) {
				case '0'...'9':
					sScreen.arg_count++;
					if (sScreen.arg_count == MAX_ARGS) {
						sScreen.state = CONSOLE_STATE_NORMAL;
						break;
					}
					sScreen.args[sScreen.arg_count] = c - '0';
					sScreen.state = CONSOLE_STATE_PARSING_ARG;
					break;
				default:
					process_vt100_command(c, true, sScreen.args, sScreen.arg_count + 1);
					sScreen.state = CONSOLE_STATE_NORMAL;
			}
			break;
		case CONSOLE_STATE_PARSING_ARG:
			// parse args
			switch (c) {
				case '0'...'9':
					sScreen.args[sScreen.arg_count] *= 10;
					sScreen.args[sScreen.arg_count] += c - '0';
					break;
				case ';':
					sScreen.state = CONSOLE_STATE_NEW_ARG;
					break;
				default:
					process_vt100_command(c, true, sScreen.args, sScreen.arg_count + 1);
					sScreen.state = CONSOLE_STATE_NORMAL;
			}
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
blue_screen_enter(bool debugOutput)
{
	sScreen.attr = debugOutput ? 0xf0 : 0x0f;
		// black on white for KDL, white on black for debug output
	sScreen.x = sScreen.y = 0;
	sScreen.state = CONSOLE_STATE_NORMAL;

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
