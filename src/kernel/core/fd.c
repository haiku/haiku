/* Operations on file descriptors
** 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


//#include <ktypes.h>
#include <OS.h>
#include <fd.h>
#include <vfs.h>
#include <Errors.h>
#include <debug.h>
#include <memheap.h>
#include <string.h>

#define CHECK_USER_ADDR(x) \
	if ((addr)(x) >= KERNEL_BASE && (addr)(x) <= KERNEL_TOP) \
		return B_BAD_ADDRESS;

#define TRACE_FD 0
#if TRACE_FD
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
	dprintf("fd[%d] = %p: type = %d, ref_count = %d, ops = %p, vnode = %p, cookie = %p, dummy = %x\n",
		fd,descriptor,descriptor->type,descriptor->ref_count,descriptor->ops,
		descriptor->vnode,descriptor->cookie,descriptor->dummy);
}
#endif

/** Allocates and initializes a new file_descriptor */

struct file_descriptor *
alloc_fd(void)
{
	struct file_descriptor *descriptor;

	descriptor = kmalloc(sizeof(struct file_descriptor));
	if (descriptor == NULL)
		return NULL;

	descriptor->vnode = NULL;
	descriptor->cookie = NULL;
	descriptor->ref_count = 1;
	descriptor->open_mode = 0;

	return descriptor;
}


int
new_fd(struct io_context *context, struct file_descriptor *descriptor)
{
	int fd = -1;
	int i;

	mutex_lock(&context->io_mutex);

	for (i = 0; i < context->table_size; i++) {
		if (!context->fds[i]) {
			fd = i;
			break;
		}
	}
	if (fd < 0) {
		fd = EMFILE;
		goto err;
	}

	context->fds[fd] = descriptor;
	context->num_used_fds++;

err:
	mutex_unlock(&context->io_mutex);

	return fd;
}


void
put_fd(struct file_descriptor *descriptor)
{
	/* Run a cleanup (fd_free) routine if there is one and free structure, but only
	 * if we've just removed the final reference to it :)
	 */
	if (atomic_add(&descriptor->ref_count, -1) == 1) {
		if (descriptor->ops->fd_close)
			descriptor->ops->fd_close(descriptor);
		if (descriptor->ops->fd_free)
			descriptor->ops->fd_free(descriptor);

		kfree(descriptor);
	}
}


struct file_descriptor *
get_fd(struct io_context *context, int fd)
{
	struct file_descriptor *descriptor = NULL;

	if (fd < 0)
		return NULL;

	mutex_lock(&context->io_mutex);

	if (fd < context->table_size)
		descriptor = context->fds[fd];
	
	if (descriptor != NULL) // fd is valid
		atomic_add(&descriptor->ref_count, 1);

	mutex_unlock(&context->io_mutex);

	return descriptor;
}


