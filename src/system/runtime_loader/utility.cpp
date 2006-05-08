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
		// Use libroot's getenv() as soon as it is available to us - the environment
		// in gProgramArgs is static.
		return gGetEnv(name);
	}

	char **environ = gProgramArgs->envp;
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
	int length = vsprintf(buffer, format, args);
	va_end(args);

	_kern_write(STDERR_FILENO, 0, buffer, length);

	return length;
}
