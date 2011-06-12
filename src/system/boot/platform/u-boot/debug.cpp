/*
 * Copyright 2004-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "serial.h"
#include "keyboard.h"

#include <boot/platform.h>
#include <boot/stdio.h>
#include <stdarg.h>


/*!	This works only after console_init() was called.
*/
extern "C" void
panic(const char* format, ...)
{
	va_list list;

	platform_switch_to_text_mode();

	puts("*** PANIC ***");

	va_start(list, format);
	vprintf(format, list);
	va_end(list);

	puts("\nPress key to reboot.");

	clear_key_buffer();
	wait_for_key();
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
