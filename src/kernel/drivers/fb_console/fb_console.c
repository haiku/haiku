/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <KernelExport.h>
#include <lock.h>
#include <devfs.h>

#include <boot/kernel_args.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "font.h"

//#define TRACE_FB_CONSOLE
#ifdef TRACE_FB_CONSOLE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

status_t fb_console_dev_init(kernel_args *args);

#define WRAP(x, limit) ((x) % (limit))
	// This version makes the sh4 compiler throw up
	// This would work: (((x) >= (limit)) ? ((x) - (limit)) : (x))
#define INC_WITH_WRAP(x, limit) WRAP((x) + 1, (limit))

enum {
	CONSOLE_OP_WRITEXY = 2376
};

#define TAB_SIZE 8
#define TAB_MASK 7

struct console_desc {
	// describe the framebuffer
	addr_t fb;
	addr_t fb_size;
	int fb_x;
	int fb_y;
	int fb_pixel_bytes;

	// describe the setup we have
	int rows;
	int columns;

	// describe the current state
	int x;
	int y;
	int color;
	int bcolor;
	int saved_x;
	int saved_y;

	// text buffers
	char *buf;
	int first_line;
	int num_lines;
	char **lines;
	uint8 *dirty_lines;
	void *render_buf;

	// renderer function
	void (*render_line)(char *line, int line_num);

	mutex lock;
	int keyboard_fd;
};

static struct console_desc console;


static void
render_line8(char *line, int line_num)
{
	int x;
	int y;
	int i;
	uint8 *fb_spot = (uint8 *)(console.fb + line_num * CHAR_HEIGHT * console.fb_x);

	for (y = 0; y < CHAR_HEIGHT; y++) {
		uint8 *render_spot = console.render_buf;
		for (x = 0; x < console.columns; x++) {
			if (line[x]) {
				uint8 bits = FONT[CHAR_HEIGHT * line[x] + y];
				for (i = 0; i < CHAR_WIDTH; i++) {
					if(bits & 1) *render_spot = console.color;
					else *render_spot = console.bcolor;
					bits >>= 1;
					render_spot++;
				}
			} else {
				// null character, ignore the rest of the line
				memset(render_spot, console.bcolor, (console.columns - x) * CHAR_WIDTH);
				break;
			}
		}
		memcpy(fb_spot, console.render_buf, console.fb_x);
		fb_spot += console.fb_x;
	}
}


static void
render_line16(char *line, int line_num)
{
	int x;
	int y;
	int i;
	uint16 *fb_spot = (uint16 *)(console.fb + line_num * CHAR_HEIGHT * console.fb_x * 2);

	for (y = 0; y < CHAR_HEIGHT; y++) {
		uint16 *render_spot = console.render_buf;
		for (x = 0; x < console.columns; x++) {
			if (line[x]) {
				uint8 bits = FONT[CHAR_HEIGHT * line[x] + y];
				for (i = 0; i < CHAR_WIDTH; i++) {
					if(bits & 1) *render_spot = console.color;
					else *render_spot = console.bcolor;
					bits >>= 1;
					render_spot++;
				}
			} else {
				// null character, ignore the rest of the line
				memset(render_spot, console.bcolor, (console.columns - x) * CHAR_WIDTH * 2);
				break;
			}
		}
		memcpy(fb_spot, console.render_buf, console.fb_x * 2);
		fb_spot += console.fb_x;
	}
}


static void
render_line32(char *line, int line_num)
{
	int x;
	int y;
	int i;
	uint32 *fb_spot = (uint32 *)(console.fb + line_num * CHAR_HEIGHT * console.fb_x * 4);

	for (y = 0; y < CHAR_HEIGHT; y++) {
		uint32 *render_spot = console.render_buf;
		for (x = 0; x < console.columns; x++) {
			if (line[x]) {
				uint8 bits = FONT[CHAR_HEIGHT * line[x] + y];
				for (i = 0; i < CHAR_WIDTH; i++) {
					if(bits & 1) *render_spot = console.color;
					else *render_spot = console.bcolor;
					bits >>= 1;
					render_spot++;
				}
			} else {
				// null character, ignore the rest of the line
				memset(render_spot, console.bcolor, (console.columns - x) * CHAR_WIDTH * 4);
				break;
			}
		}
		memcpy(fb_spot, console.render_buf, console.fb_x * 4);
		fb_spot += console.fb_x;
	}
}


/** scans through the lines, seeing if any needs to be repainted */

