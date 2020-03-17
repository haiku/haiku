/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <string.h>

#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>

#include "efi_platform.h"
#include "serial.h"


static char sBuffer[16384];
static uint32 sBufferPosition;


static void
syslog_write(const char* buffer, size_t length)
{
	if (sBufferPosition + length > sizeof(sBuffer))
		return;
	memcpy(sBuffer + sBufferPosition, buffer, length);
	sBufferPosition += length;
}


static void
dprintf_args(const char *format, va_list args)
{
	char buffer[512];
	int length = vsnprintf(buffer, sizeof(buffer), format, args);
	if (length == 0)
		return;

	syslog_write(buffer, length);
	serial_puts(buffer, length);
}


extern "C" void
dprintf(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	dprintf_args(format, args);
	va_end(args);
}


extern "C" void
panic(const char *format, ...)
{
	va_list args;

	platform_switch_to_text_mode();

	puts("*** PANIC ***");

	va_start(args, format);
	vprintf(format, args);
	va_end(args);

	while (true)
		kBootServices->Stall(1000000);
}


char*
platform_debug_get_log_buffer(size_t *_size)
{
	if (_size != NULL)
		*_size = sizeof(sBuffer);

	return sBuffer;
}
