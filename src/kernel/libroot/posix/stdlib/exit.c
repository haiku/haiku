/*
 * Copyright (c) 2002, OpenBeOS Project
 *
 * This file is part of the OpenBeOS system interface.
 * It is distributed under the terms of the OpenBeOS license.
 *
 *
 * Author(s):
 * Daniel Reinhold (danielre@users.sf.net)
 *
 */

#include <syscalls.h>
#include <stdio.h>
#include <stdlib.h>

// ToDo: move this puppy to a more standard location
#include "../stdio/local.h"

// ToDo: this is supposed to be declared in <stdlib.h>
int  atexit(void (*func)(void));



#define _EXIT_STACK_SIZE_ 32

static void (*_Exit_Stack[_EXIT_STACK_SIZE_])(void) = {0};
static int    _Exit_SP = 0;


int
atexit(void (*func)(void))
{
	// push the function pointer onto the exit stack
	
	if (_Exit_SP < _EXIT_STACK_SIZE_) {
		_Exit_Stack[_Exit_SP++] = func;
		return 0;
	}
	
	return -1;
}


void
exit(int status)
{	
	// unwind the exit stack, calling the registered functions
	while (_Exit_SP > 0)
		(*_Exit_Stack[--_Exit_SP])();
	
	// close all open files
	(*__cleanup)();
	
	// exit with status code
	sys_exit(status);
}
