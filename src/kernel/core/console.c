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

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>


// ToDo: this mechanism will be changed so that these lines can go away
#ifdef ARCH_x86
#	include <arch/x86/console_dev.h>
#else
enum {
	CONSOLE_OP_WRITEXY = 2376
};
#endif


#include <fb_console.h>


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


int
con_init(kernel_args *args)
{
	/* now run the oddballs we have, consoles at present */
	// ToDo: this mechanism will be changed so that these lines can go away
#ifdef ARCH_x86
	console_dev_init(args);
#endif
	fb_console_dev_init(args);

	console_fd = open("/dev/console", O_RDWR);
	return console_fd >= 0;
}
