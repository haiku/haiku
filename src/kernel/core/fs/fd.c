/* Operations on file descriptors
** 
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <OS.h>

#include <fd.h>
#include <vfs.h>
#include <syscalls.h>

#include <malloc.h>
#include <string.h>


//#define TRACE_FD
#ifdef TRACE_FD
#	define TRACE(x) dprintf x
#	define PRINT(x) dprintf x
#else
#	define TRACE(x)
#	define PRINT(x)
#endif


/*** General fd routines ***/


#ifdef DEBUG
void dump_fd(int fd, struct file_descriptor *descriptor);

void
dump_fd(int fd,struct file_descriptor *descriptor)
{
	dprintf("fd[%d] = %p: type = %ld, ref_count = %ld, ops = %p, u.vnode = %p, u.mount = %p, cookie = %p, open_mode = %lx, pos = %Ld\n",
		fd, descriptor, descriptor->type, descriptor->ref_count, descriptor->ops,
		descriptor->u.vnode, descriptor->u.mount, descriptor->cookie, descriptor->open_mode, descriptor->pos);
}
#endif

/** Allocates and initializes a new file_descriptor */

struct file_descriptor *
alloc_fd(void)
{
	struct file_descriptor *descriptor;

	descriptor = malloc(sizeof(struct file_descriptor));
	if (descriptor == NULL)
		return NULL;

	descriptor->u.vnode = NULL;
	descriptor->cookie = NULL;
	descriptor->ref_count = 1;
	descriptor->open_mode = 0;
	descriptor->pos = 0;

	return descriptor;
}


int
new_fd(struct io_context *context, struct file_descriptor *descriptor)
{
	int fd = -1;
	uint32 i;

	mutex_lock(&context->io_mutex);

	for (i = 0; i < context->table_size; i++) {
		if (!context->fds[i]) {
			fd = i;
			break;
		}
	}
	if (fd < 0) {
		fd = B_NO_MORE_FDS;
		goto err;
	}

	context->fds[fd] = descriptor;
	context->num_used_fds++;

err:
	mutex_unlock(&context->io_mutex);

	return fd;
}


/**	Reduces the descriptor's reference counter, and frees all resources
 *	if necessary.
 */

void
put_fd(struct file_descriptor *descriptor)
{
	TRACE(("put_fd(descriptor = %p [ref = %ld, cookie = %p])\n", descriptor, descriptor->ref_count, descriptor->cookie));

	// free the descriptor if we don't need it anymore
	if (atomic_add(&descriptor->ref_count, -1) == 1) {
		// close the underlying object (and free any resources allocated there)
		if (descriptor->ops->fd_close)
			descriptor->ops->fd_close(descriptor);
		if (descriptor->ops->fd_free)
			descriptor->ops->fd_free(descriptor);

		free(descriptor);
	}
}


struct file_descriptor *
get_fd(struct io_context *context, int fd)
{
	struct file_descriptor *descriptor = NULL;

	if (fd < 0)
		return NULL;

	mutex_lock(&context->io_mutex);

	if ((uint32)fd < context->table_size)
		descriptor = context->fds[fd];
	
	if (descriptor != NULL) // fd is valid
		atomic_add(&descriptor->ref_count, 1);

	mutex_unlock(&context->io_mutex);

	return descriptor;
}


/**	Removes the file descriptor in the specified slot, and
 *	reduces its reference counter.
 */

static void
remove_fd(struct io_context *context, int fd)
{
	struct file_descriptor *descriptor = NULL;

	if (fd < 0)
		return;

	mutex_lock(&context->io_mutex);

	if ((uint32)fd < context->table_size)
		descriptor = context->fds[fd];

	if (descriptor)	{	// fd is valid
		context->fds[fd] = NULL;
		context->num_used_fds--;
	}

	mutex_unlock(&context->io_mutex);

	if (descriptor)
		put_fd(descriptor);
}


static int
dup_fd(int fd, bool kernel)
{
	struct io_context *context = get_current_io_context(kernel);
	struct file_descriptor *descriptor;
	int status;

	TRACE(("dup_fd: fd = %d\n", fd));

	// Try to get the fd structure
	descriptor = get_fd(context, fd);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	// now put the fd in place
	status = new_fd(context, descriptor);
	if (status < 0)
		put_fd(descriptor);

	return status;
}


