/*
 * Copyright 2013-2025, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Hardware specs taken from the linux and BSDs drivers, thanks a lot!
 *
 * References:
 *	- https://cgit.freebsd.org/src/tree/sys/dev/atkbdc/psm.c?h=releng/14.3
 *	- https://cvsweb.openbsd.org/cgi-bin/cvsweb/src/sys/dev/pckbc/?only_with_tag=OPENBSD_7_8_BASE
 *	- https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/drivers/input/mouse/elantech.c?h=v6.17
 *	- https://www.kernel.org/doc/html/v4.16/input/devices/elantech.html
 *
 * Authors:
 *		Jérôme Duval <korli@users.berlios.de>
 *		Samuel Rodríguez Pérez <samuelrp84@gmail.com>
 */


#include "ps2_elantech.h"

#include <stdlib.h>
#include <string.h>

#include <keyboard_mouse_driver.h>

#include "ps2_service.h"


//#define TRACE_PS2_ELANTECH
#ifdef TRACE_PS2_ELANTECH
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...)
#endif

const char* kElantechPath[4] = {
	"input/touchpad/ps2/elantech_0",
	"input/touchpad/ps2/elantech_1",
	"input/touchpad/ps2/elantech_2",
	"input/touchpad/ps2/elantech_3"
};


#define ELANTECH_CMD_GET_ID				0x00
#define ELANTECH_CMD_GET_VERSION		0x01
#define ELANTECH_CMD_GET_CAPABILITIES	0x02
#define ELANTECH_CMD_GET_SAMPLE			0x03
#define ELANTECH_CMD_GET_RESOLUTION		0x04
#define ELANTECH_CMD_GET_ICBODY			0x05


#define ELANTECH_CMD_REGISTER_READ		0x10
#define ELANTECH_CMD_REGISTER_WRITE		0x11
#define ELANTECH_CMD_REGISTER_READWRITE	0x00
#define ELANTECH_CMD_PS2_CUSTOM_CMD		0xf8

// touchpad proportions
#define EDGE_MOTION_WIDTH	55

#define MIN_PRESSURE		0
#define REAL_MAX_PRESSURE	50
#define MAX_PRESSURE		255

#define ELANTECH_HISTORY_SIZE	256

#define STATUS_PACKET	0x0
#define HEAD_PACKET		0x1
#define MOTION_PACKET	0x2

static touchpad_specs gHardwareSpecs;


static status_t
elantech_process_packet_v4(elantech_cookie *cookie, touchpad_movement *_event,
	uint8 packet[PS2_PACKET_ELANTECH]);


/* Common legend
 * L: Left mouse button pressed
 * R: Right mouse button pressed
 * N: number of fingers on touchpad
 * X: absolute x value (horizontal)
 * Y: absolute y value (vertical)
 * W; width of the finger touch
 * P: pressure
 */
static status_t
get_elantech_movement(elantech_cookie *cookie, touchpad_movement *_event, bigtime_t timeout)
{
	uint8 packet[PS2_PACKET_ELANTECH];

	status_t status = acquire_sem_etc(cookie->sem, 1, B_CAN_INTERRUPT | B_RELATIVE_TIMEOUT,
		timeout);
	if (status < B_OK)
		return status;

	if (!cookie->dev->active) {
		TRACE("ELANTECH: read_event: Error device no longer active\n");
		return B_ERROR;
	}

	if (packet_buffer_read(cookie->ring_buffer, packet,
			cookie->dev->packet_size) != cookie->dev->packet_size) {
		TRACE("ELANTECH: error copying buffer\n");
		return B_ERROR;
	}

	return elantech_process_packet_v4(cookie, _event, packet);
}


