/*
** Copyright 2001, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <unistd.h>

#include <errno.h>
#include <pthread.h>

#include <syscall_utils.h>

#include <syscalls.h>


int
close(int fd)
{
	RETURN_AND_SET_ERRNO_TEST_CANCEL(_kern_close(fd));
}
