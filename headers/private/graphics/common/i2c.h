/*
 * Copyright 2003, Thomas Kurschel. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _I2C_H
#define _I2C_H


#include <OS.h>


// timing for i2c bus
typedef struct i2c_timing {
	// general timing as defined by standard 
	// (in microseconds for 100kHz/400kHz mode)
	int buf;		// bus free between start and stop (4.7/1.3)
	int hd_sta;		// hold time start condition (4.0/0.6)
	int low;		// low period of clock (4.7/1.3)
	int high;		// high period of clock (4.0/0.6)
	int su_sta;		// setup time of repeated start condition (4.7/0.6)
	int hd_dat;		// hold time data (5.0/- for CBUS, 0/0 for I2C)
	int su_dat;		// setup time data (0.250/0.100)
	int r;			// maximum raise time of clock and data signal (1.0/0.3)
	int f;			// maximum fall time of clock and data signal (0.3/0.3)
	int su_sto;		// setup time for stop condition (4.0/0.6)
	
	// clock stretching limits, not part of i2c standard
	int start_timeout;	// max. delay of start condition
	int byte_timeout;	// max. delay of first bit of byte
	int bit_timeout;	// max. delay of one bit within a byte transmission
	int ack_start_timeout;	// max. delay of acknowledge start
	
	// other timeouts, not part of i2c standard
	int ack_timeout;	// timeout of waiting for acknowledge
} i2c_timing;


// set signals on bus
typedef status_t (*i2c_set_signals)(void *cookie, int clock, int data);
// read signals from bus
typedef status_t (*i2c_get_signals)(void *cookie, int *clock, int *data);

// i2c bus definition
typedef struct i2c_bus {
	void *cookie;					// user-defined cookie
	i2c_timing timing;
	i2c_set_signals set_signals;	// callback to set signals
	i2c_get_signals get_signals;	// callback to detect signals
} i2c_bus;


#ifdef __cplusplus
extern "C" {
#endif

// send and receive data via i2c bus
status_t i2c_send_receive(const i2c_bus *bus, int slave_address,
	const uint8 *writeBuffer, size_t writeLength, uint8 *readBuffer,
	size_t readLength);

// fill <timing> with standard 100kHz bus timing
void i2c_get100k_timing(i2c_timing *timing);

// fill <timing> with standard 400kHz bus timing
// (as timing resolution is 1 microsecond, we cannot reach full speed!)
void i2c_get400k_timing(i2c_timing *timing);

#ifdef __cplusplus
}
#endif

#endif	/* _I2C_H */
