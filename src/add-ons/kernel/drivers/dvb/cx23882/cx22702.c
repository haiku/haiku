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
#include <string.h>
#include "cx22702.h"
#include "dtt7592.h"
#include "config.h"
#include "dvb.h"

#define TRACE_CX22702
#ifdef TRACE_CX22702
  #define TRACE dprintf
#else
  #define TRACE(a...)
#endif


#if 0
static void
cx22702_reg_dump(i2c_bus *bus)
{
	int i;
	for (i = 0; i < 256; i++) {
		uint8 data;
		if (cx22702_reg_read(bus, i, &data) != B_OK)
			dprintf("cx22702_reg 0x%02x error\n", i);
		else
			dprintf("cx22702_reg 0x%02x value 0x%02x\n", i, data);
	}
}
#endif


status_t
cx22702_reg_write(i2c_bus *bus, uint8 reg, uint8 data)
{
	status_t res;
	uint8 buf[2] = {reg, data};
	res = i2c_write(bus, I2C_ADDR_DEMOD, buf, 2);
	if (res != B_OK)
		TRACE("cx22702_reg_write error, reg 0x%02x, value 0x%02x\n", reg, data);
	return res;
}


status_t
cx22702_reg_read(i2c_bus *bus, uint8 reg, uint8 *data)
{
	status_t res;
	res = i2c_xfer(bus, I2C_ADDR_DEMOD, &reg, 1, data, 1);
	if (res != B_OK)
		TRACE("cx22702_reg_read error, reg 0x%02x\n", reg);
	return res;
}


status_t
cx22702_init(i2c_bus *bus)
{
	if (cx22702_reg_write(bus, 0x00, 0x02) != B_OK) return B_ERROR;
	if (cx22702_reg_write(bus, 0x00, 0x00) != B_OK) return B_ERROR;
	snooze(10000);
	if (cx22702_reg_write(bus, 0x00, 0x00) != B_OK) return B_ERROR;
	if (cx22702_reg_write(bus, 0x09, 0x01) != B_OK) return B_ERROR;
	if (cx22702_reg_write(bus, 0x0B, 0x04) != B_OK) return B_ERROR;
	if (cx22702_reg_write(bus, 0x0C, 0x00) != B_OK) return B_ERROR;
	if (cx22702_reg_write(bus, 0x0D, 0x80) != B_OK) return B_ERROR;
	if (cx22702_reg_write(bus, 0x26, 0x80) != B_OK) return B_ERROR;
	if (cx22702_reg_write(bus, 0x2D, 0xff) != B_OK) return B_ERROR;
	if (cx22702_reg_write(bus, 0xDC, 0x00) != B_OK) return B_ERROR;
	if (cx22702_reg_write(bus, 0xE4, 0x00) != B_OK) return B_ERROR;
	if (cx22702_reg_write(bus, 0xF8, 0x02) != B_OK) return B_ERROR;
	if (cx22702_reg_write(bus, 0x00, 0x01) != B_OK) return B_ERROR;
	return B_OK;
}


status_t
cx22702_get_frequency_info(i2c_bus *bus, dvb_frequency_info_t *info)
{
	memset(info, 0, sizeof(*info));
	info->frequency_min = 149000000;
	info->frequency_max = 860000000;
	info->frequency_step = 166667;
	return B_OK;
}


status_t
cx22702_set_tuning_parameters(i2c_bus *bus, const dvb_t_tuning_parameters_t *params)
{
	uint8 data;
	status_t res;

	if (cx22702_reg_write(bus, 0x00, 0x00) != B_OK)
		return B_ERROR;
	
	res = dtt7592_set_frequency(bus, params->frequency, params->bandwidth);
	if (res != B_OK)
		return res;
	
	if (cx22702_reg_read(bus, 0x0c, &data) != B_OK)
		 return B_ERROR;
	switch (params->inversion) {
		case DVB_INVERSION_ON:		data |= 0x01; break;
		case DVB_INVERSION_OFF:		data &= ~0x01; break;
		default:					return B_ERROR;
	}
	switch (params->bandwidth) {
		case DVB_BANDWIDTH_6_MHZ:	data = (data & ~0x10) | 0x20; break;
		case DVB_BANDWIDTH_7_MHZ:	data = (data & ~0x20) | 0x10; break;
		case DVB_BANDWIDTH_8_MHZ:	data &= ~0x30;	break;
		default:					return B_ERROR;
	}
	if (cx22702_reg_write(bus, 0x0c, data) != B_OK)
		return B_ERROR;
	
	switch (params->modulation) {
		case DVB_MODULATION_QPSK:	data = 0x00; break;
		case DVB_MODULATION_16_QAM:	data = 0x08; break;
		case DVB_MODULATION_64_QAM:	data = 0x10; break;
		default:					return B_ERROR;
	}
	switch (params->hierarchy) {
		case DVB_HIERARCHY_NONE:	break;
		case DVB_HIERARCHY_1:		data |= 0x01; break;
		case DVB_HIERARCHY_2:		data |= 0x02; break;
		case DVB_HIERARCHY_4:		data |= 0x03; break;
		default:					return B_ERROR;
	}
	if (cx22702_reg_write(bus, 0x06, data) != B_OK)
		return B_ERROR;
	
	switch (params->code_rate_hp) {
		case DVB_FEC_NONE:			data = 0x00; break;
		case DVB_FEC_1_2:			data = 0x00; break;
		case DVB_FEC_2_3:			data = 0x08; break;
		case DVB_FEC_3_4:			data = 0x10; break;
		case DVB_FEC_5_6:			data = 0x18; break;
		case DVB_FEC_6_7:			data = 0x20; break;
		default:					return B_ERROR;
	}
	switch (params->code_rate_lp) {
		case DVB_FEC_NONE:			break;
		case DVB_FEC_1_2:			break;
		case DVB_FEC_2_3:			data |= 0x01; break;
		case DVB_FEC_3_4:			data |= 0x02; break;
		case DVB_FEC_5_6:			data |= 0x03; break;
		case DVB_FEC_6_7:			data |= 0x04; break;
		default:					return B_ERROR;
	}
	if (cx22702_reg_write(bus, 0x07, data) != B_OK)
		return B_ERROR;
	
	switch (params->transmission_mode) {
		case DVB_TRANSMISSION_MODE_2K:	data = 0x00; break;
		case DVB_TRANSMISSION_MODE_8K:	data = 0x01; break;
		default:						return B_ERROR;
	}
	switch (params->guard_interval) {
		case DVB_GUARD_INTERVAL_1_4:	data |= 0x0c; break;
		case DVB_GUARD_INTERVAL_1_8:	data |= 0x08; break;
		case DVB_GUARD_INTERVAL_1_16:	data |= 0x04; break;
		case DVB_GUARD_INTERVAL_1_32:	break;
		default: return B_ERROR;
	}
	if (cx22702_reg_write(bus, 0x08, data) != B_OK)
		return B_ERROR;

	if (cx22702_reg_read(bus, 0x0b, &data) != B_OK)
		 return B_ERROR;
	if (cx22702_reg_write(bus, 0x0b, data | 0x02) != B_OK)
		return B_ERROR;

	if (cx22702_reg_write(bus, 0x00, 0x01) != B_OK)
		return B_ERROR;

//	cx22702_reg_dump(bus);

	return B_OK;
}


