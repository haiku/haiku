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
#include <drivers.h>
#include <Errors.h>
#include <devfs.h>

#include <arch/cpu.h>
#include <arch/int.h>

#include <string.h>
#include <stdio.h>

#include <arch/x86/console_dev.h>

static unsigned int origin = 0;

#define SCREEN_START 0xb8000
#define SCREEN_END   0xc0000
#define LINES 25
#define COLUMNS 80
#define NPAR 16
#define TAB_SIZE 8
#define TAB_MASK 7

#define TEXT_INDEX 0x3d4
#define TEXT_DATA  0x3d5

#define TEXT_CURSOR_LO 0x0f
#define TEXT_CURSOR_HI 0x0e

#define scr_end (origin+LINES*COLUMNS*2)

static unsigned int pos = 0;
static unsigned int x = 0;
static unsigned int y = 0;
static unsigned int bottom=LINES;
static unsigned int lines=LINES;
static unsigned int columns=COLUMNS;
static unsigned char attr=0x07;

static mutex console_lock;
static int keyboard_fd = -1;

static void update_cursor(unsigned int x, unsigned int y)
{
	short int pos = y*columns + x;

	out8(TEXT_CURSOR_LO, TEXT_INDEX);
	out8((char)pos, TEXT_DATA);
	out8(TEXT_CURSOR_HI, TEXT_INDEX);
	out8((char)(pos >> 8), TEXT_DATA);
}

static void gotoxy(unsigned int new_x,unsigned int new_y)
{
	if (new_x>=columns || new_y>=lines)
		return;
	x = new_x;
	y = new_y;
	pos = origin+((y*columns+x)<<1);
}

static void scrup(void)
{
	unsigned long i;

	// move the screen up one
	memcpy((void *)origin, (void *)origin+2*COLUMNS, 2*(LINES-1)*COLUMNS);

	// set the new position to the beginning of the last line
	pos = origin + (LINES-1)*COLUMNS*2;

	// clear the bottom line
	for(i = pos; i < scr_end; i += 2) {
		*(unsigned short *)i = 0x0720;
	}
}

static void lf(void)
{
	if (y+1<bottom) {
		y++;
		pos += columns<<1;
		return;
	}
	scrup();
}

static void cr(void)
{
	pos -= x<<1;
	x=0;
}

static void del(void)
{
	if (x > 0) {
		pos -= 2;
		x--;
		*(unsigned short *)pos = 0x0720;
	}
}

static int saved_x=0;
static int saved_y=0;

static void save_cur(void)
{
	saved_x=x;
	saved_y=y;
}

static void restore_cur(void)
{
	x=saved_x;
	y=saved_y;
	pos=origin+((y*columns+x)<<1);
}

static char console_putch(const char c)
{
	if(++x>=COLUMNS) {
		cr();
		lf();
	}

	*(char *)pos = c;
	*(char *)(pos+1) = attr;

	pos += 2;

	return c;
}

static void tab(void)
{
	x = (x + TAB_SIZE) & ~TAB_MASK;
	if (x>=COLUMNS) {
		x -= COLUMNS;
		lf();
	}
	pos=origin+((y*columns+x)<<1);
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

static ssize_t console_read(void * cookie, off_t pos, void *buf, size_t *len)
{
	/* XXX - optimistic!! */
	*len = sys_read(keyboard_fd, buf, 0, *len);
	return 0;
}

static ssize_t _console_write(const void *buf, size_t len)
{
	size_t i;
	const char *c;

	for(i=0; i<len; i++) {
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
			case '\t':
				tab();
				break;
			case '\0':
				break;
			default:
				console_putch(*c);
		}
	}
	return len;
}

static ssize_t console_write(void * cookie, off_t pos, const void *buf, size_t *len)
{
	ssize_t err;

//	dprintf("console_write: entry, len = %d\n", *len);

	mutex_lock(&console_lock);

	err = _console_write(buf, *len);
	if (err < 0)
		*len = 0;
	else {
		*len = err;
		err = 0;
	}
	 
	update_cursor(x, y);

	mutex_unlock(&console_lock);

	return err;
}

static int console_ioctl(void * cookie, uint32 op, void *buf, size_t len)
{
	int err;

	switch(op) {
		case CONSOLE_OP_WRITEXY: {
			int x,y;
			mutex_lock(&console_lock);

			x = ((int *)buf)[0];
			y = ((int *)buf)[1];

			save_cur();
			gotoxy(x, y);
			if(_console_write(((char *)buf) + 2*sizeof(int), len - 2*sizeof(int)) > 0)
				err = 0; // we're okay
			else
				err = ERR_IO_ERROR;
			restore_cur();
			mutex_unlock(&console_lock);
			break;
		}
		default:
			err = ERR_INVALID_ARGS;
	}

	return err;
}

device_hooks console_hooks = {
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

int console_dev_init(kernel_args *ka)
{
	if(!ka->fb.enabled) {
		dprintf("con_init: mapping vid mem\n");
		vm_map_physical_memory(vm_get_kernel_aspace_id(), "vid_mem", (void *)&origin, REGION_ADDR_ANY_ADDRESS,
			SCREEN_END - SCREEN_START, LOCK_RW|LOCK_KERNEL, SCREEN_START);
		dprintf("con_init: mapped vid mem to virtual address 0x%x\n", origin);

		pos = origin;

		gotoxy(0, ka->cons_line);
		update_cursor(x, y);

		mutex_init(&console_lock, "console_lock");
		keyboard_fd = sys_open("/dev/keyboard", STREAM_TYPE_DEVICE, 0);
		if(keyboard_fd < 0)
			panic("console_dev_init: error opening /dev/keyboard\n");

		// create device node
		devfs_publish_device("console", NULL, &console_hooks);
	}

	return 0;
}

