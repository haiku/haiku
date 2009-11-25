/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

//! Operations on file descriptors

#include "fd.h"

#include <stdlib.h>

#include "fssh_atomic.h"
#include "fssh_fcntl.h"
#include "fssh_kernel_export.h"
#include "fssh_kernel_priv.h"
#include "fssh_string.h"
#include "fssh_uio.h"
#include "syscalls.h"


//#define TRACE_FD
#ifdef TRACE_FD
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


namespace FSShell {


io_context* gKernelIOContext;


/*** General fd routines ***/


#ifdef DEBUG
void dump_fd(int fd, struct file_descriptor *descriptor);

void
dump_fd(int fd,struct file_descriptor *descriptor)
{
	fssh_dprintf("fd[%d] = %p: type = %d, ref_count = %d, ops = %p, u.vnode = %p, u.mount = %p, cookie = %p, open_mode = %x, pos = %Ld\n",
		fd, descriptor, (int)descriptor->type, (int)descriptor->ref_count, descriptor->ops,
		descriptor->u.vnode, descriptor->u.mount, descriptor->cookie, (int)descriptor->open_mode, descriptor->pos);
}
#endif


/** Allocates and initializes a new file_descriptor */

struct file_descriptor *
alloc_fd(void)
{
	struct file_descriptor *descriptor;

	descriptor = (file_descriptor*)malloc(sizeof(struct file_descriptor));
	if (descriptor == NULL)
		return NULL;

	descriptor->u.vnode = NULL;
	descriptor->cookie = NULL;
	descriptor->ref_count = 1;
	descriptor->open_count = 0;
	descriptor->open_mode = 0;
	descriptor->pos = 0;

	return descriptor;
}


bool
fd_close_on_exec(struct io_context *context, int fd)
{
	return CHECK_BIT(context->fds_close_on_exec[fd / 8], fd & 7) ? true : false;
}


void
fd_set_close_on_exec(struct io_context *context, int fd, bool closeFD)
{
	if (closeFD)
		context->fds_close_on_exec[fd / 8] |= (1 << (fd & 7));
	else
		context->fds_close_on_exec[fd / 8] &= ~(1 << (fd & 7));
}


/** Searches a free slot in the FD table of the provided I/O context, and inserts
 *	the specified descriptor into it.
 */

int
new_fd_etc(struct io_context *context, struct file_descriptor *descriptor,
	int firstIndex)
{
	int fd = -1;
	uint32_t i;

	fssh_mutex_lock(&context->io_mutex);

	for (i = firstIndex; i < context->table_size; i++) {
		if (!context->fds[i]) {
			fd = i;
			break;
		}
	}
	if (fd < 0) {
		fd = FSSH_B_NO_MORE_FDS;
		goto err;
	}

	context->fds[fd] = descriptor;
	context->num_used_fds++;
	fssh_atomic_add(&descriptor->open_count, 1);

err:
	fssh_mutex_unlock(&context->io_mutex);

	return fd;
}


int
new_fd(struct io_context *context, struct file_descriptor *descriptor)
{
	return new_fd_etc(context, descriptor, 0);
}


/**	Reduces the descriptor's reference counter, and frees all resources
 *	when it's no longer used.
 */

void
put_fd(struct file_descriptor *descriptor)
{
	int32_t previous = fssh_atomic_add(&descriptor->ref_count, -1);

	TRACE(("put_fd(descriptor = %p [ref = %ld, cookie = %p])\n",
		descriptor, descriptor->ref_count, descriptor->cookie));

	// free the descriptor if we don't need it anymore
	if (previous == 1) {
		// free the underlying object
		if (descriptor->ops != NULL && descriptor->ops->fd_free != NULL)
			descriptor->ops->fd_free(descriptor);

		free(descriptor);
	} else if ((descriptor->open_mode & FSSH_O_DISCONNECTED) != 0
		&& previous - 1 == descriptor->open_count
		&& descriptor->ops != NULL) {
		// the descriptor has been disconnected - it cannot
		// be accessed anymore, let's close it (no one is
		// currently accessing this descriptor)

		if (descriptor->ops->fd_close)
			descriptor->ops->fd_close(descriptor);
		if (descriptor->ops->fd_free)
			descriptor->ops->fd_free(descriptor);

		// prevent this descriptor from being closed/freed again
		descriptor->open_count = -1;
		descriptor->ref_count = -1;
		descriptor->ops = NULL;
		descriptor->u.vnode = NULL;

		// the file descriptor is kept intact, so that it's not
		// reused until someone explicetly closes it
	}
}


/**	Decrements the open counter of the file descriptor and invokes
 *	its close hook when appropriate.
 */

void
close_fd(struct file_descriptor *descriptor)
{
	if (fssh_atomic_add(&descriptor->open_count, -1) == 1) {
		vfs_unlock_vnode_if_locked(descriptor);

		if (descriptor->ops != NULL && descriptor->ops->fd_close != NULL)
			descriptor->ops->fd_close(descriptor);
	}
}


/**	This descriptor's underlying object will be closed and freed
 *	as soon as possible (in one of the next calls to put_fd() -
 *	get_fd() will no longer succeed on this descriptor).
 *	This is useful if the underlying object is gone, for instance
 *	when a (mounted) volume got removed unexpectedly.
 */

void
disconnect_fd(struct file_descriptor *descriptor)
{
	descriptor->open_mode |= FSSH_O_DISCONNECTED;
}


void
inc_fd_ref_count(struct file_descriptor *descriptor)
{
	fssh_atomic_add(&descriptor->ref_count, 1);
}


struct file_descriptor *
get_fd(struct io_context *context, int fd)
{
	struct file_descriptor *descriptor = NULL;

	if (fd < 0)
		return NULL;

	fssh_mutex_lock(&context->io_mutex);

	if ((uint32_t)fd < context->table_size)
		descriptor = context->fds[fd];

	if (descriptor != NULL) {
		// Disconnected descriptors cannot be accessed anymore
		if (descriptor->open_mode & FSSH_O_DISCONNECTED)
			descriptor = NULL;
		else
			inc_fd_ref_count(descriptor);
	}

	fssh_mutex_unlock(&context->io_mutex);

	return descriptor;
}


/**	Removes the file descriptor from the specified slot.
 */

static struct file_descriptor *
remove_fd(struct io_context *context, int fd)
{
	struct file_descriptor *descriptor = NULL;

	if (fd < 0)
		return NULL;

	fssh_mutex_lock(&context->io_mutex);

	if ((uint32_t)fd < context->table_size)
		descriptor = context->fds[fd];

	if (descriptor)	{
		// fd is valid
		context->fds[fd] = NULL;
		fd_set_close_on_exec(context, fd, false);
		context->num_used_fds--;

		if (descriptor->open_mode & FSSH_O_DISCONNECTED)
			descriptor = NULL;
	}

	fssh_mutex_unlock(&context->io_mutex);

	return descriptor;
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
		return FSSH_B_FILE_ERROR;

	// now put the fd in place
	status = new_fd(context, descriptor);
	if (status < 0)
		put_fd(descriptor);
	else {
		fssh_mutex_lock(&context->io_mutex);
		fd_set_close_on_exec(context, status, false);
		fssh_mutex_unlock(&context->io_mutex);
	}

	return status;
}


/**	POSIX says this should be the same as:
 *		close(newfd);
 *		fcntl(oldfd, F_DUPFD, newfd);
 *
 *	We do dup2() directly to be thread-safe.
 */

static int
dup2_fd(int oldfd, int newfd, bool kernel)
{
	struct file_descriptor *evicted = NULL;
	struct io_context *context;

	TRACE(("dup2_fd: ofd = %d, nfd = %d\n", oldfd, newfd));

	// quick check
	if (oldfd < 0 || newfd < 0)
		return FSSH_B_FILE_ERROR;

	// Get current I/O context and lock it
	context = get_current_io_context(kernel);
	fssh_mutex_lock(&context->io_mutex);

	// Check if the fds are valid (mutex must be locked because
	// the table size could be changed)
	if ((uint32_t)oldfd >= context->table_size
		|| (uint32_t)newfd >= context->table_size
		|| context->fds[oldfd] == NULL) {
		fssh_mutex_unlock(&context->io_mutex);
		return FSSH_B_FILE_ERROR;
	}

	// Check for identity, note that it cannot be made above
	// because we always want to return an error on invalid
	// handles
	if (oldfd != newfd) {
		// Now do the work
		evicted = context->fds[newfd];
		fssh_atomic_add(&context->fds[oldfd]->ref_count, 1);
		fssh_atomic_add(&context->fds[oldfd]->open_count, 1);
		context->fds[newfd] = context->fds[oldfd];

		if (evicted == NULL)
			context->num_used_fds++;
	}

	fd_set_close_on_exec(context, newfd, false);

	fssh_mutex_unlock(&context->io_mutex);

	// Say bye bye to the evicted fd
	if (evicted) {
		close_fd(evicted);
		put_fd(evicted);
	}

	return newfd;
}


fssh_status_t
select_fd(int fd, uint8_t event, uint32_t ref, struct select_sync *sync, bool kernel)
{
// 	struct file_descriptor *descriptor;
// 	fssh_status_t status;
//
// 	TRACE(("select_fd(fd = %d, event = %u, ref = %lu, selectsync = %p)\n", fd, event, ref, sync));
//
// 	descriptor = get_fd(get_current_io_context(kernel), fd);
// 	if (descriptor == NULL)
// 		return FSSH_B_FILE_ERROR;
//
// 	if (descriptor->ops->fd_select) {
// 		status = descriptor->ops->fd_select(descriptor, event, ref, sync);
// 	} else {
// 		// if the I/O subsystem doesn't support select(), we will
// 		// immediately notify the select call
// 		status = notify_select_event((void *)sync, ref, event);
// 	}
//
// 	put_fd(descriptor);
// 	return status;

	return FSSH_B_BAD_VALUE;
}


fssh_status_t
deselect_fd(int fd, uint8_t event, struct select_sync *sync, bool kernel)
{
// 	struct file_descriptor *descriptor;
// 	fssh_status_t status;
//
// 	TRACE(("deselect_fd(fd = %d, event = %u, selectsync = %p)\n", fd, event, sync));
//
// 	descriptor = get_fd(get_current_io_context(kernel), fd);
// 	if (descriptor == NULL)
// 		return FSSH_B_FILE_ERROR;
//
// 	if (descriptor->ops->fd_deselect)
// 		status = descriptor->ops->fd_deselect(descriptor, event, sync);
// 	else
// 		status = FSSH_B_OK;
//
// 	put_fd(descriptor);
// 	return status;

	return FSSH_B_BAD_VALUE;
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


struct vnode *
fd_vnode(struct file_descriptor *descriptor)
{
	switch (descriptor->type) {
		case FDTYPE_FILE:
		case FDTYPE_DIR:
		case FDTYPE_ATTR_DIR:
		case FDTYPE_ATTR:
			return descriptor->u.vnode;
	}

	return NULL;
}


static fssh_status_t
common_close(int fd, bool kernel)
{
	struct io_context *io = get_current_io_context(kernel);
	struct file_descriptor *descriptor = remove_fd(io, fd);

	if (descriptor == NULL)
		return FSSH_B_FILE_ERROR;

#ifdef TRACE_FD
	if (!kernel)
		TRACE(("_user_close(descriptor = %p)\n", descriptor));
#endif

	close_fd(descriptor);
	put_fd(descriptor);
		// the reference associated with the slot

	return FSSH_B_OK;
}


//	#pragma mark -
//	Kernel calls


fssh_ssize_t
_kern_read(int fd, fssh_off_t pos, void *buffer, fssh_size_t length)
{
	struct file_descriptor *descriptor;
	fssh_ssize_t bytesRead;

	descriptor = get_fd(get_current_io_context(true), fd);
	if (!descriptor)
		return FSSH_B_FILE_ERROR;
	if ((descriptor->open_mode & FSSH_O_RWMASK) == FSSH_O_WRONLY) {
		put_fd(descriptor);
		return FSSH_B_FILE_ERROR;
	}

	if (pos == -1)
		pos = descriptor->pos;

	if (descriptor->ops->fd_read) {
		bytesRead = descriptor->ops->fd_read(descriptor, pos, buffer, &length);
		if (bytesRead >= FSSH_B_OK) {
			if (length > FSSH_SSIZE_MAX)
				bytesRead = FSSH_SSIZE_MAX;
			else
				bytesRead = (fssh_ssize_t)length;

			descriptor->pos = pos + length;
		}
	} else
		bytesRead = FSSH_B_BAD_VALUE;

	put_fd(descriptor);
	return bytesRead;
}


fssh_ssize_t
_kern_readv(int fd, fssh_off_t pos, const fssh_iovec *vecs, fssh_size_t count)
{
	struct file_descriptor *descriptor;
	fssh_ssize_t bytesRead = 0;
	fssh_status_t status;
	uint32_t i;

	descriptor = get_fd(get_current_io_context(true), fd);
	if (!descriptor)
		return FSSH_B_FILE_ERROR;
	if ((descriptor->open_mode & FSSH_O_RWMASK) == FSSH_O_WRONLY) {
		put_fd(descriptor);
		return FSSH_B_FILE_ERROR;
	}

	if (pos == -1)
		pos = descriptor->pos;

	if (descriptor->ops->fd_read) {
		for (i = 0; i < count; i++) {
			fssh_size_t length = vecs[i].iov_len;
			status = descriptor->ops->fd_read(descriptor, pos, vecs[i].iov_base, &length);
			if (status < FSSH_B_OK) {
				bytesRead = status;
				break;
			}

			if ((uint32_t)bytesRead + length > FSSH_SSIZE_MAX)
				bytesRead = FSSH_SSIZE_MAX;
			else
				bytesRead += (fssh_ssize_t)length;

			pos += vecs[i].iov_len;
		}
	} else
		bytesRead = FSSH_B_BAD_VALUE;

	descriptor->pos = pos;
	put_fd(descriptor);
	return bytesRead;
}


fssh_ssize_t
_kern_write(int fd, fssh_off_t pos, const void *buffer, fssh_size_t length)
{
	struct file_descriptor *descriptor;
	fssh_ssize_t bytesWritten;

	descriptor = get_fd(get_current_io_context(true), fd);
	if (descriptor == NULL)
		return FSSH_B_FILE_ERROR;
	if ((descriptor->open_mode & FSSH_O_RWMASK) == FSSH_O_RDONLY) {
		put_fd(descriptor);
		return FSSH_B_FILE_ERROR;
	}

	if (pos == -1)
		pos = descriptor->pos;

	if (descriptor->ops->fd_write) {
		bytesWritten = descriptor->ops->fd_write(descriptor, pos, buffer, &length);
		if (bytesWritten >= FSSH_B_OK) {
			if (length > FSSH_SSIZE_MAX)
				bytesWritten = FSSH_SSIZE_MAX;
			else
				bytesWritten = (fssh_ssize_t)length;

			descriptor->pos = pos + length;
		}
	} else
		bytesWritten = FSSH_B_BAD_VALUE;

	put_fd(descriptor);
	return bytesWritten;
}


fssh_ssize_t
_kern_writev(int fd, fssh_off_t pos, const fssh_iovec *vecs, fssh_size_t count)
{
	struct file_descriptor *descriptor;
	fssh_ssize_t bytesWritten = 0;
	fssh_status_t status;
	uint32_t i;

	descriptor = get_fd(get_current_io_context(true), fd);
	if (!descriptor)
		return FSSH_B_FILE_ERROR;
	if ((descriptor->open_mode & FSSH_O_RWMASK) == FSSH_O_RDONLY) {
		put_fd(descriptor);
		return FSSH_B_FILE_ERROR;
	}

	if (pos == -1)
		pos = descriptor->pos;

	if (descriptor->ops->fd_write) {
		for (i = 0; i < count; i++) {
			fssh_size_t length = vecs[i].iov_len;
			status = descriptor->ops->fd_write(descriptor, pos, vecs[i].iov_base, &length);
			if (status < FSSH_B_OK) {
				bytesWritten = status;
				break;
			}

			if ((uint32_t)bytesWritten + length > FSSH_SSIZE_MAX)
				bytesWritten = FSSH_SSIZE_MAX;
			else
				bytesWritten += (fssh_ssize_t)length;

			pos += vecs[i].iov_len;
		}
	} else
		bytesWritten = FSSH_B_BAD_VALUE;

	descriptor->pos = pos;
	put_fd(descriptor);
	return bytesWritten;
}


fssh_off_t
_kern_seek(int fd, fssh_off_t pos, int seekType)
{
	struct file_descriptor *descriptor;

	descriptor = get_fd(get_current_io_context(true), fd);
	if (!descriptor)
		return FSSH_B_FILE_ERROR;

	if (descriptor->ops->fd_seek)
		pos = descriptor->ops->fd_seek(descriptor, pos, seekType);
	else
		pos = FSSH_ESPIPE;

	put_fd(descriptor);
	return pos;
}


fssh_status_t
_kern_ioctl(int fd, uint32_t op, void *buffer, fssh_size_t length)
{
	struct file_descriptor *descriptor;
	int status;

	TRACE(("sys_ioctl: fd %d\n", fd));

	descriptor = get_fd(get_current_io_context(true), fd);
	if (descriptor == NULL)
		return FSSH_B_FILE_ERROR;

	if (descriptor->ops->fd_ioctl)
		status = descriptor->ops->fd_ioctl(descriptor, op, buffer, length);
	else
		status = FSSH_EOPNOTSUPP;

	put_fd(descriptor);
	return status;
}


fssh_ssize_t
_kern_read_dir(int fd, struct fssh_dirent *buffer, fssh_size_t bufferSize, uint32_t maxCount)
{
	struct file_descriptor *descriptor;
	fssh_ssize_t retval;

	TRACE(("sys_read_dir(fd = %d, buffer = %p, bufferSize = %ld, count = %lu)\n",fd, buffer, bufferSize, maxCount));

	descriptor = get_fd(get_current_io_context(true), fd);
	if (descriptor == NULL)
		return FSSH_B_FILE_ERROR;

	if (descriptor->ops->fd_read_dir) {
		uint32_t count = maxCount;
		retval = descriptor->ops->fd_read_dir(descriptor, buffer, bufferSize, &count);
		if (retval >= 0)
			retval = count;
	} else
		retval = FSSH_EOPNOTSUPP;

	put_fd(descriptor);
	return retval;
}


fssh_status_t
_kern_rewind_dir(int fd)
{
	struct file_descriptor *descriptor;
	fssh_status_t status;

	TRACE(("sys_rewind_dir(fd = %d)\n",fd));

	descriptor = get_fd(get_current_io_context(true), fd);
	if (descriptor == NULL)
		return FSSH_B_FILE_ERROR;

	if (descriptor->ops->fd_rewind_dir)
		status = descriptor->ops->fd_rewind_dir(descriptor);
	else
		status = FSSH_EOPNOTSUPP;

	put_fd(descriptor);
	return status;
}


fssh_status_t
_kern_close(int fd)
{
	return common_close(fd, true);
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

}	// namespace FSShell
