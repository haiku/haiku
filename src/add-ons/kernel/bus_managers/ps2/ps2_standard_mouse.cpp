/*
 * Copyright 2001-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors (in chronological order):
 *		Elad Lahav, elad@eldarshany.com
 *		Stefano Ceccherini, burton666@libero.it
 *		Axel Dörfler, axeld@pinc-software.de
 *		Marcus Overhagen, marcus@overhagen.de
 *		Clemens Zeidler, czeidler@gmx.de
 *		John Scipione, jscipione@gmail.com
 */


/*!	PS/2 mouse device driver

	A PS/2 mouse is connected to the IBM 8042 controller, and gets its
	name from the IBM PS/2 personal computer, which was the first to
	use this device. All resources are shared between the keyboard, and
	the mouse, referred to as the "Auxiliary Device".

	I/O:
	~~~
	The controller has 3 I/O registers:
	1. Status (input), mapped to port 64h
	2. Control (output), mapped to port 64h
	3. Data (input/output), mapped to port 60h

	Data:
	~~~~
	A packet read from the mouse data port is composed of
	three bytes:
		byte 0: status byte, where
			- bit 7: Y overflow (1 = true)
			- bit 6: X overflow (1 = true)
			- bit 5: MSB of Y offset
			- bit 4: MSB of X offset
			- bit 3: Syncronization bit (always 1)
			- bit 2: Middle button (1 = down)
			- bit 1: Right button (1 = down)
			- bit 0: Left button (1 = down)
		byte 1: X position change, since last probed (-127 to +127)
		byte 2: Y position change, since last probed (-127 to +127)

	Intellimouse mice send a four byte packet, where the first three
	bytes are the same as standard mice, and the last one reports the
	Z position, which is, usually, the wheel movement.

	Interrupts:
	~~~~~~~~~~
	The PS/2 mouse device is connected to interrupt 12.
	The controller uses 3 consecutive interrupts to inform the computer
	that it has new data. On the first the data register holds the status
	byte, on the second the X offset, and on the 3rd the Y offset.
*/


#include <stdlib.h>
#include <string.h>

#include <keyboard_mouse_driver.h>

#include "ps2_service.h"
#include "ps2_standard_mouse.h"


//#define TRACE_PS2_MOUSE
#ifdef TRACE_PS2_MOUSE
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...)
#endif


const char* kStandardMousePath[4] = {
	"input/mouse/ps2/standard_0",
	"input/mouse/ps2/standard_1",
	"input/mouse/ps2/standard_2",
	"input/mouse/ps2/standard_3"
};

const char* kIntelliMousePath[4] = {
	"input/mouse/ps2/intelli_0",
	"input/mouse/ps2/intelli_1",
	"input/mouse/ps2/intelli_2",
	"input/mouse/ps2/intelli_3"
};


//!	Set sampling rate of the ps2 port.
static inline status_t
ps2_set_sample_rate(ps2_dev* dev, uint8 rate)
{
	return ps2_dev_command(dev, PS2_CMD_SET_SAMPLE_RATE, &rate, 1, NULL, 0);
}


//!	Converts a packet received by the mouse to a "movement".
static void
ps2_packet_to_movement(standard_mouse_cookie* cookie, uint8 packet[],
	mouse_movement* pos)
{
	int buttons = packet[0] & 7;
	int xDelta = ((packet[0] & 0x10) ? ~0xff : 0) | packet[1];
	int yDelta = ((packet[0] & 0x20) ? ~0xff : 0) | packet[2];
	int xDeltaWheel = 0;
	int yDeltaWheel = 0;
	bigtime_t currentTime = system_time();

	if (buttons != 0 && cookie->buttons_state == 0) {
		if (cookie->click_last_time + cookie->click_speed > currentTime)
			cookie->click_count++;
		else
			cookie->click_count = 1;

		cookie->click_last_time = currentTime;
	}

	cookie->buttons_state = buttons;

	if (cookie->flags & F_MOUSE_TYPE_INTELLIMOUSE) {
		yDeltaWheel = packet[3] & 0x07;
 		if (packet[3] & 0x08)
			yDeltaWheel |= ~0x07;
	}
#if 0
	if (cookie->flags & F_MOUSE_TYPE_2WHEELS) {
		switch (packet[3] & 0x0F) {
			case 0x01: yDeltaWheel = +1; break; // wheel 1 down
			case 0x0F: yDeltaWheel = -1; break; // wheel 1 up
			case 0x02: xDeltaWheel = +1; break; // wheel 2 down
			case 0x0E: xDeltaWheel = -1; break; // wheel 2 up
		}
	}
#endif

#if 0
	TRACE("packet: %02x %02x %02x %02x: xd %d, yd %d, 0x%x (%d), w-xd %d, "
		"w-yd %d\n", packet[0], packet[1], packet[2], packet[3],
		xDelta, yDelta, buttons, cookie->click_count, xDeltaWheel,
		yDeltaWheel);
#endif

	if (pos != NULL) {
		pos->xdelta = xDelta;
		pos->ydelta = yDelta;
		pos->buttons = buttons;
		pos->clicks = cookie->click_count;
		pos->modifiers = 0;
		pos->timestamp = currentTime;
		pos->wheel_ydelta = yDeltaWheel;
		pos->wheel_xdelta = xDeltaWheel;

		TRACE("ps2: ps2_packet_to_movement xdelta: %d, ydelta: %d, buttons %x, "
			"clicks: %d, timestamp %" B_PRIdBIGTIME "\n",
			xDelta, yDelta, buttons, cookie->click_count, currentTime);
	}
}


