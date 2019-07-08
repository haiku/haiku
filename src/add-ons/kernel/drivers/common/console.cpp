/*
 * Copyright 2005-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <Drivers.h>
#include <KernelExport.h>

#include <console.h>
#include <lock.h>

#include <string.h>
#include <stdio.h>
#include <termios.h>


#define DEVICE_NAME "console"

#define TAB_SIZE 8
#define TAB_MASK 7

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
	SCREEN_ERASE_WHOLE,
	SCREEN_ERASE_UP,
	SCREEN_ERASE_DOWN
} erase_screen_mode;

typedef enum {
	LINE_ERASE_WHOLE,
	LINE_ERASE_LEFT,
	LINE_ERASE_RIGHT
} erase_line_mode;

#define MAX_ARGS 8

static struct console_desc {
	mutex	lock;

	int32	lines;
	int32	columns;

	uint8	attr;
	uint8	saved_attr;
	bool	bright_attr;
	bool	reverse_attr;

	int32	x;						/* current x coordinate */
	int32	y;						/* current y coordinate */
	int32	saved_x;				/* used to save x and y */
	int32	saved_y;

	int32	scroll_top;	/* top of the scroll region */
	int32	scroll_bottom;	/* bottom of the scroll region */

	/* state machine */
	console_state state;
	int32	arg_count;
	int32	args[MAX_ARGS];

	char	module_name[B_FILE_NAME_LENGTH];
	console_module_info *module;
} sConsole;

int32 api_version = B_CUR_DRIVER_API_VERSION;

static int32 sOpenMask;


static inline void
update_cursor(struct console_desc *console, int x, int y)
{
	console->module->move_cursor(x, y);
}


static void
gotoxy(struct console_desc *console, int newX, int newY)
{
	if (newX >= console->columns)
		newX = console->columns - 1;
	if (newX < 0)
		newX = 0;
	if (newY >= console->lines)
		newY = console->lines - 1;
	if (newY < 0)
		newY = 0;

	console->x = newX;
	console->y = newY;
}


static void
reset_console(struct console_desc *console)
{
	console->attr = 0x0f;
	console->scroll_top = 0;
	console->scroll_bottom = console->lines - 1;
	console->bright_attr = true;
	console->reverse_attr = false;
}


/*!	Scroll from the cursor line up to the top of the scroll region up one line.
*/
static void
scrup(struct console_desc *console)
{
	// see if cursor is outside of scroll region
	if (console->y < console->scroll_top || console->y > console->scroll_bottom)
		return;

	if (console->y - console->scroll_top > 1) {
		// move the screen up one
		console->module->blit(0, console->scroll_top + 1, console->columns,
			console->y - console->scroll_top, 0, console->scroll_top);
	}

	// clear the bottom line
	console->module->fill_glyph(0, console->y, console->columns, 1, ' ',
		console->attr);
}


/*!	Scroll from the cursor line down to the bottom of the scroll region down
	one line.
*/
static void
scrdown(struct console_desc *console)
{
	// see if cursor is outside of scroll region
	if (console->y < console->scroll_top || console->y > console->scroll_bottom)
		return;

	if (console->scroll_bottom - console->y > 1) {
		// move the screen down one
		console->module->blit(0, console->y, console->columns,
			console->scroll_bottom - console->y, 0, console->y + 1);
	}

	// clear the top line
	console->module->fill_glyph(0, console->y, console->columns, 1, ' ',
		console->attr);
}


static void
lf(struct console_desc *console)
{
	//dprintf("lf: y %d x %d scroll_top %d scoll_bottom %d\n", console->y, console->x, console->scroll_top, console->scroll_bottom);

	if (console->y == console->scroll_bottom ) {
 		// we hit the bottom of our scroll region
 		scrup(console);
	} else if(console->y < console->scroll_bottom) {
		console->y++;
	}
}


static void
rlf(struct console_desc *console)
{
	if (console->y == console->scroll_top) {
 		// we hit the top of our scroll region
 		scrdown(console);
	} else if (console->y > console->scroll_top) {
		console->y--;
	}
}


