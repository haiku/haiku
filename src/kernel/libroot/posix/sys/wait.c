/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <syscalls.h>

#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>


// ToDo: properly implement waitpid(), and waitid()


pid_t
wait(int *_status)
{
	return waitpid(-1, _status, 0);
}


pid_t
waitpid(pid_t pid, int *_status, int options)
{
	fprintf(stderr, "waitpid(pid = %ld, options = %d): NOT IMPLEMENTED\n", pid, options);
	return -1;
}


int
waitid(idtype_t idType, id_t id, siginfo_t *info, int options)
{
	fprintf(stderr, "waitid(): NOT IMPLEMENTED\n");
	return -1;
}

