/*
 * Copyright 2003-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdarg.h>
#include <string.h>

#include <boot/platform.h>
#include <boot/stdio.h>
#include <platform/openfirmware/openfirmware.h>


static char sBuffer[16384];
static uint32 sBufferPosition;


static inline void
syslog_write(const char* buffer, size_t length)
{
	if (sBufferPosition + length > sizeof(sBuffer))
		return;
	memcpy(sBuffer + sBufferPosition, buffer, length);
	sBufferPosition += length;
}


extern "C" void
panic(const char* format, ...)
{
	// TODO: this works only after console_init() was called.
	va_list list;

	puts("*** PANIC ***");

	va_start(list, format);
	vprintf(format, list);
	va_end(list);

	of_exit();
}


static inline void
dprintf_args(const char *format, va_list args)
{
	char buffer[512];
	int length = vsnprintf(buffer, sizeof(buffer), format, args);
	if (length == 0)
		return;

	syslog_write(buffer, length);
	printf("%s", buffer);
}


extern "C" void
dprintf(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	dprintf_args(format, args);
	va_end(args);
}




char*
platform_debug_get_log_buffer(size_t* _size)
{
	if (_size != NULL)
		*_size = sizeof(sBuffer);

	return sBuffer;
}
