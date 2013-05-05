/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "platform_debug.h"

#include "gpio.h"
#include "serial.h"

#include <boot/platform.h>
#include <boot/stdio.h>
#include <stdarg.h>


extern "C" void platform_video_puts(const char* string);

extern addr_t gPeripheralBase;
static bool sLedState = true;


void
debug_delay(int time)
{
	for (; time >= 0; time--)
		for (int i = 0; i < 10000; i++)
			asm volatile ("nop");
}


void
debug_set_led(bool on)
{
	gpio_write(gPeripheralBase + GPIO_BASE, 16, on ? 0 : 1);
	sLedState = on;
}


void
debug_toggle_led(int count, int delay)
{
	for (count *= 2; count > 0; count--) {
		debug_set_led(!sLedState);
		debug_delay(delay);
	}
}


void
debug_blink_number(int number)
{
	bool previousState = sLedState;

	debug_set_led(false);
	debug_delay(DEBUG_DELAY_LONG);

	while (number > 0) {
		debug_toggle_led(number % 10 + 1, DEBUG_DELAY_SHORT);
		debug_delay(DEBUG_DELAY_LONG);
		number /= 10;
	}

	debug_set_led(true);
	debug_delay(DEBUG_DELAY_LONG);

	debug_set_led(previousState);
}


void
debug_halt()
{
	while (true)
		debug_delay(DEBUG_DELAY_LONG);
}


void
debug_assert(bool condition)
{
	if (condition)
		return;

	debug_toggle_led(20, 20);
	debug_halt();
}


extern "C" void
debug_puts(const char* string, int32 length)
{
	platform_video_puts(string);
	serial_puts(string, length);
}


extern "C" void
panic(const char* format, ...)
{
	const char hint[] = "\n*** PANIC ***\n";
	char buffer[512];
	va_list list;
	int length;

	debug_puts(hint, sizeof(hint));
	va_start(list, format);
	length = vsnprintf(buffer, sizeof(buffer), format, list);
	va_end(list);

	if (length >= (int)sizeof(buffer))
		length = sizeof(buffer) - 1;

	debug_puts(buffer, length);
	//fprintf(stderr, "%s", buffer);

	debug_puts("\nPress key to reboot.", 21);

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

	debug_puts(buffer, length);

	if (platform_boot_options() & BOOT_OPTION_DEBUG_OUTPUT)
		fprintf(stderr, "%s", buffer);
}


char*
platform_debug_get_log_buffer(size_t* _size)
{
	return NULL;
}
