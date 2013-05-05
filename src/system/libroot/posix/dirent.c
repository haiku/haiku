/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <dirent.h>
#include <dirent_private.h>

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <errno_private.h>
#include <syscalls.h>
#include <syscall_utils.h>


#define DIR_BUFFER_SIZE	4096


struct __DIR {
	int				fd;
	short			next_entry;
	unsigned short	entries_left;
	long			seek_position;
	long			current_position;
	struct dirent	first_entry;
};


static int
do_seek_dir(DIR* dir)
{
	if (dir->seek_position == dir->current_position)
		return 0;

	// If the seek position lies before the current position (the usual case),
	// rewind to the beginning.
	if (dir->seek_position < dir->current_position) {
		status_t status = _kern_rewind_dir(dir->fd);
		if (status < 0) {
			__set_errno(status);
			return -1;
		}

		dir->current_position = 0;
		dir->entries_left = 0;
	}

	// Now skip entries until we have reached seek_position.
	while (dir->seek_position > dir->current_position) {
		ssize_t count;
		long toSkip = dir->seek_position - dir->current_position;
		if (toSkip == dir->entries_left) {
			// we have to skip exactly all of the currently buffered entries
			dir->current_position = dir->seek_position;
			dir->entries_left = 0;
			return 0;
		}

		if (toSkip < dir->entries_left) {
			// we have to skip only some of the buffered entries
			for (; toSkip > 0; toSkip--) {
				struct dirent* entry = (struct dirent*)
					((uint8*)&dir->first_entry + dir->next_entry);
				dir->entries_left--;
				dir->next_entry += entry->d_reclen;
			}

			dir->current_position = dir->seek_position;
			return 0;
		}

		// we have to skip more than the currently buffered entries
		dir->current_position += dir->entries_left;
		dir->entries_left = 0;

		count = _kern_read_dir(dir->fd, &dir->first_entry,
			(char*)dir + DIR_BUFFER_SIZE - (char*)&dir->first_entry, USHRT_MAX);
		if (count <= 0) {
			if (count < 0)
				__set_errno(count);

			// end of directory
			return -1;
		}

		dir->next_entry = 0;
		dir->entries_left = count;
	}

	return 0;
}


// #pragma mark - private API


DIR*
__create_dir_struct(int fd)
{
	/* allocate the memory for the DIR structure */

	DIR* dir = (DIR*)malloc(DIR_BUFFER_SIZE);
	if (dir == NULL) {
		__set_errno(B_NO_MEMORY);
		return NULL;
	}

	dir->fd = fd;
	dir->entries_left = 0;
	dir->seek_position = 0;
	dir->current_position = 0;

	return dir;
}


// #pragma mark - public API


DIR*
fdopendir(int fd)
{
	DIR* dir;

	// Since our standard file descriptors can't be used as directory file
	// descriptors, we have to open a fresh one explicitly.
	int dirFD = _kern_open_dir(fd, NULL);
	if (dirFD < 0) {
		__set_errno(dirFD);
		return NULL;
	}

	// Since applications are allowed to use the file descriptor after a call
	// to fdopendir() without changing its state (like for other *at()
	// functions), we cannot close it now.
	// We dup2() the new FD to the previous location instead.
	if (dup2(dirFD, fd) == -1)
		close(fd);
	else {
		close(dirFD);
		dirFD = fd;
		fcntl(dirFD, F_SETFD, FD_CLOEXEC);
			// reset close-on-exec which is cleared by dup()
	}

	dir = __create_dir_struct(dirFD);
	if (dir == NULL) {
		close(dirFD);
		return NULL;
	}

	return dir;
}


DIR*
opendir(const char* path)
{
	DIR* dir;

	int fd = _kern_open_dir(-1, path);
	if (fd < 0) {
		__set_errno(fd);
		return NULL;
	}

	// allocate the DIR structure
	if ((dir = __create_dir_struct(fd)) == NULL) {
		_kern_close(fd);
		return NULL;
	}

	return dir;
}


int
closedir(DIR* dir)
{
	int status;

	if (dir == NULL) {
		__set_errno(B_BAD_VALUE);
		return -1;
	}

	status = _kern_close(dir->fd);

	free(dir);

	RETURN_AND_SET_ERRNO(status);
}


struct dirent*
readdir(DIR* dir)
{
	ssize_t count;

	if (dir->seek_position != dir->current_position) {
		if (do_seek_dir(dir) != 0)
			return NULL;
	}

	if (dir->entries_left > 0) {
		struct dirent *dirent
			= (struct dirent *)((uint8 *)&dir->first_entry + dir->next_entry);

		dir->entries_left--;
		dir->next_entry += dirent->d_reclen;
		dir->seek_position++;
		dir->current_position++;

		return dirent;
	}

	// we need to retrieve new entries

	count = _kern_read_dir(dir->fd, &dir->first_entry,
		(char*)dir + DIR_BUFFER_SIZE - (char*)&dir->first_entry, USHRT_MAX);
	if (count <= 0) {
		if (count < 0)
			__set_errno(count);

		// end of directory
		return NULL;
	}

	dir->entries_left = count - 1;
	dir->next_entry = dir->first_entry.d_reclen;
	dir->seek_position++;
	dir->current_position++;

	return &dir->first_entry;
}


int
readdir_r(DIR* dir, struct dirent* entry, struct dirent** _result)
{
	ssize_t count = _kern_read_dir(dir->fd, entry, sizeof(struct dirent)
		+ B_FILE_NAME_LENGTH, 1);
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
rewinddir(DIR* dir)
{
	dir->seek_position = 0;
}


void
seekdir(DIR* dir, long int position)
{
	dir->seek_position = position;
}


long int
telldir(DIR* dir)
{
	return dir->seek_position;
}


int
dirfd(DIR* dir)
{
	return dir->fd;
}


int
alphasort(const struct dirent** entry1, const struct dirent** entry2)
{
	return strcmp((*entry1)->d_name, (*entry2)->d_name);
}


int
scandir(const char* path, struct dirent*** _entryArray,
	int (*selectFunc)(const struct dirent*),
	int (*compareFunc)(const struct dirent** entry1,
		const struct dirent** entry2))
{
	struct dirent** array = NULL;
	size_t arrayCapacity = 0;
	size_t arrayCount = 0;

	DIR* dir = opendir(path);
	if (dir == NULL)
		return -1;

	while (true) {
		struct dirent* copiedEntry;

		struct dirent* entry = readdir(dir);
		if (entry == NULL)
			break;

		// Check whether or not we should include this entry
		if (selectFunc != NULL && !selectFunc(entry))
			continue;

		copiedEntry = malloc(entry->d_reclen);
		if (copiedEntry == NULL)
			goto error;

		memcpy(copiedEntry, entry, entry->d_reclen);

		// Put it into the array

		if (arrayCount == arrayCapacity) {
			struct dirent** newArray;

			// Enlarge array
			if (arrayCapacity == 0)
				arrayCapacity = 64;
			else
				arrayCapacity *= 2;

			newArray = realloc(array, arrayCapacity * sizeof(void*));
			if (newArray == NULL) {
				free(copiedEntry);
				goto error;
			}

			array = newArray;
		}

		array[arrayCount++] = copiedEntry;
	}

	closedir(dir);

	if (arrayCount > 0 && compareFunc != NULL) {
		qsort(array, arrayCount, sizeof(void*),
			(int (*)(const void*, const void*))compareFunc);
	}

	*_entryArray = array;
	return arrayCount;

error:
	closedir(dir);

	while (arrayCount-- > 0)
		free(array[arrayCount]);
	free(array);

	return -1;
}
