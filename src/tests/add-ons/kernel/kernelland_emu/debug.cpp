/*
 * Copyright 2002-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de.
 *		Axel Dörfler, axeld@pinc-software.de.
 */

#include <signal.h>
#define _KERNEL_MODE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>


bool gDebugOutputEnabled = true;


extern "C" uint64
parse_expression(const char* arg)
{
	return strtoull(arg, NULL, 0);
}


extern "C" bool
set_debug_variable(const char* variableName, uint64 value)
{
	return true;
}


extern "C" bool
print_debugger_command_usage(const char* command)
{
	return true;
}


extern "C" status_t
add_debugger_command_etc(const char* name, debugger_command_hook func,
	const char* description, const char* usage, uint32 flags)
{
	return B_OK;
}


extern "C" int
add_debugger_command(const char *name, int (*func)(int, char **),
	const char *desc)
{
	return B_OK;
}


extern "C" int
remove_debugger_command(const char * name, int (*func)(int, char **))
{
	return B_OK;
}


extern "C" void
panic(const char *format, ...)
{
	char buffer[1024];

	strcpy(buffer, "PANIC: ");
	int32 prefixLen = strlen(buffer);
	int bufferSize = sizeof(buffer) - prefixLen;

	va_list args;
	va_start(args, format);
	vsnprintf(buffer + prefixLen, bufferSize - 1, format, args);
	va_end(args);

	buffer[sizeof(buffer) - 1] = '\0';

	debugger(buffer);
}


void
kernel_debugger(const char *message)
{
	debugger(message);
}


extern "C" void
dprintf(const char *format,...)
{
	if (!gDebugOutputEnabled)
		return;

	va_list args;
	va_start(args, format);
	printf("\33[34m");
	vprintf(format, args);
	printf("\33[0m");
	fflush(stdout);
	va_end(args);
}


extern "C" void
dprintf_no_syslog(const char *format,...)
{
	if (!gDebugOutputEnabled)
		return;

	va_list args;
	va_start(args, format);
	printf("\33[34m");
	vprintf(format, args);
	printf("\33[0m");
	fflush(stdout);
	va_end(args);
}


extern "C" void
kprintf(const char *format,...)
{
	va_list args;
	va_start(args, format);
	printf("\33[35m");
	vprintf(format, args);
	printf("\33[0m");
	fflush(stdout);
	va_end(args);
}


extern "C" void
dump_block(const char *buffer, int size, const char *prefix)
{
	const int DUMPED_BLOCK_SIZE = 16;
	int i;

	for (i = 0; i < size;) {
		int start = i;

		dprintf(prefix);
		for (; i < start + DUMPED_BLOCK_SIZE; i++) {
			if (!(i % 4))
				dprintf(" ");

			if (i >= size)
				dprintf("  ");
			else
				dprintf("%02x", *(unsigned char *)(buffer + i));
		}
		dprintf("  ");

		for (i = start; i < start + DUMPED_BLOCK_SIZE; i++) {
			if (i < size) {
				char c = buffer[i];

				if (c < 30)
					dprintf(".");
				else
					dprintf("%c", c);
			} else
				break;
		}
		dprintf("\n");
	}
}
