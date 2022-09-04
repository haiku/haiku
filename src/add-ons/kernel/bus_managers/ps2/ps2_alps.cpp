/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * The alps_model_info struct and all the hardware specs are taken from the
 * linux driver, thanks a lot!
 *
 * Authors:
 *		Clemens Zeidler (haiku@Clemens-Zeidler.de)
 */


#include "ps2_alps.h"

#include <stdlib.h>
#include <string.h>

#include <keyboard_mouse_driver.h>

#include "ps2_service.h"


//#define TRACE_PS2_ALPS
#ifdef TRACE_PS2_APLS
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...)
#endif


const char* kALPSPath[4] = {
	"input/touchpad/ps2/alps_0",
	"input/touchpad/ps2/alps_1",
	"input/touchpad/ps2/alps_2",
	"input/touchpad/ps2/alps_3"
};


typedef struct alps_model_info {
	uint8		id[3];
	uint8		firstByte;
	uint8		maskFirstByte;
	uint8		flags;
} alps_model_info;


#define ALPS_OLDPROTO           0x01	// old style input
#define ALPS_DUALPOINT          0x02	// touchpad has trackstick
#define ALPS_PASS               0x04    // device has a pass-through port

#define ALPS_WHEEL              0x08	// hardware wheel present
#define ALPS_FW_BK_1            0x10	// front & back buttons present
#define ALPS_FW_BK_2            0x20	// front & back buttons present
#define ALPS_FOUR_BUTTONS       0x40	// 4 direction button present
#define ALPS_PS2_INTERLEAVED    0x80	// 3-byte PS/2 packet interleaved with
										// 6-byte ALPS packet

static const struct alps_model_info gALPSModelInfos[] = {
	{{0x32, 0x02, 0x14}, 0xf8, 0xf8, ALPS_PASS | ALPS_DUALPOINT},
		// Toshiba Salellite Pro M10
//	{{0x33, 0x02, 0x0a}, 0x88, 0xf8, ALPS_OLDPROTO},
		// UMAX-530T
	{{0x53, 0x02, 0x0a}, 0xf8, 0xf8, 0},
	{{0x53, 0x02, 0x14}, 0xf8, 0xf8, 0},
	{{0x60, 0x03, 0xc8}, 0xf8, 0xf8, 0},
		// HP ze1115
	{{0x63, 0x02, 0x0a}, 0xf8, 0xf8, 0},
	{{0x63, 0x02, 0x14}, 0xf8, 0xf8, 0},
	{{0x63, 0x02, 0x28}, 0xf8, 0xf8, ALPS_FW_BK_2},
		// Fujitsu Siemens S6010
//	{{0x63, 0x02, 0x3c}, 0x8f, 0x8f, ALPS_WHEEL},
		// Toshiba Satellite S2400-103
	{{0x63, 0x02, 0x50}, 0xef, 0xef, ALPS_FW_BK_1},
		// NEC Versa L320
	{{0x63, 0x02, 0x64}, 0xf8, 0xf8, 0},
	{{0x63, 0x03, 0xc8}, 0xf8, 0xf8, ALPS_PASS | ALPS_DUALPOINT},
		// Dell Latitude D800
	{{0x73, 0x00, 0x0a}, 0xf8, 0xf8, ALPS_DUALPOINT},
		// ThinkPad R61 8918-5QG, x301
	{{0x73, 0x02, 0x0a}, 0xf8, 0xf8, 0},
	{{0x73, 0x02, 0x14}, 0xf8, 0xf8, ALPS_FW_BK_2},
		// Ahtec Laptop
	{{0x20, 0x02, 0x0e}, 0xf8, 0xf8, ALPS_PASS | ALPS_DUALPOINT},
		// XXX
	{{0x22, 0x02, 0x0a}, 0xf8, 0xf8, ALPS_PASS | ALPS_DUALPOINT},
	{{0x22, 0x02, 0x14}, 0xff, 0xff, ALPS_PASS | ALPS_DUALPOINT},
		// Dell Latitude D600
//	{{0x62, 0x02, 0x14}, 0xcf, 0xcf,  ALPS_PASS | ALPS_DUALPOINT
//		| ALPS_PS2_INTERLEAVED},
		// Dell Latitude E5500, E6400, E6500, Precision M4400
	{{0x73, 0x02, 0x50}, 0xcf, 0xcf, ALPS_FOUR_BUTTONS},
		// Dell Vostro 1400
//	{{0x52, 0x01, 0x14}, 0xff, 0xff, ALPS_PASS | ALPS_DUALPOINT
//		| ALPS_PS2_INTERLEAVED},
		// Toshiba Tecra A11-11L
	{{0, 0, 0}, 0, 0, 0}
};


static alps_model_info* sFoundModel = NULL;