static status_t
elantech_process_packet_v4(elantech_cookie *cookie, touchpad_movement *_event,
	uint8 packet[PS2_PACKET_ELANTECH])
{
	touchpad_movement event;

	int invalidAt = 0;

	if (cookie->crcEnabled) {
		if ((packet[3] & 0x08) != 0x00)
			invalidAt = 1;
	} else if (cookie->icVersion == 7 && cookie->samples[1] == 0x2A) {
		if ((packet[3] & 0x1c) != 0x10)
			invalidAt = 2;
	} else {
		if (!((packet[0] & 0x08) == 0x00 && (packet[3] & 0x1c) == 0x10))
			invalidAt = 3;
	}

	if (invalidAt != 0) {
		TRACE("ELANTECH: Failed v4 sanity check at %d.\n", invalidAt);
		return B_ERROR;
	}

	uint8 packet_type = packet[3] & 3;
	TRACE("ELANTECH: packet type %d\n", packet_type);
	TRACE("ELANTECH: packet content 0x%02x%02x%02x%02x%02x%02x\n",
		packet[0], packet[1], packet[2], packet[3],
		packet[4], packet[5]);
	switch (packet_type) {
		case STATUS_PACKET:
			/*               7   6   5   4   3   2   1   0 (LSB)
			 * -------------------------------------------
			 * ipacket[0]:   .   .   .   .   0   1   R   L
			 * ipacket[1]:   .   .   .  F4  F3  F2  F1  F0
			 * ipacket[2]:   .   .   .   .   .   .   .   .
			 * ipacket[3]:   .   .   .   1   0   0   0   0
			 * ipacket[4]:  PL   .   .   .   .   .   .   .
			 * ipacket[5]:   .   .   .   .   .   .   .   .
			 * -------------------------------------------
			 * Fn: finger n is on touchpad
			 * PL: palm
			 * HV ver4 sends a status packet to indicate that the numbers
			 * or identities of the fingers has been changed
			 */
			//fingers, no palm
			cookie->fingers = (packet[4] & 0x80) == 0 ? packet[1] & 0x1f: 0;
			TRACE("ELANTECH: Fingers %" B_PRId32 ", raw %x (STATUS)\n",
				cookie->fingers, packet[1]);
			break;
		case HEAD_PACKET:
			/*               7   6   5   4   3   2   1   0 (LSB)
			 * -------------------------------------------
			 * ipacket[0]:  W3  W2  W1  W0   0   1   R   L
			 * ipacket[1]:  P7  P6  P5  P4 X11 X10  X9  X8
			 * ipacket[2]:  X7  X6  X5  X4  X3  X2  X1  X0
			 * ipacket[3]: ID2 ID1 ID0   1   0   0   0   1
			 * ipacket[4]:  P3  P1  P2  P0 Y11 Y10  Y9  Y8
			 * ipacket[5]:  Y7  Y6  Y5  Y4  Y3  Y2  Y1  Y0
			 * -------------------------------------------
			 * ID: finger id
			 * HW ver 4 sends head packets in two cases:
			 * 1. One finger touch and movement.
			 * 2. Next after status packet to tell new finger positions.
			 */
			TRACE("ELANTECH: Fingers %d, raw %x (HEAD)\n", (packet[3] & 0xe0) >>5, packet[3]);
			// only process first finger
			if ((packet[3] & 0xe0) != 0x20)
				return B_OK;

			event.zPressure = (packet[1] & 0xf0) | ((packet[4] & 0xf0) >> 4);

			cookie->previousZ = event.zPressure;

			cookie->x = event.xPosition = ((packet[1] & 0xf) << 8) | packet[2];
			cookie->y = event.yPosition = ((packet[4] & 0xf) << 8) | packet[5];
			TRACE("ELANTECH: Pos: %" B_PRId32 ":%" B_PRId32 "\n (HEAD)",
				cookie->x, cookie->y);
			TRACE("ELANTECH: buttons 0x%x x %" B_PRIu32 " y %" B_PRIu32
				" z %d\n", event.buttons, event.xPosition, event.yPosition,
				event.zPressure);
			break;
		case MOTION_PACKET:
			/*               7   6   5   4   3   2   1   0 (LSB)
			 * -------------------------------------------
			 * ipacket[0]: ID2 ID1 ID0  OF   0   1   R   L
			 * ipacket[1]: DX7 DX6 DX5 DX4 DX3 DX2 DX1 DX0
			 * ipacket[2]: DY7 DY6 DY5 DY4 DY3 DY2 DY1 DY0
			 * ipacket[3]: ID2 ID1 ID0   1   0   0   1   0
			 * ipacket[4]: DX7 DX6 DX5 DX4 DX3 DX2 DX1 DX0
			 * ipacket[5]: DY7 DY6 DY5 DY4 DY3 DY2 DY1 DY0
			 * -------------------------------------------
			 * OF: delta overflows (> 127 or < -128), in this case
			 *     firmware sends us (delta x / 5) and (delta y / 5)
			 * ID: finger id
			 * DX: delta x (two's complement)
			 * XY: delta y (two's complement)
			 * byte 0 ~ 2 for one finger
			 * byte 3 ~ 5 for another finger
			 */
			TRACE("ELANTECH: Fingers %d, raw %x (MOTION)\n", (packet[3] & 0xe0) >>5, packet[3]);			//Most likely palm
			if (cookie->fingers == 0) return B_OK;
			//handle overflow and delta values
			if ((packet[0] & 0x10) != 0) {
				event.xPosition = cookie->x += 5 * (int8)packet[1];
				event.yPosition = cookie->y += 5 * (int8)packet[2];
			} else {
				event.xPosition = cookie->x += (int8)packet[1];
				event.yPosition = cookie->y += (int8)packet[2];
			}
			TRACE("ELANTECH: Pos: %" B_PRId32 ":%" B_PRId32 " (Motion)\n",
				cookie->x, cookie->y);

			break;
		default:
			dprintf("ELANTECH: unknown packet type %d\n", packet_type);
			return B_ERROR;
	}

	event.buttons = 0;
	event.fingerWidth = cookie->fingers == 1 ? 4 :0;

	*_event = event;
	return B_OK;
}