static void
cr(struct console_desc *console)
{
	console->x = 0;
}


static void
del(struct console_desc *console)
{
	if (console->x > 0) {
		console->x--;
	} else if (console->y > 0) {
        console->y--;
        console->x = console->columns - 1;
    } else {
        //This doesn't work...
        //scrdown(console);
        //console->y--;
        //console->x = console->columns - 1;
        return;
    }
	console->module->put_glyph(console->x, console->y, ' ', console->attr);
}


static void
erase_line(struct console_desc *console, erase_line_mode mode)
{
	switch (mode) {
		case LINE_ERASE_WHOLE:
			console->module->fill_glyph(0, console->y, console->columns, 1, ' ',
				console->attr);
			break;
		case LINE_ERASE_LEFT:
			console->module->fill_glyph(0, console->y, console->x + 1, 1, ' ',
				console->attr);
			break;
		case LINE_ERASE_RIGHT:
			console->module->fill_glyph(console->x, console->y,
				console->columns - console->x, 1, ' ', console->attr);
			break;
		default:
			return;
	}
}


static void
erase_screen(struct console_desc *console, erase_screen_mode mode)
{
	switch (mode) {
		case SCREEN_ERASE_WHOLE:
			console->module->clear(console->attr);
			break;
		case SCREEN_ERASE_UP:
			console->module->fill_glyph(0, 0, console->columns, console->y + 1,
				' ', console->attr);
			break;
		case SCREEN_ERASE_DOWN:
			console->module->fill_glyph(console->y, 0, console->columns,
				console->lines - console->y, ' ', console->attr);
			break;
		default:
			return;
	}
}


static void
save_cur(struct console_desc *console, bool saveAttrs)
{
	console->saved_x = console->x;
	console->saved_y = console->y;

	if (saveAttrs)
		console->saved_attr = console->attr;
}


static void
restore_cur(struct console_desc *console, bool restoreAttrs)
{
	console->x = console->saved_x;
	console->y = console->saved_y;

	if (restoreAttrs)
		console->attr = console->saved_attr;
}


static char
console_putch(struct console_desc *console, const char c)
{
	if (++console->x >= console->columns) {
		cr(console);
		lf(console);
	}
	console->module->put_glyph(console->x - 1, console->y, c, console->attr);
	return c;
}


static void
tab(struct console_desc *console)
{
	console->x = (console->x + TAB_SIZE) & ~TAB_MASK;
	if (console->x >= console->columns) {
		console->x -= console->columns;
		lf(console);
	}
}


static void
set_scroll_region(struct console_desc *console, int top, int bottom)
{
	if (top < 0)
		top = 0;
	if (bottom >= console->lines)
		bottom = console->lines - 1;
	if (top > bottom)
		return;

	console->scroll_top = top;
	console->scroll_bottom = bottom;
}