// touchpad proportions
#define EDGE_MOTION_WIDTH	55
// increase the touchpad size a little bit
#define AREA_START_X		40
#define AREA_END_X			987
#define AREA_START_Y		40
#define AREA_END_Y			734

#define MIN_PRESSURE		15
#define REAL_MAX_PRESSURE	70
#define MAX_PRESSURE		115


#define ALPS_HISTORY_SIZE	256


static touchpad_specs gHardwareSpecs;


/* Data taken from linux driver:
ALPS absolute Mode - new format
byte 0:  1    ?    ?    ?    1    ?    ?    ?
byte 1:  0   x6   x5   x4   x3   x2   x1   x0
byte 2:  0  x10   x9   x8   x7    ?  fin  ges
byte 3:  0   y9   y8   y7    1    M    R    L
byte 4:  0   y6   y5   y4   y3   y2   y1   y0
byte 5:  0   z6   z5   z4   z3   z2   z1   z0
*/
static status_t
get_alps_movment(alps_cookie *cookie, touchpad_read *_read)
{
	status_t status;
	touchpad_movement event;
	uint8 event_buffer[PS2_PACKET_ALPS];

	status = acquire_sem_etc(cookie->sem, 1, B_CAN_INTERRUPT | B_RELATIVE_TIMEOUT,
		_read->timeout);
	if (status < B_OK)
		return status;

	if (!cookie->dev->active) {
		TRACE("ALPS: read_event: Error device no longer active\n");
		return B_ERROR;
	}

	if (packet_buffer_read(cookie->ring_buffer, event_buffer,
			cookie->dev->packet_size) != cookie->dev->packet_size) {
		TRACE("ALPS: error copying buffer\n");
		return B_ERROR;
	}

	event.buttons = event_buffer[3] & 7;
	event.zPressure = event_buffer[5];

	// finger on touchpad
	if (event_buffer[2] & 0x2) {
		// finger with normal width
		event.fingerWidth = 4;
	} else {
		event.fingerWidth = 3;
	}

	// tab gesture
	if (event_buffer[2] & 0x1) {
		event.zPressure = 60;
		event.fingerWidth = 4;
	}
	// if hardware tab gesture is off a z pressure of 16 is reported
	if (cookie->previousZ == 0 && event.fingerWidth == 4 && event.zPressure == 16)
		event.zPressure = 60;

	cookie->previousZ = event.zPressure;

	event.xPosition = event_buffer[1] | ((event_buffer[2] & 0x78) << 4);
	event.yPosition = event_buffer[4] | ((event_buffer[3] & 0x70) << 3);

	// check for trackpoint even (z pressure 127)
	if (sFoundModel->flags & ALPS_DUALPOINT && event.zPressure == 127) {
		mouse_movement movement;
		movement.xdelta = event.xPosition > 383 ? event.xPosition - 768
			: event.xPosition;
		movement.ydelta = event.yPosition > 255
			? event.yPosition - 512 : event.yPosition;
		movement.wheel_xdelta = 0;
		movement.wheel_ydelta = 0;
		movement.buttons = event.buttons;
		movement.timestamp = system_time();

		_read->event = MS_READ;
		_read->u.mouse = movement;
	} else {
		event.yPosition = AREA_END_Y - (event.yPosition - AREA_START_Y);

		_read->event = MS_READ_TOUCHPAD;
		_read->u.touchpad = event;
	}

	return status;
}


status_t
probe_alps(ps2_dev* dev)
{
	int i;
	uint8 val[3];
	TRACE("ALPS: probe\n");

	val[0] = 0;
	if (ps2_dev_command(dev, PS2_CMD_MOUSE_SET_RES, val, 1, NULL, 0) != B_OK
		|| ps2_dev_command(dev, PS2_CMD_MOUSE_SET_SCALE11, NULL, 0, NULL, 0)
			!= B_OK
		|| ps2_dev_command(dev, PS2_CMD_MOUSE_SET_SCALE11, NULL, 0, NULL, 0)
			!= B_OK
		|| ps2_dev_command(dev, PS2_CMD_MOUSE_SET_SCALE11, NULL, 0, NULL, 0)
			!= B_OK)
		return B_ERROR;

	if (ps2_dev_command(dev, PS2_CMD_MOUSE_GET_INFO, NULL, 0, val, 3)
		!= B_OK)
		return B_ERROR;

	if (val[0] != 0 || val[1] != 0 || (val[2] != 10 && val[2] != 100))
		return B_ERROR;

	val[0] = 0;
	if (ps2_dev_command(dev, PS2_CMD_MOUSE_SET_RES, val, 1, NULL, 0) != B_OK
		|| ps2_dev_command(dev, PS2_CMD_MOUSE_SET_SCALE21, NULL, 0, NULL, 0)
			!= B_OK
		|| ps2_dev_command(dev, PS2_CMD_MOUSE_SET_SCALE21, NULL, 0, NULL, 0)
			!= B_OK
		|| ps2_dev_command(dev, PS2_CMD_MOUSE_SET_SCALE21, NULL, 0, NULL, 0)
			!= B_OK)
		return B_ERROR;

	if (ps2_dev_command(dev, PS2_CMD_MOUSE_GET_INFO, NULL, 0, val, 3)
		!= B_OK)
		return B_ERROR;

	for (i = 0; ; i++) {
		const alps_model_info* info = &gALPSModelInfos[i];
		if (info->id[0] == 0) {
			INFO("ALPS not supported: %2.2x %2.2x %2.2x\n", val[0], val[1],
				val[2]);
			return B_ERROR;
		}

		if (info->id[0] == val[0] && info->id[1] == val[1]
			&& info->id[2] == val[2]) {
			sFoundModel = (alps_model_info*)info;
			INFO("ALPS found: %2.2x %2.2x %2.2x\n", val[0], val[1], val[2]);
			break;
		}
	}

	dev->name = kALPSPath[dev->idx];
	dev->packet_size = PS2_PACKET_ALPS;

	return B_OK;
}


