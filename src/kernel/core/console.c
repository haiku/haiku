/* This file contains the kprintf stuff and console init */

/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <console.h>
#include <arch/debug.h>
#include <arch/int.h>
#include <smp.h>
#include <OS.h>
#include <vfs.h>
#include <debug.h>
#include <fd.h>

#include <stdarg.h>
#include <stdio.h>

// from con.h
enum {
	CONSOLE_OP_WRITEXY = 2376
};

struct console_op_xy_struct {
	int x;
	int y;
	char buf[256];
};

static int console_fd = -1;


void
kprintf(const char *fmt, ...)
{
	int ret = 0;
	va_list args;
	char temp[256];

	if (console_fd >= 0) {
		va_start(args, fmt);
		ret = vsprintf(temp,fmt,args);
		va_end(args);
	
		_kern_write(console_fd, 0, temp, ret);
	}
}


void
kprintf_xy(int x, int y, const char *fmt, ...)
{
	int ret = 0;
	va_list args;
	struct console_op_xy_struct buf;

	if (console_fd >= 0) {
		va_start(args, fmt);
		ret = vsprintf(buf.buf,fmt,args);
		va_end(args);
	
		buf.x = x;
		buf.y = y;
		_kern_ioctl(console_fd, CONSOLE_OP_WRITEXY, &buf, ret + sizeof(buf.x) + sizeof(buf.y));
	}
}


int
con_init(kernel_args *ka)
{
	dprintf("con_init: entry\n");

	console_fd = _kern_open("/dev/console", 0);
	dprintf("console_fd = %d\n", console_fd);

	return 0;
}
