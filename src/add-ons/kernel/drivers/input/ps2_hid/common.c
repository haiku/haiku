/*
 * Copyright 2004-2005 Haiku, Inc.
 * Distributed under the terms of the Haiku License.
 *
 * common.c:
 * PS/2 hid device driver
 * Authors (in chronological order):
 *		Stefano Ceccherini (burton666@libero.it)
 */


#include <Drivers.h>
#include <string.h>

#include "common.h"


#define DEVICE_MOUSE_NAME		"input/mouse/ps2/0"
#define DEVICE_KEYBOARD_NAME	"input/keyboard/at/0"

int32 api_version = B_CUR_DRIVER_API_VERSION;
isa_module_info *gIsa = NULL;

device_hooks
keyboard_hooks = {
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


device_hooks
mouse_hooks = {
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


/** Wait until the control port is ready to be written. This requires that
 *	the "Input buffer full" and "Output buffer full" bits will both be set
 *	to 0. Returns true if the control port is ready to be written, false
 *	if 10ms have passed since the function has been called, and the control
 *	port is still busy.
 */

bool
wait_write_ctrl(void)
{
	int8 read;
	int32 tries = 100;
	TRACE(("wait_write_ctrl()\n"));
	do {
		read = gIsa->read_io_8(PS2_PORT_CTRL);
		spin(100);
	} while ((read & (PS2_IBUF_FULL | PS2_OBUF_FULL)) && tries-- > 0);

	return tries > 0;
}


/** Wait until the data port is ready to be written. This requires that
 *	the "Input buffer full" bit will be set to 0.
 */

bool
wait_write_data(void)
{
	int8 read;
	int32 tries = 100;
	TRACE(("wait_write_data()\n"));
	do {
		read = gIsa->read_io_8(PS2_PORT_CTRL);
		spin(100);
	} while ((read & PS2_IBUF_FULL) && tries-- > 0);

	return tries > 0;
}


/** Wait until the data port can be read from. This requires that the
 *	"Output buffer full" bit will be set to 1.
 */

bool
wait_read_data(void)
{
	int8 read;
	int32 tries = 100;
	TRACE(("wait_read_data()\n"));
	do {
		read = gIsa->read_io_8(PS2_PORT_CTRL);
		spin(100);
	} while (!(read & PS2_OBUF_FULL) && tries-- > 0);

	return tries > 0;
}

/** Get the ps2 command byte.	
 */

int8
get_command_byte(void)
{
	int8 read = 0;

	TRACE(("set_command_byte()\n"));
	if (wait_write_ctrl()) {
		gIsa->write_io_8(PS2_PORT_CTRL, PS2_CTRL_READ_CMD);
		if (wait_read_data())
			read = gIsa->read_io_8(PS2_PORT_DATA);
	}

	return read;
}


/** Set the ps2 command byte.
 *	Parameters:
 *	unsigned char, byte to write
 */

void
set_command_byte(unsigned char cmd)
{
	TRACE(("set_command_byte()\n"));
	if (wait_write_ctrl()) {
		gIsa->write_io_8(PS2_PORT_CTRL, PS2_CTRL_WRITE_CMD);
		if (wait_write_data())
			gIsa->write_io_8(PS2_PORT_DATA, cmd);
	}
}


/** Reads a single byte from the data port.
 *	Return value:
 *	unsigned char, byte read
 */

uint8
read_data_byte(void)
{
	TRACE(("read_data_byte()\n"));
	if (wait_read_data()) {
		TRACE(("read_data_byte(): ok\n"));
		return gIsa->read_io_8(PS2_PORT_DATA);
	} 
	
	TRACE(("read_data_byte(): timeout\n"));
	
	return PS2_ERROR;
}


/** Writes a byte to the mouse device. Uses the control port to indicate
 *	that the byte is sent to the auxiliary device (mouse), instead of the
 *	keyboard.
 *	Parameters:
 *	unsigned char, byte to write
 */

void
write_aux_byte(unsigned char cmd)
{
	TRACE(("write_aux_byte()\n"));
	if (wait_write_ctrl()) {
		gIsa->write_io_8(PS2_PORT_CTRL, PS2_CTRL_WRITE_AUX);

		if (wait_write_data())
			gIsa->write_io_8(PS2_PORT_DATA, cmd);
	}
}


//	#pragma mark -
//	driver interface


status_t
init_hardware(void)
{
	return B_OK;
}


static const char *
kDevices[] = {
	DEVICE_KEYBOARD_NAME,
	DEVICE_MOUSE_NAME, 
	NULL
};


const char **
publish_devices(void)
{
	return kDevices;
}


device_hooks *
find_device(const char *name)
{
	if (!strcmp(name, DEVICE_MOUSE_NAME))
		return &mouse_hooks;
	else if (!strcmp(name, DEVICE_KEYBOARD_NAME))
		return &keyboard_hooks;

	return NULL;
}


status_t
init_driver(void)
{
	status_t status;

	status = get_module(B_ISA_MODULE_NAME, (module_info **)&gIsa);
	if (status < B_OK) {
		TRACE(("Failed getting isa module: %s\n", strerror(status)));	
		return status;
	}

	return B_OK;
}


void
uninit_driver(void)
{
	put_module(B_ISA_MODULE_NAME);	
}
