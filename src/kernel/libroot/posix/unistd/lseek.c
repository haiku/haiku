/* 
** Copyright 2001, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <unistd.h>
#include <syscalls.h>


off_t
lseek(int fd, off_t pos, int whence)
{
	return sys_seek(fd, pos, whence);
}
