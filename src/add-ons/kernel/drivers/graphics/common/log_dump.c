/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	Fast logger - functions to create dump
*/


#include <stdio.h>
#include <OS.h>

#include "log_coll.h"
#include "log_dump.h"
#include "log_enum.h"
#include "log_names.h"

system_info sysinfo;

// dump one entry
static void log_printentry( FILE *logfile, log_entry *entry )
{
	uint64 time;
	uint32 min, sec, mill, mic;
		
	time = entry->tsc / (sysinfo.cpu_clock_speed / 1000000);
	mic = time % 1000;
	time /= 1000;
	mill = time % 1000;
	time /= 1000;
	sec = time % 60;
	time /= 60;
	min = time;
	
	fprintf( logfile, "%03ld:%02ld:%03ld.%03ld ", min, sec, mill, mic );
	if( entry->what < sizeof( log_names ) / sizeof( log_names[0] ) )
		fprintf( logfile, log_names[entry->what] );
	else
		fprintf( logfile, "unknown %ld", (uint32)entry->what );
		
	if( entry->num_args > 0 ) {
		uint32 i;
		
		fprintf( logfile, " (" );
		for( i = 0; i < entry->num_args; ++i ) {
			if( i > 0 )
				fprintf( logfile, ", " );
				
			fprintf( logfile, "0x%08lx", entry->args[i] );
		}
		fprintf( logfile, ")" );
	}
	
	fprintf( logfile, "\n" );
}


// dump entire log
void log_printall( FILE *logfile, char *buffer, uint32 buffer_len )
{
	uint32 pos;
	
	get_system_info( &sysinfo );

	for( pos = 0; pos < buffer_len;  ) {
		log_entry *entry;
		
		entry = (log_entry *)(buffer + pos);
		log_printentry( logfile, entry/*, &tsc*/ );
		pos += sizeof( log_entry ) + (entry->num_args - 1) * sizeof( uint32 );
	}
}
