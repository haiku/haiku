/* fd.c
 *
 * Operations on file descriptors...
 * see fd.h for the definitions
 *
 */

#include <ktypes.h>
#include <OS.h>
#include <fd.h>
#include <vfs.h>
#include <Errors.h>
#include <debug.h>
#include <memheap.h>
#include <string.h>

#define CHECK_USER_ADDR(x) \
	if ((addr)(x) >= KERNEL_BASE && (addr)(x) <= KERNEL_TOP) \
		return EINVAL; 

#define CHECK_SYS_ADDR(x) \
	if ((addr)(x) < KERNEL_BASE && (addr)(x) >= KERNEL_TOP) \
		return EINVAL; 


#define PRINT(x) dprintf x


/*** General fd routines ***/


/** Allocates and initializes a new file_descriptor */

struct file_descriptor *
alloc_fd(void)
{
	struct file_descriptor *f;

	f = kmalloc(sizeof(struct file_descriptor));
	if(f) {
		f->vnode = NULL;
		f->cookie = NULL;
		f->ref_count = 1;
	}
	return f;
}


int
new_fd(struct io_context *ioctx, struct file_descriptor *f)
{
	int fd = -1;
	int i;

	mutex_lock(&ioctx->io_mutex);

	for (i = 0; i < ioctx->table_size; i++) {
		if (!ioctx->fds[i]) {
			fd = i;
			break;
		}
	}
	if (fd < 0) {
		fd = ERR_NO_MORE_HANDLES;
		goto err;
	}

	ioctx->fds[fd] = f;
	ioctx->num_used_fds++;

err:
	mutex_unlock(&ioctx->io_mutex);

	return fd;
}


void
put_fd(struct file_descriptor *f)
{
	/* Run a cleanup (fd_free) routine if there is one and free structure, but only
	 * if we've just removed the final reference to it :)
	 */
	if (atomic_add(&f->ref_count, -1) == 1) {
		if (f->ops->fd_free)
			f->ops->fd_free(f);
		kfree(f);
	}
}


struct file_descriptor *
get_fd(struct io_context *ioctx, int fd)
{
	struct file_descriptor *f;

	mutex_lock(&ioctx->io_mutex);

	if (fd >= 0 && fd < ioctx->table_size && ioctx->fds[fd]) {
		// valid fd
		f = ioctx->fds[fd];
		atomic_add(&f->ref_count, 1);
 	} else {
		f = NULL;
	}

	mutex_unlock(&ioctx->io_mutex);
	return f;
}


void
remove_fd(struct io_context *ioctx, int fd)
{
	struct file_descriptor *f;

	mutex_lock(&ioctx->io_mutex);

	if (fd >= 0 && fd < ioctx->table_size && ioctx->fds[fd]) {
		// valid fd
		f = ioctx->fds[fd];
		ioctx->fds[fd] = NULL;
		ioctx->num_used_fds--;
	} else {
		f = NULL;
	}

	mutex_unlock(&ioctx->io_mutex);

	if (f)
		put_fd(f);
}


//	#pragma mark -
/*** USER routines ***/ 


ssize_t
user_read(int fd, void *buffer, off_t pos, size_t length)
{
	struct file_descriptor *descriptor;
	ssize_t retval;

	/* This is a user_function, so abort if we have a kernel address */
	CHECK_USER_ADDR(buffer)

	descriptor = get_fd(get_current_io_context(false), fd);
	if (!descriptor)
		return EBADF;

	if (descriptor->ops->fd_read) {
		retval = descriptor->ops->fd_read(descriptor, buffer, pos, &length);
		if (retval >= 0)
			retval = (ssize_t)length;
	} else
		retval = EINVAL;

	put_fd(descriptor);
	return retval;
}


ssize_t
user_write(int fd, const void *buffer, off_t pos, size_t length)
{
	struct file_descriptor *descriptor;
	ssize_t retval = 0;

	CHECK_USER_ADDR(buffer)

	descriptor = get_fd(get_current_io_context(false), fd);
	if (!descriptor)
		return EBADF;

	if (descriptor->ops->fd_write) {
		retval = descriptor->ops->fd_write(descriptor, buffer, pos, &length);
		if (retval >= 0)
			retval = (ssize_t)length;
	} else
		retval = EINVAL;

	put_fd(descriptor);
	return retval;
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

	return status;
}


