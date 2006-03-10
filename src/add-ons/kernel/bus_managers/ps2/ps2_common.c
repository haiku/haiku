/*
 * Copyright 2004-2006 Haiku, Inc.
 * Distributed under the terms of the Haiku License.
 *
 * common.c:
 * PS/2 hid device driver
 * Authors (in chronological order):
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 *      Marcus Overhagen <marcus@overhagen.de>
 */


#include <string.h>

#include "ps2_common.h"
#include "ps2_service.h"
#include "ps2_dev.h"

isa_module_info *gIsa = NULL;

static int32 sIgnoreInterrupts = 0;

bool	gActiveMultiplexingEnabled = false;
sem_id	gControllerSem;


uint8
ps2_read_ctrl(void)
{
	return gIsa->read_io_8(PS2_PORT_CTRL);
}


uint8
ps2_read_data(void)
{
	return gIsa->read_io_8(PS2_PORT_DATA);
}


void
ps2_write_ctrl(uint8 ctrl)
{
	TRACE(("ps2_write_ctrl 0x%02x\n", ctrl));

	gIsa->write_io_8(PS2_PORT_CTRL, ctrl);
}


void
ps2_write_data(uint8 data)
{
	TRACE(("ps2_write_data 0x%02x\n", data));

	gIsa->write_io_8(PS2_PORT_DATA, data);
}


status_t
ps2_wait_read(void)
{
	int i;
	for (i = 0; i < PS2_CTRL_WAIT_TIMEOUT / 50; i++) {
		if (ps2_read_ctrl() & PS2_STATUS_OUTPUT_BUFFER_FULL)
			return B_OK;
		snooze(50);
	}
	return B_ERROR;
}


status_t
ps2_wait_write(void)
{
	int i;
	for (i = 0; i < PS2_CTRL_WAIT_TIMEOUT / 50; i++) {
		if (!(ps2_read_ctrl() & PS2_STATUS_INPUT_BUFFER_FULL))
			return B_OK;
		snooze(50);
	}
	return B_ERROR;
}


//	#pragma mark -

void
ps2_flush(void)
{
	int i;

	acquire_sem(gControllerSem);
	atomic_add(&sIgnoreInterrupts, 1);

	for (i = 0; i < 64; i++) {
		uint8 ctrl;
		uint8 data;
		ctrl = ps2_read_ctrl();
		if (!(ctrl & PS2_STATUS_OUTPUT_BUFFER_FULL))
			break;
		data = ps2_read_data();
		dprintf("ps2: ps2_flush: ctrl 0x%02x, data 0x%02x (%s)\n", ctrl, data, (ctrl & PS2_STATUS_AUX_DATA) ? "aux" : "keyb");
		snooze(100);
	}

	atomic_add(&sIgnoreInterrupts, -1);
	release_sem(gControllerSem);
}


static status_t
ps2_setup_command_byte()
{
	status_t res;
	uint8 cmdbyte;
	
	res = ps2_command(PS2_CTRL_READ_CMD, NULL, 0, &cmdbyte, 1);
	dprintf("ps2: get command byte: res 0x%08lx, cmdbyte 0x%02x\n", res, cmdbyte);
	if (res != B_OK)
		cmdbyte = 0x47;
		
	cmdbyte |= PS2_BITS_TRANSLATE_SCANCODES | PS2_BITS_KEYBOARD_INTERRUPT | PS2_BITS_AUX_INTERRUPT;
	cmdbyte &= ~(PS2_BITS_KEYBOARD_DISABLED | PS2_BITS_MOUSE_DISABLED);
		
	res = ps2_command(PS2_CTRL_WRITE_CMD, &cmdbyte, 1, NULL, 0);
	dprintf("ps2: set command byte: res 0x%08lx, cmdbyte 0x%02x\n", res, cmdbyte);
	
	return res;	
}