//!	Read a mouse event from the mouse events chain buffer.
static status_t
standard_mouse_read_event(standard_mouse_cookie* cookie,
	mouse_movement* movement)
{
	uint8 packet[PS2_MAX_PACKET_SIZE];
	status_t status;

	TRACE("ps2: standard_mouse_read_event\n");

	status = acquire_sem_etc(cookie->standard_mouse_sem, 1, B_CAN_INTERRUPT, 0);
	TRACE("ps2: standard_mouse_read_event acquired\n");
	if (status < B_OK)
		return status;

	if (!cookie->dev->active) {
		TRACE("ps2: standard_mouse_read_event: Error device no longer "
			"active\n");
		return B_ERROR;
	}

	if (packet_buffer_read(cookie->standard_mouse_buffer, packet,
		cookie->dev->packet_size) != cookie->dev->packet_size) {
		TRACE("ps2: error copying buffer\n");
		return B_ERROR;
	}

	if (!(packet[0] & 8))
		panic("ps2: got broken data from packet_buffer_read\n");

	ps2_packet_to_movement(cookie, packet, movement);
	return B_OK;
}


//	#pragma mark - Interrupt handler functions


void
standard_mouse_disconnect(ps2_dev* dev)
{
	// the mouse device might not be opened at this point
	INFO("ps2: ps2_standard_mouse_disconnect %s\n", dev->name);
	if (dev->flags & PS2_FLAG_OPEN)
		release_sem(((standard_mouse_cookie*)dev->cookie)->standard_mouse_sem);
}


/*!	Interrupt handler for the mouse device. Called whenever the I/O
	controller generates an interrupt for the PS/2 mouse. Reads mouse
	information from the data port, and stores it, so it can be accessed
	by read() operations. The full data is obtained using 3 consecutive
	calls to the handler, each holds a different byte on the data port.
*/
int32
standard_mouse_handle_int(ps2_dev* dev)
{
	standard_mouse_cookie* cookie = (standard_mouse_cookie*)dev->cookie;
	const uint8 data = dev->history[0].data;

	TRACE("ps2: standard mouse: %1x\t%1x\t%1x\t%1x\t%1x\t%1x\t%1x\t%1x\n",
		data >> 7 & 1, data >> 6 & 1, data >> 5 & 1,
		data >> 4 & 1, data >> 3 & 1, data >> 2 & 1,
		data >> 1 & 1, data >> 0 & 1);

	if (cookie->packet_index == 0 && (data & 8) == 0) {
		INFO("ps2: bad mouse data, trying resync\n");
		return B_HANDLED_INTERRUPT;
	}

	// Workarounds for active multiplexing keyboard controllers
	// that lose data or send them to the wrong port.
	if (cookie->packet_index == 0 && (data & 0xc0) != 0) {
		INFO("ps2: strange mouse data, x/y overflow, trying resync\n");
		return B_HANDLED_INTERRUPT;
	}

	cookie->buffer[cookie->packet_index++] = data;

	if (cookie->packet_index != dev->packet_size) {
		// packet not yet complete
		return B_HANDLED_INTERRUPT;
	}

	// complete packet is assembled

	cookie->packet_index = 0;
	if (packet_buffer_write(cookie->standard_mouse_buffer,
		cookie->buffer, dev->packet_size) != dev->packet_size) {
		// buffer is full, drop new data
		return B_HANDLED_INTERRUPT;
	}

	release_sem_etc(cookie->standard_mouse_sem, 1, B_DO_NOT_RESCHEDULE);

	return B_INVOKE_SCHEDULER;
}


