/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include <stdio.h>
#include "syscalls.h"


// see OS.h for the reason why we need this
thread_id _kfind_thread_(const char *name);

thread_id
_kfind_thread_(const char *name)
{
	// ToDo: _kfind_thread_()
	printf("find_thread(): Not yet implemented!!\n");
	return B_ERROR;
}