static status_t
synaptics_dev_send_command(ps2_dev* dev, uint8 cmd, uint8 *in, int in_count)
{
	if (ps2_dev_sliced_command(dev, cmd) != B_OK
		|| ps2_dev_command(dev, PS2_CMD_MOUSE_GET_INFO, NULL, 0, in, in_count)
		!= B_OK) {
		TRACE("ELANTECH: synaptics_dev_send_command failed\n");
		return B_ERROR;
	}
	return B_OK;
}


static status_t
elantech_dev_send_command(ps2_dev* dev, uint8 cmd, uint8 *in, int in_count)
{
	if (ps2_dev_command(dev, ELANTECH_CMD_PS2_CUSTOM_CMD) != B_OK
		|| ps2_dev_command(dev, cmd) != B_OK
		|| ps2_dev_command(dev, PS2_CMD_MOUSE_GET_INFO, NULL, 0, in, in_count)
		!= B_OK) {
		TRACE("ELANTECH: elantech_dev_send_command failed\n");
		return B_ERROR;
	}
	return B_OK;
}


//	#pragma mark - exported functions


status_t
probe_elantech(ps2_dev* dev)
{
	uint8 val[3];
	TRACE("ELANTECH: probe\n");

	ps2_dev_command(dev, PS2_CMD_MOUSE_RESET_DIS);

	if (ps2_dev_command(dev, PS2_CMD_DISABLE) != B_OK
		|| ps2_dev_command(dev, PS2_CMD_MOUSE_SET_SCALE11) != B_OK
		|| ps2_dev_command(dev, PS2_CMD_MOUSE_SET_SCALE11) != B_OK
		|| ps2_dev_command(dev, PS2_CMD_MOUSE_SET_SCALE11) != B_OK) {
		TRACE("ELANTECH: not found (1)\n");
		return B_ERROR;
	}

	if (ps2_dev_command(dev, PS2_CMD_MOUSE_GET_INFO, NULL, 0, val, 3)
		!= B_OK) {
		TRACE("ELANTECH: not found (2)\n");
		return B_ERROR;
	}

	if (val[0] != 0x3c || val[1] != 0x3 || (val[2] != 0xc8 && val[2] != 0x0)) {
		TRACE("ELANTECH: not found (3)\n");
		return B_ERROR;
	}

	val[0] = 0;
	if (synaptics_dev_send_command(dev, ELANTECH_CMD_GET_VERSION, val, 3)
		!= B_OK) {
		TRACE("ELANTECH: not found (4)\n");
		return B_ERROR;
	}

	uint32 fwVersion = ((val[0] << 16) | (val[1] << 8) | val[2]);
	bool v1 = fwVersion < 0x020030 || fwVersion == 0x020600;
	bool v2 = (val[0] & 0xf) == 4 || ((val[0] & 0xf) == 2 && !v1);
	bool v3 = (val[0] & 0xf) == 5;
	bool v4 = (val[0] & 0xf) >= 6 && (val[0] & 0xf) <= 15;

	if (val[0] == 0x0 || val[2] == 10 || val[2] == 20 || val[2] == 40
		|| val[2] == 60 || val[2] == 80 || val[2] == 100 || val[2] == 200) {
		TRACE("ELANTECH: not found (5)\n");
		return B_ERROR;
	}

	// TODO: Update supported after testing all versions.
	bool supported = !v4 || !v3 || !v2 || !v1;

	if (supported && v3) {
		uint8 samples[3];
		if (elantech_dev_send_command(dev, ELANTECH_CMD_GET_SAMPLE, samples, 3) != B_OK) {
			TRACE("ELANTECH: failed to query sample data\n");
			return B_ERROR;
		}

		if (samples[1] == 0x74) {
			TRACE("ELANTECH: Absolute mode broken, forcing standard PS/2 protocol\n");
			supported = false;
		}
	}

	if (supported) {
		INFO("Elantech version %02X%02X%02X detected.\n",
			val[0], val[1], val[2]);
	} else {
		INFO("Elantech version %02X%02X%02X, under developement! Using fallback.\n",
			val[0], val[1], val[2]);
	}

	dev->name = kElantechPath[dev->idx];
	dev->packet_size = PS2_PACKET_ELANTECH;

	return supported ? B_OK : B_ERROR;
}


