/*
 *  Copyright (c) 2002, OpenBeOS Project.
 *  All rights reserved.
 *  Distributed under the terms of the OpenBeOS license. 
 *
 *
 *  exit.c:
 *  implements the standard C library functions:
 *    abort, atexit, exit
 *
 *
 *  Author(s):
 *  Daniel Reinhold (danielre@users.sf.net)
 *
 */

#include <syscalls.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>

// ToDo: move this puppy to a more standard location
#include "../stdio/local.h"

extern void _thread_do_exit_notification(void);


static void (*_Exit_Stack[ATEXIT_MAX])(void) = {0};
static int    _Exit_SP = 0;



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
	
	if (_Exit_SP < ATEXIT_MAX) {
		_Exit_Stack[_Exit_SP++] = func;
		return 0;
	}
	
	return -1;
}


void
exit(int status)
{
	// BeOS on exit notification for the main thread
	_thread_do_exit_notification();

	// unwind the exit stack, calling the registered functions
	while (_Exit_SP > 0)
		(*_Exit_Stack[--_Exit_SP])();
	
	// close all open files
	(*__cleanup)();
	
	// exit with status code
	sys_exit(status);
}