static void
repaint(void)
{
	int i;
	int line_num;

	line_num = console.first_line;
	for (i = 0; i < console.rows; i++) {
		//TRACE(("line_num = %d\n", line_num));
		if (console.dirty_lines[line_num]) {
			//TRACE(("repaint(): rendering line %d %d, func = %p\n", line_num, i, console.render_line));
			console.render_line(console.lines[line_num], i);
			console.dirty_lines[line_num] = 0;
		}
		line_num = INC_WITH_WRAP(line_num, console.num_lines);
	}
}


static void
scrup(void)
{
	int i;
	int line_num;
	int last_line;

	// move the pointer to the top line down one
	console.first_line = INC_WITH_WRAP(console.first_line, console.num_lines);

	TRACE(("scrup: first_line now %d\n", console.first_line));

	line_num = console.first_line;
	for (i = 0; i < console.rows; i++) {
		console.dirty_lines[line_num] = 1;
		line_num = INC_WITH_WRAP(line_num, console.num_lines);
	}

	// clear out the last line
	last_line = WRAP(console.first_line + console.rows, console.num_lines);
	console.lines[last_line][0] = 0;
}


static void
lf(void)
{
	if (console.y + 1 < console.rows) {
		console.y++;
		return;
	}
	scrup();
}


static void
cr(void)
{
	console.x = 0;
}


static void
del(void)
{
	int target_line = WRAP(console.first_line + console.y, console.num_lines);

	if (console.x > 0) {
		console.x--;
		console.lines[target_line][console.x] = ' ';
		console.dirty_lines[target_line] = 1;
	}
}

#if 0
static void
save_cur(void)
{
	console.saved_x = console.x;
	console.saved_y = console.y;
}


static void
restore_cur(void)
{
	console.x = console.saved_x;
	console.y = console.saved_y;
}
#endif


static char
console_put_character(const char c)
{
	int target_line;

	if (console.x + 1 >= console.columns) {
		cr();
		lf();
	}

	target_line = WRAP(console.first_line + console.y, console.num_lines);
	console.lines[target_line][console.x++] = c;
	console.lines[target_line][console.x] = 0;
	console.dirty_lines[target_line] = 1;

 	return c;
}


static void
tab(void)
{
	int i = console.x;

	console.x = (console.x + TAB_SIZE) & ~TAB_MASK;
	if (console.x >= console.columns) {
		console.x -= console.columns;
		i = 0;
		lf();
	}

	// We need to insert spaces, or else the whole line is going to be ignored.
	for (; i < console.x; i++)
		console.lines[console.first_line + console.y][i] = ' ';

	// There is no need to mark the line dirty
}


static status_t
console_open(const char *name, uint32 flags, void **cookie)
{
	return B_OK;
}


static status_t
console_freecookie(void * cookie)
{
	return B_OK;
}


static status_t
console_close(void * cookie)
{
	return B_OK;
}


static status_t
console_read(void * cookie, off_t pos, void *buffer, size_t *_length)
{
	ssize_t bytesRead = read_pos(console.keyboard_fd, 0, buffer, *_length);
	if (bytesRead >= 0) {
		*_length = bytesRead;
		return B_OK;
	}

	return bytesRead;
}


static status_t
_console_write(const void *buffer, size_t *_length)
{
	size_t length = *_length;
	size_t i;
	const char *c;

	for (i = 0; i < length; i++) {
		c = &((const char *)buffer)[i];
		switch (*c) {
			case '\n':
				cr();
				lf();
				break;
			case '\r':
				cr();
				break;
			case 0x8: // backspace
				del();
				break;
			case '\t': // tab
				tab();
				break;
			case '\0':
				break;
			default:
				console_put_character(*c);
		}
	}
	return B_OK;
}


static status_t
console_write(void * cookie, off_t pos, const void *buffer, size_t *_length)
{
	status_t err;

	TRACE(("console_write: text = \"%s\", len = %lu\n", (char *)buffer, *_length));

	mutex_lock(&console.lock);

	err = _console_write(buffer, _length);
//	update_cursor(x, y);
	repaint();

	mutex_unlock(&console.lock);

	return err;
}


static status_t
console_ioctl(void *cookie, uint32 op, void *buffer, size_t len)
{
	int err;
	
	switch (op) {
/*		case CONSOLE_OP_WRITEXY:
		{
			size_t wlen;
			int x,y;
			mutex_lock(&console.lock);

			x = ((int *)buffer)[0];
			y = ((int *)buffer)[1];

			save_cur();
//			gotoxy(x, y);
			wlen = len - 2 * sizeof(int);
			if (_console_write(((char *)buffer) + 2 * sizeof(int), &wlen) == B_OK)
				err = 0; // we're okay
			else
				err = EIO;
			restore_cur();
			mutex_unlock(&console.lock);
			break;
		}
*/		default:
			err = EINVAL;
	}

	return err;
}