status_t
switch_hardware_tab(ps2_dev* dev, bool on)
{
	uint8 val[3];
	uint8 arg = 0x00;
	uint8 command = PS2_CMD_MOUSE_SET_RES;
	if (on) {
		arg = 0x0A;
		command = PS2_CMD_SET_TYPEMATIC;
	}
	if (ps2_dev_command(dev, PS2_CMD_MOUSE_GET_INFO, NULL, 0, val, 3) != B_OK
		|| ps2_dev_command(dev, PS2_CMD_DISABLE, NULL, 0, NULL, 0) != B_OK
		|| ps2_dev_command(dev, PS2_CMD_DISABLE, NULL, 0, NULL, 0) != B_OK
		|| ps2_dev_command(dev, command, &arg, 1, NULL, 0) != B_OK)
		return B_ERROR;

	return B_OK;
}


status_t
enable_passthrough(ps2_dev* dev, bool on)
{
	uint8 command = PS2_CMD_MOUSE_SET_SCALE11;
	if (on)
		command = PS2_CMD_MOUSE_SET_SCALE21;

	if (ps2_dev_command(dev, command, NULL, 0, NULL, 0) != B_OK
		|| ps2_dev_command(dev, command, NULL, 0, NULL, 0) != B_OK
		|| ps2_dev_command(dev, command, NULL, 0, NULL, 0) != B_OK
		|| ps2_dev_command(dev, PS2_CMD_DISABLE, NULL, 0, NULL, 0) != B_OK)
		return B_ERROR;

	return B_OK;
}


status_t
alps_open(const char *name, uint32 flags, void **_cookie)
{
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

	alps_cookie* cookie = (alps_cookie*)malloc(sizeof(alps_cookie));
	if (cookie == NULL)
		goto err1;
	memset(cookie, 0, sizeof(*cookie));

	cookie->previousZ = 0;
	*_cookie = cookie;

	cookie->dev = dev;
	dev->cookie = cookie;
	dev->disconnect = &alps_disconnect;
	dev->handle_int = &alps_handle_int;

	gHardwareSpecs.edgeMotionWidth = EDGE_MOTION_WIDTH;

	gHardwareSpecs.areaStartX = AREA_START_X;
	gHardwareSpecs.areaEndX = AREA_END_X;
	gHardwareSpecs.areaStartY = AREA_START_Y;
	gHardwareSpecs.areaEndY = AREA_END_Y;

	gHardwareSpecs.minPressure = MIN_PRESSURE;
	gHardwareSpecs.realMaxPressure = REAL_MAX_PRESSURE;
	gHardwareSpecs.maxPressure = MAX_PRESSURE;

	dev->packet_size = PS2_PACKET_ALPS;

	cookie->ring_buffer = create_packet_buffer(
		ALPS_HISTORY_SIZE * dev->packet_size);
	if (cookie->ring_buffer == NULL) {
		TRACE("ALPS: can't allocate mouse actions buffer\n");
		goto err2;
	}
	// create the mouse semaphore, used for synchronization between
	// the interrupt handler and the read operation
	cookie->sem = create_sem(0, "ps2_alps_sem");
	if (cookie->sem < 0) {
		TRACE("ALPS: failed creating semaphore!\n");
		goto err3;
	}

	if ((sFoundModel->flags & ALPS_PASS) != 0
		&& enable_passthrough(dev, true) != B_OK)
		goto err4;

	// switch tap mode off
	if (switch_hardware_tab(dev, false) != B_OK)
		goto err4;

	// init the alps device to absolut mode
	if (ps2_dev_command(dev, PS2_CMD_DISABLE, NULL, 0, NULL, 0) != B_OK
		|| ps2_dev_command(dev, PS2_CMD_DISABLE, NULL, 0, NULL, 0) != B_OK
		|| ps2_dev_command(dev, PS2_CMD_DISABLE, NULL, 0, NULL, 0) != B_OK
		|| ps2_dev_command(dev, PS2_CMD_DISABLE, NULL, 0, NULL, 0) != B_OK
		|| ps2_dev_command(dev, PS2_CMD_ENABLE, NULL, 0, NULL, 0) != B_OK)
		goto err4;

	if ((sFoundModel->flags & ALPS_PASS) != 0
		&& enable_passthrough(dev, false) != B_OK)
		goto err4;

	if (ps2_dev_command(dev, PS2_CMD_MOUSE_SET_STREAM, NULL, 0, NULL, 0) != B_OK)
		goto err4;

	if (ps2_dev_command(dev, PS2_CMD_ENABLE, NULL, 0, NULL, 0) != B_OK)
		goto err4;

	atomic_or(&dev->flags, PS2_FLAG_ENABLED);

	TRACE("ALPS: open %s success\n", name);
	return B_OK;

err4:
	delete_sem(cookie->sem);
err3:
	delete_packet_buffer(cookie->ring_buffer);
err2:
	free(cookie);
err1:
	atomic_and(&dev->flags, ~PS2_FLAG_OPEN);

	TRACE("ALPS: open %s failed\n", name);
	return B_ERROR;
}


