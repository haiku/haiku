/*
** Copyright 2002-2004, The Haiku Team. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Author(s): Daniel Reinhold (danielre@users.sf.net)
**            Axel Dörfler, axeld@pinc-software.de
**
*/

/* implements the standard C library functions:
 * abort, atexit, exit
 */


#include <syscalls.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>


extern void _IO_cleanup(void);
extern void _thread_do_exit_notification(void);

static void (*sExitStack[ATEXIT_MAX])(void) = {0};
static int32 sExitStackIndex = 0;


void
abort()
{
	raise(SIGABRT);
	exit(EXIT_FAILURE);
}


int
atexit(void (*func)(void))
{
	// push the function pointer onto the exit stack
	int32 index = atomic_add(&sExitStackIndex, 1);

	if (index >= ATEXIT_MAX)
		return -1;

	sExitStack[index] = func;
	return 0;
}


void
exit(int status)
{
	// BeOS on exit notification for the main thread
	_thread_do_exit_notification();

	// unwind the exit stack, calling the registered functions
	while (sExitStackIndex-- > 0)
		(*sExitStack[sExitStackIndex])();

	// close all open files
	_IO_cleanup();

	// exit with status code
	_kern_exit(status);
}

