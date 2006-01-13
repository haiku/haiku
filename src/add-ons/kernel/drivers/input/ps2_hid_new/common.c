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


#include <Drivers.h>
#include <string.h>

#include "common.h"


#define DEVICE_MOUSE_NAME		"input/mouse/ps2/0"
#define DEVICE_KEYBOARD_NAME	"input/keyboard/at/0"

int32 api_version = B_CUR_DRIVER_API_VERSION;

static device_hooks sKeyboardDeviceHooks = {
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

static device_hooks sMouseDeviceHooks = {
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
sem_id gDeviceOpenSemaphore;

static int32 sInitialized = 0;
static uint8 sCommandByte = 0;

static sem_id sResultSemaphore;
static sem_id sResultOwnerSemaphore;
static uint8 *sResultBuffer;
static int32 sResultBytes;


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
	gIsa->write_io_8(PS2_PORT_CTRL, ctrl);
}


inline void
ps2_write_data(uint8 data)
{
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


void
ps2_flush()
{
	int i;
	for (i = 0; i < 64; i++) {
		uint8 ctrl;
		uint8 data;
		ctrl = ps2_read_ctrl();
		if (!(ctrl & PS2_STATUS_OUTPUT_BUFFER_FULL))
			return;
		data = ps2_read_data();
		TRACE(("ps2_flush: ctrl 0x%02x, data 0x%02x (%s)\n", ctrl, data, (ctrl & PS2_STATUS_MOUSE_DATA) ? "mouse" : "keyb"));
		snooze(100);
	}
}


//	#pragma mark -



static int32 
ps2_interrupt(void* cookie)
{
	uint8 ctrl;
	uint8 data;
	
	ctrl = ps2_read_ctrl();
	if (!(ctrl & PS2_STATUS_OUTPUT_BUFFER_FULL))
		return B_UNHANDLED_INTERRUPT;
	
	data = ps2_read_data();

	TRACE(("ps2_interrupt: ctrl 0x%02x, data 0x%02x (%s)\n", ctrl, data, (ctrl & PS2_STATUS_MOUSE_DATA) ? "mouse" : "keyb"));

	if (ctrl & PS2_STATUS_MOUSE_DATA)
		return mouse_handle_int(data);
	else
		return keyboard_handle_int(data);
}



//	#pragma mark -
//	driver interface


status_t
init_hardware(void)
{
	return B_OK;
}


const char **
publish_devices(void)
{
	static char *kDevices[3];
	int index = 0;

		kDevices[index++] = DEVICE_MOUSE_NAME;

		kDevices[index++] = DEVICE_KEYBOARD_NAME;

	kDevices[index++] = NULL;

	return (const char **)kDevices;
}


device_hooks *
find_device(const char *name)
{
	if (!strcmp(name, DEVICE_MOUSE_NAME))
		return &sMouseDeviceHooks;
	else if (!strcmp(name, DEVICE_KEYBOARD_NAME))
		return &sKeyboardDeviceHooks;

	return NULL;
}


status_t
init_driver(void)
{
	status_t status;

	status = get_module(B_ISA_MODULE_NAME, (module_info **)&gIsa);
	if (status < B_OK)
		goto err_1;

	gDeviceOpenSemaphore = create_sem(1, "ps/2 open");
	if (gDeviceOpenSemaphore < B_OK)
		goto err_2;

	status = install_io_interrupt_handler(INT_PS2_KEYBOARD, &ps2_interrupt, NULL, 0);
	if (status)
		goto err_3;
	
	status = install_io_interrupt_handler(INT_PS2_MOUSE,    &ps2_interrupt, NULL, 0);
	if (status)
		goto err_4;



	//goto err_5;	
	
	
	return B_OK;

err_5:
	remove_io_interrupt_handler(INT_PS2_MOUSE,    &ps2_interrupt, NULL);
err_4:	
	remove_io_interrupt_handler(INT_PS2_KEYBOARD, &ps2_interrupt, NULL);
err_3:
	delete_sem(gDeviceOpenSemaphore);
err_2:
	put_module(B_ISA_MODULE_NAME);
err_1:
	return B_ERROR;
}


void
uninit_driver(void)
{
	remove_io_interrupt_handler(INT_PS2_MOUSE,    &ps2_interrupt, NULL);
	remove_io_interrupt_handler(INT_PS2_KEYBOARD, &ps2_interrupt, NULL);
	delete_sem(gDeviceOpenSemaphore);
	put_module(B_ISA_MODULE_NAME);
}