static status_t
ps2_setup_active_multiplexing(bool *enabled)
{
	status_t res;
	uint8 in, out;

	out = 0xf0;
	res = ps2_command(0xd3, &out, 1, &in, 1);
	if (res)
		return res;
	// Step 1, if controller is good, in does match out.
	// This test failes with MS Virtual PC.
	if (in != out)
		goto no_support;

	out = 0x56;
	res = ps2_command(0xd3, &out, 1, &in, 1);
	if (res)
		return res;
	// Step 2, if controller is good, in does match out.
	if (in != out)
		goto no_support;

	out = 0xa4;
	res = ps2_command(0xd3, &out, 1, &in, 1);
	if (res)
		return res;
	// Step 3, if the controller doesn't support active multiplexing, 
	// then in data does match out data (0xa4), else it's version number.
	if (in == out)
		goto no_support;

	// With some broken USB legacy emulation, it's 0xac, and with
	// MS Virtual PC, it's 0xa6. Since current active multiplexing
	// specification version is 1.1 (0x11), we validate the data.
	if (in > 0x9f) {
		dprintf("ps2: active multiplexing v%d.%d detected, but ignored!\n", (in >> 4), in & 0xf);
		goto no_support;
	}

	dprintf("ps2: active multiplexing v%d.%d enabled\n", (in >> 4), in & 0xf);
	ps2_command(0xa8, NULL, 0, NULL, 0);
	ps2_command(0x90, NULL, 0, NULL, 0);
	ps2_command(0xa8, NULL, 0, NULL, 0);
	ps2_command(0x91, NULL, 0, NULL, 0);
	ps2_command(0xa8, NULL, 0, NULL, 0);
	ps2_command(0x92, NULL, 0, NULL, 0);
	ps2_command(0xa8, NULL, 0, NULL, 0);
	ps2_command(0x93, NULL, 0, NULL, 0);
	ps2_command(0xa8, NULL, 0, NULL, 0);
	*enabled = true;
	return B_OK;

no_support:	
	dprintf("ps2: active multiplexing not supported\n");
	*enabled = false;
	return B_OK;
}


status_t
ps2_command(uint8 cmd, const uint8 *out, int out_count, uint8 *in, int in_count)
{
	status_t res;
	int i;
		
	acquire_sem(gControllerSem);
	atomic_add(&sIgnoreInterrupts, 1);

	dprintf("ps2: ps2_command cmd 0x%02x, out %d, in %d\n", cmd, out_count, in_count);
	for (i = 0; i < out_count; i++)
		dprintf("ps2: ps2_command out 0x%02x\n", out[i]);

	res = ps2_wait_write();
	if (res == B_OK)
		ps2_write_ctrl(cmd);
	
	for (i = 0; res == B_OK && i < out_count; i++) {
		res = ps2_wait_write();
		if (res == B_OK)
			ps2_write_data(out[i]);
		else
			dprintf("ps2: ps2_command out byte %d failed\n", i);
	}

	for (i = 0; res == B_OK && i < in_count; i++) {
		res = ps2_wait_read();
		if (res == B_OK)
			in[i] = ps2_read_data();
		else
			dprintf("ps2: ps2_command in byte %d failed\n", i);
	}

	for (i = 0; i < in_count; i++)
		dprintf("ps2: ps2_command in 0x%02x\n", in[i]);
	dprintf("ps2: ps2_command result 0x%08lx\n", res);

	atomic_add(&sIgnoreInterrupts, -1);
	release_sem(gControllerSem);
	
	return res;
}


//	#pragma mark -