status_t
cx22702_get_tuning_parameters(i2c_bus *bus, dvb_t_tuning_parameters_t *params)
{
	uint8 reg01, reg02, reg03, reg0A, reg0C;
	
	if (cx22702_reg_read(bus, 0x01, &reg01) != B_OK)
		return B_ERROR;
	if (cx22702_reg_read(bus, 0x02, &reg02) != B_OK)
		return B_ERROR;
	if (cx22702_reg_read(bus, 0x03, &reg03) != B_OK)
		return B_ERROR;
	if (cx22702_reg_read(bus, 0x0a, &reg0A) != B_OK)
		return B_ERROR;
	if (cx22702_reg_read(bus, 0x0c, &reg0C) != B_OK)
		return B_ERROR;

	memset(params, 0, sizeof(*params));
	params->inversion = (reg0C & 0x01) ? DVB_INVERSION_ON : DVB_INVERSION_OFF;
	
	// XXX TODO...
		
	return B_OK;
}


status_t
cx22702_get_status(i2c_bus *bus, dvb_status_t *status)
{
	uint8 reg0A, reg23;
	
	if (cx22702_reg_read(bus, 0x0a, &reg0A) != B_OK)
		return B_ERROR;
	if (cx22702_reg_read(bus, 0x23, &reg23) != B_OK)
		return B_ERROR;
	
	*status = 0;
	if (reg0A & 0x10)
		*status |= DVB_STATUS_LOCK | DVB_STATUS_VITERBI | DVB_STATUS_SYNC;
	if (reg0A & 0x20)
		*status |= DVB_STATUS_CARRIER;
	if (reg23 < 0xf0)
		*status |= DVB_STATUS_SIGNAL;
		
	return B_OK;
}


status_t
cx22702_get_ss(i2c_bus *bus, uint32 *ss)
{
	uint8 reg23;
	if (cx22702_reg_read(bus, 0x23, &reg23) != B_OK)
		return B_ERROR;
	*ss = reg23;
	return B_OK;
}


status_t
cx22702_get_ber(i2c_bus *bus, uint32 *ber)
{
	uint8 regDE_1, regDE_2, regDF;
	int trys;
	
	trys = 50;
	do {
		if (cx22702_reg_read(bus, 0xDE, &regDE_1) != B_OK)
			return B_ERROR;
		if (cx22702_reg_read(bus, 0xDF, &regDF) != B_OK)
			return B_ERROR;
		if (cx22702_reg_read(bus, 0xDE, &regDE_2) != B_OK)
			return B_ERROR;
	} while (regDE_1 != regDE_2 && --trys > 0);
	if (trys == 0)
		return B_ERROR;
	
	*ber = (regDE_1 & 0x7f) << 7 | (regDF & 0x7f);
	
	return B_OK;
}


status_t
cx22702_get_snr(i2c_bus *bus, uint32 *snr)
{
	uint32 ber;
	status_t stat = cx22702_get_ber(bus, &ber);
	*snr = 16384 - ber;
	return stat;
}


status_t
cx22702_get_upc(i2c_bus *bus, uint32 *upc)
{
	uint8 regE3;
	
	if (cx22702_reg_read(bus, 0xE3, &regE3) != B_OK)
		return B_ERROR;
	if (cx22702_reg_write(bus, 0xE3, 0) != B_OK)
		return B_ERROR;
	*upc = regE3;
	return B_OK;
}