static void
set_vt100_attributes(struct console_desc *console, int32 *args, int32 argCount)
{
	if (argCount == 0) {
		// that's the default (attributes off)
		argCount++;
		args[0] = 0;
	}

	for (int32 i = 0; i < argCount; i++) {
		//dprintf("set_vt100_attributes: %ld\n", args[i]);
		switch (args[i]) {
			case 0: // reset
				console->attr = 0x0f;
				console->bright_attr = true;
				console->reverse_attr = false;
				break;
			case 1: // bright
				console->bright_attr = true;
				console->attr |= 0x08; // set the bright bit
				break;
			case 2: // dim
				console->bright_attr = false;
				console->attr &= ~0x08; // unset the bright bit
				break;
			case 4: // underscore we can't do
				break;
			case 5: // blink
				console->attr |= 0x80; // set the blink bit
				break;
			case 7: // reverse
				console->reverse_attr = true;
				console->attr = ((console->attr & BMASK) >> 4)
					| ((console->attr & FMASK) << 4);
				if (console->bright_attr)
					console->attr |= 0x08;
				break;
			case 8: // hidden?
				break;

			/* foreground colors */
			case 30: console->attr = (console->attr & ~FMASK) | 0 | (console->bright_attr ? 0x08 : 0); break; // black
			case 31: console->attr = (console->attr & ~FMASK) | 4 | (console->bright_attr ? 0x08 : 0); break; // red
			case 32: console->attr = (console->attr & ~FMASK) | 2 | (console->bright_attr ? 0x08 : 0); break; // green
			case 33: console->attr = (console->attr & ~FMASK) | 6 | (console->bright_attr ? 0x08 : 0); break; // yellow
			case 34: console->attr = (console->attr & ~FMASK) | 1 | (console->bright_attr ? 0x08 : 0); break; // blue
			case 35: console->attr = (console->attr & ~FMASK) | 5 | (console->bright_attr ? 0x08 : 0); break; // magenta
			case 36: console->attr = (console->attr & ~FMASK) | 3 | (console->bright_attr ? 0x08 : 0); break; // cyan
			case 37: console->attr = (console->attr & ~FMASK) | 7 | (console->bright_attr ? 0x08 : 0); break; // white

			/* background colors */
			case 40: console->attr = (console->attr & ~BMASK) | (0 << 4); break; // black
			case 41: console->attr = (console->attr & ~BMASK) | (4 << 4); break; // red
			case 42: console->attr = (console->attr & ~BMASK) | (2 << 4); break; // green
			case 43: console->attr = (console->attr & ~BMASK) | (6 << 4); break; // yellow
			case 44: console->attr = (console->attr & ~BMASK) | (1 << 4); break; // blue
			case 45: console->attr = (console->attr & ~BMASK) | (5 << 4); break; // magenta
			case 46: console->attr = (console->attr & ~BMASK) | (3 << 4); break; // cyan
			case 47: console->attr = (console->attr & ~BMASK) | (7 << 4); break; // white
		}
	}
}


