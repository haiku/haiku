/* 
** Copyright 2001, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <unistd.h>
#include <syscalls.h>


int
dup2(int ofd, int nfd)
{
	int retval;

	retval= sys_dup2(ofd, nfd);

	if(retval< 0) {
		// set errno
	}

	return retval;
}
