#ifndef _ERRORS_H
#define _ERRORS_H

#include <limits.h>

/*-------------------------------------------------------------*/
/*----- Error baselines ---------------------------------------*/

#define B_GENERAL_ERROR_BASE		LONG_MIN
#define B_OS_ERROR_BASE				B_GENERAL_ERROR_BASE + 0x1000
#define B_APP_ERROR_BASE			B_GENERAL_ERROR_BASE + 0x2000
#define B_INTERFACE_ERROR_BASE		B_GENERAL_ERROR_BASE + 0x3000
#define B_MEDIA_ERROR_BASE			B_GENERAL_ERROR_BASE + 0x4000 
#define B_TRANSLATION_ERROR_BASE	B_GENERAL_ERROR_BASE + 0x4800
#define B_MIDI_ERROR_BASE			B_GENERAL_ERROR_BASE + 0x5000
#define B_STORAGE_ERROR_BASE		B_GENERAL_ERROR_BASE + 0x6000
#define B_POSIX_ERROR_BASE			B_GENERAL_ERROR_BASE + 0x7000
#define B_MAIL_ERROR_BASE			B_GENERAL_ERROR_BASE + 0x8000
#define B_PRINT_ERROR_BASE			B_GENERAL_ERROR_BASE + 0x9000
#define B_DEVICE_ERROR_BASE			B_GENERAL_ERROR_BASE + 0xa000

/*--- Developer-defined errors start at (B_ERRORS_END+1)----*/

#define B_ERRORS_END		(B_GENERAL_ERROR_BASE + 0xffff)


/*-------------------------------------------------------------*/
/*----- General Errors ----------------------------------------*/
enum {
	B_NO_MEMORY = B_GENERAL_ERROR_BASE,
	B_IO_ERROR,					
	B_PERMISSION_DENIED,		
	B_BAD_INDEX,				
	B_BAD_TYPE,					
	B_BAD_VALUE,				
	B_MISMATCHED_VALUES,		
	B_NAME_NOT_FOUND,			
	B_NAME_IN_USE,			
	B_TIMED_OUT,			
    B_INTERRUPTED,           
	B_WOULD_BLOCK,    
    B_CANCELED,          
	B_NO_INIT,			
	B_BUSY,					
	B_NOT_ALLOWED,				
	B_BAD_DATA,

	B_ERROR = -1,				
	B_OK = 0,
	B_NO_ERROR = 0
};

/*-------------------------------------------------------------*/
/*----- Kernel Kit Errors -------------------------------------*/
enum {
	B_BAD_SEM_ID = B_OS_ERROR_BASE,	
	B_NO_MORE_SEMS,				

	B_BAD_THREAD_ID = B_OS_ERROR_BASE + 0x100,
	B_NO_MORE_THREADS,			
	B_BAD_THREAD_STATE,			
	B_BAD_TEAM_ID,				
	B_NO_MORE_TEAMS,			

	B_BAD_PORT_ID = B_OS_ERROR_BASE + 0x200,
	B_NO_MORE_PORTS,			

	B_BAD_IMAGE_ID = B_OS_ERROR_BASE + 0x300,
	B_BAD_ADDRESS,				
	B_NOT_AN_EXECUTABLE,
	B_MISSING_LIBRARY,
	B_MISSING_SYMBOL,

	B_DEBUGGER_ALREADY_INSTALLED = B_OS_ERROR_BASE + 0x400
};


/*-------------------------------------------------------------*/
/*----- Application Kit Errors --------------------------------*/
enum
{
	B_BAD_REPLY = B_APP_ERROR_BASE,
	B_DUPLICATE_REPLY,			
	B_MESSAGE_TO_SELF,			
	B_BAD_HANDLER,
	B_ALREADY_RUNNING,
	B_LAUNCH_FAILED,
	B_AMBIGUOUS_APP_LAUNCH,
	B_UNKNOWN_MIME_TYPE,
	B_BAD_SCRIPT_SYNTAX,
	B_LAUNCH_FAILED_NO_RESOLVE_LINK,
	B_LAUNCH_FAILED_EXECUTABLE,
	B_LAUNCH_FAILED_APP_NOT_FOUND,
	B_LAUNCH_FAILED_APP_IN_TRASH,
	B_LAUNCH_FAILED_NO_PREFERRED_APP,
	B_LAUNCH_FAILED_FILES_APP_NOT_FOUND,
	B_BAD_MIME_SNIFFER_RULE
};


