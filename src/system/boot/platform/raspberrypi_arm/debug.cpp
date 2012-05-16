/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "serial.h"

#include <boot/platform.h>
#include <boot/stdio.h>
#include <stdarg.h>


extern "C" void
panic(const char* format, ...)
{
	const char hint[] = "\n*** PANIC ***\n";
	char buffer[512];
	va_list list;
	int length;

	serial_puts(hint, sizeof(hint));
	va_start(list, format);
	length = vsnprintf(buffer, sizeof(buffer), format, list);
	va_end(list);

	if (length >= (int)sizeof(buffer))
		length = sizeof(buffer) - 1;

	serial_puts(buffer, length);
	//fprintf(stderr, "%s", buffer);

	serial_puts("\nPress key to reboot.", 21);

	platform_exit();
}


extern "C" void
dprintf(const char* format, ...)
{
	char buffer[512];
	va_list list;
	int length;

	va_start(list, format);
	length = vsnprintf(buffer, sizeof(buffer), format, list);
	va_end(list);

	if (length >= (int)sizeof(buffer))
		length = sizeof(buffer) - 1;

	serial_puts(buffer, length);

	if (platform_boot_options() & BOOT_OPTION_DEBUG_OUTPUT)
		fprintf(stderr, "%s", buffer);
}


char*
platform_debug_get_log_buffer(size_t* _size)
{
	return NULL;
}