static device_hooks sFrameBufferConsoleHooks = {
	&console_open,
	&console_close,
	&console_freecookie,
	&console_ioctl,
	&console_read,
	&console_write,
	NULL,
	NULL,
};


status_t
fb_console_dev_init(kernel_args *args)
{
	status_t status;
	int i;

	if (!args->frame_buffer.enabled)
		return B_OK;

	TRACE(("fb_console_dev_init: framebuffer found at 0x%lx, x %ld, y %ld, bit depth %ld\n",
		args->frame_buffer.physical_buffer.start, args->frame_buffer.width,
		args->frame_buffer.height, args->frame_buffer.depth));

	memset(&console, 0, sizeof(console));

	status = map_physical_memory("vesa_fb", (void *)args->frame_buffer.physical_buffer.start,
		args->frame_buffer.physical_buffer.size, B_ANY_KERNEL_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, (void **)&console.fb);
	if (status < B_OK)
		return status;

	console.fb_x = args->frame_buffer.width;
	console.fb_y = args->frame_buffer.height;
	console.fb_pixel_bytes = args->frame_buffer.depth / 8;
	console.fb_size = args->frame_buffer.physical_buffer.size;

	switch (console.fb_pixel_bytes) {
		case 1:
			console.render_line = &render_line8;
			console.color = 0;		// black
			console.bcolor = 63;	// white
			break;
		case 2:
			console.render_line = &render_line16;
			console.color = 0xffff;
			console.bcolor = 0;
			break;
		case 4:
			console.render_line = &render_line32;
			console.color = 0x00ffffff;
			console.bcolor = 0;
			break;

		default:
			return 0;
	}

	TRACE(("framebuffer mapped at 0x%lx\n", console.fb));

	// figure out the number of rows/columns we have
	console.rows = console.fb_y / CHAR_HEIGHT;
	console.columns = console.fb_x / CHAR_WIDTH;
	TRACE(("%d rows %d columns\n", console.rows, console.columns));

	console.x = 0;
	console.y = 0;
	console.saved_x = 0;
	console.saved_y = 0;

	// allocate some memory for this
	console.render_buf = malloc(console.fb_x * console.fb_pixel_bytes);
	memset((void *)console.render_buf, console.bcolor, console.fb_x * console.fb_pixel_bytes);
	console.buf = malloc(console.rows * (console.columns+1));
	memset(console.buf, 0, console.rows * (console.columns+1));
	console.lines = malloc(console.rows * sizeof(char *));
	console.dirty_lines = malloc(console.rows);
	// set up the line pointers
	for (i = 0; i < console.rows; i++) {
		console.lines[i] = (char *)((addr_t)console.buf + i*(console.columns+1));
		console.dirty_lines[i] = 1;
	}
	console.num_lines = console.rows;
	console.first_line = 0;

	repaint();

	mutex_init(&console.lock, "console_lock");
	console.keyboard_fd = open("/dev/keyboard", O_RDONLY);
	if (console.keyboard_fd < 0)
		panic("fb_console_dev_init: error opening /dev/keyboard\n");

	// create device node
	if (devfs_publish_device("console", NULL, &sFrameBufferConsoleHooks) != B_OK)
		panic("fb_console: could not publish device!\n");

#if 0
{
	// displays the palette in 8 bit mode
#define BAR_WIDTH 8
	int x, y, i;
	int width = (console.fb_x - BAR_WIDTH - 1) / BAR_WIDTH;
	for (y = 0; y < console.fb_y; y++) {
		for (i = 0; i < width; i++) {
			uint8 field[BAR_WIDTH];
			for (x = 0; x < BAR_WIDTH; x++) {
				field[x] = i;
			}
			memcpy(console.fb + i*BAR_WIDTH + y*console.fb_x, field, BAR_WIDTH);
		}
	}
}
#endif

#if 0
{
	int x, y;

	for(y=0; y<console.fb_y; y++) {
		uint16 row[console.fb_x];
		for(x = 0; x<console.fb_x; x++) {
			row[x] = x;
		}
		dprintf("%d\n", y);
		memcpy(console.fb + y*console.fb_x*2, row, sizeof(row));
	}
	panic("foo\n");
}
#endif

#if 0
{
		int i,j,k;

		for(k=0;; k++) {
			for(i=0; i<ka->fb.y_size; i++) {
				uint16 row[ka->fb.x_size];
				for(j=0; j<ka->fb.x_size; j++) {
					uint8 byte = j+i+k;
					row[j] = byte;
				}
				memcpy(console.fb + i*ka->fb.x_size*2, row, sizeof(row));
			}
		}
}
#endif

	return 0;
}

