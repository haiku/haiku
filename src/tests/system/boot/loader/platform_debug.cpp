/*
 * Copyright 2003-2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
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


char*
platform_debug_get_log_buffer(size_t* _size)
{
	return NULL;
}
