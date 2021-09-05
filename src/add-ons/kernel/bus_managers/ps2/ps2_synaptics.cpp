/*
 * Copyright 2008-2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors (in chronological order):
 *		Clemens Zeidler (haiku@Clemens-Zeidler.de)
 *		Axel Dörfler, axeld@pinc-software.de
 */


//!	PS/2 synaptics touchpad


#include "ps2_synaptics.h"

#include <string.h>
#include <stdlib.h>

#include <keyboard_mouse_driver.h>

#include "ps2_service.h"


//#define TRACE_PS2_SYNAPTICS
#ifdef TRACE_PS2_SYNAPTICS
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...)
#endif

// synaptics touchpad proportions
#define SYN_EDGE_MOTION_WIDTH	50
#define SYN_AREA_OFFSET			40

#define MIN_PRESSURE			30
#define REAL_MAX_PRESSURE		100
#define MAX_PRESSURE			200

enum {
	kIdentify = 0x00,
	kReadModes = 0x01,
	kReadCapabilities = 0x02,
	kReadModelId = 0x03,
	kReadSerialNumberPrefix = 0x06,
	kReadSerialModelSuffix = 0x07,
	kReadResolutions = 0x08,
	kExtendedModelId = 0x09,
	kContinuedCapabilities = 0x0C,
	kMaximumCoordinates = 0x0D,
	kDeluxeLedInfo = 0x0E,
	kMinimumCoordinates = 0x0F,
	kTrackpointQuirk = 0x10
};

static hardware_specs gHardwareSpecs;


const char* kSynapticsPath[4] = {
	"input/touchpad/ps2/synaptics_0",
	"input/touchpad/ps2/synaptics_1",
	"input/touchpad/ps2/synaptics_2",
	"input/touchpad/ps2/synaptics_3"
};


static touchpad_info sTouchpadInfo;
static ps2_dev *sPassthroughDevice = &ps2_device[PS2_DEVICE_SYN_PASSTHROUGH];


static void
default_settings(touchpad_settings *set)
{
	memcpy(set, &kDefaultTouchpadSettings, sizeof(touchpad_settings));
}


static status_t
send_touchpad_arg_timeout(ps2_dev *dev, uint8 arg, bigtime_t timeout)
{
	int8 i;
	uint8 val[8];

	for (i = 0; i < 4; i++) {
		val[2 * i] = (arg >> (6 - 2 * i)) & 3;
		val[2 * i + 1] = 0xE8;
	}
	return ps2_dev_command_timeout(dev, 0xE8, val, 7, NULL, 0, timeout);
}


static status_t
send_touchpad_arg(ps2_dev *dev, uint8 arg)
{
	return send_touchpad_arg_timeout(dev, arg, 4000000);
}


static status_t
set_touchpad_mode(ps2_dev *dev, uint8 mode)
{
	uint8 sample_rate = SYN_CHANGE_MODE;
	send_touchpad_arg(dev, mode);
	return ps2_dev_command(dev, PS2_CMD_SET_SAMPLE_RATE, &sample_rate, 1,
		NULL, 0);
}


static status_t
get_information_query(ps2_dev *dev, uint8 extendedQueries, uint8 query,
	uint8 val[3])
{
	if (query == kTrackpointQuirk) {
		// Special case: this information query is not reported in the
		// "extended queries", but is still supported when the touchpad has
		// a pass-through port.
		if (!sTouchpadInfo.capPassThrough)
			return B_NOT_SUPPORTED;
	} else if (query > extendedQueries + 8)
		return B_NOT_SUPPORTED;

	status_t error = send_touchpad_arg(dev, query);
	if (error != B_OK)
		return error;
	return ps2_dev_command(dev, 0xE9, NULL, 0, val, 3);
}


