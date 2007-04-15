/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <KernelExport.h>
#include "i2c_core.h"

#define TRACE_I2C
#ifdef TRACE_I2C
  #define TRACE dprintf
#else
  #define TRACE(a...)
#endif

static status_t i2c_writebyte(i2c_bus *bus, uint8 byte, int *ack);
static status_t i2c_readbyte(i2c_bus *bus, uint8 *pbyte);
static status_t i2c_read_unlocked(i2c_bus *bus, int address, void *data, int size);
static status_t i2c_write_unlocked(i2c_bus *bus, int address, const void *data, int size);


struct _i2c_bus
{
	void *cookie;
	bigtime_t delay;
	bigtime_t timeout;
	i2c_set_scl set_scl;
	i2c_set_sda set_sda;
	i2c_get_scl get_scl;
	i2c_get_sda get_sda;
	sem_id sem;
};


i2c_bus *
i2c_create_bus(void *cookie,
			   int frequency,
			   bigtime_t timeout,
			   i2c_set_scl set_scl,
			   i2c_set_sda set_sda,
			   i2c_get_scl get_scl,
			   i2c_get_sda get_sda)
{
	i2c_bus *bus = malloc(sizeof(i2c_bus));
	if (!bus)
		return NULL;

	bus->sem = create_sem(1, "i2c bus access");
	if (bus->sem < 0) {
		free(bus);
		return NULL;
	}

	bus->cookie = cookie;
	bus->delay = 1000000 / frequency;
	if (bus->delay == 0)
		bus->delay = 1;
	bus->timeout = timeout;
	bus->set_scl = set_scl;
	bus->set_sda = set_sda;
	bus->get_scl = get_scl;
	bus->get_sda = get_sda;

	set_scl(cookie, 1);
	set_sda(cookie, 1);

	return bus;
}


void
i2c_delete_bus(i2c_bus *bus)
{
	if (!bus)
		return;
	delete_sem(bus->sem);
	free(bus);
}


static inline void
set_sda_low(i2c_bus *bus)
{
	bus->set_sda(bus->cookie, 0);
	snooze(bus->delay);
}


static inline void
set_sda_high(i2c_bus *bus)
{
	bus->set_sda(bus->cookie, 1);
	snooze(bus->delay);
}


static inline void
set_scl_low(i2c_bus *bus)
{
	bus->set_scl(bus->cookie, 0);
	snooze(bus->delay);
}


static inline status_t
set_scl_high(i2c_bus *bus)
{
	bigtime_t end = system_time() + bus->timeout;
	bus->set_scl(bus->cookie, 1);
	while (0 == bus->get_scl(bus->cookie)) {
		if (system_time() > end)
			return B_TIMED_OUT;
		snooze(5);
	}
	snooze(bus->delay);
	return B_OK;
}


static inline void
i2c_start(i2c_bus *bus)
{
	set_sda_low(bus);
	set_scl_low(bus);
}


static inline void
i2c_stop(i2c_bus *bus)
{
	set_sda_low(bus);
	set_scl_high(bus);
	set_sda_high(bus);
}


static inline status_t
i2c_start_address(i2c_bus *bus, int address, int read /* 1 = read, 0 = write */)
{
	status_t res;
	uint8 addr;
	int ack;
	int i;

//	TRACE("i2c_start_address: enter\n");

	addr = (address << 1) | (read & 1);
	for (i = 0; i < 5; i++) {
		i2c_start(bus);
		res = i2c_writebyte(bus, addr, &ack);
		if (res == B_OK) {
			if (ack)
				break;
			res = B_ERROR;
		}
		if (res == B_TIMED_OUT)
			break;
		i2c_stop(bus);
		snooze(50);
	}
	
//	TRACE("i2c_start_address: %s, ack %d, i %d\n", strerror(res), ack, i);
	return res;
}

	
/* write one byte and the ack/nack
 * return values:
 * B_OK => byte transmitted
 * B_ERROR => arbitration lost
 * B_TIMED_OUT => time out
 */
status_t
i2c_writebyte(i2c_bus *bus, uint8 byte, int *ack)
{
	int has_arbitration;

	int i;
	has_arbitration = 1;
	for (i = 7; i >= 0; i--) {
		int bit = (byte >> i) & 1;
		if (has_arbitration) {
			if (bit)
				set_sda_high(bus);
			else
				set_sda_low(bus);
		}
		if (set_scl_high(bus) != B_OK) {
			set_sda_high(bus); // avoid blocking the bus
			TRACE("i2c_writebyte timeout at bit %d\n", i);
			return B_TIMED_OUT;
		}
		if (has_arbitration) {
			if (bit == 1 && 0 == bus->get_sda(bus->cookie)) {
				has_arbitration = 0;
//				TRACE("i2c_writebyte lost arbitration at bit %d\n", i);
			}
		}
		set_scl_low(bus);
	}
	set_sda_high(bus);
	if (set_scl_high(bus) != B_OK) {
		TRACE("i2c_writebyte timeout at ack\n");
		return B_TIMED_OUT;
	}
	*ack = 0 == bus->get_sda(bus->cookie);
	set_scl_low(bus);

	return has_arbitration ? B_OK : B_ERROR;
}


