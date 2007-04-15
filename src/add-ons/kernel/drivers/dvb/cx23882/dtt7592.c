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

#include <KernelExport.h>
#include "dtt7592.h"
#include "config.h"

#define TRACE_DTT7592
#ifdef TRACE_DTT7592
  #define TRACE dprintf
#else
  #define TRACE(a...)
#endif


status_t
dtt7592_write(i2c_bus *bus, const uint8 data[4])
{
	status_t res;
	TRACE("dtt7592_write values 0x%02x 0x%02x 0x%02x 0x%02x\n", data[0], data[1], data[2], data[3]);
	res = i2c_write(bus, I2C_ADDR_PLL, data, 4);
	if (res != B_OK)
		TRACE("dtt7592_write error, values 0x%02x 0x%02x 0x%02x 0x%02x\n", data[0], data[1], data[2], data[3]);
	return res;
}


status_t
dtt7592_read(i2c_bus *bus, uint8 *data)
{
	status_t res;
	res = i2c_read(bus, I2C_ADDR_PLL, data, 1);
	if (res != B_OK)
		TRACE("dtt7592_read error\n");
	return res;
}


status_t
dtt7592_set_frequency(i2c_bus *bus, uint32 frequency, dvb_bandwidth_t bandwidth)
{
	status_t res;
	uint32 divider;
	uint8 data[4];
	
	divider = (frequency + 36083333) / 166666;
	if (divider > 0x7fff)
		divider = 0x7fff;
		
	TRACE("dtt7592_set_frequency frequency %ld, divider 0x%lx (%ld)\n", frequency, divider, divider);
	
	data[0] = divider >> 8;
	data[1] = (uint8)divider;
	
	if (frequency > 835000000)
		data[2] = 0xfc;
	else if (frequency > 735000000)
		data[2] = 0xf4;
	else if (frequency > 465000000)
		data[2] = 0xbc;
	else if (frequency > 445000000)
		data[2] = 0xfc;
	else if (frequency > 405000000)
		data[2] = 0xf4;
	else if (frequency > 305000000)
		data[2] = 0xbc;
	else
		data[2] = 0xb4;
	
	if (frequency > 429000000)
		data[3] = 0x08; // select UHF IV/V
	else
		data[3] = 0x02; // select VHF III

	// only used in Germany right now, where VHF channels
	// are 7 MHz wide, while UHF are 8 MHz. 
	if (bandwidth == DVB_BANDWIDTH_5_MHZ
		|| bandwidth == DVB_BANDWIDTH_6_MHZ 
		|| bandwidth == DVB_BANDWIDTH_7_MHZ) {
		data[3] |= 0x10; // activate 7 Mhz filter
	}
		
	res = dtt7592_write(bus, data);
	if (res != B_OK) {
		dprintf("dtt7592_set_frequency step 1 failed\n");
		return res;
	}

	// enable AGC
	data[2] = (data[2] & 0x40) | 0x9c;
	data[3] = 0xa0;
	res = dtt7592_write(bus, data);
	if (res != B_OK) {
		dprintf("dtt7592_set_frequency step 2 failed\n");
		return res;
	}
	
	// wait 100 ms
	snooze(100000);
	
	// disable AGC
	data[3] = 0x20;
	res = dtt7592_write(bus, data);
	if (res != B_OK) {
		dprintf("dtt7592_set_frequency step 3 failed\n");
		return res;
	}

	return B_OK;
}


#if 0
void
dtt7582_dump(i2c_bus *bus)
{
	uint8 data;
	if (B_OK != dtt7592_read(bus, &data)) {
		TRACE("dtt7582_dump failed\n");
	}
	TRACE("dtt7582_dump: 0x%02x, PLL %s, AGC %s\n", data, (data & 0x40) ? "locked" : "open", (data & 0x08) ? "active" : "off");
}


void
dtt7582_test(i2c_bus *bus)
{
	TRACE("dtt7582_test start\n");
	dtt7582_dump(bus);
	TRACE("dtt7582_test freq 1\n");
	dtt7592_set_frequency(bus, 150000000, DVB_BANDWIDTH_7_MHZ);
	dtt7582_dump(bus);
	TRACE("dtt7582_test freq 2\n");
	dtt7592_set_frequency(bus, 746000000, DVB_BANDWIDTH_8_MHZ); // Kabel 1
	dtt7582_dump(bus);
	TRACE("dtt7582_test freq 3\n");
	dtt7592_set_frequency(bus, 538000000, DVB_BANDWIDTH_7_MHZ); // VOX
	dtt7582_dump(bus);
	TRACE("dtt7582_test freq 4\n");
	dtt7592_set_frequency(bus, 896000000, DVB_BANDWIDTH_8_MHZ);
	dtt7582_dump(bus);
	TRACE("dtt7582_test freq 5\n");
	dtt7592_set_frequency(bus, 333000000, DVB_BANDWIDTH_7_MHZ);
	dtt7582_dump(bus);
}
#endif
