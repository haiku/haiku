/*
 * Copyright 2008-2010, François Revol, revol@free.fr. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "keyboard.h"
#include "nextrom.h"

#include <boot/stage2.h>
#include <boot/platform.h>
#include <boot/stdio.h>
#include <stdarg.h>

#include <Errors.h>

void mon_put(const char *s)
{
	while (s && *s)
		mg->mg_putc(*s++);
}

void mon_puts(const char *s)
{
	mon_put(s);
	mg->mg_putc('\n');
}

/*!	This works only after console_init() was called.
*/
void
panic(const char *format, ...)
{
	const char greetings[] = "\n*** PANIC ***";
	char buffer[512];
	va_list list;

	//platform_switch_to_text_mode();

	mon_puts(greetings);

	va_start(list, format);
	vsnprintf(buffer, sizeof(buffer), format, list);
	va_end(list);

	mon_puts(buffer);

	mon_puts("\nPress key to reboot.");

	clear_key_buffer();
	wait_for_key();
	platform_exit();
}


void
dprintf(const char *format, ...)
{
	char buffer[512];
	va_list list;

	va_start(list, format);
	vsnprintf(buffer, sizeof(buffer), format, list);
	va_end(list);

	//if (platform_boot_options() & BOOT_OPTION_DEBUG_OUTPUT)
	if (!gKernelArgs.frame_buffer.enabled)
		mon_put(buffer);
}


char*
platform_debug_get_log_buffer(size_t* _size)
{
	return NULL;
}
