/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "runtime_loader_private.h"

#include <syscalls.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


char *(*gGetEnv)(const char *name) = NULL;


extern "C" char *
getenv(const char *name)
{
	if (gGetEnv != NULL) {
		// Use libroot's getenv() as soon as it is available to us - the
		// environment in gProgramArgs is static.
		return gGetEnv(name);
	}

	char **environ = gProgramArgs->env;
	int32 length = strlen(name);
	int32 i;

	for (i = 0; environ[i] != NULL; i++) {
		if (!strncmp(name, environ[i], length) && environ[i][length] == '=')
			return environ[i] + length + 1;
	}

	return NULL;
}


extern "C" int
printf(const char *format, ...)
{
	char buffer[1024];
	va_list args;

	va_start(args, format);
	int length = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	_kern_write(STDERR_FILENO, 0, buffer, length);

	return length;
}


extern "C" void
dprintf(const char *format, ...)
{
	char buffer[1024];

	va_list list;
	va_start(list, format);

	vsnprintf(buffer, sizeof(buffer), format, list);
	_kern_debug_output(buffer);

	va_end(list);
}


extern "C" uint32
__swap_int32(uint32 value)
{
	return value >> 24 | ((value >> 8) & 0xff00) | value << 24
		| ((value << 8) & 0xff0000);
}


// Copied from libroot/os/thread.c:


extern "C" status_t
_get_thread_info(thread_id thread, thread_info *info, size_t size)
{
	if (info == NULL || size != sizeof(thread_info))
		return B_BAD_VALUE;

	return _kern_get_thread_info(thread, info);
}


extern "C" status_t
snooze(bigtime_t timeout)
{
	return B_ERROR;
}


/*!	Mini atoi(), so we don't have to include the libroot dependencies.
 */
extern "C" int
atoi(const char* num)
{
	int result = 0;
	while (*num >= '0' && *num <= '9') {
		result = (result * 10) + (*num - '0');
		num++;
	}

	return result;
}


#if RUNTIME_LOADER_TRACING

extern "C" void
ktrace_printf(const char *format, ...)
{
	va_list list;
	va_start(list, format);

	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), format, list);
	_kern_ktrace_output(buffer);

	va_end(list);
}

#endif // RUNTIME_LOADER_TRACING