//	#pragma mark - Setup functions


static status_t
elantech_write_reg(elantech_cookie* cookie, uint8 reg, uint8 value)
{
	if (reg < 0x7 || reg > 0x26)
		return B_BAD_VALUE;
	if (reg > 0x11 && reg < 0x20)
		return B_BAD_VALUE;

	ps2_dev* dev = cookie->dev;
	switch (cookie->version) {
		case 1:
			// TODO
			return B_ERROR;
			break;
		case 2:
			if (ps2_dev_command(dev, ELANTECH_CMD_PS2_CUSTOM_CMD) != B_OK
				|| ps2_dev_command(dev, ELANTECH_CMD_REGISTER_WRITE) != B_OK
				|| ps2_dev_command(dev, ELANTECH_CMD_PS2_CUSTOM_CMD) != B_OK
				|| ps2_dev_command(dev, reg) != B_OK
				|| ps2_dev_command(dev, ELANTECH_CMD_PS2_CUSTOM_CMD) != B_OK
				|| ps2_dev_command(dev, value) != B_OK
				|| ps2_dev_command(dev, PS2_CMD_MOUSE_SET_SCALE11) != B_OK)
				return B_ERROR;
			break;
		case 3:
			if (ps2_dev_command(dev, ELANTECH_CMD_PS2_CUSTOM_CMD) != B_OK
				|| ps2_dev_command(dev, ELANTECH_CMD_REGISTER_READWRITE) != B_OK
				|| ps2_dev_command(dev, ELANTECH_CMD_PS2_CUSTOM_CMD) != B_OK
				|| ps2_dev_command(dev, reg) != B_OK
				|| ps2_dev_command(dev, ELANTECH_CMD_PS2_CUSTOM_CMD) != B_OK
				|| ps2_dev_command(dev, value) != B_OK
				|| ps2_dev_command(dev, PS2_CMD_MOUSE_SET_SCALE11) != B_OK)
				return B_ERROR;
			break;
		case 4:
			if (ps2_dev_command(dev, ELANTECH_CMD_PS2_CUSTOM_CMD) != B_OK
				|| ps2_dev_command(dev, ELANTECH_CMD_REGISTER_READWRITE) != B_OK
				|| ps2_dev_command(dev, ELANTECH_CMD_PS2_CUSTOM_CMD) != B_OK
				|| ps2_dev_command(dev, reg) != B_OK
				|| ps2_dev_command(dev, ELANTECH_CMD_PS2_CUSTOM_CMD) != B_OK
				|| ps2_dev_command(dev, ELANTECH_CMD_REGISTER_READWRITE) != B_OK
				|| ps2_dev_command(dev, ELANTECH_CMD_PS2_CUSTOM_CMD) != B_OK
				|| ps2_dev_command(dev, value) != B_OK
				|| ps2_dev_command(dev, PS2_CMD_MOUSE_SET_SCALE11) != B_OK)
				return B_ERROR;
			break;
		default:
			TRACE("ELANTECH: read_write_reg: unknown version\n");
			return B_ERROR;
	}
	return B_OK;
}


