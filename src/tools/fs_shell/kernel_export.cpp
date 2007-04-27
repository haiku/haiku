/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include "fssh_kernel_export.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "fssh_errors.h"


fssh_thread_id
fssh_spawn_kernel_thread(fssh_thread_func function, const char *threadName,
	int32_t priority, void *arg)
{
	return FSSH_B_ERROR;
}


void
fssh_dprintf(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	vprintf(format, args);

	va_end(args);
}


void
fssh_kprintf(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	vprintf(format, args);

	va_end(args);
}


void
fssh_dump_block(const char *buffer, int size, const char *prefix)
{
}


void
fssh_panic(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	vfprintf(stderr, format, args);
	exit(1);

	va_end(args);
}

void
fssh_kernel_debugger(const char *message)
{
	fssh_panic("%s", message);
}


uint32_t
fssh_parse_expression(const char *string)
{
	return 0;
}

int
fssh_add_debugger_command(char *name, fssh_debugger_command_hook hook,
	char *help)
{
	return 0;
}


int
fssh_remove_debugger_command(char *name, fssh_debugger_command_hook hook)
{
	return 0;
}

