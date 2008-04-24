/*
 * Copyright 2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <unistd.h>

static const char* const kShells[] = {
	"/bin/sh",
	"/bin/bash",
	NULL
};
static int sShellIndex;


char *
getusershell(void)
{
	if (kShells[sShellIndex] == NULL)
		return NULL;

	return (char*)kShells[sShellIndex++];
}


void
endusershell(void)
{
	sShellIndex = 0;
}


void
setusershell(void)
{
	sShellIndex = 0;
}


