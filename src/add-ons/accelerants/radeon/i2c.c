/*
	Copyright (c) 2003, Thomas Kurschel


	Part of DDC driver
		
	I2C protocoll
*/

#include <OS.h>
#include <KernelExport.h>

#include "i2c.h"

#include "ddc_int.h"


// there's no spin in user space, but we need it to wait a couple 
// of microseconds only
// (in this case, snooze has much too much overhead)
void spin( bigtime_t delay )
{
	bigtime_t start_time = system_time();
	
	while( system_time() - start_time < delay )
		;
}


// wait until slave releases clock signal ("clock stretching")
static status_t wait_for_clk( const i2c_bus *bus, const i2c_timing *timing, 
	bigtime_t timeout )
{
	bigtime_t start_time;
	
	// wait for clock signal to raise
	spin( timing->r );

	start_time = system_time();
	
	while( 1 ) {
		int clk, data;
		
		bus->get_signals( bus->cookie, &clk, &data );
		if( clk != 0 )
			return B_OK;
			
		if( system_time() - start_time > timeout )
			return B_TIMEOUT;
			
		spin( timing->r );
	}
}


// send start or repeated start condition
static status_t send_start_condition( const i2c_bus *bus, const i2c_timing *timing )
{
	status_t res;
	
	bus->set_signals( bus->cookie, 1, 1 );
	
	res = wait_for_clk( bus, timing, timing->start_timeout );
	if( res != B_OK ) {
		SHOW_FLOW0( 2, "Timeout sending start condition" );
		return res;
	}
		
	spin( timing->su_sta );
	bus->set_signals( bus->cookie, 1, 0 );
	spin( timing->hd_sta );
	bus->set_signals( bus->cookie, 0, 0 );
	spin( timing->f );
	
	return B_OK;
}


// send stop condition
static status_t send_stop_condition( const i2c_bus *bus, const i2c_timing *timing )
{
	status_t res;
	
	bus->set_signals( bus->cookie, 0, 0 );
	spin( timing->r );
	bus->set_signals( bus->cookie, 1, 0 );
	
	// a slave may wait for us, so let elapse the acknowledge timeout
	// to make the slave release bus control
	res = wait_for_clk( bus, timing, timing->ack_timeout );
	if( res != B_OK ) {
		SHOW_FLOW0( 2, "Timeout sending stop condition" );
		return res;
	}

	spin( timing->su_sto );
	bus->set_signals( bus->cookie, 1, 1 );
	spin( timing->buf );
	
	SHOW_FLOW0( 3, "" );
	
	return B_OK;
}


// send one bit
static status_t send_bit( const i2c_bus *bus, const i2c_timing *timing, bool bit, int timeout )
{
	status_t res;
	
	//SHOW_FLOW( 3, "%d", bit & 1 );
	
	bus->set_signals( bus->cookie, 0, bit & 1 );
	spin( timing->su_dat );
	bus->set_signals( bus->cookie, 1, bit & 1 );
	
	res = wait_for_clk( bus, timing, timeout );
	if( res != B_OK ) {
		SHOW_FLOW0( 2, "Timeout when sending next bit" );
		return res;
	}
		
	spin( timing->high );
	bus->set_signals( bus->cookie, 0, bit & 1 );
	spin( timing->f + timing->low );
	
	return B_OK;
}


// send acknowledge and wait for reply
static status_t send_acknowledge( const i2c_bus *bus, const i2c_timing *timing )
{
	status_t res;
	bigtime_t start_time;
	
	// release data so slave can modify it
	bus->set_signals( bus->cookie, 0, 1 );
	spin( timing->su_dat );
	bus->set_signals( bus->cookie, 1, 1 );
	
	res = wait_for_clk( bus, timing, timing->ack_start_timeout );
	if( res != B_OK ) {
		SHOW_FLOW0( 2, "Timeout when sending acknowledge" );
		return res;
	}

	// data and clock is high, now wait for slave to pull data low
	// (according to spec, this can happen any time once clock is high)
	start_time = system_time();
	
	while( 1 ) {
		int clk, data;
		
		bus->get_signals( bus->cookie, &clk, &data );
		
		if( data == 0 )
			break;
			
		if( system_time() - start_time > timing->ack_timeout ) {
			SHOW_FLOW0( 2, "Slave didn't acknowledge byte" );
			return B_TIMEOUT;
		}
			
		spin( timing->r );
	}
	
	SHOW_FLOW0( 4, "Success!" );
	
	// make sure we've waited at least t_high 
	spin( timing->high );

	bus->set_signals( bus->cookie, 0, 1 );
	spin( timing->f + timing->low );	

	return B_OK;
}


// send byte and wait for acknowledge if <ackowledge> is true
static status_t send_byte( const i2c_bus *bus, const i2c_timing *timing,
	uint8 byte, bool acknowledge )
{
	int i;
	
	SHOW_FLOW( 2, "%x ", byte );

	for( i = 7; i >= 0; --i ) {
		status_t res;
		
		res = send_bit( bus, timing, byte >> i, 
			i == 7 ? timing->byte_timeout : timing->bit_timeout );
		if( res != B_OK )
			return res;
	}

	if( acknowledge )
		return send_acknowledge( bus, timing );
	else
		return B_OK;
}

