/*
 * Copyright 2004-2005 Haiku, Inc.
 * Distributed under the terms of the Haiku License.
 *
 * common.c:
 * PS/2 hid device driver
 * Authors (in chronological order):
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <string.h>

#include "ps2_common.h"
#include "ps2_service.h"
#include "ps2_dev.h"

device_hooks sKeyboardDeviceHooks = {
	keyboard_open,
	keyboard_close,
	keyboard_freecookie,
	keyboard_ioctl,
	keyboard_read,
	keyboard_write,
	NULL,
	NULL,
	NULL,
	NULL
};

device_hooks sMouseDeviceHooks = {
	mouse_open,
	mouse_close,
	mouse_freecookie,
	mouse_ioctl,
	mouse_read,
	mouse_write,
	NULL,
	NULL,
	NULL,
	NULL
};


isa_module_info *gIsa = NULL;

static sem_id sKbcSem;
static int32 sIgnoreInterrupts = 0;

bool gMultiplexingActive = false;


inline uint8
ps2_read_ctrl()
{
	return gIsa->read_io_8(PS2_PORT_CTRL);
}


inline uint8
ps2_read_data()
{
	return gIsa->read_io_8(PS2_PORT_DATA);
}


inline void
ps2_write_ctrl(uint8 ctrl)
{
	dprintf("ps2_write_ctrl 0x%02x\n", ctrl);

	gIsa->write_io_8(PS2_PORT_CTRL, ctrl);
}


inline void
ps2_write_data(uint8 data)
{
	dprintf("ps2_write_data 0x%02x\n", data);

	gIsa->write_io_8(PS2_PORT_DATA, data);
}


status_t
ps2_wait_read()
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
ps2_wait_write()
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
ps2_flush()
{
	int i;

	acquire_sem(sKbcSem);
	atomic_add(&sIgnoreInterrupts, 1);

	for (i = 0; i < 64; i++) {
		uint8 ctrl;
		uint8 data;
		ctrl = ps2_read_ctrl();
		if (!(ctrl & PS2_STATUS_OUTPUT_BUFFER_FULL))
			return;
		data = ps2_read_data();
		TRACE(("ps2_flush: ctrl 0x%02x, data 0x%02x (%s)\n", ctrl, data, (ctrl & PS2_STATUS_AUX_DATA) ? "aux" : "keyb"));
		snooze(100);
	}

	atomic_add(&sIgnoreInterrupts, -1);
	release_sem(sKbcSem);
}


status_t
ps2_command(uint8 cmd, const uint8 *out, int out_count, uint8 *in, int in_count)
{
	status_t res;
	int i;
	
	acquire_sem(sKbcSem);
	atomic_add(&sIgnoreInterrupts, 1);

	res = ps2_wait_write();
	if (res == B_OK)
		ps2_write_ctrl(cmd);
	
	for (i = 0; res == B_OK && i < out_count; i++) {
		res = ps2_wait_write();
		if (res == B_OK)
			ps2_write_data(out[i]);
	}

	for (i = 0; res == B_OK && i < in_count; i++) {
		res = ps2_wait_read();
		if (res == B_OK)
			in[i] = ps2_read_data();
	}

	atomic_add(&sIgnoreInterrupts, -1);
	release_sem(sKbcSem);
	
	return res;
}

status_t
ps2_keyboard_command(uint8 cmd, const uint8 *out, int out_count, uint8 *in, int in_count)
{
       return ps2_dev_command(&ps2_device[PS2_DEVICE_KEYB], cmd, out, out_count, in, in_count);
}

//	#pragma mark -

inline status_t
ps2_get_command_byte(uint8 *byte)
{
	return ps2_command(PS2_CTRL_READ_CMD, NULL, 0, byte, 1);
}


inline status_t
ps2_set_command_byte(uint8 byte)
{
	return ps2_command(PS2_CTRL_WRITE_CMD, &byte, 1, NULL, 0);
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
		if (gMultiplexingActive) {
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
ps2_init_driver(void)
{
	status_t status;

	TRACE(("ps2_hid: init_driver\n"));

	status = get_module(B_ISA_MODULE_NAME, (module_info **)&gIsa);
	if (status < B_OK)
		goto err_1;

	sKbcSem = create_sem(1, "ps/2 keyb ctrl");

	status = ps2_dev_init();

	status = ps2_service_init();

	status = install_io_interrupt_handler(INT_PS2_KEYBOARD, &ps2_interrupt, NULL, 0);
	if (status)
		goto err_3;
	
	status = install_io_interrupt_handler(INT_PS2_MOUSE,    &ps2_interrupt, NULL, 0);
	if (status)
		goto err_4;

	{
		uint8 d, in, out;
		status_t res;
		
		res = ps2_get_command_byte(&d);
		dprintf("ps2_get_command_byte: res 0x%08x, d 0x%02x\n", res, d);
		
		d |= PS2_BITS_TRANSLATE_SCANCODES | PS2_BITS_KEYBOARD_INTERRUPT | PS2_BITS_AUX_INTERRUPT;
		d &= ~(PS2_BITS_KEYBOARD_DISABLED | PS2_BITS_MOUSE_DISABLED);
		
		res = ps2_set_command_byte(d);
		dprintf("ps2_set_command_byte: res 0x%08x, d 0x%02x\n", res, d);
		
		in = 0x00;
		out = 0xf0;
		res = ps2_command(0xd3, &out, 1, &in, 1);
		dprintf("step1: res 0x%08x, out 0x%02x, in 0x%02x\n", res, out, in);

		in = 0x00;
		out = 0x56;
		res = ps2_command(0xd3, &out, 1, &in, 1);
		dprintf("step2: res 0x%08x, out 0x%02x, in 0x%02x\n", res, out, in);

		in = 0x00;
		out = 0xa4;
		res = ps2_command(0xd3, &out, 1, &in, 1);
		dprintf("step3: res 0x%08x, out 0x%02x, in 0x%02x\n", res, out, in);
		
		if (res == B_OK && in != 0xa4) {
			dprintf("found active multiplexing v%d.%d\n", (in >> 4), in & 0xf);
			gMultiplexingActive = true;

			res = ps2_command(0xa8, NULL, 0, NULL, 0);
			dprintf("step4: res 0x%08x\n", res);

			res = ps2_command(0x90, NULL, 0, NULL, 0);
			dprintf("step5: res 0x%08x\n", res);
			res = ps2_command(0xa8, NULL, 0, NULL, 0);
			dprintf("step6: res 0x%08x\n", res);

			res = ps2_command(0x91, NULL, 0, NULL, 0);
			dprintf("step7: res 0x%08x\n", res);
			res = ps2_command(0xa8, NULL, 0, NULL, 0);
			dprintf("step8: res 0x%08x\n", res);

			res = ps2_command(0x92, NULL, 0, NULL, 0);
			dprintf("step9: res 0x%08x\n", res);
			res = ps2_command(0xa8, NULL, 0, NULL, 0);
			dprintf("step10: res 0x%08x\n", res);

			res = ps2_command(0x93, NULL, 0, NULL, 0);
			dprintf("step11: res 0x%08x\n", res);
			res = ps2_command(0xa8, NULL, 0, NULL, 0);
			dprintf("step12: res 0x%08x\n", res);
			
			
		}
	}

	ps2_service_notify_device_added(&ps2_device[PS2_DEVICE_KEYB]);
	ps2_service_notify_device_added(&ps2_device[PS2_DEVICE_MOUSE]);
	if (gMultiplexingActive) {
		ps2_service_notify_device_added(&ps2_device[PS2_DEVICE_MOUSE + 1]);
		ps2_service_notify_device_added(&ps2_device[PS2_DEVICE_MOUSE + 2]);
		ps2_service_notify_device_added(&ps2_device[PS2_DEVICE_MOUSE + 3]);
	}
	
	//goto err_5;	
	
	TRACE(("ps2_hid: init_driver done!\n"));
	
	return B_OK;

err_5:
	remove_io_interrupt_handler(INT_PS2_MOUSE,    &ps2_interrupt, NULL);
err_4:	
	remove_io_interrupt_handler(INT_PS2_KEYBOARD, &ps2_interrupt, NULL);
err_3:
	ps2_service_exit();
	ps2_dev_exit();
	delete_sem(sKbcSem);
err_2:
	put_module(B_ISA_MODULE_NAME);
err_1:
	TRACE(("ps2_hid: init_driver failed!\n"));
	return B_ERROR;
}


void
ps2_uninit_driver(void)
{
	TRACE(("ps2_hid: uninit_driver\n"));
	remove_io_interrupt_handler(INT_PS2_MOUSE,    &ps2_interrupt, NULL);
	remove_io_interrupt_handler(INT_PS2_KEYBOARD, &ps2_interrupt, NULL);
	ps2_service_exit();
	ps2_dev_exit();
	delete_sem(sKbcSem);
	put_module(B_ISA_MODULE_NAME);
}
