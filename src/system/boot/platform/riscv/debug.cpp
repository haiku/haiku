/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <Htif.h>

#include <boot/platform.h>
#include <boot/stdio.h>
#include <stdarg.h>
#include <string.h>

#include <Errors.h>

extern FILE *dbgerr;


static void
dprintf_args(const char* format, va_list args)
{
	char buffer[512];
	int length = vsnprintf(buffer, sizeof(buffer), format, args);
	if (length == 0)
		return;

	//syslog_write(buffer, length);
	HtifOutString(buffer);
}


extern "C" void
dprintf(const char* format, ...)
{
	va_list args;

	va_start(args, format);
	dprintf_args(format, args);
	va_end(args);
}


void
panic(const char* format, ...)
{
	va_list args;

	dprintf("*** PANIC ***\n");

	va_start(args, format);
	dprintf_args(format, args);
	va_end(args);

	for(;;) {}
}


char*
platform_debug_get_log_buffer(size_t* _size)
{
	return NULL;
}
