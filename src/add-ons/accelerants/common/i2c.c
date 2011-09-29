/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All Rights Reserved.
 * Copyright 2003, Thomas Kurschel. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

/*!
	I2C protocol
*/

#include "i2c.h"

#include <KernelExport.h>
#include <OS.h>
#include <string.h>


//#define TRACE_I2C
#ifdef TRACE_I2C
#ifdef __cplusplus
extern "C" 
#endif
void _sPrintf(const char *format, ...);
#	define TRACE(x...) _sPrintf("I2C: " x)
#else
#	define TRACE(x...) ;
#endif


/*!
	I2c timings, rounded up (Phillips 1995 i2c bus specification, p20)
*/

//! Timing for standard mode i2c (100kHz max)
const static i2c_timing kTiming100k = {
	.buf = 5,
	.hd_sta = 4,
	.low = 5,
	.high = 4,
	.su_sta = 5,
	.hd_dat = 1,
	.su_dat = 2,
	.r = 2,
	.f = 2,
	.su_sto = 4,

	// these are unspecified, use half a clock cycle as a safe guess
	.start_timeout = 5,
	.byte_timeout = 5,
	.bit_timeout = 5,
	.ack_start_timeout = 5,
	.ack_timeout = 5
};

//! Timing for fast mode i2c (400kHz max)
const static i2c_timing kTiming400k = {
	.buf = 2,
	.hd_sta = 1,
	.low = 2,
	.high = 1,
	.su_sta = 1,
	.hd_dat = 0,
	.su_dat = 1,
	.r = 1,
	.f = 1,
	.su_sto = 1,

	// these are unspecified, use half a clock cycle as a safe guess
	.start_timeout = 2,
	.byte_timeout = 2,
	.bit_timeout = 2,
	.ack_start_timeout = 2,
	.ack_timeout = 2
};


/*!
	There's no spin in user space, but we need it to wait a couple
	of microseconds only
	(in this case, snooze has much too much overhead)
*/
void
spin(bigtime_t delay)
{
	bigtime_t startTime = system_time();

	while (system_time() - startTime < delay)
		;
}


//! Wait until slave releases clock signal ("clock stretching")
static status_t
wait_for_clk(const i2c_bus *bus, bigtime_t timeout)
{
	bigtime_t startTime;

	// wait for clock signal to raise
	spin(bus->timing.r);

	startTime = system_time();

	while (true) {
		int clk, data;

		bus->get_signals(bus->cookie, &clk, &data);
		if (clk != 0)
			return B_OK;

		if (system_time() - startTime > timeout)
			return B_TIMEOUT;

		spin(bus->timing.r);
	}
}


//!	Send start or repeated start condition
static status_t
send_start_condition(const i2c_bus *bus)
{
	status_t status;

	bus->set_signals(bus->cookie, 1, 1);

	status = wait_for_clk(bus, bus->timing.start_timeout);
	if (status != B_OK) {
		TRACE("%s: Timeout sending start condition\n", __func__);
		return status;
	}

	spin(bus->timing.su_sta);
	bus->set_signals(bus->cookie, 1, 0);
	spin(bus->timing.hd_sta);
	bus->set_signals(bus->cookie, 0, 0);
	spin(bus->timing.f);

	return B_OK;
}


//!	Send stop condition
static status_t
send_stop_condition(const i2c_bus *bus)
{
	status_t status;

	bus->set_signals(bus->cookie, 0, 0);
	spin(bus->timing.r);
	bus->set_signals(bus->cookie, 1, 0);

	// a slave may wait for us, so let elapse the acknowledge timeout
	// to make the slave release bus control
	status = wait_for_clk(bus, bus->timing.ack_timeout);
	if (status != B_OK) {
		TRACE("%s: Timeout sending stop condition\n", __func__);
		return status;
	}

	spin(bus->timing.su_sto);
	bus->set_signals(bus->cookie, 1, 1);
	spin(bus->timing.buf);

	return B_OK;
}


//!	Send one bit
static status_t
send_bit(const i2c_bus *bus, uint8 bit, int timeout)
{
	status_t status;

	//TRACE("send_bit(bit = %d)\n", bit & 1);

	bus->set_signals(bus->cookie, 0, bit & 1);
	spin(bus->timing.su_dat);
	bus->set_signals(bus->cookie, 1, bit & 1);

	status = wait_for_clk(bus, timeout);
	if (status != B_OK) {
		TRACE("%s: Timeout when sending next bit\n", __func__);
		return status;
	}

	spin(bus->timing.high);
	bus->set_signals(bus->cookie, 0, bit & 1);
	spin(bus->timing.f + bus->timing.low);

	return B_OK;
}


//!	Send acknowledge and wait for reply
static status_t
send_acknowledge(const i2c_bus *bus)
{
	status_t status;
	bigtime_t startTime;

	// release data so slave can modify it
	bus->set_signals(bus->cookie, 0, 1);
	spin(bus->timing.su_dat);
	bus->set_signals(bus->cookie, 1, 1);

	status = wait_for_clk(bus, bus->timing.ack_start_timeout);
	if (status != B_OK) {
		TRACE("%s: Timeout when sending acknowledge\n", __func__);
		return status;
	}

	// data and clock is high, now wait for slave to pull data low
	// (according to spec, this can happen any time once clock is high)
	startTime = system_time();

	while (true) {
		int clk, data;

		bus->get_signals(bus->cookie, &clk, &data);

		if (data == 0)
			break;

		if (system_time() - startTime > bus->timing.ack_timeout) {
			TRACE("%s: slave didn't acknowledge byte within ack_timeout: %ld\n",
				__func__, bus->timing.ack_timeout);
			return B_TIMEOUT;
		}

		spin(bus->timing.r);
	}

	TRACE("send_acknowledge(): Success!\n");

	// make sure we've waited at least t_high
	spin(bus->timing.high);

	bus->set_signals(bus->cookie, 0, 1);
	spin(bus->timing.f + bus->timing.low);	

	return B_OK;
}


