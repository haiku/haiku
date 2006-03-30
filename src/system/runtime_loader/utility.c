/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "runtime_loader_private.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscalls.h>


char *
getenv(const char *name)
{
	// ToDo: this should use the real environ pointer once available!
	//	(or else, any updates to the search paths while the app is running are ignored)
	char **environ = gProgramArgs->envp;
	int32 length = strlen(name);
	int32 i;

	for (i = 0; environ[i] != NULL; i++) {
		if (!strncmp(name, environ[i], length) && environ[i][length] == '=')
			return environ[i] + length + 1;
	}

	return NULL;
}


int
printf(const char *fmt, ...)
{
	va_list args;
	char buf[1024];
	int i;

	va_start(args, fmt);
	i = vsprintf(buf, fmt, args);
	va_end(args);

	_kern_write(2, 0, buf, strlen(buf));

	return i;
}