static status_t
get_synaptics_movment(synaptics_cookie *cookie, mouse_movement *movement)
{
	status_t status;
	touch_event event;
	uint8 event_buffer[PS2_MAX_PACKET_SIZE];
	uint8 wValue0, wValue1, wValue2, wValue3, wValue;
	uint32 val32;
	uint32 xTwelfBit, yTwelfBit;

	status = acquire_sem_etc(cookie->synaptics_sem, 1, B_CAN_INTERRUPT, 0);
	if (status < B_OK)
		return status;

	if (!cookie->dev->active) {
		TRACE("SYNAPTICS: read_event: Error device no longer active\n");
		return B_ERROR;
	}

	if (packet_buffer_read(cookie->synaptics_ring_buffer, event_buffer,
			cookie->dev->packet_size) != cookie->dev->packet_size) {
		TRACE("SYNAPTICS: error copying buffer\n");
		return B_ERROR;
	}

	event.buttons = event_buffer[0] & 3;
 	event.zPressure = event_buffer[2];

 	if (sTouchpadInfo.capExtended) {
 		wValue0 = event_buffer[3] >> 2 & 1;
	 	wValue1 = event_buffer[0] >> 2 & 1;
 		wValue2 = event_buffer[0] >> 4 & 1;
	 	wValue3 = event_buffer[0] >> 5 & 1;

 		wValue = wValue0;
 		wValue = wValue | (wValue1 << 1);
 		wValue = wValue | (wValue2 << 2);
 		wValue = wValue | (wValue3 << 3);

	 	event.wValue = wValue;
	 	event.gesture = false;

		// Clickpad pretends that all clicks on the touchpad are middle clicks.
		// Pass them to userspace as left clicks instead.
		if (sTouchpadInfo.capClickPad)
			event.buttons |= ((event_buffer[0] ^ event_buffer[3]) & 0x01);

		if (sTouchpadInfo.capMiddleButton || sTouchpadInfo.capFourButtons)
			event.buttons |= ((event_buffer[0] ^ event_buffer[3]) & 0x01) << 2;

		if (sTouchpadInfo.nExtendedButtons > 0) {
			if (((event_buffer[0] ^ event_buffer[3]) & 0x02) != 0) {
				// This packet includes extended buttons state. The state is
				// only reported once when one of the buttons is pressed or
				// released, so we must keep track of the buttons state.

				// The values replace the lowest bits of the X and Y coordinates
				// in the packet, we need to extract them from there.

				bool pressed;
				for (int button = 0; button < sTouchpadInfo.nExtendedButtons;
						button++) {
					// Even buttons are in the X byte
					pressed = event_buffer[4 + button % 2] >> button / 2 & 0x1;
					if (pressed) {
						sTouchpadInfo.extendedButtonsState |= 1 << button;
					} else {
						sTouchpadInfo.extendedButtonsState &= ~(1 << button);
					}
				}
			}

			event.buttons |= sTouchpadInfo.extendedButtonsState
				<< sTouchpadInfo.firstExtendedButton;
		}
 	} else {
 		bool finger = event_buffer[0] >> 5 & 1;
 		if (finger) {
 			// finger with normal width
 			event.wValue = 4;
 		}
 		event.gesture = event_buffer[0] >> 2 & 1;
 	}

 	event.xPosition = event_buffer[4];
 	event.yPosition = event_buffer[5];

 	val32 = event_buffer[1] & 0x0F;
 	event.xPosition += val32 << 8;
 	val32 = event_buffer[1] >> 4 & 0x0F;
 	event.yPosition += val32 << 8;

 	xTwelfBit = event_buffer[3] >> 4 & 1;
 	event.xPosition += xTwelfBit << 12;
 	yTwelfBit = event_buffer[3] >> 5 & 1;
 	event.yPosition += yTwelfBit << 12;

 	status = cookie->movementMaker.EventToMovement(&event, movement);

	return status;
}


