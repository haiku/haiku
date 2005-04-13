/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/* This file contains the kprintf stuff and console init */


#include <OS.h>
#include <KernelExport.h>

#include <boot/kernel_args.h>
#include <console.h>
#include <frame_buffer_console.h>

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>



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

		write_pos(console_fd, 0, temp, ret);
	}
}

#if 0
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
		ioctl(console_fd, CONSOLE_OP_WRITEXY, &buf, ret + sizeof(buf.x) + sizeof(buf.y));
	}
}
#endif

int
con_init(kernel_args *args)
{
	frame_buffer_console_init(args);
	return B_OK;
}
