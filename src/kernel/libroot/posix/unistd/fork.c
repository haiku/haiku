/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <syscalls.h>

#include <unistd.h>
#include <stdio.h>
#include <errno.h>


pid_t
fork(void)
{
	// ToDo: implement me
	// ToDo: atfork()
	fprintf(stderr, "fork(): NOT IMPLEMENTED\n");
	return -1;
}