static void
query_capability(ps2_dev *dev)
{
	uint8 val[3];
	uint8 nExtendedQueries = 0;

	get_information_query(dev, nExtendedQueries, kReadCapabilities, val);

	TRACE("SYNAPTICS: extended mode %2x\n", val[0] >> 7 & 1);
	sTouchpadInfo.capExtended = val[0] >> 7 & 1;
	TRACE("SYNAPTICS: extended queries %2x\n", val[0] >> 4 & 7);
	nExtendedQueries = val[0] >> 4 & 7;
	TRACE("SYNAPTICS: middle button %2x\n", val[0] >> 2 & 1);
	sTouchpadInfo.capMiddleButton = val[0] >> 2 & 1;

	TRACE("SYNAPTICS: sleep mode %2x\n", val[2] >> 4 & 1);
	sTouchpadInfo.capSleep = val[2] >> 4 & 1;
	TRACE("SYNAPTICS: four buttons %2x\n", val[2] >> 3 & 1);
	sTouchpadInfo.capFourButtons = val[2] >> 3 & 1;
	TRACE("SYNAPTICS: multi finger %2x\n", val[2] >> 1 & 1);
	sTouchpadInfo.capMultiFinger = val[2] >> 1 & 1;
	TRACE("SYNAPTICS: palm detection %2x\n", val[2] & 1);
	sTouchpadInfo.capPalmDetection = val[2] & 1;
	TRACE("SYNAPTICS: pass through %2x\n", val[2] >> 7 & 1);
	sTouchpadInfo.capPassThrough = val[2] >> 7 & 1;

	if (get_information_query(dev, nExtendedQueries, kExtendedModelId, val)
			!= B_OK) {
		// "Extended Model ID" is not supported, so there cannot be extra
		// buttons.
		sTouchpadInfo.nExtendedButtons = 0;
		sTouchpadInfo.firstExtendedButton = 0;
		sTouchpadInfo.capClickPad = false;
		return;
	}

	sTouchpadInfo.capClickPad = (val[0] >> 5 & 1) | (val[1] >> 0 & 1);
	TRACE("SYNAPTICS: clickpad %x\n", sTouchpadInfo.capClickPad);

	TRACE("SYNAPTICS: extended buttons %2x\n", val[1] >> 4 & 15);
	sTouchpadInfo.nExtendedButtons = val[1] >> 4 & 15;
	sTouchpadInfo.extendedButtonsState = 0;

	if (sTouchpadInfo.capMiddleButton)
		sTouchpadInfo.firstExtendedButton = 3;
	else
		sTouchpadInfo.firstExtendedButton = 2;

	// Capability 0x10 is not documented in the Synaptics Touchpad interfacing
	// guide (at least the versions I could find), but we got the information
	// from Linux patches: https://lkml.org/lkml/2015/2/6/621
	if (get_information_query(dev, nExtendedQueries, kTrackpointQuirk, val)
			!= B_OK)
		return;

	// Workaround for Thinkpad use of the extended buttons: they are
	// used as buttons for the trackpoint, so they should be reported
	// as buttons 0, 1, 2 rather than 3, 4, 5.
	TRACE("SYNAPTICS: alternate buttons %2x\n", val[0] >> 0 & 1);
	if (val[0] >> 0 & 1)
		sTouchpadInfo.firstExtendedButton = 0;
}


//	#pragma mark - exported functions


status_t
synaptics_pass_through_set_packet_size(ps2_dev *dev, uint8 size)
{
	synaptics_cookie *synapticsCookie
		= (synaptics_cookie*)dev->parent_dev->cookie;

	status_t status = ps2_dev_command(dev->parent_dev, PS2_CMD_DISABLE, NULL,
		0, NULL, 0);
	if (status < B_OK) {
		INFO("SYNAPTICS: cannot disable touchpad %s\n", dev->parent_dev->name);
		return B_ERROR;
	}

	synapticsCookie->packet_index = 0;

	if (size == 4)
		synapticsCookie->mode |= SYN_FOUR_BYTE_CHILD;
	else
		synapticsCookie->mode &= ~SYN_FOUR_BYTE_CHILD;

	set_touchpad_mode(dev->parent_dev, synapticsCookie->mode);

	status = ps2_dev_command(dev->parent_dev, PS2_CMD_ENABLE, NULL, 0, NULL, 0);
	if (status < B_OK) {
		INFO("SYNAPTICS: cannot enable touchpad %s\n", dev->parent_dev->name);
		return B_ERROR;
	}
	return status;
}


