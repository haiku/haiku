/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <syscalls.h>
#include <stdio.h>
#include <string.h>

int __stdio_init(void);	 /* keep the compiler happy, these two are not supposed */
int __stdio_deinit(void);/* to be called by anyone except crt0, and crt0 will change soon */


int __stdio_init(void)
{
	return 0;
}

int __stdio_deinit(void)
{
	return 0;
}

int printf(const char *fmt, ...)
{
	va_list args;
	char buf[1024];
	int i;

	va_start(args, fmt);
	i = vsprintf(buf, fmt, args);
	va_end(args);

	sys_write(1, buf, -1, strlen(buf));

	return i;
}

int getchar(void)
{
	char c;

	sys_read(0, &c, 0, 1);
	return c;
}
