/* Errors.h
 *
 * Error codes used by OpenBeOS.
 * 
 * NB Where the codes can be directly mapped to a POSIX
 * error value they have been.
 *
 * XXX - should be installed as support/Errors.h to keep
 *       compatibility with beos source code
 */

/* The codes shown here are the ones that Be defined. If new
 * error codes are needed they should be added AT THE END of the 
 * enums to make sure we don't damage our compatibility.
 */

#ifndef _ERRORS_H
#define _ERRORS_H

#include <errno.h>

/* XXX - paranoid...should be in limits.h */
#ifndef LONG_MIN
#define LONG_MIN          (-2147483647L-1)
#endif

/* These are the start points for the various categories of
 * error.
 * These are identical to the Be codes so we have compatibility
 * with them.
 */
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

/* This can be used to add user-defined errors, use the
 * value B_ERRORS_END +1 as the first value.
 */
#define B_ERRORS_END		(B_GENERAL_ERROR_BASE + 0xffff)

#define B_NO_ERROR   0
#define B_OK         0
#define B_ERROR     -1

enum {
	B_NO_MEMORY = B_GENERAL_ERROR_BASE, /* ENOMEM */
	B_IO_ERROR,					        /* EIO */
	B_PERMISSION_DENIED,		        /* EACCES */
	B_BAD_INDEX,				
	B_BAD_TYPE,					
	B_BAD_VALUE,                        /* EINVAL */			
	B_MISMATCHED_VALUES,		
	B_NAME_NOT_FOUND,			
	B_NAME_IN_USE,			
	B_TIMED_OUT,                        /* ETIMEDOUT */
    B_INTERRUPTED,                      /* EINTR */
	B_WOULD_BLOCK,                      /* EWOULDBLOCK == EAGAIN */
    B_CANCELED,
	B_NO_INIT,			
	B_BUSY,                             /* EBUSY */				
	B_NOT_ALLOWED,                      /* EPERM */
};

/* Kernel Kit Errors */
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

/* Application Kit Errors */
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


/* Storage Kit & File System Errors */
enum {
	B_FILE_ERROR = B_STORAGE_ERROR_BASE,     /* EBADF */     
	B_FILE_NOT_FOUND,	/* discouraged; use B_ENTRY_NOT_FOUND in new code*/
	B_FILE_EXISTS,                           /* EEXIST */
	B_ENTRY_NOT_FOUND,                       /* ENOENT */
	B_NAME_TOO_LONG,                         /* ENAMETOOLONG */
	B_NOT_A_DIRECTORY,                       /* ENOTDIR */
	B_DIRECTORY_NOT_EMPTY,                   /* ENOTEMPTY */
	B_DEVICE_FULL,                           /* ENOSPC */
	B_READ_ONLY_DEVICE,                      /* EROFS */
	B_IS_A_DIRECTORY,                        
	B_NO_MORE_FDS,				
	B_CROSS_DEVICE_LINK,		
	B_LINK_LIMIT,			    
	B_BUSTED_PIPE,				
	B_UNSUPPORTED,
	B_PARTITION_TOO_SMALL
};

/* Media Kit Errors */
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

/* Mail Kit Errors */
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

/* Printing Errors */
enum
{
	B_NO_PRINT_SERVER = B_PRINT_ERROR_BASE
};

/* Device Kit Errors */
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

/* These are the old newos errors - we should be trying to remove these
 * in favour of the error codes above!!!
 */

/* General errors */
#define ERR_GENERAL              -1
#define ERR_NO_MEMORY            ENOMEM
#define ERR_IO_ERROR             EIO
#define ERR_INVALID_ARGS         EINVAL
#define ERR_TIMED_OUT            ETIMEDOUT
#define ERR_NOT_ALLOWED          EPERM
#define ERR_PERMISSION_DENIED    EACCES
#define ERR_INVALID_BINARY       ERR_GENERAL-7
#define ERR_INVALID_HANDLE       ERR_GENERAL-8
#define ERR_NO_MORE_HANDLES      ERR_GENERAL-9
#define ERR_UNIMPLEMENTED        ENOSYS
#define ERR_TOO_BIG              EDOM
#define ERR_NOT_FOUND            ERR_GENERAL-12
#define ERR_NOT_IMPLEMENTED_YET  ERR_GENERAL-13

/* Semaphore errors */
#define ERR_SEM_GENERAL          -1024
#define ERR_SEM_DELETED          ERR_SEM_GENERAL-1
#define ERR_SEM_TIMED_OUT        ERR_SEM_GENERAL-2
#define ERR_SEM_OUT_OF_SLOTS     ERR_SEM_GENERAL-3
#define ERR_SEM_NOT_ACTIVE       ERR_SEM_GENERAL-4
#define ERR_SEM_INTERRUPTED      ERR_SEM_GENERAL-5
#define ERR_SEM_NOT_INTERRUPTABLE ERR_SEM_GENERAL-6
#define ERR_SEM_NOT_FOUND        ERR_SEM_GENERAL-7


/* Tasker errors */
#define ERR_TASK_GENERAL         -2048
#define ERR_TASK_PROC_DELETED    ERR_TASK_GENERAL-1

/* VFS errors */
#define ERR_VFS_GENERAL          -3072
#define ERR_VFS_INVALID_FS       ERR_VFS_GENERAL-1
#define ERR_VFS_NOT_MOUNTPOINT   ERR_VFS_GENERAL-2
#define ERR_VFS_PATH_NOT_FOUND   ERR_VFS_GENERAL-3
#define ERR_VFS_INSUFFICIENT_BUF ENOBUFS
#define ERR_VFS_READONLY_FS      EROFS
#define ERR_VFS_ALREADY_EXISTS   EEXIST
#define ERR_VFS_FS_BUSY          ERR_VFS_GENERAL-7
#define ERR_VFS_FD_TABLE_FULL    ERR_VFS_GENERAL-8
#define ERR_VFS_CROSS_FS_RENAME  ERR_VFS_GENERAL-9
#define ERR_VFS_DIR_NOT_EMPTY    ERR_VFS_GENERAL-10
#define ERR_VFS_NOT_DIR          ENOTDIR
#define ERR_VFS_WRONG_STREAM_TYPE   ERR_VFS_GENERAL-12
#define ERR_VFS_ALREADY_MOUNTPOINT ERR_VFS_GENERAL-13

/* VM errors */
#define ERR_VM_GENERAL           -4096
#define ERR_VM_INVALID_ASPACE    ERR_VM_GENERAL-1
#define ERR_VM_INVALID_REGION    ERR_VM_GENERAL-2
#define ERR_VM_BAD_ADDRESS       ERR_VM_GENERAL-3
#define ERR_VM_PF_FATAL          ERR_VM_GENERAL-4
#define ERR_VM_PF_BAD_ADDRESS    ERR_VM_GENERAL-5
#define ERR_VM_PF_BAD_PERM       ERR_VM_GENERAL-6
#define ERR_VM_PAGE_NOT_PRESENT  ERR_VM_GENERAL-7
#define ERR_VM_NO_REGION_SLOT    ERR_VM_GENERAL-8
#define ERR_VM_WOULD_OVERCOMMIT  ERR_VM_GENERAL-9
#define ERR_VM_BAD_USER_MEMORY   ERR_VM_GENERAL-10

/* Elf errors */
#define ERR_ELF_GENERAL		  -5120
#define ERR_ELF_RESOLVING_SYMBOL  ERR_ELF_GENERAL -1

#endif /* _ERRORS_H */