status_t
alps_close(void *_cookie)
{
	alps_cookie *cookie = (alps_cookie*)_cookie;

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
		INFO("ps2_alps: reset failed\n");
		return B_ERROR;
	}

	TRACE("ALPS: close %s done\n", cookie->dev->name);
	return B_OK;
}


status_t
alps_freecookie(void *_cookie)
{
	free(_cookie);
	return B_OK;
}


status_t
alps_ioctl(void *_cookie, uint32 op, void *buffer, size_t length)
{
	alps_cookie *cookie = (alps_cookie*)_cookie;
	touchpad_read read;
	status_t status;

	switch (op) {
		case MS_IS_TOUCHPAD:
			TRACE("ALPS: MS_IS_TOUCHPAD\n");
			if (buffer == NULL)
				return B_OK;
			return user_memcpy(buffer, &gHardwareSpecs, sizeof(gHardwareSpecs));

		case MS_READ_TOUCHPAD:
			TRACE("ALPS: MS_READ get event\n");
			if (user_memcpy(&read.timeout, &(((touchpad_read*)buffer)->timeout),
					sizeof(bigtime_t)) != B_OK)
				return B_BAD_ADDRESS;
			if ((status = get_alps_movment(cookie, &read)) != B_OK)
				return status;
			return user_memcpy(buffer, &read, sizeof(read));

		default:
			TRACE("ALPS: unknown opcode: %" B_PRIu32 "\n", op);
			return B_BAD_VALUE;
	}
}


static status_t
alps_read(void* cookie, off_t pos, void* buffer, size_t* _length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


static status_t
alps_write(void* cookie, off_t pos, const void* buffer, size_t* _length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


int32
alps_handle_int(ps2_dev* dev)
{
	alps_cookie* cookie = (alps_cookie*)dev->cookie;

	uint8 val;
	val = cookie->dev->history[0].data;
	if (cookie->packet_index == 0
		&& (val & sFoundModel->maskFirstByte) != sFoundModel->firstByte) {
		INFO("ALPS: bad header, trying resync\n");
		cookie->packet_index = 0;
		return B_UNHANDLED_INTERRUPT;
	}

	// data packages starting with a 0
	if (cookie->packet_index > 1 && (val & 0x80)) {
		INFO("ALPS: bad package data, trying resync\n");
		cookie->packet_index = 0;
		return B_UNHANDLED_INTERRUPT;
	}

 	cookie->buffer[cookie->packet_index] = val;

	cookie->packet_index++;
	if (cookie->packet_index >= 6) {
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

	return B_HANDLED_INTERRUPT;
}


void
alps_disconnect(ps2_dev *dev)
{
	alps_cookie *cookie = (alps_cookie*)dev->cookie;
	// the mouse device might not be opened at this point
	INFO("ALPS: alps_disconnect %s\n", dev->name);
	if ((dev->flags & PS2_FLAG_OPEN) != 0)
		release_sem(cookie->sem);
}


device_hooks gALPSDeviceHooks = {
	alps_open,
	alps_close,
	alps_freecookie,
	alps_ioctl,
	alps_read,
	alps_write,
};