static int
dup2_fd(int oldfd, int newfd, bool kernel)
{
	struct file_descriptor *evicted = NULL;
	struct io_context *context;

	TRACE(("dup2_fd: ofd = %d, nfd = %d\n", oldfd, newfd));

	// quick check
	if (oldfd < 0 || newfd < 0)
		return B_FILE_ERROR;

	// Get current I/O context and lock it
	context = get_current_io_context(kernel);
	mutex_lock(&context->io_mutex);

	// Check if the fds are valid (mutex must be locked because
	// the table size could be changed)
	if ((uint32)oldfd >= context->table_size
		|| (uint32)newfd >= context->table_size
		|| context->fds[oldfd] == NULL) {
		mutex_unlock(&context->io_mutex);
		return B_FILE_ERROR;
	}

	// Check for identity, note that it cannot be made above
	// because we always want to return an error on invalid
	// handles
	if (oldfd != newfd) {
		// Now do the work
		evicted = context->fds[newfd];
		atomic_add(&context->fds[oldfd]->ref_count, 1);
		context->fds[newfd] = context->fds[oldfd];
	}

	mutex_unlock(&context->io_mutex);

	// Say bye bye to the evicted fd
	if (evicted)
		put_fd(evicted);

	return newfd;
}


status_t
select_fd(int fd, uint8 event, uint32 ref, struct select_sync *sync, bool kernel)
{
	struct file_descriptor *descriptor;
	status_t status;

	TRACE(("select_fd(fd = %d, event = %u, ref = %lu, selectsync = %p)\n", fd, event, ref, sync));

	descriptor = get_fd(get_current_io_context(kernel), fd);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	if (descriptor->ops->fd_select) {
		status = descriptor->ops->fd_select(descriptor, event, ref, sync);
	} else {
		// if the I/O subsystem doesn't support select(), we will
		// immediately notify the select call
		status = notify_select_event((void *)sync, ref, event);
	}

	put_fd(descriptor);
	return status;
}


status_t
deselect_fd(int fd, uint8 event, struct select_sync *sync, bool kernel)
{
	struct file_descriptor *descriptor;
	status_t status;

	TRACE(("deselect_fd(fd = %d, event = %u, selectsync = %p)\n", fd, event, sync));

	descriptor = get_fd(get_current_io_context(kernel), fd);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	if (descriptor->ops->fd_deselect)
		status = descriptor->ops->fd_deselect(descriptor, event, sync);
	else
		status = B_OK;

	put_fd(descriptor);
	return status;
}


/** This function checks if the specified fd is valid in the current
 *	context. It can be used for a quick check; the fd is not locked
 *	so it could become invalid immediately after this check.
 */

bool
fd_is_valid(int fd, bool kernel)
{
	struct file_descriptor *descriptor = get_fd(get_current_io_context(kernel), fd);
	if (descriptor == NULL)
		return false;
	
	put_fd(descriptor);
	return true;
}


//	#pragma mark -
/*** USER routines ***/ 


ssize_t
_user_read(int fd, off_t pos, void *buffer, size_t length)
{
	struct file_descriptor *descriptor;
	ssize_t bytesRead;

	/* This is a user_function, so abort if we have a kernel address */
	if (!IS_USER_ADDRESS(buffer))
		return B_BAD_ADDRESS;

	descriptor = get_fd(get_current_io_context(false), fd);
	if (!descriptor)
		return B_FILE_ERROR;

	if (pos == -1)
		pos = descriptor->pos;

	if (descriptor->ops->fd_read) {
		bytesRead = descriptor->ops->fd_read(descriptor, pos, buffer, &length);
		if (bytesRead >= B_OK) {
			if (length > SSIZE_MAX)
				bytesRead = SSIZE_MAX;
			else
				bytesRead = (ssize_t)length;

			descriptor->pos = pos + length;
		}
	} else
		bytesRead = B_BAD_VALUE;

	put_fd(descriptor);
	return bytesRead;
}


