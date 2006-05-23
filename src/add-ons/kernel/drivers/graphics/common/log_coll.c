/*
	Copyright (c) 2002, Thomas Kurschel

	Part of Radeon driver

	Both kernel and user space part.
	(init and clean-up must be done in
	kernel space).
*/


#include "log_coll.h"

#include <KernelExport.h>
#include <OS.h>

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

typedef struct log_info_t {
	char *log_buffer;
	uint32 log_buffer_len;
	uint32 log_buffer_pos;
	area_id area;
} log_info;

#ifdef ENABLE_LOGGING


// write one log entry
void log( log_info *li, uint16 what, const uint8 num_args, ... )
{
	uint32 pos;
	va_list vl;
	log_entry *entry;
	uint32 i;
	uint32 entry_size;
	
	entry_size = sizeof( log_entry ) + (num_args - 1) * sizeof( uint32 );
	pos = atomic_add( &li->log_buffer_pos, entry_size );
		
	if( li->log_buffer_pos > li->log_buffer_len ) {
		atomic_add( &li->log_buffer_pos, -entry_size );
		return;
	}
	
	entry = (log_entry *)&li->log_buffer[pos];
	
	entry->tsc = system_time();
	entry->what = what;
	entry->num_args = num_args;
	
	va_start( vl, num_args );
	for( i = 0; i < num_args; ++i ) {
		entry->args[i] = va_arg( vl, uint32 );
	}
	va_end( vl );
}

#ifdef LOG_INCLUDE_STARTUP

// create log buffer
log_info *log_init( uint32 size )
{
	log_info *li;
	area_id area;

	// buffer must be accessible from user mem
	// to allow logging from there as well;
	// you cannot clone this area as there are
	// pointers which would break (it wouldn't be
	// hard to get rid of them, but I don't care
	// and keep it as simple as possible)
	area = create_area( "fast_logger", 
		(void **)&li, B_ANY_KERNEL_ADDRESS, 
		(sizeof( log_info ) + size + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1), 
		B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA );
		
	if( area < 0 )
		panic( "Radeon Fast logger: cannot allocate %ld byte for logging data\n", size );

	li->area = area;
	li->log_buffer = (char *)li + sizeof( log_info );
	li->log_buffer_len = size;
	li->log_buffer_pos = 0;
	
	return li;
}

// clean-up logging
void log_exit( log_info *li )
{
	li->log_buffer_pos = 0;
	//free( li->log_buffer );
	delete_area( li->area );
}

#endif

#endif


#ifdef LOG_INCLUDE_STARTUP

// get *current* size of logging data
uint32 log_getsize( log_info *li )
{
	if( li == NULL )
		return 0;
		
	dprintf( "RADEON -- log_getsize: log_pos %ld\n", li->log_buffer_pos );
	return li->log_buffer_pos;
}

// get up to max_size bytes of logging data
void log_getcopy( log_info *li, void *dest, uint32 max_size )
{
	if( li == NULL )
		return;
		
	dprintf( "RADEON -- log_getcopy: max_size %ld, log_pos %ld\n", 
		max_size, li->log_buffer_pos );
	memcpy( dest, li->log_buffer, min( li->log_buffer_pos, max_size ));
	
	li->log_buffer_pos = 0;
}

#endif