ssize_t
user_read_dir(int fd, struct dirent *buffer,size_t bufferSize)
{
	struct file_descriptor *descriptor;
	ssize_t retval;

	CHECK_USER_ADDR(buffer)

	PRINT(("user_read_dir(fd = %d, buffer = 0x%p, bufferSize = %ld)\n",fd,buffer,bufferSize));

	descriptor = get_fd(get_current_io_context(false), fd);
	if (descriptor == NULL)
		return EBADF;

	if (descriptor->ops->fd_read_dir) {
		uint32 count;
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
	ssize_t retval;

	/* This is a user_function, so abort if we have a kernel address */
	CHECK_USER_ADDR(stat)

	descriptor = get_fd(get_current_io_context(false), fd);
	if (descriptor == NULL)
		return EBADF;

	if (descriptor->ops->fd_stat) {
		// we're using the stat buffer on the stack to not have to
		// lock the given stat buffer in memory
		struct stat kstat;

		retval = descriptor->ops->fd_stat(descriptor, &kstat);
		if (retval >= 0)
			retval = user_memcpy(stat, &kstat, sizeof(*stat));
	} else
		retval = EOPNOTSUPP;

	put_fd(descriptor);
	return retval;
}


int
user_close(int fd)
{
	struct io_context *ioContext = get_current_io_context(false);
	struct file_descriptor *descriptor = get_fd(ioContext, fd);
	int retval;

	if (descriptor == NULL)
		return EBADF;

	if (descriptor->ops->fd_close)
		retval = descriptor->ops->fd_close(descriptor, fd, ioContext);
	else
		retval = EOPNOTSUPP;

	put_fd(descriptor);
	return retval;
}


//	#pragma mark -
/*** SYSTEM functions ***/


ssize_t
sys_read(int fd, void *buffer, off_t pos, size_t length)
{
	struct file_descriptor *descriptor;
	ssize_t retval;

	/* This is a sys_function, so abort if we have a kernel address */
	// I've removed those checks because we have to be able to load things
	// into user memory as well -- axeld.
	//CHECK_SYS_ADDR(buffer)

	descriptor = get_fd(get_current_io_context(true), fd);
	if (!descriptor)
		return EBADF;

	if (descriptor->ops->fd_read) {
		retval = descriptor->ops->fd_read(descriptor, buffer, pos, &length);
		if (retval >= 0)
			retval = (ssize_t)length;
	} else
		retval = EINVAL;

	put_fd(descriptor);
	return retval;
}


ssize_t
sys_write(int fd, const void *buffer, off_t pos, size_t length)
{
	struct file_descriptor *descriptor;
	ssize_t retval;

//	dprintf("sys_write: fd %d\n", fd);
//	CHECK_SYS_ADDR(buffer)
	
	descriptor = get_fd(get_current_io_context(true), fd);
	if (descriptor == NULL)
		return EBADF;

	if (descriptor->ops->fd_write) {
		retval = descriptor->ops->fd_write(descriptor, buffer, pos, &length);
		if (retval >= 0)
			retval = (ssize_t)length;

		PRINT(("sys_write(%d) = %ld (rlen = %ld)\n", fd, retval, length));
	} else
		retval = EINVAL;

	put_fd(descriptor);
	return retval;
}


int
sys_ioctl(int fd, ulong op, void *buffer, size_t length)
{
	struct file_descriptor *descriptor;
	int status;

	PRINT(("sys_ioctl: fd %d\n", fd));

	CHECK_SYS_ADDR(buffer)

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
sys_read_dir(int fd, struct dirent *buffer,size_t bufferSize)
{
	struct file_descriptor *descriptor;
	ssize_t retval;

	PRINT(("sys_read_dir(fd = %d, buffer = 0x%p, bufferSize = %ld)\n",fd,buffer,bufferSize));

	descriptor = get_fd(get_current_io_context(false), fd);
	if (descriptor == NULL)
		return EBADF;

	if (descriptor->ops->fd_read_dir) {
		uint32 count;
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
sys_close(int fd)
{
	struct io_context *ioContext = get_current_io_context(true);
	struct file_descriptor *descriptor = get_fd(ioContext, fd);
	int status;

	if (descriptor == NULL)
		return EBADF;

	if (descriptor->ops->fd_close)
		status = descriptor->ops->fd_close(descriptor, fd, ioContext);
	else
		status = EOPNOTSUPP;

	put_fd(descriptor);
	return status;
}

