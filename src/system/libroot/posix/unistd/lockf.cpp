/*
 * Copyright 2007, Vasilis Kaoutsis, kaoutsis@sch.gr.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno_private.h>


int
lockf(int fileDescriptor, int function, off_t size)
{
	struct flock fileLock;
	fileLock.l_start = 0;
	fileLock.l_len = size;
	fileLock.l_whence = SEEK_CUR;

	if (function == F_ULOCK) {
		// unlock locked sections
		fileLock.l_type = F_UNLCK;
		return fcntl(fileDescriptor, F_SETLK, &fileLock);
	} else if (function == F_LOCK) {
		// lock a section for exclusive use
		fileLock.l_type = F_WRLCK;
		return fcntl(fileDescriptor, F_SETLKW, &fileLock);
	} else if (function == F_TLOCK) {
		// test and lock a section for exclusive use
		fileLock.l_type = F_WRLCK;
		return fcntl(fileDescriptor, F_SETLK, &fileLock);
	} else if (function == F_TEST) {
		// test a section for locks by other processes
		fileLock.l_type = F_WRLCK;
		if (fcntl(fileDescriptor, F_GETLK, &fileLock) == -1)
			return -1;
		if (fileLock.l_type == F_UNLCK)
			return 0;

		__set_errno(EAGAIN);
		return -1;
	} else {
		__set_errno(EINVAL);
		return -1;
	}

	// Notes regarding standard compliance (cf. Open Group Base Specs):
	// * "The interaction between fcntl() and lockf() locks is unspecified."
	// * fcntl() locking works on a per-process level. The lockf() description
	//   is a little fuzzy on whether it works the same way. The first quote
	//   seem to describe per-thread locks (though it might actually mean
	//   "threads of other processes"), but the others quotes are strongly
	//   indicating per-process locks:
	//   - "Calls to lockf() from other threads which attempt to lock the locked
	//     file section shall either return an error value or block until the
	//     section becomes unlocked."
	//   - "All the locks for a process are removed when the process
	//     terminates."
	//   - "F_TEST shall detect if a lock by another process is present on the
	//     specified section."
	//   - "The sections locked with F_LOCK or F_TLOCK may, in whole or in part,
	//     contain or be contained by a previously locked section for the same
	//     process. When this occurs, or if adjacent locked sections would
	//     occur, the sections shall be combined into a single locked section."
	// * fcntl() and lockf() handle a 0 size argument differently. The former
	//   use the file size at the time of the call:
	//    "If the command is F_SETLKW and the process must wait for another
	//     process to release a lock, then the range of bytes to be locked shall
	//     be determined before the fcntl() function blocks. If the file size
	//     or file descriptor seek offset change while fcntl() is blocked, this
	//     shall not affect the range of bytes locked."
	//   lockf(), on the other hand, is supposed to create a lock whose size
	//   dynamically adjusts to the file size:
	//    "If size is 0, the section from the current offset through the largest
	//     possible file offset shall be locked (that is, from the current
	//     offset through the present or any future end-of-file)."
	// * The lock release handling when closing descriptors sounds a little
	//   different, though might actually mean the same.
	//   For fcntl():
	//    "All locks associated with a file for a given process shall be removed
	//     when a file descriptor for that file is closed by that process or the
	//     process holding that file descriptor terminates."
	//   For lockf():
	//    "File locks shall be released on first close by the locking process of
	//     any file descriptor for the file."
}
