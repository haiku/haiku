/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#include <unistd.h>
#include <syscalls.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#define BUFFER_SIZE 4096


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


DIR *
opendir(const char *path)
{
	DIR *dir;

	int fd = sys_open_dir(path);
	if (fd < 0)
		return NULL;

	/* allocate the memory for the DIR structure */
	if ((dir = (DIR*)malloc(sizeof(DIR) + BUFFER_SIZE)) == NULL) {
		errno = B_NO_MEMORY;
		sys_close(fd);
		return NULL;
	}

	dir->fd = fd;

	return dir;
}


int
closedir(DIR *dir)
{
	int status = sys_close(dir->fd);

	free(dir);

	RETURN_AND_SET_ERRNO(status);
}


struct dirent *
readdir(DIR *dir)
{
	/* get the next entry and return a pointer to a dirent structure 
	 * containing the data
	 */
	
	ssize_t count = sys_read_dir(dir->fd, &dir->ent, BUFFER_SIZE, 1);
	if (count <= 0) {
		if (count < 0)
			errno = count;

		return NULL;
	}
	
	return &dir->ent;
}


void
rewinddir(DIR *dirp)
{
	status_t status;
	
	status = sys_rewind_dir(dirp->fd);
	if (status < 0)
		errno = status;
}


int 
chdir(const char *path)
{
	int status = sys_setcwd(-1, path);

	RETURN_AND_SET_ERRNO(status);
}


int 
fchdir(int fd)
{
	int status = sys_setcwd(fd, NULL);

	RETURN_AND_SET_ERRNO(status);
}


char *
getcwd(char *buffer, size_t size)
{
	int status = sys_getcwd(buffer, size);
	if (status < B_OK) {
		errno = status;
		return NULL;
	}
	return buffer;
}


int
rmdir(const char *path)
{
	int status = sys_remove_dir(path);

	RETURN_AND_SET_ERRNO(status);
}

