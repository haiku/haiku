/*
 * Copyright 2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 * 
 * Copyright 2005, François Revol.
 */

/*
	Description: Implements a tty on top of the parallel port, 
				 using PLIP-like byte-by-byte protocol.
				 Low level stuff.
*/


#include <Drivers.h>
//#include <parallel_driver.h>
#include <KernelExport.h>
#include <driver_settings.h>
#include <OS.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <ISA.h>

//XXX: move to Jamfile when adding driver
#define _BUILDING_kernel 1

#if _BUILDING_kernel
#include <debug.h>
#endif

#include "laplinkll.h"

enum {
	st_sync = 0, // syncing...
	st_lsb,
	st_msb,
	st_error
};

static isa_module_info *sISAModule;

#pragma mark // raw access

static inline uint8 read_status(uint32 port)
{
	uint8 val;
	val = sISAModule->read_io_8(port+1);
	return val;
}

static inline void write_control(uint32 port, uint8 val)
{
	sISAModule->write_io_8(port+2, val);
}

static inline void write_data(uint32 port, uint8 val)
{
	sISAModule->write_io_8(port, val);
}

#pragma mark // framing

status_t ll_send_sof(laplink_state *st)
{
	uint8 v;
	int tries = LPTSOFTRIES;
	if (st->rstate != st_sync || st->wstate != st_sync)
		return B_TIMED_OUT;
	// check for idle bus
	if ((read_status(st->port) & 0xf8) != 0x80)
		return B_TIMED_OUT;
	// raise ACK
	write_data(st->port, 0x08);
	do {
		spin(LPTSPIN);
		v = read_status(st->port);
		if (st->rstate != st_sync)
			return B_TIMED_OUT;
		if (tries-- == 0)
			return B_TIMED_OUT;
	} while (!(v & 0x08));
	st->wstate = st_lsb;
	return B_OK;
}

status_t ll_check_sof(laplink_state *st)
{
	uint8 v;
	if (st->rstate != st_sync || st->wstate != st_sync)
		return EINTR;
	v = read_status(st->port);
	if ((v & 0xf8) != 0xc0)
		return EINTR;
	return B_OK;
}

status_t ll_ack_sof(laplink_state *st)
{
	write_data(st->port, 0x01); // ack the sof
	st->rstate = st_lsb;
	return B_OK;
}

status_t ll_send_eof(laplink_state *st)
{
	/*
	if (st->rstate != st_sync || st->wstate != st_sync)
		return B_TIMED_OUT;
	*/
	st->rstate = st_sync;
	st->wstate = st_sync;
	write_data(st->port, 0x00);
	return B_OK;
}

#pragma mark // nibbles

status_t ll_send_lnibble(laplink_state *st, uint8 v)
{
	int tries = LPTNIBTRIES;
	uint8 s;
	if (st->rstate != st_sync)
		goto err;
	if (st->wstate != st_lsb)
		goto err;
	write_data(st->port, v & 0x0f);
	spin(10);
	write_data(st->port, (v & 0x0f) | 0x10);
	// wait for ack
	do {
		s = read_status(st->port);
		if (tries-- == 0)
			goto err;
		spin(LPTSPIN);
	} while (s & 0x80);
	st->wstate = st_msb;
	return B_OK;
err:
	st->wstate = st_sync;
	return B_TIMED_OUT;
}

status_t ll_send_mnibble(laplink_state *st, uint8 v)
{
	int tries = LPTNIBTRIES;
	uint8 s;
	if (st->rstate != st_sync)
		goto err;
	if (st->wstate != st_msb)
		goto err;
	write_data(st->port, (v >> 4) | 0x10);
	spin(10);
	write_data(st->port, (v >> 4));
	// wait for ack
	do {
		s = read_status(st->port);
		if (tries-- == 0)
			goto err;
		spin(LPTSPIN);
	} while (!(s & 0x80));
	st->wstate = st_lsb;//st_sync;
	return B_OK;
err:
	st->wstate = st_sync;
	return B_TIMED_OUT;
}

