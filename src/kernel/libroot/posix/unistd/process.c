/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <unistd.h>
#include <syscalls.h>
#include <string.h>
#include <errno.h>


// ToDo: implement the process ID functions for real!


pid_t 
getpgrp(void)
{
	return 0;
}


pid_t 
getpid(void)
{
	return 0;
}


pid_t 
getppid(void)
{
	return 0;
}


int 
setpgid(pid_t pid, pid_t pgid)
{
	return 0;
}


pid_t 
setsid(void)
{
	return EPERM;
}


