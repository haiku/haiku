/*
	Copyright (c) 2003, Thomas Kurschel


	Part of DDC driver
		
	Test program, using Radeon Kernel Driver for I2C access.
	
	!DANGER! You can specify _any_ io-port to use for i2c 
	transfer - this is a good way to mess up your hardware.
	Usual addresses are 96, 100, 104 and 108. 
	You've been warned.
*/

#include "ddc.h"
#include "radeon_interface.h"
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <KernelExport.h>
#include <stdlib.h>

status_t get_signals( void *cookie, int *clk, int *data );
status_t set_signals( void *cookie, int clk, int data );

int io_port;

status_t get_signals( void *cookie, int *clk, int *data )
{
	int fd = (int)cookie;
	radeon_getset_i2c buffer;
	status_t res;
	
	buffer.magic = RADEON_PRIVATE_DATA_MAGIC;
	buffer.port = io_port;
	
	res = ioctl( fd, RADEON_GET_I2C_SIGNALS, &buffer, sizeof( buffer ));
	if( res != B_OK )
		return res;
		
	*clk = (buffer.value >> 9) & 1;
	*data = (buffer.value >> 8) & 1;
	
	//printf( "read: %i, %i\n", *clk, *data );
	
	return B_OK;
}

bigtime_t max_time = 0;
bigtime_t old_time;

status_t set_signals( void *cookie, int clk, int data )
{
	int fd = (int)cookie;
	radeon_getset_i2c buffer;
	status_t res;
	bigtime_t new_time;	
	
	old_time = system_time();
	
	buffer.magic = RADEON_PRIVATE_DATA_MAGIC;
	buffer.port = io_port;

	res = ioctl( fd, RADEON_GET_I2C_SIGNALS, &buffer, sizeof( buffer ));
	if( res != B_OK )
		return res;

	buffer.value &= ~((1 << 1) | (1 << 0));
	buffer.value &= ~((1 << 16) | (1 << 17));		
	buffer.value |= ((1-clk) << 17) | ((1-data) << 16);
	//buffer.value |= (1 << 16) | (1 <<17 );
	
	//printf( "write: %i, %i\n", clk, data );
	new_time = system_time();
	max_time = max( max_time, new_time - old_time );
	
	old_time = new_time;
	
				
	return ioctl( fd, RADEON_SET_I2C_SIGNALS, &buffer, sizeof( buffer ));
}

int main( int argc, char **argv )
{
	int fd;
	i2c_bus bus;
	status_t res;
	edid1_info edid;
	void *vdif;
	size_t vdif_len;
	char *name = argv[1];
	
	if( argc < 3 ) {
		fprintf( stderr, "usage: test_ddc driver_name io_port\n" );
		return 2;
	}
	
	//name = "/dev/graphics/1002_4c59_010000";
	
	fd = open( name, O_RDWR );
	if( fd < 0 ) {
		fprintf( stderr, "Cannot open device %s\n", argv[1] );
		return 3;
	}
	
	io_port = atoi( argv[2] );
	
	fprintf( stderr, "io-port: %x\n", io_port );
	
	bus.cookie = (void *)fd;
	bus.set_signals = &set_signals;
	bus.get_signals = &get_signals;
	
	old_time = system_time();
	
	res = ddc2_read_edid1( &bus, &edid, &vdif, &vdif_len );
	if( res < 0 ) {
		printf( "%i", (int)max_time );
		fprintf( stderr, "Error reading edid: %s\n", strerror( res ));
		return 1;
	}
	
	edid_dump( &edid );
	
	fprintf( stderr, "success\n" );
	return 0;
}