status_t ll_wait_lnibble(laplink_state *st, uint8 *v)
{
	int tries = LPTNIBTRIES;
	uint8 s;
	// wait for data
	do {
		s = read_status(st->port);
		if (tries-- == 0)
			goto err;
		spin(LPTSPIN);
	} while ((s & 0x80) || (s != read_status(st->port)));
	// get the nibble
	*v = (s >> 3) & 0x0f;
	st->rstate = st_msb;
	// tell we got that one
	write_data(st->port, 0x10);
	return B_OK;
err:
	st->rstate = st_sync;
	return B_TIMED_OUT;
}

status_t ll_wait_mnibble(laplink_state *st, uint8 *v)
{
	int tries = LPTNIBTRIES;
	uint8 s;
	// wait for data
	do {
		s = read_status(st->port);
		if (tries-- == 0)
			goto err;
		spin(LPTSPIN);
	} while (!(s & 0x80) || (s != read_status(st->port)));
	// get the nibble
	*v |= (s << (4-3)) & 0xf0;
	st->rstate = st_sync;
	// tell we got that one
	write_data(st->port, 0x00);
	return B_OK;
err:
	st->rstate = st_sync;
	return B_TIMED_OUT;
}

#pragma mark // byte mode

status_t ll_send_byte(laplink_state *st, uint8 v)
{
	status_t err;
	err = ll_send_sof(st);
	if (!err)
		err = ll_send_lnibble(st, v);
	if (!err)
		err = ll_send_mnibble(st, v);
	if (!err)
		err = ll_send_eof(st);
	return err;
}

status_t ll_check_byte(laplink_state *st, uint8 *v)
{
	status_t err;
	*v = 0;
	err = ll_check_sof(st);
	if (err)
		return err;
	err = ll_ack_sof(st);
	if (!err)
		err = ll_wait_lnibble(st, v);
	if (!err)
		err = ll_wait_mnibble(st, v);
	if (!err)
		err = ll_send_eof(st);
	return err;
}

status_t ll_wait_byte(laplink_state *st, uint8 *v)
{
	status_t err;
	do {
		spin(LPTSPIN);
		err = ll_check_byte(st, v);
	} while (err < B_OK);//	} while (err == B_TIMED_OUT);
	return err;
}

#pragma mark // frame mode

// unframed
static inline status_t ll_send_byte_uf(laplink_state *st, uint8 v)
{
	status_t err;
	err = ll_send_lnibble(st, v);
	if (!err)
		err = ll_send_mnibble(st, v);
	return err;
}

status_t ll_get_byte_uf(laplink_state *st, uint8 *v)
{
	status_t err;
	*v = 0;
	err = ll_wait_lnibble(st, v);
	if (!err)
		err = ll_wait_mnibble(st, v);
	return err;
}

status_t ll_send_frame(laplink_state *st, const uint8 *buff, size_t *len)
{
	status_t err;
	uint16 pktlen = *len;
	uint8 cksum = 0;
	*len = 0;
	err = ll_send_sof(st);
	if (err)
		return err;
	err = ll_send_byte_uf(st, pktlen & 0xff);
	if (err)
		goto err;
	err = ll_send_byte_uf(st, pktlen >> 8);
	if (err)
		goto err;
	for (*len = 0; *len < pktlen; (*len)++) {
		err = ll_send_byte_uf(st, buff[*len]);
		if (err)
			goto err;
		cksum += buff[*len];
	}
	err = ll_send_byte_uf(st, cksum);
	if (err)
		goto err;
	
	/*err =*/ ll_send_eof(st);
err:
	
	if (err) { // back to idle
		*len = 0;
		ll_send_eof(st);
	}
	return err;
}

