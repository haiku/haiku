/* 
** Copyright 2001, Dan Sinclair. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <sys/types.h>
#include <Errors.h>
#include <string.h>
#include <kerrors.h>


char const *
strerror(int errnum)
{
	switch (errnum) {
		/* General Errors */
		case B_NO_ERROR:
			return "No Error";

		case ERR_GENERAL:
			return "General Error";

		// B_INTERRUPTED
		case EINTR:
			return "Interrupted";
			
		// B_NO_MEMORY:
		case ENOMEM:
			return "Cannot allocate memory";

		case ERR_IO_ERROR:
			return "Input/Output error";

		case EINVAL:
			return "Invalid Arguments";

//		case ERR_TIMED_OUT:
		case ETIMEDOUT:
			return "Timed out";

//		case ERR_NOT_ALLOWED:
		case EPERM:
			return "Operation not permitted";

//		case ERR_PERMISSION_DENIED:
		case EACCES:
			return "Operation not permitted";

		case ERR_INVALID_BINARY:
			return "Invalid Binary";

		case ERR_INVALID_HANDLE:
			return "Invalid ID Handle";

		/* EMFILE */
		case ERR_NO_MORE_HANDLES:
			return "No more handles";

		case EBADF:
			return "Bad file descriptor";

		case ENOENT:
			return "No such file or directory";

		case ENFILE:
			return "Too many open files in system";
			
		case EMFILE:
			return "Too many open files";

		case ENXIO:
			return "Device not configured";

		case ESPIPE:
			return "Illegal seek";

//		case ERR_UNIMPLEMENTED:
		case ENOSYS:
			return "Unimplemented";

//		case ERR_TOO_BIG:
		case EDOM:
			return "Numerical argument out of range";

		case ERR_NOT_FOUND:
			return "Not found";

		case ERR_NOT_IMPLEMENTED_YET:
			return "Not implemented yet";


		/* Semaphore errors */
		case B_BAD_SEM_ID:
			return "Bad sem id";

		case B_NO_MORE_SEMS:
			return "Semaphore out of slots";

		case ERR_SEM_NOT_INTERRUPTABLE:
			return "Semaphore not interruptable";

		/* Tasker errors */
		case ERR_TASK_GENERAL:
			return "General Tasker error";

		case ERR_TASK_PROC_DELETED:
			return "Tasker process deleted";

		/* VFS errors */
		case ERR_VFS_GENERAL:
			return "General VFS error";

		case ERR_VFS_INVALID_FS:
			return "VFS invalid filesystem";

		case ERR_VFS_NOT_MOUNTPOINT:
			return "VFS not a mount point";

		case ERR_VFS_PATH_NOT_FOUND:
			return "VFS path not found";

//		case ERR_VFS_INSUFFICIENT_BUF:
		case ENOBUFS:
			return "VFS insufficient buffer";

//		case ERR_VFS_READONLY_FS:
		case EROFS:
			return "VFS readonly filesystem";

//		case ERR_VFS_ALREADY_EXISTS:
		case EEXIST:
			return "VFS already exists";

//		case ERR_VFS_FS_BUSY:
		case EBUSY:
			return "Device busy";

		case ERR_VFS_FD_TABLE_FULL:
			return "VFS file descriptor table full";

		case ERR_VFS_CROSS_FS_RENAME:
			return "VFS cross filesystem rename";

		case ERR_VFS_DIR_NOT_EMPTY:
			return "Directory not empty";

//		case ERR_VFS_NOT_DIR:
		case ENOTDIR:
			return "Not a directory";

		case ERR_VFS_WRONG_STREAM_TYPE:
			return "VFS wrong stream type";

		case ERR_VFS_ALREADY_MOUNTPOINT:
			return "VFS already a mount point";

		/* VM errors */
		case ERR_VM_GENERAL:
			return "General VM error";

		case ERR_VM_INVALID_ASPACE:
			return "VM invalid aspace";

		case ERR_VM_INVALID_REGION:
			return "VM invalid region";

		case ERR_VM_BAD_ADDRESS:
			return "VM bad address";

		case ERR_VM_PF_FATAL:
			return "VM PF fatal";

		case ERR_VM_PF_BAD_ADDRESS:
			return "VM PF bad address";

		case ERR_VM_PF_BAD_PERM:
			return "VM PF bad permissions";

		case ERR_VM_PAGE_NOT_PRESENT:
			return "VM page not present";

		case ERR_VM_NO_REGION_SLOT:
			return "VM no region slot";

		case ERR_VM_WOULD_OVERCOMMIT:
			return "VM would overcommit";

		case ERR_VM_BAD_USER_MEMORY:
			return "VM bad user memory";

		/* Elf errors */
		case ERR_ELF_GENERAL:
			return "General ELF error";

		case ERR_ELF_RESOLVING_SYMBOL:
			return "ELF resolving symbol";

		/* Ports errors */
		case B_BAD_PORT_ID:
			return "Port does not exist";

		case B_NO_MORE_PORTS:
			return "No more ports available";

		default:
			return "Unknown error";
	}
}