/* read one byte, don't generate ack
 */
status_t
i2c_readbyte(i2c_bus *bus, uint8 *pbyte)
{
	int i;
	uint8 byte;
	
	set_sda_high(bus);
	byte = 0;
	for (i = 7; i >= 0; i--) {
		if (set_scl_high(bus) != B_OK) {
			TRACE("i2c_readbyte timeout at bit %d\n", i);
			return B_TIMED_OUT;
		}
		byte = (byte << 1) | bus->get_sda(bus->cookie);
		set_scl_low(bus);
	}
	*pbyte = byte;
	return B_OK;
}


status_t
i2c_read_unlocked(i2c_bus *bus, int address, void *data, int size)
{
	status_t status;
	uint8 *bytes;
	
	if (size <= 0)
		return B_BAD_VALUE;
	
	status = i2c_start_address(bus, address, 1 /* 1 = read, 0 = write */);
	if (status != B_OK) {
		TRACE("i2c_read: error on i2c_start_address: %s\n", strerror(status));
		return status;
	}

	bytes = data;
	while (size > 0) {
		if (i2c_readbyte(bus, bytes) != B_OK) { // timeout
			TRACE("i2c_read: timeout error on byte %ld\n", bytes - (uint8 *)data);
			return B_TIMED_OUT;
		}
		if (size > 0)
			set_sda_low(bus); // ack
		else
			set_sda_high(bus); // nack
			
		if (set_scl_high(bus) != B_OK) {
			set_sda_high(bus); // avoid blocking the bus
			TRACE("i2c_read: timeout at ack\n");
			return B_TIMED_OUT;
		}
		set_scl_low(bus);
		set_sda_high(bus);

		size--;
		bytes++;
	}
	
	i2c_stop(bus);

	return B_OK;
}


status_t
i2c_write_unlocked(i2c_bus *bus, int address, const void *data, int size)
{
	const uint8 *bytes;
	status_t status;
	int ack;

	if (size <= 0)
		return B_BAD_VALUE;

	status = i2c_start_address(bus, address, 0 /* 1 = read, 0 = write */);
	if (status != B_OK) {
		TRACE("i2c_write: error on i2c_start_address: %s\n", strerror(status));
		return status;
	}

	bytes = data;
	while (size > 0) {
		status = i2c_writebyte(bus, *bytes, &ack);
		if (status == B_TIMED_OUT) {
			TRACE("i2c_write: timeout error on byte %ld\n", bytes - (uint8 *)data);
			return B_TIMED_OUT;
		}
		if (status != B_OK) {
			TRACE("i2c_write: arbitration lost on byte %ld\n", bytes - (uint8 *)data);
			break;
		}
		if (!ack) {
			TRACE("i2c_write: error, got NACK on byte %ld\n", bytes - (uint8 *)data);
			break;
		}
		bytes++;
		size--;
	}
	i2c_stop(bus);
	
	if (status != B_OK)
		return status;
	
	return ack ? B_OK : B_ERROR;
}


status_t
i2c_read(i2c_bus *bus, int address, void *data, int size)
{
	status_t res;
	acquire_sem(bus->sem);
	res = i2c_read_unlocked(bus, address, data, size);
	release_sem(bus->sem);
	return res;
}


status_t
i2c_write(i2c_bus *bus, int address, const void *data, int size)
{
	status_t res;
	acquire_sem(bus->sem);
	res = i2c_write_unlocked(bus, address, data, size);
	release_sem(bus->sem);
	return res;
}


status_t
i2c_xfer(i2c_bus *bus, int address,
		 const void *write_data, int write_size,
		 void *read_data, int read_size)
{
	status_t res;

	acquire_sem(bus->sem);
	
	if (write_data != 0 && write_size != 0) {
		res = i2c_write_unlocked(bus, address, write_data, write_size);
		if (res != B_OK) {
			release_sem(bus->sem);
			return res;
		}
	}

	if (read_data != 0 && read_size != 0) {
		res = i2c_read_unlocked(bus, address, read_data, read_size);
		if (res != B_OK) {
			release_sem(bus->sem);
			return res;
		}
	}

	release_sem(bus->sem);
	
	return B_OK;
}