static status_t
elantech_read_reg(elantech_cookie* cookie, uint8 reg, uint8 *value)
{
	if (reg < 0x7 || reg > 0x26)
		return B_BAD_VALUE;
	if (reg > 0x11 && reg < 0x20)
		return B_BAD_VALUE;

	ps2_dev* dev = cookie->dev;
	uint8 val[3];
	switch (cookie->version) {
		case 1:
			// TODO
			return B_ERROR;
			break;
		case 2:
			if (ps2_dev_command(dev, ELANTECH_CMD_PS2_CUSTOM_CMD) != B_OK
				|| ps2_dev_command(dev, ELANTECH_CMD_REGISTER_READ) != B_OK
				|| ps2_dev_command(dev, ELANTECH_CMD_PS2_CUSTOM_CMD) != B_OK
				|| ps2_dev_command(dev, reg) != B_OK
				|| ps2_dev_command(dev, PS2_CMD_MOUSE_GET_INFO, NULL, 0, val,
					3) != B_OK)
				return B_ERROR;
			break;
		case 3:
		case 4:
			if (ps2_dev_command(dev, ELANTECH_CMD_PS2_CUSTOM_CMD) != B_OK
				|| ps2_dev_command(dev, ELANTECH_CMD_REGISTER_READWRITE)
					!= B_OK
				|| ps2_dev_command(dev, ELANTECH_CMD_PS2_CUSTOM_CMD) != B_OK
				|| ps2_dev_command(dev, reg) != B_OK
				|| ps2_dev_command(dev, PS2_CMD_MOUSE_GET_INFO, NULL, 0, val,
					3) != B_OK)
				return B_ERROR;
			break;
		default:
			TRACE("ELANTECH: read_write_reg: unknown version\n");
			return B_ERROR;
	}
	if (cookie->version == 4)
		*value = val[1];
	else
		*value = val[0];

	return B_OK;
}


static status_t
get_resolution_v4(elantech_cookie* cookie, uint32* x, uint32* y)
{
	uint8 val[3];
	if (elantech_dev_send_command(cookie->dev, ELANTECH_CMD_GET_RESOLUTION,
		val, 3) != B_OK)
		return B_ERROR;
	*x = ((val[1] & 0xf) * 10 + 790) * 10 / 254;
	*y = (((val[1] & 0xf) >> 4) * 10 + 790) * 10 / 254;
	return B_OK;
}