status_t
passthrough_command(ps2_dev *dev, uint8 cmd, const uint8 *out, int outCount,
	uint8 *in, int inCount, bigtime_t timeout)
{
	status_t status;
	uint8 passThroughCmd = SYN_PASSTHROUGH_CMD;
	uint8 val;
	uint32 passThroughInCount = (inCount + 1) * 6;
	uint8 passThroughIn[passThroughInCount];
	int8 i;

	TRACE("SYNAPTICS: passthrough command 0x%x\n", cmd);

	status = ps2_dev_command(dev->parent_dev, PS2_CMD_DISABLE, NULL, 0,
		NULL, 0);
	if (status != B_OK)
		return status;

	for (i = -1; i < outCount; i++) {
		if (i == -1)
			val = cmd;
		else
			val = out[i];
		status = send_touchpad_arg_timeout(dev->parent_dev, val, timeout);
		if (status != B_OK)
			goto finalize;
		if (i != outCount -1) {
			status = ps2_dev_command_timeout(dev->parent_dev,
				PS2_CMD_SET_SAMPLE_RATE, &passThroughCmd, 1, NULL, 0, timeout);
			if (status != B_OK)
				goto finalize;
		}
	}
	status = ps2_dev_command_timeout(dev->parent_dev, PS2_CMD_SET_SAMPLE_RATE,
		&passThroughCmd, 1, passThroughIn, passThroughInCount, timeout);
	if (status != B_OK)
		goto finalize;

	for (i = 0; i < inCount + 1; i++) {
		uint8 *inPointer = &passThroughIn[i * 6];
		if (!IS_SYN_PT_PACKAGE(inPointer)) {
			TRACE("SYNAPTICS: not a pass throught package\n");
			goto finalize;
		}
		if (i == 0)
			continue;

		in[i - 1] = passThroughIn[i * 6 + 1];
	}

finalize:	
	status_t statusOfEnable = ps2_dev_command(dev->parent_dev, PS2_CMD_ENABLE,
			NULL, 0, NULL, 0);
	if (statusOfEnable != B_OK) {
		TRACE("SYNAPTICS: enabling of parent failed: 0x%" B_PRIx32 ".\n",
			statusOfEnable);
	}

	return status != B_OK ? status : statusOfEnable;
}


status_t
probe_synaptics(ps2_dev *dev)
{
	uint8 val[3];
	uint8 deviceId;
	status_t status;
	TRACE("SYNAPTICS: probe\n");

	// We reset the device here because it may have been left in a confused
	// state by a previous probing attempt. Some synaptics touchpads are known
	// to lockup when we attempt to detect them as IBM trackpoints.
	ps2_reset_mouse(dev);

	// Request "Identify touchpad"
	// The touchpad will delay this, until it's ready and calibrated.
	status = get_information_query(dev, 0, kIdentify, val);
	if (status != B_OK)
		return status;

	sTouchpadInfo.minorVersion = val[0];
	deviceId = val[1];
	if (deviceId != SYN_TOUCHPAD) {
		TRACE("SYNAPTICS: not found\n");
		return B_ERROR;
	}

	TRACE("SYNAPTICS: Touchpad found id:l %2x\n", deviceId);
	sTouchpadInfo.majorVersion = val[2] & 0x0F;
	TRACE("SYNAPTICS: version %d.%d\n", sTouchpadInfo.majorVersion,
		sTouchpadInfo.minorVersion);
	// version >= 4.0?
	if (sTouchpadInfo.minorVersion <= 2
		&& sTouchpadInfo.majorVersion <= 3) {
		TRACE("SYNAPTICS: too old touchpad not supported\n");
		return B_ERROR;
	}
	dev->name = kSynapticsPath[dev->idx];
	return B_OK;
}


//	#pragma mark - Device functions