status_t ll_check_frame(laplink_state *st, uint8 *buff, size_t *len)
{
	status_t err;
	uint16 pktlen = 0;
	uint16 wanted;
	uint8 cksum = 0;
	uint8 byte;
	int i;
	err = ll_check_sof(st);
	if (err)
		goto err;
	err = ll_ack_sof(st);
	if (err)
		goto err;
	// pktlen
	err = ll_get_byte_uf(st, &byte);
	if (err)
		goto err;
	pktlen = byte;
	err = ll_get_byte_uf(st, &byte);
	if (err)
		goto err;
	pktlen |= byte << 8;
	cksum = 0;
	/*if (pktlen > *len) {
		dprintf("laplink: check_frame: packet truncated from %d to %d\n", pktlen, *len);
	}*/
	wanted = MIN(pktlen, *len);
	for (*len = 0; (*len < pktlen); (*len)++) {
		err = ll_get_byte_uf(st, &byte);
		if (err)
			goto err;
		if (*len < wanted)
			buff[*len] = byte;
		cksum += byte;
	}
	err = ll_get_byte_uf(st, &byte);
	if (err)
		goto err;
	/*
	if (cksum != byte) {
		dprintf("laplink: check_frame: wrong cksum\n");
	}*/
err:
	ll_send_eof(st);
	return err;
}

status_t ll_wait_frame(laplink_state *st, uint8 *buff, size_t *len)
{
	status_t err;
	do {
		spin(LPTSPIN);
		err = ll_check_frame(st, buff, len);
	} while (err < B_OK);//	} while (err == B_TIMED_OUT);
	return err;
}

#pragma mark // kdebug io handler

#if _BUILDING_kernel
#define BUFFSZ 256

static laplink_state llst;
static char laplink_in_buf[BUFFSZ];
static char *laplink_in_ptr;
static size_t laplink_in_avail;
static char laplink_out_buf[BUFFSZ];

//XXX: cleanup
static status_t debug_init_laplink(void *kernel_settings)
{
	(void)kernel_settings;
	llst.port = LPTBASE;
	llst.rstate = st_sync;
	llst.wstate = st_sync;
	laplink_in_ptr = laplink_in_buf;
	laplink_in_avail = 0;
	return B_OK;
}

static int debug_write_laplink(int f, const char *buf, int count)
{
	status_t err;
	size_t len;
	int tries;
	int i, prev = 0;
	
	// fix up CR LF issues... to a local buffer (which will get truncated)
	if (count > 1) {
		for (i = 0; (i < BUFFSZ-1) && count; i++, buf++, count--) {
			if ((*buf == '\n') && (prev != '\r'))
				laplink_out_buf[i++] = '\r';
			laplink_out_buf[i] = *buf;
			prev = *buf;
		}
		count = i;
		buf = laplink_out_buf;
	}
	
	tries = 5;
	do {
		len = count;
		err = ll_send_frame(&llst, (const uint8 *)buf, &len);
	} while (err && tries--);
	if (err)
		return 0;
	return len;
}

static int debug_read_laplink(void)
{
	status_t err = B_OK;
	while (laplink_in_avail < 1) {
		laplink_in_avail = BUFFSZ;
		laplink_in_ptr = laplink_in_buf;
		err = ll_wait_frame(&llst, (uint8 *)laplink_in_buf, &laplink_in_avail);
		if (err)
			laplink_in_avail = 0;
	}
	laplink_in_avail--;
	return *laplink_in_ptr++;
}


static int
debugger_puts(const char *s, int32 length)
{
	return debug_write_laplink(0, s, (int)length);
}


static status_t
std_ops(int32 op, ...)
{
	void *handle;
	bool load = true;//false;

	switch (op) {
	case B_MODULE_INIT:
		handle = load_driver_settings("kernel");
		if (handle) {
			load = get_driver_boolean_parameter(handle,
				"laplinkll_debug_output", load, true);
			unload_driver_settings(handle);
		}
		if (load) {
			if (get_module(B_ISA_MODULE_NAME, (module_info **)&sISAModule) < B_OK)
				return B_ERROR;
			debug_init_laplink(NULL);
		}
		return load ? B_OK : B_ERROR;
	case B_MODULE_UNINIT:
		put_module(B_ISA_MODULE_NAME);
		return B_OK;
	}
	return B_BAD_VALUE;
}


static struct debugger_module_info sModuleInfo = {
	{
		"debugger/laplinkll/v1",
		0,
		&std_ops
	},
	NULL,
	NULL,
	debugger_puts,
	NULL
};

module_info *modules[] = { 
	(module_info *)&sModuleInfo,
	NULL
};


#endif

