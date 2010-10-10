/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "keyboard.h"
#include "amicalls.h"

#include <boot/platform.h>
#include <boot/stdio.h>
#include <stdarg.h>

#include <Errors.h>

/*!	This works only after console_init() was called.
*/
void
panic(const char *format, ...)
{
	struct AlertMessage {
		UWORD	column;
		UBYTE	line;
		char	messages[14+512];
	} alert = {
		10, 12,
		"*** PANIC ***"
	}

	const char greetings[] = "\n*** PANIC ***";
	char *buffer = &alert.messages[14];
	va_list list;

	//platform_switch_to_text_mode();

	memset(buffer, 512);

	va_start(list, format);
	vsnprintf(buffer, 512-1, format, list);
	va_end(list);

	DisplayAlert(DEADEND_ALERT, &alert, 30);

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

//	Bconput(DEV_AUX, buffer);

	//if (platform_boot_options() & BOOT_OPTION_DEBUG_OUTPUT)
//		Bconput(DEV_CONSOLE, buffer);
}


