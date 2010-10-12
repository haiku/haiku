/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "keyboard.h"
#include "rom_calls.h"

#include <boot/platform.h>
#include <boot/stdio.h>
#include <stdarg.h>
#include <string.h>

#include <Errors.h>

/*!	This works only after console_init() was called.
*/
void
panic(const char *format, ...)
{
	static struct AlertMessage {
		uint16	column1;
		uint8	line1;
		char	message[14];
		uint8	cont;
		uint16	column2;
		uint8	line2;
		char	buffer[512];
		uint8	end;
		
	} _PACKED alert = {
		10, 12,
		"*** PANIC ***",
		1,
		10, 22,
		"",
		0
	};

	char *buffer = alert.buffer;
	va_list list;

	//platform_switch_to_text_mode();

	memset(buffer, 0, 512);

	va_start(list, format);
	vsnprintf(buffer, 512, format, list);
	va_end(list);

	DisplayAlert(DEADEND_ALERT, &alert, 40);

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