/*-------------------------------------------------------------*/
/*----- Storage Kit/File System Errors ------------------------*/
enum {
	B_FILE_ERROR =B_STORAGE_ERROR_BASE,
	B_FILE_NOT_FOUND,	/* discouraged; use B_ENTRY_NOT_FOUND in new code*/
	B_FILE_EXISTS,				
	B_ENTRY_NOT_FOUND,			
	B_NAME_TOO_LONG,			
	B_NOT_A_DIRECTORY,			
	B_DIRECTORY_NOT_EMPTY,		
	B_DEVICE_FULL,				
	B_READ_ONLY_DEVICE,			
	B_IS_A_DIRECTORY,			
	B_NO_MORE_FDS,				
	B_CROSS_DEVICE_LINK,		
	B_LINK_LIMIT,			    
	B_BUSTED_PIPE,				
	B_UNSUPPORTED,
	B_PARTITION_TOO_SMALL
};


/*-------------------------------------------------------------*/
/*----- POSIX Errors ------------------------------------------*/
#define E2BIG			(B_POSIX_ERROR_BASE + 1)
#define ECHILD			(B_POSIX_ERROR_BASE + 2)
#define EDEADLK			(B_POSIX_ERROR_BASE + 3)
#define EFBIG			(B_POSIX_ERROR_BASE + 4)
#define EMLINK			(B_POSIX_ERROR_BASE + 5)
#define ENFILE			(B_POSIX_ERROR_BASE + 6)
#define ENODEV			(B_POSIX_ERROR_BASE + 7)
#define ENOLCK			(B_POSIX_ERROR_BASE + 8)
#define ENOSYS			(B_POSIX_ERROR_BASE + 9)
#define ENOTTY			(B_POSIX_ERROR_BASE + 10)
#define ENXIO			(B_POSIX_ERROR_BASE + 11)
#define ESPIPE			(B_POSIX_ERROR_BASE + 12)
#define ESRCH			(B_POSIX_ERROR_BASE + 13)
#define EFPOS			(B_POSIX_ERROR_BASE + 14)
#define ESIGPARM		(B_POSIX_ERROR_BASE + 15)
#define EDOM			(B_POSIX_ERROR_BASE + 16)
#define ERANGE			(B_POSIX_ERROR_BASE + 17)
#define EPROTOTYPE		(B_POSIX_ERROR_BASE + 18)
#define EPROTONOSUPPORT	(B_POSIX_ERROR_BASE + 19)
#define EPFNOSUPPORT	(B_POSIX_ERROR_BASE + 20)
#define EAFNOSUPPORT	(B_POSIX_ERROR_BASE + 21)
#define EADDRINUSE		(B_POSIX_ERROR_BASE + 22)
#define EADDRNOTAVAIL	(B_POSIX_ERROR_BASE + 23)
#define ENETDOWN		(B_POSIX_ERROR_BASE + 24)
#define ENETUNREACH		(B_POSIX_ERROR_BASE + 25)
#define ENETRESET		(B_POSIX_ERROR_BASE + 26)
#define ECONNABORTED	(B_POSIX_ERROR_BASE + 27)
#define ECONNRESET		(B_POSIX_ERROR_BASE + 28)
#define EISCONN			(B_POSIX_ERROR_BASE + 29)
#define ENOTCONN		(B_POSIX_ERROR_BASE + 30)
#define ESHUTDOWN		(B_POSIX_ERROR_BASE + 31)
#define ECONNREFUSED	(B_POSIX_ERROR_BASE + 32)
#define EHOSTUNREACH	(B_POSIX_ERROR_BASE + 33)
#define ENOPROTOOPT		(B_POSIX_ERROR_BASE + 34)
#define ENOBUFS			(B_POSIX_ERROR_BASE + 35)
#define EINPROGRESS		(B_POSIX_ERROR_BASE + 36)
#define EALREADY		(B_POSIX_ERROR_BASE + 37)
#define EILSEQ          (B_POSIX_ERROR_BASE + 38)
#define ENOMSG          (B_POSIX_ERROR_BASE + 39)
#define ESTALE          (B_POSIX_ERROR_BASE + 40)
#define EOVERFLOW       (B_POSIX_ERROR_BASE + 41)
#define EMSGSIZE        (B_POSIX_ERROR_BASE + 42)
#define EOPNOTSUPP      (B_POSIX_ERROR_BASE + 43)                       
#define ENOTSOCK		(B_POSIX_ERROR_BASE + 44)