void
remove_fd(struct io_context *context, int fd)
{
	struct file_descriptor *descriptor = NULL;

	if (fd < 0)
		return;

	mutex_lock(&context->io_mutex);

	if (fd < context->table_size)
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
fd_dup(int fd, bool kernel)
{
	struct io_context *context = get_current_io_context(kernel);
	struct file_descriptor *descriptor;
	int status;

	TRACE(("fd_dup: fd = %d\n", fd));

	// Try to get the fd structure
	descriptor = get_fd(context, fd);
	if (descriptor == NULL)
		return EBADF;

	// now put the fd in place
	status = new_fd(context, descriptor);
	if (status < 0)
		put_fd(descriptor);

	return status;
}


static int
fd_dup2(int oldfd, int newfd, bool kernel)
{
	struct file_descriptor *evicted = NULL;
	struct io_context *context;

	TRACE(("fd_dup2: ofd = %d, nfd = %d\n", oldfd, newfd));

	// quick check
	if (oldfd < 0 || newfd < 0)
		return EBADF;

	// Get current I/O context and lock it
	context = get_current_io_context(kernel);
	mutex_lock(&context->io_mutex);

	// Check if the fds are valid (mutex must be locked because
	// the table size could be changed)
	if (oldfd >= context->table_size
		|| newfd >= context->table_size
		|| context->fds[oldfd] == NULL) {
		mutex_unlock(&context->io_mutex);
		return EBADF;
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


//	#pragma mark -
/*** USER routines ***/ 


ssize_t
user_read(int fd, off_t pos, void *buffer, size_t length)
{
	struct file_descriptor *descriptor;
	ssize_t retval;

	/* This is a user_function, so abort if we have a kernel address */
	CHECK_USER_ADDR(buffer)

	descriptor = get_fd(get_current_io_context(false), fd);
	if (!descriptor)
		return EBADF;

	if (descriptor->ops->fd_read) {
		retval = descriptor->ops->fd_read(descriptor, pos, buffer, &length);
		if (retval >= 0)
			retval = (ssize_t)length;
	} else
		retval = EINVAL;

	put_fd(descriptor);
	return retval;
}


ssize_t
user_write(int fd, off_t pos, const void *buffer, size_t length)
{
	struct file_descriptor *descriptor;
	ssize_t retval = 0;

	CHECK_USER_ADDR(buffer)

	descriptor = get_fd(get_current_io_context(false), fd);
	if (!descriptor)
		return EBADF;

	if (descriptor->ops->fd_write) {
		retval = descriptor->ops->fd_write(descriptor, pos, buffer, &length);
		if (retval >= 0)
			retval = (ssize_t)length;
	} else
		retval = EINVAL;

	put_fd(descriptor);
	return retval;
}


off_t
user_seek(int fd, off_t pos, int seekType)
{
	struct file_descriptor *descriptor;

	descriptor = get_fd(get_current_io_context(false), fd);
	if (!descriptor)
		return EBADF;

	TRACE(("user_seek(descriptor = %p)\n",descriptor));

	if (descriptor->ops->fd_seek)
		pos = descriptor->ops->fd_seek(descriptor, pos, seekType);
	else
		pos = ESPIPE;

	put_fd(descriptor);
	return pos;
}


int
user_ioctl(int fd, ulong op, void *buffer, size_t length)
{
	struct file_descriptor *descriptor;
	int status;

	CHECK_USER_ADDR(buffer)

	PRINT(("user_ioctl: fd %d\n", fd));

	descriptor = get_fd(get_current_io_context(false), fd);
	if (!descriptor)
		return EBADF;

	if (descriptor->ops->fd_ioctl)
		status = descriptor->ops->fd_ioctl(descriptor, op, buffer, length);
	else
		status = EOPNOTSUPP;

	put_fd(descriptor);
	return status;
}


ssize_t
user_read_dir(int fd, struct dirent *buffer,size_t bufferSize,uint32 maxCount)
{
	struct file_descriptor *descriptor;
	ssize_t retval;

	CHECK_USER_ADDR(buffer)

	PRINT(("user_read_dir(fd = %d, buffer = 0x%p, bufferSize = %ld, count = %d)\n",fd,buffer,bufferSize,maxCount));

	descriptor = get_fd(get_current_io_context(false), fd);
	if (descriptor == NULL)
		return EBADF;

	if (descriptor->ops->fd_read_dir) {
		uint32 count = maxCount;
		retval = descriptor->ops->fd_read_dir(descriptor,buffer,bufferSize,&count);
		if (retval >= 0)
			retval = count;
	} else
		retval = EOPNOTSUPP;

	put_fd(descriptor);
	return retval;
}


status_t
user_rewind_dir(int fd)
{
	struct file_descriptor *descriptor;
	status_t status;

	PRINT(("user_rewind_dir(fd = %d)\n",fd));

	descriptor = get_fd(get_current_io_context(false), fd);
	if (descriptor == NULL)
		return EBADF;

	if (descriptor->ops->fd_rewind_dir)
		status = descriptor->ops->fd_rewind_dir(descriptor);
	else
		status = EOPNOTSUPP;

	put_fd(descriptor);
	return status;
}


int
user_fstat(int fd, struct stat *stat)
{
	struct file_descriptor *descriptor;
	status_t status;

	/* This is a user_function, so abort if we have a kernel address */
	CHECK_USER_ADDR(stat)

	descriptor = get_fd(get_current_io_context(false), fd);
	if (descriptor == NULL)
		return EBADF;

	TRACE(("user_fstat(descriptor = %p)\n",descriptor));

	if (descriptor->ops->fd_stat) {
		// we're using the stat buffer on the stack to not have to
		// lock the given stat buffer in memory
		struct stat kstat;

		status = descriptor->ops->fd_stat(descriptor, &kstat);
		if (status >= 0)
			status = user_memcpy(stat, &kstat, sizeof(*stat));
	} else
		status = EOPNOTSUPP;

	put_fd(descriptor);
	return status;
}


int
user_close(int fd)
{
	struct io_context *io = get_current_io_context(false);
	struct file_descriptor *descriptor = get_fd(io, fd);

	if (descriptor == NULL)
		return EBADF;

	TRACE(("user_close(descriptor = %p)\n",descriptor));

	remove_fd(io, fd);

	put_fd(descriptor);
	return B_OK;
}


int
user_dup(int fd)
{
	return fd_dup(fd, false);
}


int
user_dup2(int ofd, int nfd)
{
	return fd_dup2(ofd, nfd, false);
}


//	#pragma mark -
/*** SYSTEM functions ***/


ssize_t
sys_read(int fd, off_t pos, void *buffer, size_t length)
{
	struct file_descriptor *descriptor;
	ssize_t retval;

	descriptor = get_fd(get_current_io_context(true), fd);
	if (!descriptor)
		return EBADF;

	if (descriptor->ops->fd_read) {
		retval = descriptor->ops->fd_read(descriptor, pos, buffer, &length);
		if (retval >= 0)
			retval = (ssize_t)length;
	} else
		retval = EINVAL;

	put_fd(descriptor);
	return retval;
}


ssize_t
sys_write(int fd, off_t pos, const void *buffer, size_t length)
{
	struct file_descriptor *descriptor;
	ssize_t retval;

	descriptor = get_fd(get_current_io_context(true), fd);
	if (descriptor == NULL)
		return EBADF;

	if (descriptor->ops->fd_write) {
		retval = descriptor->ops->fd_write(descriptor, pos, buffer, &length);
		if (retval >= 0)
			retval = (ssize_t)length;

		PRINT(("sys_write(%d) = %ld (rlen = %ld)\n", fd, retval, length));
	} else
		retval = EINVAL;

	put_fd(descriptor);
	return retval;
}


off_t
sys_seek(int fd, off_t pos, int seekType)
{
	struct file_descriptor *descriptor;

	descriptor = get_fd(get_current_io_context(true), fd);
	if (!descriptor)
		return EBADF;

	if (descriptor->ops->fd_seek)
		pos = descriptor->ops->fd_seek(descriptor, pos, seekType);
	else
		pos = ESPIPE;

	put_fd(descriptor);
	return pos;
}


int
sys_ioctl(int fd, ulong op, void *buffer, size_t length)
{
	struct file_descriptor *descriptor;
	int status;

	PRINT(("sys_ioctl: fd %d\n", fd));

	descriptor = get_fd(get_current_io_context(true), fd);
	if (descriptor == NULL)
		return EBADF;

	if (descriptor->ops->fd_ioctl)
		status = descriptor->ops->fd_ioctl(descriptor, op, buffer, length);
	else
		status = EOPNOTSUPP;

	put_fd(descriptor);
	return status;
}


ssize_t
sys_read_dir(int fd, struct dirent *buffer,size_t bufferSize,uint32 maxCount)
{
	struct file_descriptor *descriptor;
	ssize_t retval;

	PRINT(("sys_read_dir(fd = %d, buffer = 0x%p, bufferSize = %ld, count = %u)\n",fd,buffer,bufferSize,maxCount));

	descriptor = get_fd(get_current_io_context(true), fd);
	if (descriptor == NULL)
		return EBADF;

	if (descriptor->ops->fd_read_dir) {
		uint32 count = maxCount;
		retval = descriptor->ops->fd_read_dir(descriptor,buffer,bufferSize,&count);
		if (retval >= 0)
			retval = count;
	} else
		retval = EOPNOTSUPP;

	put_fd(descriptor);
	return retval;
}


status_t
sys_rewind_dir(int fd)
{
	struct file_descriptor *descriptor;
	status_t status;

	PRINT(("sys_rewind_dir(fd = %d)\n",fd));

	descriptor = get_fd(get_current_io_context(true), fd);
	if (descriptor == NULL)
		return EBADF;

	if (descriptor->ops->fd_rewind_dir)
		status = descriptor->ops->fd_rewind_dir(descriptor);
	else
		status = EOPNOTSUPP;

	put_fd(descriptor);
	return status;
}


int
sys_fstat(int fd, struct stat *stat)
{
	struct file_descriptor *descriptor;
	status_t status;

	descriptor = get_fd(get_current_io_context(true), fd);
	if (descriptor == NULL)
		return EBADF;

	TRACE(("sys_fstat(descriptor = %p)\n",descriptor));

	if (descriptor->ops->fd_stat)
		status = descriptor->ops->fd_stat(descriptor, stat);
	else
		status = EOPNOTSUPP;

	put_fd(descriptor);
	return status;
}


int
sys_close(int fd)
{
	struct io_context *io = get_current_io_context(true);
	struct file_descriptor *descriptor = get_fd(io, fd);

	if (descriptor == NULL)
		return EBADF;

	remove_fd(io, fd);

	put_fd(descriptor);
	return B_OK;
}


int
sys_dup(int fd)
{
	return fd_dup(fd, true);
}


int
sys_dup2(int ofd, int nfd)
{
	return fd_dup2(ofd, nfd, true);
}

