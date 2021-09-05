/*
 * Copyright 2013, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Hardware specs taken from the linux driver, thanks a lot!
 * Based on ps2_alps.c
 *
 * Authors:
 *		Jérôme Duval <korli@users.berlios.de>
 */


#include "ps2_elantech.h"

#include <stdlib.h>
#include <string.h>

#include "ps2_service.h"


//#define TRACE_PS2_ELANTECH
#ifdef TRACE_PS2_ELANTECH
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...)
#endif


static int32 generate_event(timer* timer);


const bigtime_t kEventInterval = 1000 * 50;


class EventProducer {
public:
	EventProducer()
	{
		fFired = false;
	}

	status_t
	FireEvent(elantech_cookie* cookie, uint8* package)
	{
		fCookie = cookie;
		memcpy(fLastPackage, package, sizeof(uint8) * PS2_PACKET_ELANTECH);

		status_t status = add_timer(&fEventTimer, &generate_event,
			kEventInterval, B_ONE_SHOT_RELATIVE_TIMER);
		if (status == B_OK)
			fFired  = true;
		return status;
	}

	bool
	CancelEvent()
	{
		if (!fFired)
			return false;
		fFired = false;
		return cancel_timer(&fEventTimer);
	}

	void
	InjectEvent()
	{
		if (packet_buffer_write(fCookie->ring_buffer, fLastPackage,
			PS2_PACKET_ELANTECH) != PS2_PACKET_ELANTECH) {
			// buffer is full, drop new data
			return;
		}
		release_sem_etc(fCookie->sem, 1, B_DO_NOT_RESCHEDULE);
	}

private:
	bool				fFired;
	uint8				fLastPackage[PS2_PACKET_ELANTECH];
	timer				fEventTimer;
	elantech_cookie*		fCookie;
};


static EventProducer gEventProducer;


static int32
generate_event(timer* timer)
{
	gEventProducer.InjectEvent();
	return B_HANDLED_INTERRUPT;
}


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

static hardware_specs gHardwareSpecs;


static status_t
get_elantech_movement(elantech_cookie *cookie, mouse_movement *movement)
{
	touch_event event;
	uint8 packet[PS2_PACKET_ELANTECH];

	status_t status = acquire_sem_etc(cookie->sem, 1, B_CAN_INTERRUPT, 0);
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

	if (cookie->crcEnabled && (packet[3] & 0x08) != 0) {
		TRACE("ELANTECH: bad crc buffer\n");
		return B_ERROR;
	} else if (!cookie->crcEnabled && ((packet[0] & 0x0c) != 0x04
		|| (packet[3] & 0x1c) != 0x10)) {
		TRACE("ELANTECH: bad crc buffer\n");
		return B_ERROR;
	}
		// https://www.kernel.org/doc/html/v4.16/input/devices/elantech.html
	uint8 packet_type = packet[3] & 3;
	TRACE("ELANTECH: packet type %d\n", packet_type);
	TRACE("ELANTECH: packet content 0x%02x%02x%02x%02x%02x%02x\n",
		packet[0], packet[1], packet[2], packet[3],
		packet[4], packet[5]);
	switch (packet_type) {
		case STATUS_PACKET:
			//fingers, no palm
			cookie->fingers = (packet[4] & 0x80) == 0 ? packet[1] & 0x1f: 0;
			dprintf("ELANTECH: Fingers %" B_PRId32 ", raw %x (STATUS)\n",
				cookie->fingers, packet[1]);
			break;
		case HEAD_PACKET:
			dprintf("ELANTECH: Fingers %d, raw %x (HEAD)\n", (packet[3] & 0xe0) >>5, packet[3]);
			// only process first finger
			if ((packet[3] & 0xe0) != 0x20)
				return B_OK;

			event.zPressure = (packet[1] & 0xf0) | ((packet[4] & 0xf0) >> 4);

			cookie->previousZ = event.zPressure;

			cookie->x = event.xPosition = ((packet[1] & 0xf) << 8) | packet[2];
			cookie->y = event.yPosition = ((packet[4] & 0xf) << 8) | packet[5];
			dprintf("ELANTECH: Pos: %" B_PRId32 ":%" B_PRId32 "\n (HEAD)",
				cookie->x, cookie->y);
			TRACE("ELANTECH: buttons 0x%x x %" B_PRIu32 " y %" B_PRIu32
				" z %d\n", event.buttons, event.xPosition, event.yPosition,
				event.zPressure);
			break;
		case MOTION_PACKET:
			dprintf("ELANTECH: Fingers %d, raw %x (MOTION)\n", (packet[3] & 0xe0) >>5, packet[3]);			//Most likely palm
			if (cookie->fingers == 0) return B_OK;
			//handle overflow and delta values
			if ((packet[0] & 0x10) != 0) {
				event.xPosition = cookie->x += 5 * (int8)packet[1];
				event.yPosition = cookie->y += 5 * (int8)packet[2];
			} else {
				event.xPosition = cookie->x += (int8)packet[1];
				event.yPosition = cookie->y += (int8)packet[2];
			}
			dprintf("ELANTECH: Pos: %" B_PRId32 ":%" B_PRId32 " (Motion)\n",
				cookie->x, cookie->y);

			break;
		default:
			dprintf("ELANTECH: unknown packet type %d\n", packet_type);
			return B_ERROR;
	}

	event.buttons = 0;
	event.wValue = cookie->fingers == 1 ? 4 :0;
	status = cookie->movementMaker.EventToMovement(&event, movement);

	if (cookie->movementMaker.WasEdgeMotion()
		|| cookie->movementMaker.TapDragStarted()) {
		gEventProducer.FireEvent(cookie, packet);
	}

	return status;
}


