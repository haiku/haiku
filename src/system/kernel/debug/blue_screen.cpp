/*
 * Copyright 2005-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "blue_screen.h"

#include <KernelExport.h>
#include <frame_buffer_console.h>
#include <console.h>
#include <debug.h>
#include <arch/debug_console.h>
#include <safemode.h>

#include <string.h>
#include <stdio.h>

#include "debug_commands.h"


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
	int32	in_command_rows;
	bool	paging;
	bool	paging_timeout;
	bool	boot_debug_output;
	bool	ignore_output;

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

/*!	Scroll from the cursor line up to the top of the scroll region up one
	line.
*/
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
	bool abortCommand = false;

#if USE_SCROLLING
	// TODO: scrolling is usually too slow; we could probably just remove it
	if (sScreen.y == sScreen.rows - 1)
 		scroll_up();
 	else
 		sScreen.y++;
#else
	if (in_command_invocation())
		sScreen.in_command_rows++;
	else
		sScreen.in_command_rows = 0;

	if (sScreen.paging && ((sScreen.in_command_rows > 0
			&& ((sScreen.in_command_rows + 3) % sScreen.rows) == 0)
		|| (sScreen.boot_debug_output && sScreen.y == sScreen.rows - 1))) {
		if (sScreen.paging_timeout)
			spin(1000 * 1000 * 3);
		else {
			// Use the paging mechanism: either, we're in the debugger, and a
			// command is being executed, or we're currently showing boot debug
			// output
			const char *text = in_command_invocation()
				? "Press key to continue, Q to quit, S to skip output"
				: "Press key to continue, S to skip output, P to disable paging";
			int32 length = strlen(text);
			if (sScreen.x + length > sScreen.columns) {
				// make sure we don't overwrite too much
				text = "P";
				length = 1;
			}

			for (int32 i = 0; i < length; i++) {
				// yellow on black (or reverse, during boot)
				sModule->put_glyph(sScreen.columns - length + i, sScreen.y,
					text[i], sScreen.boot_debug_output ? 0x6f : 0xf6);
			}

			char c = kgetc();
			if (c == 's') {
				sScreen.ignore_output = true;
			} else if (c == 'q' && in_command_invocation()) {
				abortCommand = true;
				sScreen.ignore_output = true;
			} else if (c == 'p' && !in_command_invocation())
				sScreen.paging = false;
			else if (c == 't' && !in_command_invocation())
				sScreen.paging_timeout = true;

			// remove on screen text again
			sModule->fill_glyph(sScreen.columns - length, sScreen.y, length,
				1, ' ', sScreen.attr);
		}

		if (sScreen.in_command_rows > 0)
			sScreen.in_command_rows += 2;
	}
	if (sScreen.y == sScreen.rows - 1) {
		sScreen.y = 0;
		sModule->fill_glyph(0, 0, sScreen.columns, 2, ' ', sScreen.attr);
	} else
		sScreen.y++;
#endif

#if NO_CLEAR
	if (sScreen.y + 2 < sScreen.rows) {
		sModule->fill_glyph(0, (sScreen.y + 2) % sScreen.rows, sScreen.columns,
			1, ' ', sScreen.attr);
	}
#endif
	sScreen.x = 0;

	if (abortCommand) {
		abort_debugger_command();
			// should not return
	}
}