ssize_t
_user_readv(int fd, off_t pos, const iovec *userVecs, size_t count)
{
	struct file_descriptor *descriptor;
	ssize_t bytesRead = 0;
	status_t status;
	iovec *vecs;
	uint32 i;

	/* This is a user_function, so abort if we have a kernel address */
	if (!IS_USER_ADDRESS(userVecs))
		return B_BAD_ADDRESS;

	vecs = malloc(sizeof(iovec) * count);
	if (vecs == NULL)
		return B_NO_MEMORY;

	if (user_memcpy(vecs, userVecs, sizeof(iovec) * count) < B_OK) {
		status = B_BAD_ADDRESS;
		goto err;
	}

	descriptor = get_fd(get_current_io_context(false), fd);
	if (!descriptor) {
		status = B_FILE_ERROR;
		goto err;
	}

	if (pos == -1)
		pos = descriptor->pos;

	if (descriptor->ops->fd_read) {
		for (i = 0; i < count; i++) {
			size_t length = vecs[i].iov_len;
			status = descriptor->ops->fd_read(descriptor, pos, vecs[i].iov_base, &length);
			if (status < B_OK) {
				bytesRead = status;
				break;
			}

			if ((uint32)bytesRead + length > SSIZE_MAX)
				bytesRead = SSIZE_MAX;
			else
				bytesRead += (ssize_t)length;

			descriptor->pos = pos + length;
		}
	} else
		bytesRead = B_BAD_VALUE;

	put_fd(descriptor);
	return bytesRead;

err:
	free(vecs);
	return status;
}


ssize_t
_user_write(int fd, off_t pos, const void *buffer, size_t length)
{
	struct file_descriptor *descriptor;
	ssize_t bytesWritten = 0;

	if (IS_KERNEL_ADDRESS(buffer))
		return B_BAD_ADDRESS;

	descriptor = get_fd(get_current_io_context(false), fd);
	if (!descriptor)
		return B_FILE_ERROR;

	if (pos == -1)
		pos = descriptor->pos;

	if (descriptor->ops->fd_write) {
		bytesWritten = descriptor->ops->fd_write(descriptor, pos, buffer, &length);
		if (bytesWritten >= B_OK) {
			if (length > SSIZE_MAX)
				bytesWritten = SSIZE_MAX;
			else
				bytesWritten = (ssize_t)length;

			descriptor->pos = pos + length;
		}
	} else
		bytesWritten = B_BAD_VALUE;

	put_fd(descriptor);
	return bytesWritten;
}


ssize_t
_user_writev(int fd, off_t pos, const iovec *userVecs, size_t count)
{
	struct file_descriptor *descriptor;
	ssize_t bytesWritten = 0;
	status_t status;
	iovec *vecs;
	uint32 i;

	/* This is a user_function, so abort if we have a kernel address */
	if (!IS_USER_ADDRESS(userVecs))
		return B_BAD_ADDRESS;

	vecs = malloc(sizeof(iovec) * count);
	if (vecs == NULL)
		return B_NO_MEMORY;

	if (user_memcpy(vecs, userVecs, sizeof(iovec) * count) < B_OK) {
		status = B_BAD_ADDRESS;
		goto err;
	}

	descriptor = get_fd(get_current_io_context(false), fd);
	if (!descriptor) {
		status = B_FILE_ERROR;
		goto err;
	}

	if (pos == -1)
		pos = descriptor->pos;

	if (descriptor->ops->fd_write) {
		for (i = 0; i < count; i++) {
			size_t length = vecs[i].iov_len;
			status = descriptor->ops->fd_write(descriptor, pos, vecs[i].iov_base, &length);
			if (status < B_OK) {
				bytesWritten = status;
				break;
			}

			if ((uint32)bytesWritten + length > SSIZE_MAX)
				bytesWritten = SSIZE_MAX;
			else
				bytesWritten += (ssize_t)length;

			descriptor->pos = pos + length;
		}
	} else
		bytesWritten = B_BAD_VALUE;

	put_fd(descriptor);
	return bytesWritten;

err:
	free(vecs);
	return status;
}


off_t
_user_seek(int fd, off_t pos, int seekType)
{
	struct file_descriptor *descriptor;

	descriptor = get_fd(get_current_io_context(false), fd);
	if (!descriptor)
		return B_FILE_ERROR;

	TRACE(("user_seek(descriptor = %p)\n", descriptor));

	if (descriptor->ops->fd_seek)
		pos = descriptor->ops->fd_seek(descriptor, pos, seekType);
	else
		pos = ESPIPE;

	put_fd(descriptor);
	return pos;
}


