/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include "serial.h"

#include <boot/platform.h>
#include <boot/stdio.h>
#include <stdarg.h>


/** This works only after console_init() was called.
 */

void
panic(const char *format, ...)
{
	va_list list;

	platform_switch_to_text_mode();

	puts("*** PANIC ***");

	va_start(list, format);
	vprintf(format, list);
	va_end(list);

	// ToDo: add "press key to reboot" functionality
	for (;;)
		asm("hlt");
}


void
dprintf(const char *format, ...)
{
	char buffer[512];
	va_list list;
	int length;

	va_start(list, format);
	length = vsnprintf(buffer, sizeof(buffer), format, list);
	va_end(list);

	serial_puts(buffer, length);

	if (platform_boot_options() & BOOT_OPTION_DEBUG_OUTPUT)
		fprintf(stderr, "%s", buffer);
}

