/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <unistd.h>
#include <syscalls.h>
#include <sys/resource.h>
#include <errno.h>


int 
getdtablesize(void)
{
	struct rlimit rlimit;
	if (getrlimit(RLIMIT_NOFILE, &rlimit) < 0)
		return 0;

	return rlimit.rlim_cur;
}


long 
sysconf(int name)
{
	// ToDo: implement sysconf()
	return -1;
}


long 
fpathconf(int fd, int name)
{
	// ToDo: implement fpathconf()
	return -1;
}


long 
pathconf(const char *path, int name)
{
	// ToDo: implement pathconf()
	return -1;
}