//!	Send byte and wait for acknowledge if <ackowledge> is true
static status_t
send_byte(const i2c_bus *bus, uint8 byte, bool acknowledge)
{
	int i;

	//TRACE("%s: (byte = %x)\n", __func__, byte);

	for (i = 7; i >= 0; --i) {
		status_t status = send_bit(bus, byte >> i,
			i == 7 ? bus->timing.byte_timeout : bus->timing.bit_timeout);
		if (status != B_OK)
			return status;
	}

	if (acknowledge)
		return send_acknowledge(bus);

	return B_OK;
}


//!	Send slave address, obeying 10-bit addresses and general call addresses
static status_t
send_slave_address(const i2c_bus *bus, int slaveAddress, bool isWrite)
{
	status_t status;

	TRACE("%s: 0x%X\n", __func__, slaveAddress);
	status = send_byte(bus, (slaveAddress & 0xfe) | !isWrite, true);
	if (status != B_OK)
		return status;

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
	if ((slaveAddress & 0xff) != 0 && (slaveAddress & 0xf8) != 0xf0)
		return B_OK;

	return send_byte(bus, slaveAddress >> 8, true);
		// send second byte if required
}


//!	Receive one bit
static status_t
receive_bit(const i2c_bus *bus, bool *bit, int timeout)
{
	status_t status;
	int clk, data;

	bus->set_signals(bus->cookie, 1, 1);
		// release clock

	// wait for slave to raise clock
	status = wait_for_clk(bus, timeout);
	if (status != B_OK) {
		TRACE("%s: Timeout waiting for bit sent by slave\n", __func__);
		return status;
	}

	bus->get_signals(bus->cookie, &clk, &data);
		// sample data

	spin(bus->timing.high);
		// leave clock high for minimal time

	bus->set_signals(bus->cookie, 0, 1);
		// pull clock low so slave waits for us before next bit

	spin(bus->timing.f + bus->timing.low);
		// let it settle and leave it low for minimal time
		// to make sure slave has finished bit transmission too

	*bit = data;
	return B_OK;
}


/*!
	Send positive acknowledge afterwards if <acknowledge> is true,
	else send negative one
*/
static status_t
receive_byte(const i2c_bus *bus, uint8 *resultByte, bool acknowledge)
{
	uint8 byte = 0;
	int i;

	// pull clock low to let slave wait for us
	bus->set_signals(bus->cookie, 0, 1);

	for (i = 7; i >= 0; i--) {
		bool bit;

		status_t status = receive_bit(bus, &bit,
			i == 7 ? bus->timing.byte_timeout : bus->timing.bit_timeout);
		if (status != B_OK)
			return status;

		byte = (byte << 1) | bit;
	}

	//SHOW_FLOW(3, "%x ", byte);

	*resultByte = byte;

	return send_bit(bus, acknowledge ? 0 : 1, bus->timing.bit_timeout);
}


//!	Send multiple bytes
static status_t
send_bytes(const i2c_bus *bus, const uint8 *writeBuffer, ssize_t writeLength)
{
	TRACE("%s: (length = %ld)\n", __func__, writeLength);

	for (; writeLength > 0; --writeLength, ++writeBuffer) {
		status_t status = send_byte(bus, *writeBuffer, true);
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


//!	Receive multiple bytes
static status_t
receive_bytes(const i2c_bus *bus, uint8 *readBuffer, ssize_t readLength)
{
	TRACE("%s: (length = %ld)\n", __func__, readLength);

	for (; readLength > 0; --readLength, ++readBuffer) {
		status_t status = receive_byte(bus, readBuffer, readLength > 1);
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


//	#pragma mark - exported functions


//!	Combined i2c send+receive format
status_t
i2c_send_receive(const i2c_bus *bus, int slaveAddress, const uint8 *writeBuffer,
	size_t writeLength, uint8 *readBuffer, size_t readLength)
{
	status_t status = send_start_condition(bus);
	if (status != B_OK)
		return status;

	status = send_slave_address(bus, slaveAddress, true);
	if (status != B_OK)
		goto err;

	status = send_bytes(bus, writeBuffer, writeLength);
	if (status != B_OK)
		goto err;

	status = send_start_condition(bus);
	if (status != B_OK)
		return status;

	status = send_slave_address(bus, slaveAddress, false);
	if (status != B_OK)
		goto err;

	status = receive_bytes(bus, readBuffer, readLength);
	if (status != B_OK)
		goto err;

	return send_stop_condition(bus);

err:
	TRACE("%s: Cancelling transmission\n", __func__);
	send_stop_condition(bus);
	return status;
}


void
i2c_get100k_timing(i2c_timing *timing)
{
	// AKA standard i2c mode
	memcpy(timing, &kTiming100k, sizeof(i2c_timing));
}


void
i2c_get400k_timing(i2c_timing *timing)
{
	// AKA fast i2c mode
	memcpy(timing, &kTiming400k, sizeof(i2c_timing));
}
