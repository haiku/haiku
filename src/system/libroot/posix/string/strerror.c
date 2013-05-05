/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT license.
 *
 * Copyright 2001, Dan Sinclair. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <string.h>
#include <stdio.h>

#include <SupportDefs.h>


static const struct error_base {
	int			base;
	const char	*name;
} kErrorBases[] = {
	{B_GENERAL_ERROR_BASE, "General "},
	{B_OS_ERROR_BASE, "OS "},
	{B_APP_ERROR_BASE, "Application Kit "},
	{B_INTERFACE_ERROR_BASE, "Interface Kit "},
	{B_MEDIA_ERROR_BASE, "Media Kit "},
	{B_TRANSLATION_ERROR_BASE, "Translation Kit "},
	{B_MIDI_ERROR_BASE, "Midi Kit "},
	{B_STORAGE_ERROR_BASE, "Storage Kit "},
	{B_POSIX_ERROR_BASE, "POSIX "},
	{B_MAIL_ERROR_BASE, "Mail Kit "},
	{B_PRINT_ERROR_BASE, "Print "},
	{B_DEVICE_ERROR_BASE, "Device "},
	{B_ERRORS_END, "Application "},
};
static const uint32 kNumErrorBases = sizeof(kErrorBases)
	/ sizeof(struct error_base);