static int32 
ps2_interrupt(void* cookie)
{
	uint8 ctrl;
	uint8 data;
	ps2_dev *dev;
	
	ctrl = ps2_read_ctrl();
	if (!(ctrl & PS2_STATUS_OUTPUT_BUFFER_FULL))
		return B_UNHANDLED_INTERRUPT;
		
	if (atomic_get(&sIgnoreInterrupts)) {
		TRACE(("ps2_interrupt: ignoring, ctrl 0x%02x (%s)\n", ctrl, (ctrl & PS2_STATUS_AUX_DATA) ? "aux" : "keyb"));
		return B_HANDLED_INTERRUPT;
	}
	
	data = ps2_read_data();

	if (ctrl & PS2_STATUS_AUX_DATA) {
		uint8 idx;
		if (gActiveMultiplexingEnabled) {
			idx = ctrl >> 6;
			TRACE(("ps2_interrupt: ctrl 0x%02x, data 0x%02x (mouse %d)\n", ctrl, data, idx));
		} else {
			idx = 0;
			TRACE(("ps2_interrupt: ctrl 0x%02x, data 0x%02x (aux)\n", ctrl, data));
		}
		dev = &ps2_device[PS2_DEVICE_MOUSE + idx];
	} else {
		TRACE(("ps2_interrupt: ctrl 0x%02x, data 0x%02x (keyb)\n", ctrl, data));
		dev = &ps2_device[PS2_DEVICE_KEYB];
	}
	
	return ps2_dev_handle_int(dev, data);
}


//	#pragma mark - driver interface


status_t
ps2_init(void)
{
	status_t status;

	TRACE(("ps2: init\n"));

	status = get_module(B_ISA_MODULE_NAME, (module_info **)&gIsa);
	if (status < B_OK)
		goto err_1;

	gControllerSem = create_sem(1, "ps/2 keyb ctrl");
	
	ps2_flush();

	status = ps2_dev_init();

	status = ps2_service_init();

	status = install_io_interrupt_handler(INT_PS2_KEYBOARD, &ps2_interrupt, NULL, 0);
	if (status)
		goto err_3;
	
	status = install_io_interrupt_handler(INT_PS2_MOUSE,    &ps2_interrupt, NULL, 0);
	if (status)
		goto err_4;
		
	status = ps2_setup_command_byte();
	if (status) {
		dprintf("ps2: setting up command byte failed\n");
		goto err_5;
	}

	status = ps2_setup_active_multiplexing(&gActiveMultiplexingEnabled);
	if (status) {
		dprintf("ps2: setting up active multiplexing failed\n");
		goto err_5;
	}

	ps2_service_notify_device_added(&ps2_device[PS2_DEVICE_KEYB]);
	ps2_service_notify_device_added(&ps2_device[PS2_DEVICE_MOUSE]);
	if (gActiveMultiplexingEnabled) {
		ps2_service_notify_device_added(&ps2_device[PS2_DEVICE_MOUSE + 1]);
		ps2_service_notify_device_added(&ps2_device[PS2_DEVICE_MOUSE + 2]);
		ps2_service_notify_device_added(&ps2_device[PS2_DEVICE_MOUSE + 3]);
	}
		
	TRACE(("ps2_hid: init_driver done!\n"));
	
	return B_OK;

err_5:
	remove_io_interrupt_handler(INT_PS2_MOUSE,    &ps2_interrupt, NULL);
err_4:	
	remove_io_interrupt_handler(INT_PS2_KEYBOARD, &ps2_interrupt, NULL);
err_3:
	ps2_service_exit();
	ps2_dev_exit();
	delete_sem(gControllerSem);
err_2:
	put_module(B_ISA_MODULE_NAME);
err_1:
	TRACE(("ps2_hid: init_driver failed!\n"));
	return B_ERROR;
}


void
ps2_uninit(void)
{
	TRACE(("ps2: uninit\n"));
	remove_io_interrupt_handler(INT_PS2_MOUSE,    &ps2_interrupt, NULL);
	remove_io_interrupt_handler(INT_PS2_KEYBOARD, &ps2_interrupt, NULL);
	ps2_service_exit();
	ps2_dev_exit();
	delete_sem(gControllerSem);
	put_module(B_ISA_MODULE_NAME);
}