#define ENOMEM			B_NO_MEMORY
#define EACCES			B_PERMISSION_DENIED
#define EINTR			B_INTERRUPTED
#define EIO				B_IO_ERROR
#define EBUSY			B_BUSY
#define EFAULT			B_BAD_ADDRESS
#define ETIMEDOUT		B_TIMED_OUT
#define EAGAIN 			B_WOULD_BLOCK	/* SysV compatibility */
#define EWOULDBLOCK 	B_WOULD_BLOCK	/* BSD compatibility */
#define EBADF			B_FILE_ERROR
#define EEXIST			B_FILE_EXISTS
#define EINVAL			B_BAD_VALUE
#define ENAMETOOLONG	B_NAME_TOO_LONG
#define ENOENT			B_ENTRY_NOT_FOUND
#define EPERM			B_NOT_ALLOWED
#define ENOTDIR			B_NOT_A_DIRECTORY
#define EISDIR			B_IS_A_DIRECTORY
#define ENOTEMPTY		B_DIRECTORY_NOT_EMPTY
#define ENOSPC			B_DEVICE_FULL
#define EROFS			B_READ_ONLY_DEVICE
#define EMFILE			B_NO_MORE_FDS
#define EXDEV			B_CROSS_DEVICE_LINK
#define ELOOP			B_LINK_LIMIT
#define ENOEXEC			B_NOT_AN_EXECUTABLE
#define EPIPE			B_BUSTED_PIPE

/*-------------------------------------------------------------*/
/*----- Media Kit Errors --------------------------------------*/
enum {
  B_STREAM_NOT_FOUND = B_MEDIA_ERROR_BASE,
  B_SERVER_NOT_FOUND,
  B_RESOURCE_NOT_FOUND,
  B_RESOURCE_UNAVAILABLE,
  B_BAD_SUBSCRIBER,
  B_SUBSCRIBER_NOT_ENTERED,
  B_BUFFER_NOT_AVAILABLE,
  B_LAST_BUFFER_ERROR
};

/*-------------------------------------------------------------*/
/*----- Mail Kit Errors ---------------------------------------*/
enum
{
	B_MAIL_NO_DAEMON = B_MAIL_ERROR_BASE,
	B_MAIL_UNKNOWN_USER,
	B_MAIL_WRONG_PASSWORD,
	B_MAIL_UNKNOWN_HOST,
	B_MAIL_ACCESS_ERROR,
	B_MAIL_UNKNOWN_FIELD,
	B_MAIL_NO_RECIPIENT,
	B_MAIL_INVALID_MAIL
};

/*-------------------------------------------------------------*/
/*----- Printing Errors --------------------------------------*/
enum
{
	B_NO_PRINT_SERVER = B_PRINT_ERROR_BASE
};

/*-------------------------------------------------------------*/
/*----- Device Kit Errors -------------------------------------*/
enum
{
	B_DEV_INVALID_IOCTL = B_DEVICE_ERROR_BASE,
	B_DEV_NO_MEMORY,
	B_DEV_BAD_DRIVE_NUM,
	B_DEV_NO_MEDIA,
	B_DEV_UNREADABLE,
	B_DEV_FORMAT_ERROR,
	B_DEV_TIMEOUT,
	B_DEV_RECALIBRATE_ERROR,
	B_DEV_SEEK_ERROR,
	B_DEV_ID_ERROR,
	B_DEV_READ_ERROR,
	B_DEV_WRITE_ERROR,
	B_DEV_NOT_READY,
	B_DEV_MEDIA_CHANGED,
	B_DEV_MEDIA_CHANGE_REQUESTED,
	B_DEV_RESOURCE_CONFLICT,
	B_DEV_CONFIGURATION_ERROR,
	B_DEV_DISABLED_BY_USER,
	B_DEV_DOOR_OPEN
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _ERRORS_H */