static void
erase_line(erase_line_mode mode)
{
	switch (mode) {
		case LINE_ERASE_WHOLE:
			sModule->fill_glyph(0, sScreen.y, sScreen.columns, 1, ' ',
				sScreen.attr);
			break;
		case LINE_ERASE_LEFT:
			sModule->fill_glyph(0, sScreen.y, sScreen.x + 1, 1, ' ',
				sScreen.attr);
			break;
		case LINE_ERASE_RIGHT:
			sModule->fill_glyph(sScreen.x, sScreen.y, sScreen.columns
				- sScreen.x, 1, ' ', sScreen.attr);
			break;
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
				sScreen.attr = sScreen.boot_debug_output ? 0xf0 : 0x0f;
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
				sScreen.attr = ((sScreen.attr & BMASK) >> 4)
					| ((sScreen.attr & FMASK) << 4);
				if (sScreen.bright_attr)
					sScreen.attr |= 0x08;
				break;
			case 8: // hidden?
				break;

			/* foreground colors */
			case 30: // black
			case 31: // red
			case 32: // green
			case 33: // yellow
			case 34: // blue
			case 35: // magenta
			case 36: // cyan
			case 37: // white
			{
				const uint8 colors[] = {0, 4, 2, 6, 1, 5, 3, 7};
				sScreen.attr = (sScreen.attr & ~FMASK) | colors[args[i] - 30]
					| (sScreen.bright_attr ? 0x08 : 0);
				break;
			}

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
process_vt100_command(const char c, bool seenBracket, int32 *args,
	int32 argCount)
{
	bool ret = true;

//	kprintf("process_vt100_command: c '%c', argCount %ld, arg[0] %ld, arg[1] %ld, seenBracket %d\n",
//		c, argCount, args[0], args[1], seenBracket);

	if (seenBracket) {
		switch (c) {
			case 'H':	// set cursor position
			case 'f':
			{
				int32 row = argCount > 0 ? args[0] : 1;
				int32 col = argCount > 1 ? args[1] : 1;
				if (row > 0)
					row--;
				if (col > 0)
					col--;
				move_cursor(col, row);
				break;
			}
			case 'A':	// move up
			{
				int32 deltaY = argCount > 0 ? -args[0] : -1;
				if (deltaY == 0)
					deltaY = -1;
				move_cursor(sScreen.x, sScreen.y + deltaY);
				break;
			}
			case 'e':
			case 'B':	// move down
			{
				int32 deltaY = argCount > 0 ? args[0] : 1;
				if (deltaY == 0)
					deltaY = 1;
				move_cursor(sScreen.x, sScreen.y + deltaY);
				break;
			}
			case 'D':	// move left
			{
				int32 deltaX = argCount > 0 ? -args[0] : -1;
				if (deltaX == 0)
					deltaX = -1;
				move_cursor(sScreen.x + deltaX, sScreen.y);
				break;
			}
			case 'a':
			case 'C':	// move right
			{
				int32 deltaX = argCount > 0 ? args[0] : 1;
				if (deltaX == 0)
					deltaX = 1;
				move_cursor(sScreen.x + deltaX, sScreen.y);
				break;
			}
			case '`':
			case 'G':	// set X position
			{
				int32 newX = argCount > 0 ? args[0] : 1;
				if (newX > 0)
					newX--;
				move_cursor(newX, sScreen.y);
				break;
			}
			case 'd':	// set y position
			{
				int32 newY = argCount > 0 ? args[0] : 1;
				if (newY > 0)
					newY--;
				move_cursor(sScreen.x, newY);
				break;
			}
#if 0
			case 's':	// save current cursor
				save_cur(console, false);
				break;
			case 'u':	// restore cursor
				restore_cur(console, false);
				break;
			case 'r':	// set scroll region
			{
				int32 low = argCount > 0 ? args[0] : 1;
				int32 high = argCount > 1 ? args[1] : sScreen.lines;
				if (low <= high)
					set_scroll_region(console, low - 1, high - 1);
				break;
			}
			case 'L':	// scroll virtual down at cursor
			{
				int32 lines = argCount > 0 ? args[0] : 1;
				while (lines > 0) {
					scrdown(console);
					lines--;
				}
				break;
			}
			case 'M':	// scroll virtual up at cursor
			{
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
					// TODO: real tab...
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
					sScreen.arg_count = 0;
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
					sScreen.args[0] = 0;
					process_vt100_command(c, false, sScreen.args, 0);
					sScreen.state = CONSOLE_STATE_NORMAL;
			}
			break;
		case CONSOLE_STATE_SEEN_BRACKET:
			switch (c) {
				case '0'...'9':
					sScreen.arg_count = 0;
					sScreen.args[0] = c - '0';
					sScreen.state = CONSOLE_STATE_PARSING_ARG;
					break;
				case '?':
					// private DEC mode parameter follows - we ignore those
					// anyway
					break;
				default:
					process_vt100_command(c, true, sScreen.args, 0);
					sScreen.state = CONSOLE_STATE_NORMAL;
					break;
			}
			break;
		case CONSOLE_STATE_NEW_ARG:
			switch (c) {
				case '0'...'9':
					if (++sScreen.arg_count == MAX_ARGS) {
						sScreen.state = CONSOLE_STATE_NORMAL;
						break;
					}
					sScreen.args[sScreen.arg_count] = c - '0';
					sScreen.state = CONSOLE_STATE_PARSING_ARG;
					break;
				default:
					process_vt100_command(c, true, sScreen.args,
						sScreen.arg_count + 1);
					sScreen.state = CONSOLE_STATE_NORMAL;
					break;
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
					process_vt100_command(c, true, sScreen.args,
						sScreen.arg_count + 1);
					sScreen.state = CONSOLE_STATE_NORMAL;
					break;
			}
	}
}


static int
set_paging(int argc, char **argv)
{
	if (argc > 1 && !strcmp(argv[1], "--help")) {
		kprintf("usage: %s [on|off]\n", argv[0]);
		return 0;
	}

	if (argc == 1)
		sScreen.paging = !sScreen.paging;
	else if (!strcmp(argv[1], "on"))
		sScreen.paging = true;
	else if (!strcmp(argv[1], "off"))
		sScreen.paging = false;
	else
		sScreen.paging = parse_expression(argv[1]) != 0;

	kprintf("paging is turned %s now.\n", sScreen.paging ? "on" : "off");
	return 0;
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
	sScreen.paging = !get_safemode_boolean(
		"disable_onscreen_paging", false);
	sScreen.paging_timeout = false;

	add_debugger_command("paging", set_paging, "Enable or disable paging");
	return B_OK;
}


status_t
blue_screen_enter(bool debugOutput)
{
	sScreen.attr = debugOutput ? 0xf0 : 0x0f;
		// black on white for KDL, white on black for debug output
	sScreen.boot_debug_output = debugOutput;
	sScreen.ignore_output = false;

	sScreen.x = sScreen.y = 0;
	sScreen.state = CONSOLE_STATE_NORMAL;

	if (sModule == NULL)
		return B_NO_INIT;

	sModule->get_size(&sScreen.columns, &sScreen.rows);
#if !NO_CLEAR
	sModule->clear(sScreen.attr);
#else
	sModule->fill_glyph(0, sScreen.y, sScreen.columns, 3, ' ', sScreen.attr);
#endif
	return B_OK;
}


bool
blue_screen_paging_enabled(void)
{
	return sScreen.paging;
}


void
blue_screen_set_paging(bool enabled)
{
	sScreen.paging = enabled;
}


void
blue_screen_clear_screen(void)
{
	sModule->clear(sScreen.attr);
	move_cursor(0, 0);
}


int
blue_screen_try_getchar(void)
{
	return arch_debug_blue_screen_try_getchar();
}


char
blue_screen_getchar(void)
{
	return arch_debug_blue_screen_getchar();
}


void
blue_screen_putchar(char c)
{
	if (sScreen.ignore_output
		&& (in_command_invocation() || sScreen.boot_debug_output))
		return;

	sScreen.ignore_output = false;
	hide_cursor();

	parse_character(c);

	update_cursor(sScreen.x, sScreen.y);
}


void
blue_screen_puts(const char *text)
{
	if (sScreen.ignore_output
		&& (in_command_invocation() || sScreen.boot_debug_output))
		return;

	sScreen.ignore_output = false;
	hide_cursor();

	while (text[0] != '\0') {
		parse_character(text[0]);
		text++;
	}

	update_cursor(sScreen.x, sScreen.y);
}
