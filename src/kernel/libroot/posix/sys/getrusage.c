/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <sys/resource.h>
#include <syscalls.h>
#include <errno.h>


int
getrusage(int who, struct rusage *rusage)
{
	// ToDo: implement me!
	errno = B_BAD_VALUE;
	return -1;
}