//	#pragma mark - probe_standard_mouse()


status_t
probe_standard_mouse(ps2_dev* dev)
{
	status_t status;
	uint8 deviceId = 0;

	// get device id
	status = ps2_dev_command(dev, PS2_CMD_GET_DEVICE_ID, NULL, 0,
		&deviceId, 1);
	if (status != B_OK) {
		INFO("ps2: probe_mouse get device id failed\n");
		return B_ERROR;
	}

	TRACE("ps2: probe_mouse device id: %2x\n", deviceId);

	// check for MS Intellimouse
	if (deviceId == 0) {
		uint8 alternate_device_id;
		status  = ps2_set_sample_rate(dev, 200);
		status |= ps2_set_sample_rate(dev, 100);
		status |= ps2_set_sample_rate(dev, 80);
		status |= ps2_dev_command(dev, PS2_CMD_GET_DEVICE_ID, NULL, 0,
			&alternate_device_id, 1);
		if (status == 0) {
			TRACE("ps2: probe_mouse alternate device id: %2x\n",
				alternate_device_id);
			deviceId = alternate_device_id;
		}
	}

	if (deviceId == PS2_DEV_ID_STANDARD
		|| deviceId == PS2_DEV_ID_TOUCHPAD_RICATECH) {
		INFO("ps2: probe_mouse Standard PS/2 mouse found\n");
		dev->name = kStandardMousePath[dev->idx];
		dev->packet_size = PS2_PACKET_STANDARD;
	} else if (deviceId == PS2_DEV_ID_INTELLIMOUSE) {
		dev->name = kIntelliMousePath[dev->idx];
		dev->packet_size = PS2_PACKET_INTELLIMOUSE;
		INFO("ps2: probe_mouse Extended PS/2 mouse found\n");
	} else {
		INFO("ps2: probe_mouse Error unknown device id.\n");
		return B_ERROR;
	}

	return B_OK;
}


//	#pragma mark - Device functions


status_t
standard_mouse_open(const char* name, uint32 flags, void** _cookie)
{
	standard_mouse_cookie* cookie;
	status_t status;
	ps2_dev* dev = NULL;
	int i;

	TRACE("ps2: standard_mouse_open %s\n", name);

	for (dev = NULL, i = 0; i < PS2_DEVICE_COUNT; i++) {
		if (0 == strcmp(ps2_device[i].name, name)) {
			dev = &ps2_device[i];
			break;
		}
#if 0
		if (0 == strcmp(g_passthrough_dev.name, name)) {
			dev = &g_passthrough_dev;
			isSynapticsPTDevice = true;
			break;
		}
#endif
	}

	if (dev == NULL) {
		TRACE("ps2: dev = NULL\n");
		return B_ERROR;
	}

	if (atomic_or(&dev->flags, PS2_FLAG_OPEN) & PS2_FLAG_OPEN)
		return B_BUSY;

	cookie = (standard_mouse_cookie*)malloc(sizeof(standard_mouse_cookie));
	if (cookie == NULL)
		goto err1;

	*_cookie = cookie;
	memset(cookie, 0, sizeof(*cookie));

	cookie->dev = dev;
	dev->cookie = cookie;
	dev->disconnect = &standard_mouse_disconnect;
	dev->handle_int = &standard_mouse_handle_int;

	if (strstr(dev->name, "standard") != NULL)
		cookie->flags = F_MOUSE_TYPE_STANDARD;

	if (strstr(dev->name, "intelli") != NULL)
		cookie->flags = F_MOUSE_TYPE_INTELLIMOUSE;

	cookie->standard_mouse_buffer
		= create_packet_buffer(MOUSE_HISTORY_SIZE * dev->packet_size);
	if (cookie->standard_mouse_buffer == NULL) {
		TRACE("ps2: can't allocate mouse actions buffer\n");
		goto err2;
	}

	// create the mouse semaphore, used for synchronization between
	// the interrupt handler and the read operation
	cookie->standard_mouse_sem = create_sem(0, "ps2_standard_mouse_sem");
	if (cookie->standard_mouse_sem < 0) {
		TRACE("ps2: failed creating PS/2 mouse semaphore!\n");
		goto err3;
	}

	status = ps2_dev_command(dev, PS2_CMD_ENABLE, NULL, 0, NULL, 0);
	if (status < B_OK) {
		INFO("ps2: cannot enable mouse %s\n", name);
		goto err4;
	}

	atomic_or(&dev->flags, PS2_FLAG_ENABLED);


	TRACE("ps2: standard_mouse_open %s success\n", name);
	return B_OK;

err4:
	delete_sem(cookie->standard_mouse_sem);
err3:
	delete_packet_buffer(cookie->standard_mouse_buffer);
err2:
	free(cookie);
err1:
	atomic_and(&dev->flags, ~PS2_FLAG_OPEN);

	TRACE("ps2: standard_mouse_open %s failed\n", name);
	return B_ERROR;
}