static status_t
get_range(elantech_cookie* cookie, uint32* x_min, uint32* y_min, uint32* x_max,
	uint32* y_max, uint32 *width)
{
	uint8 val[3];
	switch (cookie->version) {
		case 1:
			*x_min = 32;
			*y_min = 32;
			*x_max = 544;
			*y_max = 344;
			*width = 0;
			break;
		case 2:
			// TODO
			break;
		case 3:
			if ((cookie->send_command)(cookie->dev, ELANTECH_CMD_GET_ID, val, 3)
				!= B_OK) {
				return B_ERROR;
			}
			*x_min = 0;
			*y_min = 0;
			*x_max = ((val[0] & 0xf) << 8) | val[1];
			*y_max = ((val[0] & 0xf0) << 4) | val[2];
			*width = 0;
			break;
		case 4:
			if ((cookie->send_command)(cookie->dev, ELANTECH_CMD_GET_ID, val, 3)
				!= B_OK) {
				return B_ERROR;
			}
			*x_min = 0;
			*y_min = 0;
			*x_max = ((val[0] & 0xf) << 8) | val[1];
			*y_max = ((val[0] & 0xf0) << 4) | val[2];
			if (cookie->capabilities[1] < 2 || cookie->capabilities[1] > *x_max)
				return B_ERROR;
			*width = *x_max / (cookie->capabilities[1] - 1);
			break;
	}
	return B_OK;
}


static status_t
enable_absolute_mode(elantech_cookie* cookie)
{
	status_t status = B_OK;
	switch (cookie->version) {
		case 1:
			status = elantech_write_reg(cookie, 0x10, 0x16);
			if (status == B_OK)
				status = elantech_write_reg(cookie, 0x11, 0x8f);
			break;
		case 2:
			status = elantech_write_reg(cookie, 0x10, 0x54);
			if (status == B_OK)
				status = elantech_write_reg(cookie, 0x11, 0x88);
			if (status == B_OK)
				status = elantech_write_reg(cookie, 0x12, 0x60);
			break;
		case 3:
			status = elantech_write_reg(cookie, 0x10, 0xb);
			break;
		case 4:
			status = elantech_write_reg(cookie, 0x7, 0x1);
			break;

	}

	if (status == B_OK && cookie->version < 4) {
		uint8 val;

		for (uint8 retry = 0; retry < 5; retry++) {
			status = elantech_read_reg(cookie, 0x10, &val);
			if (status != B_OK)
				break;
			snooze(2000);
		}
	}

	return status;
}


//	#pragma mark - Device functions


