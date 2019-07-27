/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include "fssh_kernel_export.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fssh_errors.h"


fssh_thread_id
fssh_spawn_kernel_thread(fssh_thread_func function, const char *threadName,
	int32_t priority, void *arg)
{
	return FSSH_B_ERROR;
}


fssh_status_t
fssh_wait_for_thread(fssh_thread_id thread, fssh_status_t *_returnCode)
{
	return FSSH_B_ERROR;
}


fssh_status_t
fssh_user_memcpy(void *dest, const void *source, fssh_size_t length)
{
	memcpy(dest, source, length);
	return FSSH_B_OK;
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

	va_end(args);

//	exit(1);
	int* badAddress = 0;
	*badAddress = 42;
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
fssh_add_debugger_command(const char *name, fssh_debugger_command_hook hook,
	const char *help)
{
	return 0;
}


int
fssh_remove_debugger_command(char *name, fssh_debugger_command_hook hook)
{
	return 0;
}

