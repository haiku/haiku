/*
 * Copyright 2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <unistd.h>


static int sShellIndex;


char *
getusershell(void)
{
	if (sShellIndex++ == 0)
		return "/bin/sh";

	return NULL;
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