static bool
process_vt100_command(struct console_desc *console, const char c,
	bool seenBracket, int32 *args, int32 argCount)
{
	bool ret = true;

	//dprintf("process_vt100_command: c '%c', argCount %ld, arg[0] %ld, arg[1] %ld, seenBracket %d\n",
	//	c, argCount, args[0], args[1], seenBracket);

	if (seenBracket) {
		switch(c) {
			case 'H': // set cursor position
			case 'f':
			{
				int32 row = argCount > 0 ? args[0] : 1;
				int32 col = argCount > 1 ? args[1] : 1;
				if (row > 0)
					row--;
				if (col > 0)
					col--;
				gotoxy(console, col, row);
				break;
			}
			case 'A':	// move up
			{
				int32 deltaY = argCount > 0 ? -args[0] : -1;
				if (deltaY == 0)
					deltaY = -1;
				gotoxy(console, console->x, console->y + deltaY);
				break;
			}
			case 'e':
			case 'B':	// move down
			{
				int32 deltaY = argCount > 0 ? args[0] : 1;
				if (deltaY == 0)
					deltaY = 1;
				gotoxy(console, console->x, console->y + deltaY);
				break;
			}
			case 'D':	// move left
			{
				int32 deltaX = argCount > 0 ? -args[0] : -1;
				if (deltaX == 0)
					deltaX = -1;
				gotoxy(console, console->x + deltaX, console->y);
				break;
			}
			case 'a':
			case 'C':	// move right
			{
				int32 deltaX = argCount > 0 ? args[0] : 1;
				if (deltaX == 0)
					deltaX = 1;
				gotoxy(console, console->x + deltaX, console->y);
				break;
			}
			case '`':
			case 'G':	// set X position
			{
				int32 newX = argCount > 0 ? args[0] : 1;
				if (newX > 0)
					newX--;
				gotoxy(console, newX, console->y);
				break;
			}
			case 'd':	// set y position
			{
				int32 newY = argCount > 0 ? args[0] : 1;
				if (newY > 0)
					newY--;
				gotoxy(console, console->x, newY);
				break;
			}
			case 's':	// save current cursor
				save_cur(console, false);
				break;
			case 'u':	// restore cursor
				restore_cur(console, false);
				break;
			case 'r':	// set scroll region
			{
				int32 low = argCount > 0 ? args[0] : 1;
				int32 high = argCount > 1 ? args[1] : console->lines;
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
			case 'K':
				if (argCount == 0 || args[0] == 0) {
					// erase to end of line
					erase_line(console, LINE_ERASE_RIGHT);
				} else if (argCount > 0) {
					if (args[0] == 1)
						erase_line(console, LINE_ERASE_LEFT);
					else if (args[0] == 2)
						erase_line(console, LINE_ERASE_WHOLE);
				}
				break;
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
			case 'm':
				if (argCount >= 0)
					set_vt100_attributes(console, args, argCount);
				break;
			default:
				ret = false;
		}
	} else {
		switch (c) {
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
			default:
				ret = false;
		}
	}

	return ret;
}


static ssize_t
_console_write(struct console_desc *console, const void *buffer, size_t length)
{
	const char *c;
	size_t pos = 0;

	while (pos < length) {
		c = &((const char *)buffer)[pos++];

		switch (console->state) {
			case CONSOLE_STATE_NORMAL:
				// just output the stuff
				switch (*c) {
					case '\n':
						lf(console);
						break;
					case '\r':
						cr(console);
						break;
					case 0x8: // backspace
						del(console);
						break;
					case '\t':
						tab(console);
						break;
					case '\a':
						// beep
						dprintf("<BEEP>\n");
						break;
					case '\0':
						break;
					case 0x1b:
						// escape character
						console->arg_count = 0;
						console->state = CONSOLE_STATE_GOT_ESCAPE;
						break;
					default:
						console_putch(console, *c);
				}
				break;
			case CONSOLE_STATE_GOT_ESCAPE:
				// Look for either commands with no argument, or the '['
				// character
				switch (*c) {
					case '[':
						console->state = CONSOLE_STATE_SEEN_BRACKET;
						break;
					default:
						console->args[0] = 0;
						process_vt100_command(console, *c, false, console->args,
							0);
						console->state = CONSOLE_STATE_NORMAL;
						break;
				}
				break;
			case CONSOLE_STATE_SEEN_BRACKET:
				switch (*c) {
					case '0'...'9':
						console->arg_count = 0;
						console->args[console->arg_count] = *c - '0';
						console->state = CONSOLE_STATE_PARSING_ARG;
						break;
					case '?':
						// Private DEC mode parameter follows - we ignore those
						// anyway
						// TODO: check if it was really used in combination with
						// a mode command
						break;
					default:
						process_vt100_command(console, *c, true, console->args,
							0);
						console->state = CONSOLE_STATE_NORMAL;
						break;
				}
				break;
			case CONSOLE_STATE_NEW_ARG:
				switch (*c) {
					case '0'...'9':
						console->arg_count++;
						if (console->arg_count == MAX_ARGS) {
							console->state = CONSOLE_STATE_NORMAL;
							break;
						}
						console->args[console->arg_count] = *c - '0';
						console->state = CONSOLE_STATE_PARSING_ARG;
						break;
					default:
						process_vt100_command(console, *c, true, console->args,
							console->arg_count + 1);
						console->state = CONSOLE_STATE_NORMAL;
						break;
				}
				break;
			case CONSOLE_STATE_PARSING_ARG:
				// parse args
				switch (*c) {
					case '0'...'9':
						console->args[console->arg_count] *= 10;
						console->args[console->arg_count] += *c - '0';
						break;
					case ';':
						console->state = CONSOLE_STATE_NEW_ARG;
						break;
					default:
						process_vt100_command(console, *c, true, console->args,
							console->arg_count + 1);
						console->state = CONSOLE_STATE_NORMAL;
						break;
				}
		}
	}

	return pos;
}


//	#pragma mark -


static status_t
console_open(const char *name, uint32 flags, void **cookie)
{
	if (atomic_or(&sOpenMask, 1) == 1)
		return B_BUSY;

	*cookie = &sConsole;

	status_t status = get_module(sConsole.module_name, (module_info **)&sConsole.module);
	if (status == B_OK)
		sConsole.module->clear(0x0f);

	return status;
}


static status_t
console_freecookie(void *cookie)
{
	if (sConsole.module != NULL) {
		put_module(sConsole.module_name);
		sConsole.module = NULL;
	}

	atomic_and(&sOpenMask, ~1);

	return B_OK;
}


static status_t
console_close(void *cookie)
{
//	dprintf("console_close: entry\n");

	return 0;
}


static status_t
console_read(void *cookie, off_t pos, void *buffer, size_t *_length)
{
	return B_NOT_ALLOWED;
}


static status_t
console_write(void *cookie, off_t pos, const void *buffer, size_t *_length)
{
	struct console_desc *console = (struct console_desc *)cookie;
	ssize_t written = 0;
	status_t status = B_OK;

#if 0
{
	const char *input = (const char *)buffer;
	dprintf("console_write (%lu bytes): \"", *_length);
	for (uint32 i = 0; i < *_length; i++) {
		if (input[i] < ' ')
			dprintf("(%d:0x%x)", input[i], input[i]);
		else
			dprintf("%c", input[i]);
	}
	dprintf("\"\n");
}
#endif

	mutex_lock(&console->lock);

	update_cursor(console, -1, -1); // hide it

	size_t bytesLeft = *_length;
	const char *str = (const char*)buffer;
	while (bytesLeft > 0) {
		char localBuffer[512];
		size_t chunkSize = min_c(sizeof(localBuffer), bytesLeft);
		if (user_memcpy(localBuffer, str, chunkSize) < B_OK) {
			status = B_BAD_ADDRESS;
			break;
		}
		written += _console_write(console, localBuffer, chunkSize);
		str += chunkSize;
		bytesLeft -= chunkSize;
	}
	update_cursor(console, console->x, console->y);

	mutex_unlock(&console->lock);

	if (status == B_OK)
		*_length = written;
	return status;
}


static status_t
console_ioctl(void *cookie, uint32 op, void *buffer, size_t length)
{
	struct console_desc *console = (struct console_desc *)cookie;

	if (op == TIOCGWINSZ) {
		struct winsize size;
		size.ws_xpixel = size.ws_col = console->columns;
		size.ws_ypixel = size.ws_row = console->lines;

		return user_memcpy(buffer, &size, sizeof(struct winsize));
	}

	return B_BAD_VALUE;
}


//	#pragma mark -


status_t
init_hardware(void)
{
	// iterate through the list of console modules until we find one that accepts the job
	void *cookie = open_module_list("console");
	if (cookie == NULL)
		return B_ERROR;

	bool found = false;

	char buffer[B_FILE_NAME_LENGTH];
	size_t bufferSize = sizeof(buffer);

	while (read_next_module_name(cookie, buffer, &bufferSize) == B_OK) {
		dprintf("con_init: trying module %s\n", buffer);
		if (get_module(buffer, (module_info **)&sConsole.module) == B_OK) {
			strlcpy(sConsole.module_name, buffer, sizeof(sConsole.module_name));
			put_module(buffer);
			found = true;
			break;
		}

		bufferSize = sizeof(buffer);
	}

	if (found) {
		// set up the console structure
		mutex_init(&sConsole.lock, "console lock");
		sConsole.module->get_size(&sConsole.columns, &sConsole.lines);

		reset_console(&sConsole);
		gotoxy(&sConsole, 0, 0);
		save_cur(&sConsole, true);
	}

	close_module_list(cookie);
	return found ? B_OK : B_ERROR;
}


const char **
publish_devices(void)
{
	static const char *devices[] = {
		DEVICE_NAME,
		NULL
	};

	return devices;
}


device_hooks *
find_device(const char *name)
{
	static device_hooks hooks = {
		&console_open,
		&console_close,
		&console_freecookie,
		&console_ioctl,
		&console_read,
		&console_write,
		/* Leave select/deselect/readv/writev undefined. The kernel will
		 * use its own default implementation. The basic hooks above this
		 * line MUST be defined, however. */
		NULL,
		NULL,
		NULL,
		NULL
	};

	if (!strcmp(name, DEVICE_NAME))
		return &hooks;

	return NULL;
}


status_t
init_driver(void)
{
	return B_OK;
}


void
uninit_driver(void)
{
}

