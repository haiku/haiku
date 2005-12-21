/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <boot/platform.h>

#include <stdio.h>
#include <stdarg.h>


void
panic(const char *format, ...)
{
	va_list args;

	puts("*** PANIC ***");
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}


void
dprintf(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