status_t
_user_ioctl(int fd, ulong op, void *buffer, size_t length)
{
	struct file_descriptor *descriptor;
	int status;

	if (IS_KERNEL_ADDRESS(buffer))
		return B_BAD_ADDRESS;

	PRINT(("user_ioctl: fd %d\n", fd));

	descriptor = get_fd(get_current_io_context(false), fd);
	if (!descriptor)
		return B_FILE_ERROR;

	if (descriptor->ops->fd_ioctl)
		status = descriptor->ops->fd_ioctl(descriptor, op, buffer, length);
	else
		status = EOPNOTSUPP;

	put_fd(descriptor);
	return status;
}


ssize_t
_user_read_dir(int fd, struct dirent *buffer, size_t bufferSize, uint32 maxCount)
{
	struct file_descriptor *descriptor;
	ssize_t retval;

	if (IS_KERNEL_ADDRESS(buffer))
		return B_BAD_ADDRESS;

	PRINT(("user_read_dir(fd = %d, buffer = %p, bufferSize = %ld, count = %lu)\n", fd, buffer, bufferSize, maxCount));

	descriptor = get_fd(get_current_io_context(false), fd);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	if (descriptor->ops->fd_read_dir) {
		uint32 count = maxCount;
		retval = descriptor->ops->fd_read_dir(descriptor, buffer, bufferSize, &count);
		if (retval >= 0)
			retval = count;
	} else
		retval = EOPNOTSUPP;

	put_fd(descriptor);
	return retval;
}


status_t
_user_rewind_dir(int fd)
{
	struct file_descriptor *descriptor;
	status_t status;

	PRINT(("user_rewind_dir(fd = %d)\n", fd));

	descriptor = get_fd(get_current_io_context(false), fd);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	if (descriptor->ops->fd_rewind_dir)
		status = descriptor->ops->fd_rewind_dir(descriptor);
	else
		status = EOPNOTSUPP;

	put_fd(descriptor);
	return status;
}


status_t
_user_close(int fd)
{
	struct io_context *io = get_current_io_context(false);
	struct file_descriptor *descriptor = get_fd(io, fd);

	if (descriptor == NULL)
		return B_FILE_ERROR;

	TRACE(("user_close(descriptor = %p)\n", descriptor));

	remove_fd(io, fd);

	put_fd(descriptor);
	return B_OK;
}


int
_user_dup(int fd)
{
	return dup_fd(fd, false);
}


int
_user_dup2(int ofd, int nfd)
{
	return dup2_fd(ofd, nfd, false);
}


//	#pragma mark -
/*** SYSTEM functions ***/


ssize_t
_kern_read(int fd, off_t pos, void *buffer, size_t length)
{
	struct file_descriptor *descriptor;
	ssize_t bytesRead;

	descriptor = get_fd(get_current_io_context(true), fd);
	if (!descriptor)
		return B_FILE_ERROR;

	if (pos == -1)
		pos = descriptor->pos;

	if (descriptor->ops->fd_read) {
		bytesRead = descriptor->ops->fd_read(descriptor, pos, buffer, &length);
		if (bytesRead >= B_OK) {
			if (length > SSIZE_MAX)
				bytesRead = SSIZE_MAX;
			else
				bytesRead = (ssize_t)length;

			descriptor->pos = pos + length;
		}
	} else
		bytesRead = B_BAD_VALUE;

	put_fd(descriptor);
	return bytesRead;
}


ssize_t
_kern_readv(int fd, off_t pos, const iovec *vecs, size_t count)
{
	struct file_descriptor *descriptor;
	ssize_t bytesRead = 0;
	status_t status;
	uint32 i;

	descriptor = get_fd(get_current_io_context(false), fd);
	if (!descriptor)
		return B_FILE_ERROR;

	if (pos == -1)
		pos = descriptor->pos;

	if (descriptor->ops->fd_read) {
		for (i = 0; i < count; i++) {
			size_t length = vecs[i].iov_len;
			status = descriptor->ops->fd_read(descriptor, pos, vecs[i].iov_base, &length);
			if (status < B_OK) {
				bytesRead = status;
				break;
			}

			if ((uint32)bytesRead + length > SSIZE_MAX)
				bytesRead = SSIZE_MAX;
			else
				bytesRead += (ssize_t)length;

			descriptor->pos = pos + length;
		}
	} else
		bytesRead = B_BAD_VALUE;

	put_fd(descriptor);
	return bytesRead;
}


