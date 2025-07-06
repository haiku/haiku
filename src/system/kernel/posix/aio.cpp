/*
 * Copyright 2023 Jules ALTIS. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <aio.h>
#include <syscalls.h>
#include <errno.h>
#include <fcntl.h> // For O_CREAT etc.
#include <stdlib.h> // For malloc/free in userspace lib
#include <kits/debug/Debug.h>


// Userspace POSIX AIO functions (will live in libroot or similar)

int
aio_read(struct aiocb *aiocbp)
{
	if (aiocbp == NULL) {
		errno = EINVAL;
		return -1;
	}
	// TODO: Validate other aiocbp fields as per POSIX?
	// (aio_fildes, aio_buf, aio_nbytes, aio_offset)
	// Or rely on kernel for deeper validation.

	return _kern_aio_read(aiocbp);
}

int
aio_write(struct aiocb *aiocbp)
{
	if (aiocbp == NULL) {
		errno = EINVAL;
		return -1;
	}
	// TODO: Validate as above.
	return _kern_aio_write(aiocbp);
}

int
aio_fsync(int op, struct aiocb *aiocbp)
{
	if (aiocbp == NULL || (op != O_SYNC && op != O_DSYNC)) {
		errno = EINVAL;
		return -1;
	}
	// TODO: Implement _kern_aio_fsync
	// return _kern_aio_fsync(op, aiocbp);
	errno = ENOSYS;
	return -1;
}

int
aio_error(const struct aiocb *aiocbp)
{
	if (aiocbp == NULL) {
		errno = EINVAL;
		return -1;
	}
	// This call is not supposed to set errno if aiocbp is valid.
	// The error code is the return value.
	return _kern_aio_error(aiocbp);
}

ssize_t
aio_return(struct aiocb *aiocbp)
{
	if (aiocbp == NULL) {
		errno = EINVAL;
		return -1;
	}
	// This call is not supposed to set errno if aiocbp is valid.
	// The return value of the I/O op (or -1 on error) is returned.
	return _kern_aio_return(aiocbp);
}

int
aio_suspend(const struct aiocb * const list[], int nent,
	const struct timespec *timeout)
{
	if (nent < 0) {
		errno = EINVAL;
		return -1;
	}
	if (nent == 0)
		return 0;
	if (list == NULL) {
		errno = EINVAL;
		return -1;
	}
	// TODO: Validate list entries?
	return _kern_aio_suspend(list, nent, timeout);
}

int
aio_cancel(int fildes, struct aiocb *aiocbp)
{
	// If aiocbp is NULL, attempt to cancel all outstanding AIO operations for fildes.
	// If fildes is -1 and aiocbp is NULL, attempt to cancel all for the process. (This is non-POSIX?)
	// For now, only support cancelling a specific aiocbp.
	if (aiocbp == NULL && fildes == -1) {
		// Standard says if aiocbp is NULL, cancel all for fildes.
		// If fildes is also -1, this is ambiguous or non-standard.
		// Let's stick to POSIX: aiocbp==NULL means all for fildes.
		// If fildes is also -1, that's an error for "all for fildes"
		errno = EINVAL; // Or EBADF if fildes is invalid for "all for fildes"
		return -1;
	}
	// If aiocbp is not NULL, fildes is ignored by POSIX.
	// However, some systems might use fildes to validate aiocbp->aio_fildes.
	// For now, we pass fildes to kernel and let it decide.

	return _kern_aio_cancel(fildes, aiocbp);
}

int
lio_listio(int mode, struct aiocb * const list[], int nent,
	struct sigevent *sig)
{
	if (nent < 0 || (nent > 0 && list == NULL)) {
		errno = EINVAL;
		return -1;
	}
	if (mode != LIO_WAIT && mode != LIO_NOWAIT) {
		errno = EINVAL;
		return -1;
	}
	// TODO: Validate list entries if nent > 0
	// TODO: Validate sigevent if mode == LIO_NOWAIT && sig != NULL

	return _kern_lio_listio(mode, list, nent, sig);
}

// Kernel-side AIO implementation (stubs for now)
// These would typically live in a different file compiled into the kernel.
// For now, they are here to allow userland part to compile.

extern "C" {

// These are the syscalls that the userspace functions above will call.
// Their implementation will be the core of the AIO feature.

status_t
_kern_aio_read(struct aiocb *aiocbp)
{
	// 1. Copy aiocb from userspace.
	// 2. Validate file descriptor and buffer.
	// 3. Create a kernel internal AIO control block.
	// 4. Create an IORequest. Set its team_id.
	// 5. Submit the IORequest using vfs_asynchronous_read_pages or similar.
	//    The callback for the IORequest will update the kernel AIO control block
	//    and trigger notification.
	// 6. Store the kernel AIO control block so aio_error/aio_return can find it.
	// 7. Return 0 on successful queueing, -1 on error (setting errno).
	dprintf("TODO: _kern_aio_read called\n");
	errno = ENOSYS;
	return -1;
}

status_t
_kern_aio_write(struct aiocb *aiocbp)
{
	// Similar to _kern_aio_read
	dprintf("TODO: _kern_aio_write called\n");
	errno = ENOSYS;
	return -1;
}

status_t
_kern_aio_error(const struct aiocb *aiocbp)
{
	// 1. Find the kernel internal AIO control block corresponding to user_aiocbp.
	// 2. Return its error status (e.g., EINPROGRESS, ECANCELED, or actual I/O error).
	dprintf("TODO: _kern_aio_error called\n");
	// To avoid setting errno, we'd return the error code directly or EINPROGRESS
	return EINPROGRESS; // Placeholder
}

ssize_t
_kern_aio_return(struct aiocb *aiocbp)
{
	// 1. Find the kernel internal AIO control block.
	// 2. If not complete, or error already returned, behavior is undefined (or error).
	//    POSIX says: "If aio_error() returns [EINPROGRESS], aio_return() is not specified to return a value."
	//    "After a call to aio_return() for a specific asynchronous operation,
	//     the application shall not call aio_return() or aio_error() for that operation again."
	// 3. Return the I/O operation's return value (bytes read/written or -1).
	// 4. Mark the AIO operation as "return status retrieved" to prevent multiple calls.
	//    Free kernel resources associated with this specific AIO if appropriate.
	dprintf("TODO: _kern_aio_return called\n");
	errno = EINVAL; // Placeholder if called inappropriately
	return -1;
}

status_t
_kern_aio_suspend(const struct aiocb * const list[], int nent,
	const struct timespec *timeout)
{
	// 1. Iterate through the list of user_aiocbp.
	// 2. For each, find its kernel counterpart.
	// 3. Wait (with timeout) for any of these operations to complete.
	//    This will involve kernel synchronization primitives (condition variables, semaphores).
	dprintf("TODO: _kern_aio_suspend called\n");
	errno = ENOSYS;
	return -1;
}

status_t
_kern_aio_cancel(int fildes, struct aiocb *aiocbp)
{
	// 1. If aiocbp is not NULL:
	//    a. Find the kernel AIO control block.
	//    b. Attempt to cancel the underlying IORequest. This might involve
	//       communicating with the I/O scheduler.
	//    c. Update AIO status to ECANCELED if successful, or report if it couldn't be
	//       canceled (already in progress/completed).
	//    d. Return AIO_CANCELED, AIO_NOTCANCELED, or AIO_ALLDONE.
	// 2. If aiocbp is NULL:
	//    a. Iterate all AIO operations for `fildes` for the current process.
	//    b. Attempt to cancel each.
	//    c. Return AIO_CANCELED, AIO_NOTCANCELED, or AIO_ALLDONE.
	dprintf("TODO: _kern_aio_cancel called\n");
	errno = ENOSYS; // Function itself returns -1 on error, not the AIO_* codes here.
	                // The AIO_* codes are the *return value* of aio_cancel on success.
	return -1;      // Placeholder for error in syscall setup
}

status_t
_kern_lio_listio(int mode, struct aiocb * const list[], int nent,
	struct sigevent *sig)
{
	// 1. Validate inputs.
	// 2. For each aiocb in the list:
	//    a. Perform an operation similar to aio_read/aio_write but don't
	//       return to userspace immediately. Queue them all.
	//    b. Keep track of the number of submitted operations.
	// 3. If mode is LIO_WAIT:
	//    a. Wait for all submitted operations to complete.
	//    b. Return 0 on success, -1 on error (some operations failed).
	// 4. If mode is LIO_NOWAIT:
	//    a. If `sig` is not NULL, arrange for the signal to be sent when all
	//       operations in the list complete.
	//    b. Return 0 on successful queueing, -1 on error.
	dprintf("TODO: _kern_lio_listio called\n");
	errno = ENOSYS;
	return -1;
}

} // extern "C"