static void
default_settings(touchpad_settings *set)
{
	memcpy(set, &kDefaultTouchpadSettings, sizeof(touchpad_settings));
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

	if (val[0] == 0x0 || val[2] == 10 || val[2] == 20 || val[2] == 40
		|| val[2] == 60 || val[2] == 80 || val[2] == 100 || val[2] == 200) {
		TRACE("ELANTECH: not found (5)\n");
		return B_ERROR;
	}

	INFO("Elantech version %02X%02X%02X, under developement! Using fallback.\n",
		val[0], val[1], val[2]);

	dev->name = kElantechPath[dev->idx];
	dev->packet_size = PS2_PACKET_ELANTECH;

	return B_ERROR;
}


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
	*x = (val[1] & 0xf) * 10 + 790;
	*y = ((val[1] & 0xf) >> 4) * 10 + 790;
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

	if (cookie->version < 4) {
		uint8 val;

		for (uint8 retry = 0; retry < 5; retry++) {
			status = elantech_read_reg(cookie, 0x10, &val);
			if (status != B_OK)
				break;
			snooze(100);
		}
	}

	return status;
}


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

	cookie->movementMaker.Init();
	cookie->previousZ = 0;
	*_cookie = cookie;

	cookie->dev = dev;
	dev->cookie = cookie;
	dev->disconnect = &elantech_disconnect;
	dev->handle_int = &elantech_handle_int;

	default_settings(&cookie->settings);

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
			case 6:
			case 7:
				cookie->version = 4;
				break;
			default:
				TRACE("ELANTECH: unknown version!\n");
				goto err4;
		}
	}
	INFO("ELANTECH: version 0x%" B_PRIu32 " (0x%" B_PRIu32 ")\n",
		cookie->version, cookie->fwVersion);

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
	if (get_resolution_v4(cookie, &x_res, &y_res) != B_OK) {
		TRACE("ELANTECH: get resolution failed!\n");
		goto err4;
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

	cookie->movementMaker.SetSettings(&cookie->settings);
	cookie->movementMaker.SetSpecs(&gHardwareSpecs);

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
	gEventProducer.CancelEvent();

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
		INFO("ps2: reset failed\n");
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
	mouse_movement movement;
	status_t status;

	switch (op) {
		case MS_READ:
			TRACE("ELANTECH: MS_READ get event\n");
			if ((status = get_elantech_movement(cookie, &movement)) != B_OK)
				return status;
			return user_memcpy(buffer, &movement, sizeof(movement));

		case MS_IS_TOUCHPAD:
			TRACE("ELANTECH: MS_IS_TOUCHPAD\n");
			return B_OK;

		case MS_SET_TOUCHPAD_SETTINGS:
			TRACE("ELANTECH: MS_SET_TOUCHPAD_SETTINGS\n");
			user_memcpy(&cookie->settings, buffer, sizeof(touchpad_settings));
			return B_OK;

		case MS_SET_CLICKSPEED:
			TRACE("ELANTECH: ioctl MS_SETCLICK (set click speed)\n");
			return user_memcpy(&cookie->movementMaker.click_speed, buffer,
				sizeof(bigtime_t));

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

	// we got a real event cancel the fake event
	gEventProducer.CancelEvent();

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
