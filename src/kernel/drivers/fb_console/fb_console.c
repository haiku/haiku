/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <console.h>
#include <debug.h>
#include <memheap.h>
#include <int.h>
#include <vm.h>
#include <lock.h>
#include <devfs.h>
#include <drivers.h>
#include <Errors.h>

#include <arch/cpu.h>
#include <arch/int.h>

#include <string.h>
#include <stdio.h>

#include "font.h"

int fb_console_dev_init(kernel_args *ka);

#if 0
// this version makes the sh4 compiler throw up
#define WRAP(x, limit) ((x) % (limit))
#define INC_WITH_WRAP(x, limit) WRAP((x) + 1, (limit))
#else
#define WRAP(x, limit) (((x) >= (limit)) ? ((x) - (limit)) : (x))
#define INC_WITH_WRAP(x, limit) WRAP((x) + 1, (limit))
#endif

enum {
	CONSOLE_OP_WRITEXY = 2376
};

#define TAB_SIZE 8
#define TAB_MASK 7

struct console_desc {
	// describe the framebuffer
	addr fb;
	addr fb_size;
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

static void render_line16(char *line, int line_num)
{
	int x;
	int y;
	int i;
	uint16 *fb_spot = (uint16 *)(console.fb + line_num * CHAR_HEIGHT * console.fb_x * 2);

	for(y = 0; y < CHAR_HEIGHT; y++) {
		uint16 *render_spot = console.render_buf;
		for(x = 0; x < console.columns; x++) {
			if(line[x]) {
				uint8 bits = FONT[CHAR_HEIGHT * line[x] + y];
				for(i = 0; i < CHAR_WIDTH; i++) {
					if(bits & 1) *render_spot = console.color;
					else *render_spot = console.bcolor;
					bits >>= 1;
					render_spot++;
				}
			} else {
				// null character, ignore the rest of the line
				memset(render_spot, 0, (console.columns - x) * CHAR_WIDTH * 2);
				break;
			}
		}
		memcpy(fb_spot, console.render_buf, console.fb_x * 2);
		fb_spot += console.fb_x;
	}
}

static void render_line32(char *line, int line_num)
{
	int x;
	int y;
	int i;
	uint32 *fb_spot = (uint32 *)(console.fb + line_num * CHAR_HEIGHT * console.fb_x * 4);

	for(y = 0; y < CHAR_HEIGHT; y++) {
		uint32 *render_spot = console.render_buf;
		for(x = 0; x < console.columns; x++) {
			if(line[x]) {
				uint8 bits = FONT[CHAR_HEIGHT * line[x] + y];
				for(i = 0; i < CHAR_WIDTH; i++) {
					if(bits & 1) *render_spot = console.color;
					else *render_spot = console.bcolor;
					bits >>= 1;
					render_spot++;
				}
			} else {
				// null character, ignore the rest of the line
				memset(render_spot, 0, (console.columns - x) * CHAR_WIDTH * 4);
				break;
			}

		}
		memcpy(fb_spot, console.render_buf, console.fb_x * 4);
		fb_spot += console.fb_x;
	}
}

// scans through the lines, seeing if any needs to be repainted
static void repaint()
{
	int i;
	int line_num;

	line_num = console.first_line;
	for(i = 0; i < console.rows; i++) {
//		dprintf("line_num = %d\n", line_num);
		if(console.dirty_lines[line_num]) {
//			dprintf("repaint(): rendering line %d %d\n", line_num, i);
			console.render_line(console.lines[line_num], i);
			console.dirty_lines[line_num] = 0;
		}
		line_num = INC_WITH_WRAP(line_num, console.num_lines);
	}
}

static void scrup(void)
{
	int i;
	int line_num;
	int last_line;

	// move the pointer to the top line down one
	console.first_line = INC_WITH_WRAP(console.first_line, console.num_lines);

//	dprintf("scrup: first_line now %d\n", console.first_line);

	line_num = console.first_line;
	for(i=0; i<console.rows; i++) {
		console.dirty_lines[line_num] = 1;
		line_num = INC_WITH_WRAP(line_num, console.num_lines);
	}

	// clear out the last line
	last_line = WRAP(console.first_line + console.rows, console.num_lines);
	console.lines[last_line][0] = 0;
}

static void lf(void)
{
	if(console.y + 1 < console.rows) {
		console.y++;
		return;
	}
	scrup();
}

static void cr(void)
{
	console.x = 0;
}

static void del(void)
{
	int target_line = WRAP(console.first_line + console.y, console.num_lines);

	if(console.x > 0) {
		console.x--;
		console.lines[target_line][console.x] = ' ';
		console.dirty_lines[target_line] = 1;
	}
}

static void save_cur(void)
{
	console.saved_x = console.x;
	console.saved_y = console.y;
}

static void restore_cur(void)
{
	console.x = console.saved_x;
	console.y = console.saved_y;
}

static char console_putch(const char c)
{
	int target_line;

	if(console.x+1 >= console.columns) {
		cr();
		lf();
	}

	target_line = WRAP(console.first_line + console.y, console.num_lines);
//	dprintf("target_line = %d\n", target_line);
	console.lines[target_line][console.x++] = c;
	console.lines[target_line][console.x] = 0;
	console.dirty_lines[target_line] = 1;

 	return c;
}

static void tab(void)
{
	console.x = (console.x + TAB_SIZE) & ~TAB_MASK;
	if (console.x >= console.columns) {
		console.x -= console.columns;
		lf();
	}
}

static int console_open(const char *name, uint32 flags, void **cookie)
{
	return 0;
}

static int console_freecookie(void * cookie)
{
	return 0;
}

static int console_seek(void * cookie, off_t pos, int st)
{
//	dprintf("console_seek: entry\n");

	return EPERM;
}

static int console_close(void * cookie)
{
//	dprintf("console_close: entry\n");

	return 0;
}

static ssize_t console_read(void * cookie, off_t pos, void *buf, 
                            size_t *len)
{
	return sys_read(console.keyboard_fd, buf, 0, *len);
}

static ssize_t _console_write(const void *buf, size_t *len)
{
	size_t i;
	const char *c;

	for(i=0; i < *len; i++) {
		c = &((const char *)buf)[i];
		switch(*c) {
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
				console_putch(*c);
		}
	}
	return 0;
}

static ssize_t console_write(void * cookie, off_t pos, 
                             const void *buf, size_t *len)
{
	ssize_t err;

//	dprintf("console_write: entry, len = %d\n", len);

	mutex_lock(&console.lock);

	err = _console_write(buf, len);
//	update_cursor(x, y);
	repaint();

	mutex_unlock(&console.lock);

	return err;
}

static int console_ioctl(void * cookie, uint32 op, void *buf, size_t len)
{
	int err;
	size_t wlen;
	
	switch(op) {
		case CONSOLE_OP_WRITEXY: {
			int x,y;
			mutex_lock(&console.lock);

			x = ((int *)buf)[0];
			y = ((int *)buf)[1];

			save_cur();
//			gotoxy(x, y);
			wlen = len - 2 * sizeof(int);
			if(_console_write(((char *)buf) + 2*sizeof(int), &wlen) == 0)
				err = 0; // we're okay
			else
				err = ERR_IO_ERROR;
			restore_cur();
			mutex_unlock(&console.lock);
			break;
		}
		default:
			err = ERR_INVALID_ARGS;
	}

	return err;
}

device_hooks fb_console_hooks = {
	&console_open,
	&console_close,
	&console_freecookie,
	&console_ioctl,
	&console_read,
	&console_write,
	NULL,
	NULL,
//	NULL,
//	NULL
};

int fb_console_dev_init(kernel_args *ka)
{
	if(ka->fb.enabled) {
		int i;

		dprintf("fb_console_dev_init: framebuffer found at 0x%lx, x %d, y %d, bit depth %d\n",
			ka->fb.mapping.start, ka->fb.x_size, ka->fb.y_size, ka->fb.bit_depth);

		memset(&console, 0, sizeof(console));

		if(ka->fb.already_mapped) {
			console.fb = ka->fb.mapping.start;
		} else {
			vm_map_physical_memory(vm_get_kernel_aspace_id(), "vesa_fb", (void *)&console.fb, REGION_ADDR_ANY_ADDRESS,
				ka->fb.mapping.size, LOCK_RW|LOCK_KERNEL, ka->fb.mapping.start);
		}

		console.fb_x = ka->fb.x_size;
		console.fb_y = ka->fb.y_size;
		console.fb_pixel_bytes = ka->fb.bit_depth / 8;
		console.fb_size = ka->fb.mapping.size;

		switch(console.fb_pixel_bytes) {
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
			case 1:
			default:
				return 0;
		}

		dprintf("framebuffer mapped at 0x%lx\n", console.fb);

		// figure out the number of rows/columns we have
		console.rows = console.fb_y / CHAR_HEIGHT;
		console.columns = console.fb_x / CHAR_WIDTH;
		dprintf("%d rows %d columns\n", console.rows, console.columns);

		console.x = 0;
		console.y = 0;
		console.saved_x = 0;
		console.saved_y = 0;

		dprintf("console %p\n", &console);

		// allocate some memory for this
		console.render_buf = kmalloc(console.fb_x * console.fb_pixel_bytes);
		memset((void *)console.render_buf, 0, console.fb_x * console.fb_pixel_bytes);
		console.buf = kmalloc(console.rows * (console.columns+1));
		memset(console.buf, 0, console.rows * (console.columns+1));
		console.lines = kmalloc(console.rows * sizeof(char *));
		console.dirty_lines = kmalloc(console.rows);
		// set up the line pointers
		for(i=0; i<console.rows; i++) {
			console.lines[i] = (char *)((addr)console.buf + i*(console.columns+1));
			console.dirty_lines[i] = 1;
		}
		console.num_lines = console.rows;
		console.first_line = 0;

		repaint();

		mutex_init(&console.lock, "console_lock");
		console.keyboard_fd = sys_open("/dev/keyboard", STREAM_TYPE_DEVICE, 0);
		if(console.keyboard_fd < 0)
			panic("fb_console_dev_init: error opening /dev/keyboard\n");

		// create device node
		devfs_publish_device("console", NULL, &fb_console_hooks);

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
	}

	return 0;
}