static char *
error_description(int error)
{
	switch (error) {
		// General Errors

		case B_NO_ERROR:
			return "No error";
		case B_ERROR:
			return "General system error";

		case B_NO_MEMORY:
		case B_POSIX_ENOMEM:
			// ENOMEM
			return "Out of memory";
		case B_IO_ERROR:
			// EIO
			return "I/O error";
		case B_PERMISSION_DENIED:
			// EACCES
			return "Permission denied";
		case B_BAD_INDEX:
			return "Index not in range for the data set";
		case B_BAD_TYPE:
			return "Bad argument type passed to function";
		case B_BAD_VALUE:
			// EINVAL
			return "Invalid Argument";
		case B_MISMATCHED_VALUES:
			return "Mismatched values passed to function";
		case B_NAME_NOT_FOUND:
			return "Name not found";
		case B_NAME_IN_USE:
			return "Name in use";
		case B_TIMED_OUT:
			// ETIMEDOUT
			return "Operation timed out";
		case B_INTERRUPTED:
			// EINTR
			return "Interrupted system call";
		case B_WOULD_BLOCK:
			// EAGAIN
			// EWOULDBLOCK
			return "Operation would block";
		case B_CANCELED:
			return "Operation canceled";
		case B_NO_INIT:
			return "Initialization failed";
		case B_BUSY:
			// EBUSY
			return "Device/File/Resource busy";
		case B_NOT_ALLOWED:
			// EPERM
			return "Operation not allowed";
		case B_BAD_DATA:
			return "Bad data";

		// Kernel Kit Errors

		case B_BAD_SEM_ID:
			return "Bad semaphore ID";
		case B_NO_MORE_SEMS:
			return "No more semaphores";

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

		case B_BAD_PORT_ID:
			return "Bad port ID";
		case B_NO_MORE_PORTS:
			return "No more ports available";	// "No more ports"

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

		// Application Kit Errors

		case B_BAD_REPLY:
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
		case B_NOT_A_MESSAGE:
			return "Data is not a message";
		case B_SHUTDOWN_CANCELLED:
			return "System shutdown cancelled";
		case B_SHUTTING_DOWN:
			return "System shutting down";

		// Storage Kit Errors

		case B_FILE_ERROR:
			// EBADF
			return "Bad file descriptor";
		case B_FILE_NOT_FOUND:
		case B_ENTRY_NOT_FOUND:
			// ENOENT
			return "No such file or directory";
		case B_FILE_EXISTS:
			// EEXIST
			return "File or Directory already exists";
		case B_NAME_TOO_LONG:
			//	ENAMETOOLONG
			return "File name too long";
		case B_NOT_A_DIRECTORY:
			// ENOTDIR
			return "Not a directory";
		case B_DIRECTORY_NOT_EMPTY:
			// ENOTEMPTY
			return "Directory not empty";
		case B_DEVICE_FULL:
			// ENOSPC
			return "No space left on device";
		case B_READ_ONLY_DEVICE:
			// EROFS:
			return "Read-only file system";
		case B_IS_A_DIRECTORY:
			// EISDIR
			return "Is a directory";
		case B_NO_MORE_FDS:
			// EMFILE
			return "Too many open files";
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

		// Media Kit Errors

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
		case B_MEDIA_SYSTEM_FAILURE:
			return "System failure";
		case B_MEDIA_BAD_NODE:
			return "Bad media node";
		case B_MEDIA_NODE_BUSY:
			return "Media node busy";
		case B_MEDIA_BAD_FORMAT:
			return "Bad media format";
		case B_MEDIA_BAD_BUFFER:
			return "Bad buffer";
		case B_MEDIA_TOO_MANY_NODES:
			return "Too many nodes";
		case B_MEDIA_TOO_MANY_BUFFERS:
			return "Too many buffers";
		case B_MEDIA_NODE_ALREADY_EXISTS:
			return "Media node already exists";
		case B_MEDIA_BUFFER_ALREADY_EXISTS:
			return "Buffer already exists";
		case B_MEDIA_CANNOT_SEEK:
			return "Cannot seek";
		case B_MEDIA_CANNOT_CHANGE_RUN_MODE:
			return "Cannot change run mode";
		case B_MEDIA_APP_ALREADY_REGISTERED:
			return "Application already registered";
		case B_MEDIA_APP_NOT_REGISTERED:
			return "Application not registered";
		case B_MEDIA_CANNOT_RECLAIM_BUFFERS:
			return "Cannot reclaim buffers";
		case B_MEDIA_BUFFERS_NOT_RECLAIMED:
			return "Buffers not reclaimed";
		case B_MEDIA_TIME_SOURCE_STOPPED:
			return "Time source stopped";
		case B_MEDIA_TIME_SOURCE_BUSY:
			return "Time source busy";
		case B_MEDIA_BAD_SOURCE:
			return "Bad source";
		case B_MEDIA_BAD_DESTINATION:
			return "Bad destination";
		case B_MEDIA_ALREADY_CONNECTED:
			return "Already connected";
		case B_MEDIA_NOT_CONNECTED:
			return "Not connected";
		case B_MEDIA_BAD_CLIP_FORMAT:
			return "Bad clipping format";
		case B_MEDIA_ADDON_FAILED:
			return "Media addon failed";
		case B_MEDIA_ADDON_DISABLED:
			return "Media addon disabled";
		case B_MEDIA_CHANGE_IN_PROGRESS:
			return "Change in progress";
		case B_MEDIA_STALE_CHANGE_COUNT:
			return "Stale change count";
		case B_MEDIA_ADDON_RESTRICTED:
			return "Media addon restricted";
		case B_MEDIA_NO_HANDLER:
			return "No handler";
		case B_MEDIA_DUPLICATE_FORMAT:
			return "Duplicate format";
		case B_MEDIA_REALTIME_DISABLED:
			return "Realtime disabled";
		case B_MEDIA_REALTIME_UNAVAILABLE:
			return "Realtime unavailable";

		// Mail Kit Errors

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
			return "Invalid mail";

		// Printing Errors

		case B_NO_PRINT_SERVER:
			return "No print server";

		// Device Kit Errors

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

		// the commented out ones are really strange error codes...
		//case B_DEV_INVALID_PIPE:

		case B_DEV_CRC_ERROR:
			return "Device check-sum error";
		case B_DEV_STALLED:
			return "Device stalled";

		//case B_DEV_BAD_PID:
		//case B_DEV_UNEXPECTED_PID:

		case B_DEV_DATA_OVERRUN:
			return "Device data overrun";
		case B_DEV_DATA_UNDERRUN:
			return "Device data underrun";
		case B_DEV_FIFO_OVERRUN:
			return "Device FIFO overrun";
		case B_DEV_FIFO_UNDERRUN:
			return "Device FIFO underrun";
		case B_DEV_PENDING:
			return "Device pending";
		case B_DEV_MULTIPLE_ERRORS:
			return "Multiple device errors";
		case B_DEV_TOO_LATE:
			return "Device too late";

		// Translation Kit Errors

		case B_NO_TRANSLATOR:
			return "No translator found";
		case B_ILLEGAL_DATA:
			return "Illegal data";

		// Other POSIX Errors

		case ENFILE:
			return "File table overflow";
		case ENXIO:
			return "Device not accessible";
		case ESPIPE:
			return "Seek not allowed on file descriptor";
		case ENOSYS:
			return "Function not implemented";
		case EDOM:
			return "Numerical argument out of range";	// "Domain Error"
		case ENOBUFS:
			return "No buffer space available";
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
			return "Stale file handle";

		case EOVERFLOW:
			return "Value too large for defined type";
		case EMSGSIZE:
			return "Message too long";
		case EOPNOTSUPP:
			return "Operation not supported";
		case ENOTSOCK:
			return "Socket operation on non-socket";
		case EBADMSG:
			return "Bad message";
		case ECANCELED:
			return "Operation canceled";
		case EDESTADDRREQ:
			return "Destination address required";
		case EDQUOT:
			return "Reserved";
		case EIDRM:
			return "Identifier removed";
		case EMULTIHOP:
			return "Reserved";
		case ENODATA:
			return "No message available";
		case ENOLINK:
			return "Reserved";
		case ENOSR:
			return "No STREAM resources";
		case ENOSTR:
			return "Not a STREAM";
		case ENOTSUP:
			return "Not supported";
		case EPROTO:
			return "Protocol error";
		case ETIME:
			return "STREAM ioctl() timeout";
		case ETXTBSY:
			return "Text file busy";
		case ENOATTR:
			return "No such attribute";

		default:
			return NULL;
	}
}


char *
strerror(int error)
{
	static char unknown[48];
	uint32 i;

	char *description = error_description(error);
	if (description != NULL)
		return description;

	if (error < B_OK) {
		const char *system = "";
		for (i = 0; i < kNumErrorBases; i++) {
			if (kErrorBases[i].base <= error
				&& ((i + 1 < kNumErrorBases && kErrorBases[i + 1].base > error)
					|| i + 1 == kNumErrorBases)) {
				system = kErrorBases[i].name;
				break;
			}
		}
		sprintf(unknown, "Unknown %sError (%d)", system, error);
	} else
		sprintf(unknown, "No Error (%d)", error);

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
		// TODO: could return ERANGE if buffer is too small
}

