/* 
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include "syscalls.h"

#include <stdio.h>
#include <stdlib.h>


void
debugger(const char *message)
{
	// ToDo: implement debugger() properly
	puts("*** debugger call - not yet implemented ***");
	printf("%s\n", message);

	abort();
}


const int
disable_debugger(int state)
{
	// ToDo: implement disable_debugger()
	return 0;
}

