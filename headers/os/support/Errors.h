#ifndef _ERRORS_H
#define _ERRORS_H

/* This file contains error codes as defined by BeOS R5.
 * 
 * There are 2 other files that contain error codes,
 * codes starting E are found in posix/errno.h
 * codes starting with ERR_ are kernel only and found in
 * private/kernel/kerrors.h
 */

#include <errno.h>

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
//#define B_POSIX_ERROR_BASE			B_GENERAL_ERROR_BASE + 0x7000
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
