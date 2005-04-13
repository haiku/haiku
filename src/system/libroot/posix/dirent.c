/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <syscalls.h>

#include <dirent.h>
#include <errno.h>
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

	int fd = _kern_open_dir(-1, path);
	if (fd < 0) {
		errno = fd;
		return NULL;
	}

	/* allocate the memory for the DIR structure */
	if ((dir = (DIR *)malloc(sizeof(DIR) + BUFFER_SIZE)) == NULL) {
		errno = B_NO_MEMORY;
		_kern_close(fd);
		return NULL;
	}

	dir->fd = fd;

	return dir;
}


int
closedir(DIR *dir)
{
	int status = _kern_close(dir->fd);

	free(dir);

	RETURN_AND_SET_ERRNO(status);
}


struct dirent *
readdir(DIR *dir)
{
	/* get the next entry and return a pointer to a dirent structure 
	 * containing the data
	 */
	
	ssize_t count = _kern_read_dir(dir->fd, &dir->ent, BUFFER_SIZE, 1);
	if (count <= 0) {
		if (count < 0)
			errno = count;

		return NULL;
	}
	
	return &dir->ent;
}


int
readdir_r(DIR *dir, struct dirent *entry, struct dirent **_result)
{
	ssize_t count = _kern_read_dir(dir->fd, entry, sizeof(struct dirent) + B_FILE_NAME_LENGTH, 1);
	if (count < B_OK)
		return count;

	if (count == 0) {
		// end of directory
		*_result = NULL;
	} else
		*_result = entry;

	return 0;
}


void
rewinddir(DIR *dir)
{
	status_t status = _kern_rewind_dir(dir->fd);
	if (status < 0)
		errno = status;
}


/** This is no POSIX compatible call; it's not exported in the headers
 *	but here for BeOS compatiblity.
 */

// ToDo: disable this for the kernel build! (will be possible once we use Kernel build rules for the kernel only)

int dirfd(DIR *dir);

int
dirfd(DIR *dir)
{
	return dir->fd;
}