status_t
synaptics_open(const char *name, uint32 flags, void **_cookie)
{
	status_t status;
	synaptics_cookie *cookie;
	ps2_dev *dev;
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

	cookie = (synaptics_cookie*)malloc(sizeof(synaptics_cookie));
	if (cookie == NULL)
		goto err1;
	memset(cookie, 0, sizeof(*cookie));

	cookie->movementMaker.Init();
	*_cookie = cookie;

	cookie->dev = dev;
	dev->cookie = cookie;
	dev->disconnect = &synaptics_disconnect;
	dev->handle_int = &synaptics_handle_int;

	default_settings(&cookie->settings);

	gHardwareSpecs.edgeMotionWidth = SYN_EDGE_MOTION_WIDTH;

	gHardwareSpecs.areaStartX = SYN_AREA_START_X;
	gHardwareSpecs.areaEndX = SYN_AREA_END_X;
	gHardwareSpecs.areaStartY = SYN_AREA_START_Y;
	gHardwareSpecs.areaEndY = SYN_AREA_END_Y;

	gHardwareSpecs.minPressure = MIN_PRESSURE;
	gHardwareSpecs.realMaxPressure = REAL_MAX_PRESSURE;
	gHardwareSpecs.maxPressure = MAX_PRESSURE;

	cookie->movementMaker.SetSettings(&cookie->settings);
	cookie->movementMaker.SetSpecs(&gHardwareSpecs);

	dev->packet_size = PS2_PACKET_SYNAPTICS;

	cookie->synaptics_ring_buffer
		= create_packet_buffer(SYNAPTICS_HISTORY_SIZE * dev->packet_size);
	if (cookie->synaptics_ring_buffer == NULL) {
		TRACE("ps2: can't allocate mouse actions buffer\n");
		goto err2;
	}

	// create the mouse semaphore, used for synchronization between
	// the interrupt handler and the read operation
	cookie->synaptics_sem = create_sem(0, "ps2_synaptics_sem");
	if (cookie->synaptics_sem < 0) {
		TRACE("SYNAPTICS: failed creating semaphore!\n");
		goto err3;
	}
	query_capability(dev);

	// create pass through dev
	if (sTouchpadInfo.capPassThrough) {
		TRACE("SYNAPTICS: pass through detected\n");
		sPassthroughDevice->parent_dev = dev;
		sPassthroughDevice->idx = dev->idx;
		ps2_service_notify_device_added(sPassthroughDevice);
	}

	// Set Mode
	if (sTouchpadInfo.capExtended)
		cookie->mode = SYN_ABSOLUTE_W_MODE;
	else
		cookie->mode = SYN_ABSOLUTE_MODE;

	status = set_touchpad_mode(dev, cookie->mode);
	if (status < B_OK) {
		INFO("SYNAPTICS: cannot set mode %s\n", name);
		goto err4;
	}

	status = ps2_dev_command(dev, PS2_CMD_ENABLE, NULL, 0, NULL, 0);
	if (status < B_OK) {
		INFO("SYNAPTICS: cannot enable touchpad %s\n", name);
		goto err4;
	}

	atomic_or(&dev->flags, PS2_FLAG_ENABLED);

	TRACE("SYNAPTICS: open %s success\n", name);
	return B_OK;

err4:
	delete_sem(cookie->synaptics_sem);
err3:
	delete_packet_buffer(cookie->synaptics_ring_buffer);
err2:
	free(cookie);
err1:
	atomic_and(&dev->flags, ~PS2_FLAG_OPEN);

	TRACE("SYNAPTICS: synaptics_open %s failed\n", name);
	return B_ERROR;
}


status_t
synaptics_close(void *_cookie)
{
	status_t status;
	synaptics_cookie *cookie = (synaptics_cookie*)_cookie;

	ps2_dev_command_timeout(cookie->dev, PS2_CMD_DISABLE, NULL, 0, NULL, 0,
		150000);

	delete_packet_buffer(cookie->synaptics_ring_buffer);
	delete_sem(cookie->synaptics_sem);

	atomic_and(&cookie->dev->flags, ~PS2_FLAG_OPEN);
	atomic_and(&cookie->dev->flags, ~PS2_FLAG_ENABLED);

	// Reset the touchpad so it generate standard ps2 packets instead of
	// extended ones. If not, BeOS is confused with such packets when rebooting
	// without a complete shutdown.
	status = ps2_reset_mouse(cookie->dev);
	if (status != B_OK) {
		INFO("ps2: reset failed\n");
		return B_ERROR;
	}

	if (sTouchpadInfo.capPassThrough)
		ps2_service_notify_device_removed(sPassthroughDevice);

	TRACE("SYNAPTICS: close %s done\n", cookie->dev->name);
	return B_OK;
}


status_t
synaptics_freecookie(void *_cookie)
{
	free(_cookie);
	return B_OK;
}


