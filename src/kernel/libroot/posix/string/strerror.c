/* 
** Copyright 2001, Dan Sinclair. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <Errors.h>
#include <string.h>
#include <stdio.h>
#include <kerrors.h>


static char *
error_description(int error)
{
	switch (error) {
		/* General Errors */
		case B_NO_ERROR:
			return "No Error";

		case B_ERROR:
			return "General Error";

		case B_INTERRUPTED:
			// EINTR
			return "Interrupted system call";

		case B_NO_MEMORY:
			// ENOMEM
			return "Cannot allocate memory";	// "No memory"

		case B_IO_ERROR:
			// EIO
			return "Input/Output error";	// "I/O Error"

		case B_BAD_VALUE:
			// EINVAL
			return "Invalid Argument";

		case B_TIMED_OUT:
			// ETIMEDOUT
			return "Timed out";	// "Operation timed out"

		case B_NOT_ALLOWED:
			// EPERM
			return "Operation not permitted";	// "Operation not allowed"

		case B_PERMISSION_DENIED:
			// EACCES
			return "Operation not permitted";	// "Permission denied"

		case ERR_INVALID_BINARY:
			return "Invalid Binary";

		case ERR_INVALID_HANDLE:
			return "Invalid ID Handle";

		case ERR_NO_MORE_HANDLES:
			return "No more handles";

		case B_FILE_ERROR:
			// EBADF
			return "Bad file descriptor";

		case B_ENTRY_NOT_FOUND:
			// ENOENT
			return "No such file or directory";

		case ENFILE:
			return "Too many open files in system"; // "File table overflow"

		case B_NO_MORE_FDS:
			// EMFILE
			return "Too many open files";

		case ENXIO:
			return "Device not configured";	// "No such device"

		case ESPIPE:
			return "Illegal seek";	// "Seek not allowed on file descriptor"

		case ENOSYS:
			return "Unimplemented";	// "Function not implemented"

		case EDOM:
			return "Numerical argument out of range";	// "Domain Error"

		case ERR_NOT_FOUND:
			return "Not found";

		case ERR_NOT_IMPLEMENTED_YET:
			return "Not implemented yet";

		/* Semaphore errors */
		case B_BAD_SEM_ID:
			return "Bad semaphore id";

		case B_NO_MORE_SEMS:
			return "Semaphore out of slots";	// "No more semaphores"

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

		case ENOBUFS:
			return "VFS insufficient buffer";	// "No buffer space available"

		case B_READ_ONLY_DEVICE:
			// EROFS:
			return "VFS readonly filesystem";	// "Read-only file system"

		case B_FILE_EXISTS:
			// EEXIST
			return "VFS already exists";	// "File or Directory already exists"

		case B_BUSY:
			// EBUSY
			return "Device busy";	// "Device/File/Resource busy"

		case ERR_VFS_FD_TABLE_FULL:
			return "VFS file descriptor table full";

		case ERR_VFS_CROSS_FS_RENAME:
			return "VFS cross filesystem rename";

		case ERR_VFS_DIR_NOT_EMPTY:
			return "Directory not empty";

		case B_NOT_A_DIRECTORY:
			// ENOTDIR
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
			return "Port does not exist";	// "Bad port ID"

		case B_NO_MORE_PORTS:
			return "No more ports available";	// "No more ports"

		case B_BAD_INDEX:
			return "Index not in range for the data set";

		case B_BAD_TYPE:
			return "Bad argument type passed to function";

		case B_MISMATCHED_VALUES:
			return "Mismatched values passed to function";

		case B_NAME_NOT_FOUND:
			return "Name not found";

		case B_NAME_IN_USE:
			return "Name in use";

		case B_WOULD_BLOCK:
			// EAGAIN
			// EWOULDBLOCK
			return "Operation would block";

		case B_CANCELED:
			return "Operation canceled";

		case B_NO_INIT:
			return "Initialization failed";

		case B_BAD_THREAD_ID:
			return "Bad thread ID";

		case B_NO_MORE_THREADS:
			return "No more threads";

		case B_BAD_THREAD_STATE:
			return "Thread is inappropriate state";

		case B_BAD_TEAM_ID:
			return "Operation on invalid team";

		case B_NO_MORE_TEAMS:
			return "No more teams";

		case B_BAD_IMAGE_ID:
			return "Bad image ID";

		case B_BAD_ADDRESS:
			// EFAULT
			return "Bad address";

		case B_NOT_AN_EXECUTABLE:
			// ENOEXEC
			return "Not an executable";

		case B_MISSING_LIBRARY:
			return "Missing library";

		case B_MISSING_SYMBOL:
			return "Symbol not found";

		case B_DEBUGGER_ALREADY_INSTALLED:
			return "Debugger already installed for this team";

		case B_APP_ERROR_BASE:
			// B_BAD_REPLY
			return "Invalid or unwanted reply";

		case B_DUPLICATE_REPLY:
			return "Duplicate reply";

		case B_MESSAGE_TO_SELF:
			return "Can't send message to self";

		case B_BAD_HANDLER:
			return "Bad handler";

		case B_ALREADY_RUNNING:
			return "Already running";

		case B_LAUNCH_FAILED:
			return "Launch failed";

		case B_AMBIGUOUS_APP_LAUNCH:
			return "Ambiguous app launch";

		case B_UNKNOWN_MIME_TYPE:
			return "Unknown MIME type";

		case B_BAD_SCRIPT_SYNTAX:
			return "Bad script syntax";

		case B_LAUNCH_FAILED_NO_RESOLVE_LINK:
			return "Could not resolve a link";

		case B_LAUNCH_FAILED_EXECUTABLE:
			return "File is mistakenly marked as executable";

		case B_LAUNCH_FAILED_APP_NOT_FOUND:
			return "Application could not be found";

		case B_LAUNCH_FAILED_APP_IN_TRASH:
			return "Application is in the trash";

		case B_LAUNCH_FAILED_NO_PREFERRED_APP:
			return "There is no preferred application for this type of file";

		case B_LAUNCH_FAILED_FILES_APP_NOT_FOUND:
			return "This file has a preferred app, but it could not be found";

		case B_BAD_MIME_SNIFFER_RULE:
			return "Bad sniffer rule";

		case B_STREAM_NOT_FOUND:
			return "Stream not found";

		case B_SERVER_NOT_FOUND:
			return "Server not found";

		case B_RESOURCE_NOT_FOUND:
			return "Resource not found";

		case B_RESOURCE_UNAVAILABLE:
			return "Resource unavailable";

		case B_BAD_SUBSCRIBER:
			return "Bad subscriber";

		case B_SUBSCRIBER_NOT_ENTERED:
			return "Subscriber not entered";

		case B_BUFFER_NOT_AVAILABLE:
			return "Buffer not available";

		case B_LAST_BUFFER_ERROR:
			return "Last buffer";

		case B_FILE_NOT_FOUND:
			return "File not found";

		case B_NAME_TOO_LONG:
			//	ENAMETOOLONG
			return "File name too long";

		case B_DIRECTORY_NOT_EMPTY:
			// ENOTEMPTY
			return "Directory not empty";

		case B_DEVICE_FULL:
			// ENOSPC
			return "No space left on device";

		case B_IS_A_DIRECTORY:
			// EISDIR
			return "Is a directory";

		case B_CROSS_DEVICE_LINK:
			// EXDEV
			return "Cross-device link";

		case B_LINK_LIMIT:
			// ELOOP
			return "Too many symbolic links";

		case B_BUSTED_PIPE:
			// EPIPE
			return "Broken pipe";

		case B_UNSUPPORTED:
			return "Operation not supported";

		case B_PARTITION_TOO_SMALL:
			return "Partition too small to contain filesystem";

		case E2BIG:
			return "Argument too big";

		case ECHILD:
			return "No child process";

		case EDEADLK:
			return "Resource deadlock";

		case EFBIG:
			return "File too large";

		case EMLINK:
			return "Too many links";

		case ENODEV:
			return "No such device";

		case ENOLCK:
			return "No record locks available";

		case ENOTTY:
			return "Not a tty";

		case ESRCH:
			return "No such process";

		case EFPOS:
			return "File Position Error";

		case ESIGPARM:
			return "Signal Error";

		case ERANGE:
			return "Range Error";

		case EPROTOTYPE:
			return "Protocol wrong type for socket";

		case EPROTONOSUPPORT:
			return "Protocol not supported";

		case EPFNOSUPPORT:
			return "Protocol family not supported";

		case EAFNOSUPPORT:
			return "Address family not supported by protocol family";

		case EADDRINUSE:
			return "Address already in use";

		case EADDRNOTAVAIL:
			return "Can't assign requested address";

		case ENETDOWN:
			return "Network is down";

		case ENETUNREACH:
			return "Network is unreachable";

		case ENETRESET:
			return "Network dropped connection on reset";

		case ECONNABORTED:
			return "Software caused connection abort";

		case ECONNRESET:
			return "Connection reset by peer";

		case EISCONN:
			return "Socket is already connected";

		case ENOTCONN:
			return "Socket is not connected";

		case ESHUTDOWN:
			return "Can't send after socket shutdown";

		case ECONNREFUSED:
			return "Connection refused";

		case EHOSTUNREACH:
			return "No route to host";

		case ENOPROTOOPT:
			return "Protocol option not available";

		case EINPROGRESS:
			return "Operation now in progress";

		case EALREADY:
			return "Operation already in progress";

		case EILSEQ:
			return "Illegal byte sequence";

		case ENOMSG:
			return "No message of desired type";

		case ESTALE:
			return "Stale NFS file handle";

		case EOVERFLOW:
			return "Value too large for defined type";

		case EMSGSIZE:
			return "Message too long";

		case EOPNOTSUPP:
			return "Operation not supported";

		case ENOTSOCK:
			return "Socket operation on non-socket";

		case B_MAIL_NO_DAEMON:
			return "No mail daemon";

		case B_MAIL_UNKNOWN_USER:
			return "Unknown mail user";

		case B_MAIL_WRONG_PASSWORD:
			return "Wrong password (mail)";

		case B_MAIL_UNKNOWN_HOST:
			return "Mail unknown host";

		case B_MAIL_ACCESS_ERROR:
			return "Mail access error";

		case B_MAIL_UNKNOWN_FIELD:
			return "Unknown mail field";

		case B_MAIL_NO_RECIPIENT:
			return "No mail recipient";

		case B_MAIL_INVALID_MAIL:
			return "Invaild mail";

		case B_PRINT_ERROR_BASE:
			// B_NO_PRINT_SERVER
			return "No print server";

		case B_DEV_INVALID_IOCTL:
			return "Invalid device ioctl";

		case B_DEV_NO_MEMORY:
			return "No device memory";

		case B_DEV_BAD_DRIVE_NUM:
			return "Bad drive number";

		case B_DEV_NO_MEDIA:
			return "No media present";

		case B_DEV_UNREADABLE:
			return "Device unreadable";

		case B_DEV_FORMAT_ERROR:
			return "Device format error";

		case B_DEV_TIMEOUT:
			return "Device timeout";

		case B_DEV_RECALIBRATE_ERROR:
			return "Device recalibrate error";

		case B_DEV_SEEK_ERROR:
			return "Device seek error";

		case B_DEV_ID_ERROR:
			return "Device ID error";

		case B_DEV_READ_ERROR:
			return "Device read error";

		case B_DEV_WRITE_ERROR:
			return "Device write error";

		case B_DEV_NOT_READY:
			return "Device not ready";

		case B_DEV_MEDIA_CHANGED:
			return "Device media changed";

		case B_DEV_MEDIA_CHANGE_REQUESTED:
			return "Device media change requested";

		case B_DEV_RESOURCE_CONFLICT:
			return "Resource conflict";

		case B_DEV_CONFIGURATION_ERROR:
			return "Configuration error";

		case B_DEV_DISABLED_BY_USER:
			return "Disabled by user";

		case B_DEV_DOOR_OPEN:
			return "Drive door open";

		case B_NOT_A_MESSAGE:
			return "Data is not a message";

		default:
			return NULL;
	}
}


char *
strerror(int error)
{
	static char unknown[28];

	char *description = error_description(error);
	if (description != NULL)
		return description;

	sprintf(unknown, "Unknown Error (%d)", error);
	return unknown;
}


int
strerror_r(int error, char *buffer, size_t bufferSize)
{
	char *description = error_description(error);
	if (description == NULL)
		return EINVAL;

	strlcpy(buffer, description, bufferSize);
	return 0;
		// ToDo: could return ERANGE if buffer is too small
}

