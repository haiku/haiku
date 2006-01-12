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
static bool sKeyboardDetected = false;
static bool sMouseDetected = false;

static sem_id sResultSemaphore;
static sem_id sResultOwnerSemaphore;
static uint8 *sResultBuffer;
static int32 sResultBytes;


/**	Wait until the specified status bits are cleared or set, depending on the
 *	second parameter.
 *	This currently busy waits, but should nonetheless be avoided in interrupt
 *	handlers.
 */

static status_t
wait_for_status(int32 bits, bool set)
{
	int8 read;
	int32 tries = 100;

	TRACE(("wait_write_ctrl(bits = %lx, %s)\n", bits, set ? "set" : "cleared"));

	while (tries-- > 0) {
		read = gIsa->read_io_8(PS2_PORT_CTRL);
		if (((read & bits) == bits) == set)
			return B_OK;

		spin(100);
	}

	return B_ERROR;
}


static inline status_t
wait_for_bits_cleared(int32 bits)
{
	return wait_for_status(bits, false);
}


static inline status_t
wait_for_bits_set(int32 bits)
{
	return wait_for_status(bits, true);
}


/**	Reads the command byte from the 8042 controller.
 *	Since the read goes through the data port, this function must not be
 *	called when the keyboard driver is up and running.
 */

static status_t
get_command_byte(uint8 *data)
{
	TRACE(("get_command_byte()\n"));

	if (ps2_write_ctrl(PS2_CTRL_READ_CMD) == B_OK)
		return ps2_read_data(data);

	return B_ERROR;
}


//	#pragma mark -


/** Wait until the control port is ready to be written. This requires that
 *	the "Input buffer full" and "Output buffer full" bits will both be set
 *	to 0. Returns true if the control port is ready to be written, false
 *	if 10ms have passed since the function has been called, and the control
 *	port is still busy.
 */

status_t
ps2_write_ctrl(uint8 data)
{
	if (wait_for_bits_cleared(PS2_STATUS_INPUT_BUFFER_FULL | PS2_STATUS_OUTPUT_BUFFER_FULL) != B_OK)
		return B_ERROR;

	gIsa->write_io_8(PS2_PORT_CTRL, data);
	return B_OK;
}


/** Wait until the data port is ready to be written, and then writes \a data to it.
 */

status_t
ps2_write_data(uint8 data)
{
	if (wait_for_bits_cleared(PS2_STATUS_INPUT_BUFFER_FULL) != B_OK)
		return B_ERROR;

	gIsa->write_io_8(PS2_PORT_DATA, data);
	return B_OK;
}


/** Wait until the data port can be read from, and then transfers the byte read
 *	to /a data.
 */

status_t
ps2_read_data(uint8 *data)
{
	if (wait_for_bits_set(PS2_STATUS_OUTPUT_BUFFER_FULL) != B_OK)
		return B_ERROR;

	*data = gIsa->read_io_8(PS2_PORT_DATA);
	TRACE(("ps2_read_data(): read %u\n", *data));
	return B_OK;
}


/** Get the PS/2 command byte. This cannot fail, since we're using our buffered
 *	data, read out in init_driver().
 */

uint8
ps2_get_command_byte(void)
{
	TRACE(("ps2_get_command_byte(): command byte = %x\n", sCommandByte));

	return sCommandByte;
}


/** Set the ps2 command byte.
 */

status_t
ps2_set_command_byte(uint8 command)
{
	TRACE(("set_command_byte(command = %x)\n", command));

	if (ps2_write_ctrl(PS2_CTRL_WRITE_CMD) == B_OK
		&& ps2_write_data(command) == B_OK) {
		sCommandByte = command;
		return B_OK;
	}

	return B_ERROR;
}


bool
ps2_handle_result(uint8 data)
{
	int32 bytesLeft;

	if (sResultBuffer == NULL
		|| (bytesLeft = atomic_add(&sResultBytes, -1)) <= 0)
		return false;

	*(sResultBuffer++) = data;
	if (bytesLeft == 1)
		release_sem_etc(sResultSemaphore, 1, B_DO_NOT_RESCHEDULE);
	return true;
}


void
ps2_claim_result(uint8 *buffer, size_t bytes)
{
	acquire_sem(sResultOwnerSemaphore);

	sResultBuffer = buffer;
	sResultBytes = bytes;
}


void
ps2_unclaim_result(void)
{
	sResultBytes = 0;
	sResultBuffer = NULL;

	release_sem(sResultOwnerSemaphore);
}


status_t
ps2_wait_for_result(void)
{
	status_t status = acquire_sem_etc(sResultSemaphore, 1, B_RELATIVE_TIMEOUT, 100000);
		// 0.1 secs for now

	ps2_unclaim_result();
	return status;
}


void
ps2_common_uninitialize(void)
{
	// do we still need the resources?
	if (atomic_add(&sInitialized, -1) > 1)
		return;

	delete_sem(sResultSemaphore);
	delete_sem(sResultOwnerSemaphore);
}


status_t
ps2_common_initialize(void)
{
	if (atomic_add(&sInitialized, 1) > 0) {
		// we're already initialized
		return B_OK;
	}

	sResultSemaphore = create_sem(0, "ps/2 result");
	if (sResultSemaphore < B_OK)
		return sResultSemaphore;

	sResultOwnerSemaphore = create_sem(1, "ps/2 result owner");
	if (sResultOwnerSemaphore < B_OK) {
		delete_sem(sResultSemaphore);
		return sResultOwnerSemaphore;
	}

	sResultBytes = 0;
	sResultBuffer = NULL;

	return B_OK;
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

	if (sMouseDetected)
		kDevices[index++] = DEVICE_MOUSE_NAME;

	if (sKeyboardDetected)
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
	if (status < B_OK) {
		TRACE(("Failed getting isa module: %s\n", strerror(status)));	
		return status;
	}

	// Try to probe for the mouse first, as this can hang the keyboard if no
	// mouse is found.
	// Probing the mouse first and initializing the keyboard later appearantly
	// clears the keyboard stall.

	if (probe_mouse(NULL) == B_OK)
		sMouseDetected = true;
	else
		dprintf("ps2_hid: no mouse detected!\n");

	if (probe_keyboard() == B_OK)
		sKeyboardDetected = true;
	else
		dprintf("ps2_hid: no keyboard detected!\n");

	// If there is no keyboard or mouse, we don't need to publish ourselves
	if (!sKeyboardDetected && !sMouseDetected) {
		put_module(B_ISA_MODULE_NAME);
		return B_ERROR;
	}

	// A semaphore to synchronize open keyboard and mouse devices

	gDeviceOpenSemaphore = create_sem(1, "ps/2 open");
	if (gDeviceOpenSemaphore < B_OK)
		return gDeviceOpenSemaphore;

	return get_command_byte(&sCommandByte);
}


void
uninit_driver(void)
{
	delete_sem(gDeviceOpenSemaphore);
	put_module(B_ISA_MODULE_NAME);	
}