status_t
elantech_open(const char *name, uint32 flags, void **_cookie)
{
	TRACE("ELANTECH: open %s\n", name);
	ps2_dev* dev;
	int i;
	for (dev = NULL, i = 0; i < PS2_DEVICE_COUNT; i++) {
		if (0 == strcmp(ps2_device[i].name, name)) {
			dev = &ps2_device[i];
			break;
		}
	}

	if (dev == NULL) {
		TRACE("ps2: dev = NULL\n");
		return B_ERROR;
	}

	if (atomic_or(&dev->flags, PS2_FLAG_OPEN) & PS2_FLAG_OPEN)
		return B_BUSY;

	uint32 x_min = 0, x_max = 0, y_min = 0, y_max = 0, width = 0;

	elantech_cookie* cookie = (elantech_cookie*)malloc(
		sizeof(elantech_cookie));
	if (cookie == NULL)
		goto err1;
	memset(cookie, 0, sizeof(*cookie));

	cookie->previousZ = 0;
	*_cookie = cookie;

	cookie->dev = dev;
	dev->cookie = cookie;
	dev->disconnect = &elantech_disconnect;
	dev->handle_int = &elantech_handle_int;

	dev->packet_size = PS2_PACKET_ELANTECH;

	cookie->ring_buffer = create_packet_buffer(
		ELANTECH_HISTORY_SIZE * dev->packet_size);
	if (cookie->ring_buffer == NULL) {
		TRACE("ELANTECH: can't allocate mouse actions buffer\n");
		goto err2;
	}
	// create the mouse semaphore, used for synchronization between
	// the interrupt handler and the read operation
	cookie->sem = create_sem(0, "ps2_elantech_sem");
	if (cookie->sem < 0) {
		TRACE("ELANTECH: failed creating semaphore!\n");
		goto err3;
	}

	uint8 val[3];
	if (synaptics_dev_send_command(dev, ELANTECH_CMD_GET_VERSION, val, 3)
		!= B_OK) {
		TRACE("ELANTECH: get version failed!\n");
		goto err4;
	}
	cookie->fwVersion = (val[0] << 16) | (val[1] << 8) | val[2];
	if (cookie->fwVersion < 0x020030 || cookie->fwVersion == 0x020600)
		cookie->version = 1;
	else {
		switch (val[0] & 0xf) {
			case 2:
			case 4:
				cookie->version = 2;
				break;
			case 5:
				cookie->version = 3;
				break;
			case 6 ... 15:
				cookie->version = 4;
				break;
			default:
				TRACE("ELANTECH: unknown version!\n");
				goto err4;
		}
	}
	INFO("ELANTECH: version 0x%" B_PRIu32 " (0x%" B_PRIu32 ")\n",
		cookie->version, cookie->fwVersion);

	cookie->icVersion = (cookie->fwVersion & 0x0f0000) >> 16;

	if (cookie->version >= 3)
		cookie->send_command = &elantech_dev_send_command;
	else
		cookie->send_command = &synaptics_dev_send_command;
	cookie->crcEnabled = (cookie->fwVersion & 0x4000) == 0x4000;

	if ((cookie->send_command)(cookie->dev, ELANTECH_CMD_GET_CAPABILITIES,
		cookie->capabilities, 3) != B_OK) {
		TRACE("ELANTECH: get capabilities failed!\n");
		return B_ERROR;
	}

	if (cookie->version != 1) {
		if (cookie->send_command(cookie->dev, ELANTECH_CMD_GET_SAMPLE, cookie->samples, 3)
			!= B_OK) {
			TRACE("ELANTECH: failed to query sample data\n");
			return B_ERROR;
		}
	}

	if (enable_absolute_mode(cookie) != B_OK) {
		TRACE("ELANTECH: failed enabling absolute mode!\n");
		goto err4;
	}
	TRACE("ELANTECH: enabled absolute mode!\n");

	if (get_range(cookie, &x_min, &y_min, &x_max, &y_max, &width) != B_OK) {
		TRACE("ELANTECH: get range failed!\n");
		goto err4;
	}

	TRACE("ELANTECH: range x %" B_PRIu32 "-%" B_PRIu32 " y %" B_PRIu32
		"-%" B_PRIu32 " (%" B_PRIu32 ")\n", x_min, x_max, y_min, y_max, width);

	uint32 x_res, y_res;
	x_res = 31;
	y_res = 31;
	if (cookie->version == 4) {
		if (get_resolution_v4(cookie, &x_res, &y_res) != B_OK) {
			TRACE("ELANTECH: get resolution failed!\n");
		}
	}

	TRACE("ELANTECH: resolution x %" B_PRIu32 " y %" B_PRIu32 " (dpi)\n",
		x_res, y_res);

	gHardwareSpecs.edgeMotionWidth = EDGE_MOTION_WIDTH;

	gHardwareSpecs.areaStartX = x_min;
	gHardwareSpecs.areaEndX = x_max;
	gHardwareSpecs.areaStartY = y_min;
	gHardwareSpecs.areaEndY = y_max;

	gHardwareSpecs.minPressure = MIN_PRESSURE;
	gHardwareSpecs.realMaxPressure = REAL_MAX_PRESSURE;
	gHardwareSpecs.maxPressure = MAX_PRESSURE;

	if (ps2_dev_command(dev, PS2_CMD_ENABLE, NULL, 0, NULL, 0) != B_OK)
		goto err4;

	atomic_or(&dev->flags, PS2_FLAG_ENABLED);

	TRACE("ELANTECH: open %s success\n", name);
	return B_OK;

err4:
	delete_sem(cookie->sem);
err3:
	delete_packet_buffer(cookie->ring_buffer);
err2:
	free(cookie);
err1:
	atomic_and(&dev->flags, ~PS2_FLAG_OPEN);

	TRACE("ELANTECH: open %s failed\n", name);
	return B_ERROR;
}


