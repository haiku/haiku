/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <stdlib.h>
#include <string.h>


extern const char *__progname;

const char *_getprogname(void);


const char *
_getprogname(void)
{
	return __progname;
}


void
setprogname(const char *programName)
{
	const char *slash = strrchr(programName, '/');
	if (slash != NULL)
		__progname = slash + 1;
	else
		__progname = programName;
}

#pragma weak getprogname=_getprogname
