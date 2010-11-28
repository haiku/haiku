/* 
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdlib.h>
#include <unistd.h>
#include <syscalls.h>


extern void _IO_cleanup(void);

void
_exit(int status)
{
	// close all open files
	_IO_cleanup();

	// exit with status code
	_kern_exit_team(status);
}


void
_Exit(int status)
{
	_exit(status);
}