status_t
elantech_close(void *_cookie)
{
	elantech_cookie *cookie = (elantech_cookie*)_cookie;

	ps2_dev_command_timeout(cookie->dev, PS2_CMD_DISABLE, NULL, 0, NULL, 0,
		150000);

	delete_packet_buffer(cookie->ring_buffer);
	delete_sem(cookie->sem);

	atomic_and(&cookie->dev->flags, ~PS2_FLAG_OPEN);
	atomic_and(&cookie->dev->flags, ~PS2_FLAG_ENABLED);

	// Reset the touchpad so it generate standard ps2 packets instead of
	// extended ones. If not, BeOS is confused with such packets when rebooting
	// without a complete shutdown.
	status_t status = ps2_reset_mouse(cookie->dev);
	if (status != B_OK) {
		INFO("ps2_elantech: reset failed\n");
		return B_ERROR;
	}

	TRACE("ELANTECH: close %s done\n", cookie->dev->name);
	return B_OK;
}


status_t
elantech_freecookie(void *_cookie)
{
	free(_cookie);
	return B_OK;
}


status_t
elantech_ioctl(void *_cookie, uint32 op, void *buffer, size_t length)
{
	elantech_cookie *cookie = (elantech_cookie*)_cookie;
	touchpad_read read;
	status_t status;

	switch (op) {
		case MS_IS_TOUCHPAD:
			TRACE("ELANTECH: MS_IS_TOUCHPAD\n");
			if (buffer == NULL)
				return B_OK;
			return user_memcpy(buffer, &gHardwareSpecs, sizeof(gHardwareSpecs));

		case MS_READ_TOUCHPAD:
			TRACE("ELANTECH: MS_READ get event\n");
			if (user_memcpy(&read.timeout, &(((touchpad_read*)buffer)->timeout),
					sizeof(bigtime_t)) != B_OK)
				return B_BAD_ADDRESS;
			if ((status = get_elantech_movement(cookie, &read.u.touchpad, read.timeout)) != B_OK)
				return status;
			read.event = MS_READ_TOUCHPAD;
			return user_memcpy(buffer, &read, sizeof(read));

		default:
			INFO("ELANTECH: unknown opcode: 0x%" B_PRIx32 "\n", op);
			return B_BAD_VALUE;
	}
}


static status_t
elantech_read(void* cookie, off_t pos, void* buffer, size_t* _length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


static status_t
elantech_write(void* cookie, off_t pos, const void* buffer, size_t* _length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


int32
elantech_handle_int(ps2_dev* dev)
{
	elantech_cookie* cookie = (elantech_cookie*)dev->cookie;

	uint8 val;
	val = cookie->dev->history[0].data;
 	cookie->buffer[cookie->packet_index] = val;
	cookie->packet_index++;

	if (cookie->packet_index < PS2_PACKET_ELANTECH)
		return B_HANDLED_INTERRUPT;

	cookie->packet_index = 0;
	if (packet_buffer_write(cookie->ring_buffer,
				cookie->buffer, cookie->dev->packet_size)
			!= cookie->dev->packet_size) {
		// buffer is full, drop new data
		return B_HANDLED_INTERRUPT;
	}
	release_sem_etc(cookie->sem, 1, B_DO_NOT_RESCHEDULE);
	return B_INVOKE_SCHEDULER;
}


void
elantech_disconnect(ps2_dev *dev)
{
	elantech_cookie *cookie = (elantech_cookie*)dev->cookie;
	// the mouse device might not be opened at this point
	INFO("ELANTECH: elantech_disconnect %s\n", dev->name);
	if ((dev->flags & PS2_FLAG_OPEN) != 0)
		release_sem(cookie->sem);
}


device_hooks gElantechDeviceHooks = {
	elantech_open,
	elantech_close,
	elantech_freecookie,
	elantech_ioctl,
	elantech_read,
	elantech_write,
};