ssize_t
_kern_write(int fd, off_t pos, const void *buffer, size_t length)
{
	struct file_descriptor *descriptor;
	ssize_t bytesWritten;

	descriptor = get_fd(get_current_io_context(true), fd);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	if (pos == -1)
		pos = descriptor->pos;

	if (descriptor->ops->fd_write) {
		bytesWritten = descriptor->ops->fd_write(descriptor, pos, buffer, &length);
		if (bytesWritten >= B_OK) {
			if (length > SSIZE_MAX)
				bytesWritten = SSIZE_MAX;
			else
				bytesWritten = (ssize_t)length;

			descriptor->pos = pos + length;
		}
	} else
		bytesWritten = B_BAD_VALUE;

	put_fd(descriptor);
	return bytesWritten;
}


ssize_t
_kern_writev(int fd, off_t pos, const iovec *vecs, size_t count)
{
	struct file_descriptor *descriptor;
	ssize_t bytesWritten = 0;
	status_t status;
	uint32 i;

	descriptor = get_fd(get_current_io_context(false), fd);
	if (!descriptor)
		return B_FILE_ERROR;

	if (pos == -1)
		pos = descriptor->pos;

	if (descriptor->ops->fd_write) {
		for (i = 0; i < count; i++) {
			size_t length = vecs[i].iov_len;
			status = descriptor->ops->fd_write(descriptor, pos, vecs[i].iov_base, &length);
			if (status < B_OK) {
				bytesWritten = status;
				break;
			}

			if ((uint32)bytesWritten + length > SSIZE_MAX)
				bytesWritten = SSIZE_MAX;
			else
				bytesWritten += (ssize_t)length;

			descriptor->pos = pos + length;
		}
	} else
		bytesWritten = B_BAD_VALUE;

	put_fd(descriptor);
	return bytesWritten;
}


off_t
_kern_seek(int fd, off_t pos, int seekType)
{
	struct file_descriptor *descriptor;

	descriptor = get_fd(get_current_io_context(true), fd);
	if (!descriptor)
		return B_FILE_ERROR;

	if (descriptor->ops->fd_seek)
		pos = descriptor->ops->fd_seek(descriptor, pos, seekType);
	else
		pos = ESPIPE;

	put_fd(descriptor);
	return pos;
}


status_t
_kern_ioctl(int fd, ulong op, void *buffer, size_t length)
{
	struct file_descriptor *descriptor;
	int status;

	PRINT(("sys_ioctl: fd %d\n", fd));

	descriptor = get_fd(get_current_io_context(true), fd);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	if (descriptor->ops->fd_ioctl)
		status = descriptor->ops->fd_ioctl(descriptor, op, buffer, length);
	else
		status = EOPNOTSUPP;

	put_fd(descriptor);
	return status;
}


ssize_t
_kern_read_dir(int fd, struct dirent *buffer, size_t bufferSize, uint32 maxCount)
{
	struct file_descriptor *descriptor;
	ssize_t retval;

	PRINT(("sys_read_dir(fd = %d, buffer = %p, bufferSize = %ld, count = %lu)\n",fd, buffer, bufferSize, maxCount));

	descriptor = get_fd(get_current_io_context(true), fd);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	if (descriptor->ops->fd_read_dir) {
		uint32 count = maxCount;
		retval = descriptor->ops->fd_read_dir(descriptor, buffer, bufferSize, &count);
		if (retval >= 0)
			retval = count;
	} else
		retval = EOPNOTSUPP;

	put_fd(descriptor);
	return retval;
}


status_t
_kern_rewind_dir(int fd)
{
	struct file_descriptor *descriptor;
	status_t status;

	PRINT(("sys_rewind_dir(fd = %d)\n",fd));

	descriptor = get_fd(get_current_io_context(true), fd);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	if (descriptor->ops->fd_rewind_dir)
		status = descriptor->ops->fd_rewind_dir(descriptor);
	else
		status = EOPNOTSUPP;

	put_fd(descriptor);
	return status;
}


status_t
_kern_close(int fd)
{
	struct io_context *io = get_current_io_context(true);
	struct file_descriptor *descriptor = get_fd(io, fd);

	if (descriptor == NULL)
		return B_FILE_ERROR;

	remove_fd(io, fd);

	put_fd(descriptor);
	return B_OK;
}


int
_kern_dup(int fd)
{
	return dup_fd(fd, true);
}


int
_kern_dup2(int ofd, int nfd)
{
	return dup2_fd(ofd, nfd, true);
}

