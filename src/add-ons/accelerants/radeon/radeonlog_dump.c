/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	Fast logger - application to write log file
*/


#include <stdio.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include "log_dump.h"
#include <OS.h>
#include "radeon_interface.h"


// usage: "radeonlog_dump device_name"
// result gets written into "radeonlog" in home directory
int main(int argc, char **argv)
{
	int device;
	uint32 size;
	char *buffer;
	status_t res;
	FILE *logfile;
	const char *logfile_name;
	
	if( argc < 2 ) {
		fprintf( stderr, "radeonlog: missing device name\n" );
		return 3;
	}
	
	device = open( argv[1], O_RDONLY );
	
	if( device < 0 ) {
		fprintf( stderr, "radeonlog: cannot open log helper %s (%s)\n", 
			argv[1], strerror( device ));
		return 3;
	}
	
	logfile_name = "/boot/home/radeonlog";
	
	logfile = fopen( logfile_name, "at" );
	
	if( logfile == NULL ) {
		fprintf( stderr, "idelog: cannot open log file %s\n", logfile_name );
		return 3;
	}

	if( (res = ioctl( device, RADEON_GET_LOG_SIZE, &size, sizeof( size ))) != B_OK ) {
		fprintf( stderr, "idelog: RADEON_GET_LOG_SIZE failed, %s\n", strerror( res ));
		return 3;
	}

	fprintf( logfile, "buffer size: %ld\n", size );
	
	buffer = malloc( size + sizeof( int32 ));
	((uint32*)buffer)[0] = size;

	if( (res = ioctl( device, RADEON_GET_LOG_DATA, buffer, size )) != B_OK ) {
		fprintf( stderr, "idelog: RADEON_GET_LOG_DATA failed, %s\n", strerror( res ));
		return 3;
	}

	log_printall( logfile, buffer, size );
	
	fclose( logfile );
	
	return 0;
}
