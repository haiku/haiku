/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
	
	Fast logger
	
	As syslog is very slow and tends to loose
	data if its buffer overflows (which occurs much
	too often), this module provides a fast (and memory-
	wasting) logging mechanism. You need a seperate 
	application to retrieve the log.
	
	Everything is thread-safe.
*/


#ifndef __LOG_COLL_H__
#define __LOG_COLL_H__

#include <SupportDefs.h>

// by undefining this flag, all logging functions
// are resolved to empty space, so don't add
// extra tests in your code
#undef ENABLE_LOGGING
//#define ENABLE_LOGGING


// add log entry with 0..3 (uint32) data
#define LOG( li, what ) log( li, what, 0 )
#define LOG1( li, what, arg1 ) log( li, what, 1, arg1 );
#define LOG2( li, what, arg1, arg2 ) log( li, what, 2, arg1, arg2 );
#define LOG3( li, what, arg1, arg2, arg3 ) log( li, what, 3, arg1, arg2, arg3 );


// one log entry
typedef struct log_entry_t {
	uint64 tsc;
	uint16 what;
	uint8 num_args;
	uint32 args[1];
} log_entry;

struct log_info_t;

#if defined(__cplusplus)
extern "C" {
#endif


#ifdef ENABLE_LOGGING
void log( struct log_info_t *li, uint16 what, const uint8 num_args, ... );
#else
#define log( a, b, c... )
#endif


// define LOG_INCLUDE_STARTUP in your device driver
#ifdef LOG_INCLUDE_STARTUP

uint32 log_getsize( struct log_info_t *li );
void log_getcopy( struct log_info_t *li, void *dest, uint32 max_size );

#ifdef ENABLE_LOGGING

struct log_info_t *log_init( uint32 size );
void log_exit( struct log_info_t *li );

#else

#define log_init( a ) NULL
#define log_exit( a )

#endif

#endif

#if defined(__cplusplus)
}
#endif


#endif
