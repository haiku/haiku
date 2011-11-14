/*
 * Copyright 2011, François Revol, revol@free.fr.
 * Copyright 2003-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdarg.h>

#include <boot/platform.h>
#include <boot/stdio.h>
#include <boot/platform/cfe/cfe.h>


extern "C" void
panic(const char* format, ...)
{
	// TODO: this works only after console_init() was called.
	va_list list;

	puts("*** PANIC ***");

	va_start(list, format);
	vprintf(format, list);
	va_end(list);

	cfe_exit(CFE_FLG_WARMSTART, 2);
}


extern "C" void
dprintf(const char* format, ...)
{
	va_list list;

	va_start(list, format);
	vprintf(format, list);
	va_end(list);
}


char*
platform_debug_get_log_buffer(size_t* _size)
{
	return NULL;
}