static status_t
synaptics_read(void *cookie, off_t pos, void *buffer, size_t *_length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


static status_t
synaptics_write(void *cookie, off_t pos, const void *buffer, size_t *_length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


status_t
synaptics_ioctl(void *_cookie, uint32 op, void *buffer, size_t length)
{
	synaptics_cookie *cookie = (synaptics_cookie*)_cookie;
	mouse_movement movement;
	status_t status;

	switch (op) {
		case MS_READ:
			TRACE("SYNAPTICS: MS_READ get event\n");
			if ((status = get_synaptics_movment(cookie, &movement)) != B_OK)
				return status;
			return user_memcpy(buffer, &movement, sizeof(movement));

		case MS_IS_TOUCHPAD:
			TRACE("SYNAPTICS: MS_IS_TOUCHPAD\n");
			return B_OK;

		case MS_SET_TOUCHPAD_SETTINGS:
			TRACE("SYNAPTICS: MS_SET_TOUCHPAD_SETTINGS");
			user_memcpy(&cookie->settings, buffer, sizeof(touchpad_settings));
			return B_OK;

		case MS_SET_CLICKSPEED:
			TRACE("SYNAPTICS: ioctl MS_SETCLICK (set click speed)\n");
			return user_memcpy(&cookie->movementMaker.click_speed, buffer,
				sizeof(bigtime_t));

		default:
			TRACE("SYNAPTICS: unknown opcode: %" B_PRIu32 "\n", op);
			return B_DEV_INVALID_IOCTL;
	}
}


int32
synaptics_handle_int(ps2_dev *dev)
{
	synaptics_cookie *cookie = (synaptics_cookie*)dev->cookie;
	uint8 val;

	val = cookie->dev->history[0].data;

	if ((cookie->packet_index == 0 || cookie->packet_index == 3)
		&& (val & 8) != 0) {
		INFO("SYNAPTICS: bad mouse data, trying resync\n");
		cookie->packet_index = 0;
		return B_UNHANDLED_INTERRUPT;
	}
	if (cookie->packet_index == 0 && val >> 6 != 0x02) {
	 	TRACE("SYNAPTICS: first package begins not with bit 1, 0\n");
		return B_UNHANDLED_INTERRUPT;
 	}
 	if (cookie->packet_index == 3 && val >> 6 != 0x03) {
	 	TRACE("SYNAPTICS: third package begins not with bit 1, 1\n");
	 	cookie->packet_index = 0;
		return B_UNHANDLED_INTERRUPT;
 	}
 	cookie->buffer[cookie->packet_index] = val;

	cookie->packet_index++;
	if (cookie->packet_index >= 6) {
		cookie->packet_index = 0;

		// check if package is a pass through package if true pass it
		// too the pass through interrupt handle
		if (sPassthroughDevice->active
			&& sPassthroughDevice->handle_int != NULL
			&& IS_SYN_PT_PACKAGE(cookie->buffer)) {
			status_t status;

			sPassthroughDevice->history[0].data = cookie->buffer[1];
			sPassthroughDevice->handle_int(sPassthroughDevice);
			sPassthroughDevice->history[0].data = cookie->buffer[4];
			sPassthroughDevice->handle_int(sPassthroughDevice);
			sPassthroughDevice->history[0].data = cookie->buffer[5];
			status = sPassthroughDevice->handle_int(sPassthroughDevice);

			if (cookie->dev->packet_size == 4) {
				sPassthroughDevice->history[0].data = cookie->buffer[2];
				status = sPassthroughDevice->handle_int(sPassthroughDevice);
			}
			return status;
		}

		if (packet_buffer_write(cookie->synaptics_ring_buffer,
				cookie->buffer, cookie->dev->packet_size)
			!= cookie->dev->packet_size) {
			// buffer is full, drop new data
			return B_HANDLED_INTERRUPT;
		}
		release_sem_etc(cookie->synaptics_sem, 1, B_DO_NOT_RESCHEDULE);

		return B_INVOKE_SCHEDULER;
	}

	return B_HANDLED_INTERRUPT;
}


void
synaptics_disconnect(ps2_dev *dev)
{
	synaptics_cookie *cookie = (synaptics_cookie*)dev->cookie;
	// the mouse device might not be opened at this point
	INFO("SYNAPTICS: synaptics_disconnect %s\n", dev->name);
	if ((dev->flags & PS2_FLAG_OPEN) != 0)
		release_sem(cookie->synaptics_sem);
}


device_hooks gSynapticsDeviceHooks = {
	synaptics_open,
	synaptics_close,
	synaptics_freecookie,
	synaptics_ioctl,
	synaptics_read,
	synaptics_write,
};