// send slave address, obeying 10-bit addresses and general call addresses
static status_t send_slave_address( const i2c_bus *bus, 
	const i2c_timing *timing, int slave_address, bool is_write )
{
	status_t res;
	
	res = send_byte( bus, timing, (slave_address & 0xfe) | !is_write, true );
	if( res != B_OK )
		return res;
		
	// there are the following special cases if the first byte looks like:
	// - 0000 0000 - general call address (second byte with address follows)
	// - 0000 0001 - start byte
	// - 0000 001x - CBus address
	// - 0000 010x - address reserved for different bus format
	// - 0000 011x |
	// - 0000 1xxx |-> reserved
	// - 1111 1xxx |
	// - 1111 0xxx - 10 bit address  (second byte contains remaining 8 bits)
	
	// the lsb is 0 for write and 1 for read (except for general call address)
	if( (slave_address & 0xff) != 0 &&
		(slave_address & 0xf8) != 0xf0 )
		return B_OK;

	// send second byte if required		
	return send_byte( bus, timing, slave_address >> 8, true );
}


// receive one bit
static status_t receive_bit( const i2c_bus *bus, const i2c_timing *timing, 
	bool *bit, int timeout )
{
	status_t res;
	int clk, data;

	// release clock
	bus->set_signals( bus->cookie, 1, 1 );

	// wait for slave to raise clock
	res = wait_for_clk( bus, timing, timeout );
	if( res != B_OK ) {
		SHOW_FLOW0( 2, "Timeout waiting for bit sent by slave" );
		return res;
	}
	
	// sample data
	bus->get_signals( bus->cookie, &clk, &data );
	// leave clock high for minimal time
	spin( timing->high );
	// pull clock low so slave waits for us before next bit
	bus->set_signals( bus->cookie, 0, 1 );
	// let it settle and leave it low for minimal time
	// to make sure slave has finished bit transmission too
	spin( timing->f + timing->low);

	*bit = data;	
	return B_OK;
}

// receive byte
// send positive acknowledge afterwards if <acknowledge> is true, else send negative one
static status_t receive_byte( const i2c_bus *bus, const i2c_timing *timing, 
	uint8 *res_byte, bool acknowledge )
{
	uint8 byte = 0;
	int i;
	
	// pull clock low to let slave wait for us
	bus->set_signals( bus->cookie, 0, 1 );

	for( i = 7; i >= 0; --i ) {
		status_t res;
		bool bit;
		
		res = receive_bit( bus, timing, &bit,
			i == 7 ? timing->byte_timeout : timing->bit_timeout );
		if( res != B_OK )
			return res;
			
		byte = (byte << 1) | bit;
	}
	
	//SHOW_FLOW( 3, "%x ", byte );
	
	*res_byte = byte;
	
	return send_bit( bus, timing, acknowledge ? 0 : 1, timing->bit_timeout );
}

// send multiple bytes
static status_t send_bytes( const i2c_bus *bus, const i2c_timing *timing, 
	const uint8 *write_buffer, ssize_t write_len )
{
	SHOW_FLOW( 3, "len=%ld", write_len );
	
	for( ; write_len > 0; --write_len, ++write_buffer ) {
		status_t res;
		
		res = send_byte( bus, timing, *write_buffer, true );
		if( res != B_OK )
			return res;
	}
	
	return B_OK;
}

// receive multiple bytes
static status_t receive_bytes( const i2c_bus *bus, const i2c_timing *timing, 
	uint8 *read_buffer, ssize_t read_len )
{
	SHOW_FLOW( 3, "len=%ld", read_len );
	
	for( ; read_len > 0; --read_len, ++read_buffer ) {
		status_t res;
		
		res = receive_byte( bus, timing, read_buffer, read_len > 1 );
		if( res != B_OK )
			return res;
	}
	
	return B_OK;
}

// combined i2c send+receive format
status_t i2c_send_receive( const i2c_bus *bus, const i2c_timing *timing,
	int slave_address, 
	const uint8 *write_buffer, size_t write_len, 
	uint8 *read_buffer, size_t read_len )
{
	status_t res;
	
	res = send_start_condition( bus, timing );
	if( res != B_OK )
		return res;
		
	res = send_slave_address( bus, timing, slave_address, true );
	if( res != B_OK )
		goto err;
		
	res = send_bytes( bus, timing, write_buffer, write_len );
	if( res != B_OK )
		goto err;
		
	res = send_start_condition( bus, timing );
	if( res != B_OK )
		return res;
		
	res = send_slave_address( bus, timing, slave_address, false );
	if( res != B_OK )
		goto err;
		
	res = receive_bytes( bus, timing, read_buffer, read_len );
	if( res != B_OK )
		goto err;
		
	res = send_stop_condition( bus, timing );
	return res;
	
err:
	SHOW_FLOW0( 2, "Cancelling transmission" );
	send_stop_condition( bus, timing );
	return res;
}

// timining for 100kHz bus (fractional parts are rounded up)
i2c_timing i2c_timing_100k = 
{
	buf : 5,
	hd_sta : 4,
	low : 5,
	high : 4,
	su_sta : 5,
	hd_dat : 0,
	su_dat : 1,
	r : 1,
	f : 1,
	su_sto : 4,
	
	// as these are unspecified, we use half a clock cycle as a safe guess
	start_timeout : 5,
	byte_timeout : 5,
	bit_timeout : 5,
	ack_start_timeout : 5,
	ack_timeout : 5
};

// timing for 400 kHz bus
// (argh! heavy up-rounding here)
i2c_timing i2c_timing_400k = 
{
	buf : 2,
	hd_sta : 1,
	low : 2,
	high : 1,
	su_sta : 1,
	hd_dat : 0,
	su_dat : 1,
	r : 1,
	f : 1,
	su_sto : 1,
	
	// see i2c_timing_100k
	start_timeout : 2,
	byte_timeout : 2,
	bit_timeout : 2,
	ack_start_timeout : 2,
	ack_timeout : 2
};

void i2c_get100k_timing( i2c_timing *timing )
{
	*timing = i2c_timing_100k;
}

void i2c_get400k_timing( i2c_timing *timing )
{
	*timing = i2c_timing_400k;
}