status_t
standard_mouse_close(void* _cookie)
{
	standard_mouse_cookie* cookie = (standard_mouse_cookie*)_cookie;

	TRACE("ps2: standard_mouse_close %s enter\n", cookie->dev->name);

	ps2_dev_command_timeout(cookie->dev, PS2_CMD_DISABLE, NULL, 0, NULL, 0,
		150000);

	delete_packet_buffer(cookie->standard_mouse_buffer);
	delete_sem(cookie->standard_mouse_sem);

	atomic_and(&cookie->dev->flags, ~PS2_FLAG_OPEN);
	atomic_and(&cookie->dev->flags, ~PS2_FLAG_ENABLED);

	TRACE("ps2: standard_mouse_close %s done\n", cookie->dev->name);
	return B_OK;
}


status_t
standard_mouse_freecookie(void* _cookie)
{
	standard_mouse_cookie* cookie = (standard_mouse_cookie*)_cookie;
	free(cookie);
	return B_OK;
}


static status_t
standard_mouse_read(void* cookie, off_t pos, void* buffer, size_t* _length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


static status_t
standard_mouse_write(void* cookie, off_t pos, const void* buffer,
	size_t* _length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


status_t
standard_mouse_ioctl(void* _cookie, uint32 op, void* buffer, size_t length)
{
	standard_mouse_cookie* cookie = (standard_mouse_cookie*)_cookie;

	switch (op) {
		case MS_NUM_EVENTS:
		{
			int32 count;
			TRACE("ps2: ioctl MS_NUM_EVENTS\n");
			get_sem_count(cookie->standard_mouse_sem, &count);
			return count;
		}

		case MS_READ:
		{
			mouse_movement movement;
			status_t status;
			TRACE("ps2: ioctl MS_READ\n");
			if ((status = standard_mouse_read_event(cookie, &movement)) < B_OK)
				return status;
//			TRACE("%s %d %d %d %d\n", cookie->dev->name,
//				movement.xdelta, movement.ydelta, movement.buttons,
//				movement.clicks);
			return user_memcpy(buffer, &movement, sizeof(movement));
		}

		case MS_SET_TYPE:
			TRACE("ps2: ioctl MS_SET_TYPE not implemented\n");
			return B_BAD_VALUE;

		case MS_SET_MAP:
			TRACE("ps2: ioctl MS_SET_MAP (set mouse mapping) not "
				"implemented\n");
			return B_BAD_VALUE;

		case MS_GET_ACCEL:
			TRACE("ps2: ioctl MS_GET_ACCEL (get mouse acceleration) not "
				"implemented\n");
			return B_BAD_VALUE;

		case MS_SET_ACCEL:
			TRACE("ps2: ioctl MS_SET_ACCEL (set mouse acceleration) not "
				"implemented\n");
			return B_BAD_VALUE;

		case MS_SET_CLICKSPEED:
			TRACE("ps2: ioctl MS_SETCLICK (set click speed)\n");
			return user_memcpy(&cookie->click_speed, buffer, sizeof(bigtime_t));

		default:
			TRACE("ps2: ioctl unknown mouse opcode: %" B_PRIu32 "\n", op);
			return B_DEV_INVALID_IOCTL;
	}
}


device_hooks gStandardMouseDeviceHooks = {
	standard_mouse_open,
	standard_mouse_close,
	standard_mouse_freecookie,
	standard_mouse_ioctl,
	standard_mouse_read,
	standard_mouse_write,
};
