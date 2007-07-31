/*
 * Copyright 2003, Thomas Kurschel. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


/*!
	Part of DDC driver	
	Main DDC communication
*/


#include "ddc_int.h"
#include "ddc.h"
#include "i2c.h"

#include <KernelExport.h>
#include <OS.h>

#include <stdlib.h>


#define READ_RETRIES 4
	// number of retries to read ddc data

#if 0
/*!	Verify checksum of ddc data
	(some monitors have a broken checksum - bad luck for them)
*/
static status_t
verify_checksum(const uint8 *data, size_t len)
{
	uint32 index;
	uint8 sum = 0;
	uint8 all_or = 0;
	
	for (index = 0; index < len; ++index, ++data) {
		sum += *data;
		all_or |= *data;
	}
	
	if (all_or == 0) {
		SHOW_ERROR0(2, "DDC information contains zeros only");
		return B_ERROR;
	}
	
	if (sum != 0) {
		SHOW_ERROR0(2, "Checksum error of DDC information");
		return B_IO_ERROR;
	}
		
	return B_OK;
}
#endif


//!	Read ddc2 data from monitor
static status_t
ddc2_read(const i2c_bus *bus, int start, uint8 *buffer, size_t len)
{
	uint8 write_buffer[2];
	i2c_timing timing;
	int i;
	status_t res = B_OK;

	write_buffer[0] = start & 0xff;	
	write_buffer[1] = (start >> 8) & 0xff;

	i2c_get100k_timing(&timing);

	timing.start_timeout = 550;
	timing.byte_timeout = 2200;
	timing.bit_timeout = 40;
	timing.ack_start_timeout = 40;
	timing.ack_timeout = 40;

	for (i = 0; i < READ_RETRIES; ++i) {
		res = i2c_send_receive(bus, &timing, 
			0xa0, write_buffer, start < 0x100 ? 1 : 2, 
			buffer, len);
		// don't verify checksum - it's often broken
		if (res == B_OK /*&& verify_checksum( buffer, len ) == B_OK*/)
			break;

		res = B_ERROR;
	}

	return res;
}


/*!
	Reading VDIF has not been tested. 
	it seems that almost noone supports VDIF which makes testing hard,
	but what's the point anyway?
*/
#if 0
static status_t
ddc2_read_vdif(const i2c_bus *bus, int start, 
	void **vdif, size_t *vdif_len)
{
	status_t res;
	uint8 *data, *cur_data;
	int i;
	uint8 buffer[64];
	
	*vdif = NULL;
	*vdif_len = 0;
	
	res = ddc2_read(bus, start, buffer, 64);
	SHOW_INFO(2, "%x", buffer[0]);
	if (res != B_OK || buffer[0] == 0)
		return B_OK;

	// each block is 63 bytes plus 1 checksum long
	// we strip the checksum but store data directly into
	// buffer, so we need an extra byte for checksum of the last block
	data = malloc(buffer[0] * 63 + 1);
	if (data == NULL)
		return B_NO_MEMORY;

	cur_data = data;
	for (i = 0; i < buffer[0]; ++i) {
		ddc2_read(bus, start + i * 64, cur_data, 64);
		// strip checksum byte
		cur_data += 63;
	}

	*vdif_len = buffer[0] * 63;
	*vdif = data;
	return B_OK;
}
#endif


//! Read EDID and VDIF from monitor via ddc2
status_t
ddc2_read_edid1(const i2c_bus *bus, edid1_info *edid, 
	void **vdif, size_t *vdif_len)
{
	status_t res;
	edid1_raw raw;

	res = ddc2_read(bus, 0, (uint8 *)&raw, sizeof(raw));
	if (res != B_OK)
		return res;

	edid_decode(edid, &raw);

	*vdif = NULL;
	*vdif_len = 0;

	// skip vdif as long as it's not tested	
#if 0	
	res = ddc2_read_vdif(bus, sizeof(raw) * (edid->num_sections + 1),
		vdif, vdif_len);
	if (res != B_OK)
		return res;
#endif
	
	return B_OK;
}
