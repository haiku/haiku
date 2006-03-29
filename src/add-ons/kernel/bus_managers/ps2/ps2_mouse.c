/*
 * Copyright 2001-2006 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * PS/2 mouse device driver
 *
 * Authors (in chronological order):
 * 		Elad Lahav (elad@eldarshany.com)
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 *      Marcus Overhagen <marcus@overhagen.de>
 */

/*
 * A PS/2 mouse is connected to the IBM 8042 controller, and gets its
 * name from the IBM PS/2 personal computer, which was the first to
 * use this device. All resources are shared between the keyboard, and
 * the mouse, referred to as the "Auxiliary Device".
 *
 * I/O:
 * ~~~
 * The controller has 3 I/O registers:
 * 1. Status (input), mapped to port 64h
 * 2. Control (output), mapped to port 64h
 * 3. Data (input/output), mapped to port 60h
 *
 * Data:
 * ~~~~
 * A packet read from the mouse data port is composed of
 * three bytes:
 * byte 0: status byte, where
 * - bit 7: Y overflow (1 = true)
 * - bit 6: X overflow (1 = true)
 * - bit 5: MSB of Y offset
 * - bit 4: MSB of X offset
 * - bit 3: Syncronization bit (always 1)
 * - bit 2: Middle button (1 = down)
 * - bit 1: Right button (1 = down)
 * - bit 0: Left button (1 = down)
 * byte 1: X position change, since last probed (-127 to +127)
 * byte 2: Y position change, since last probed (-127 to +127)
 * 
 * Intellimouse mice send a four byte packet, where the first three
 * bytes are the same as standard mice, and the last one reports the
 * Z position, which is, usually, the wheel movement.
 *
 * Interrupts:
 * ~~~~~~~~~~
 * The PS/2 mouse device is connected to interrupt 12, which means that
 * it uses the second interrupt controller (handles INT8 to INT15). In
 * order for this interrupt to be enabled, both the 5th interrupt of
 * the second controller AND the 3rd interrupt of the first controller
 * (cascade mode) should be unmasked.
 * This is all done inside install_io_interrupt_handler(), no need to
 * worry about it anymore
 * The controller uses 3 consecutive interrupts to inform the computer
 * that it has new data. On the first the data register holds the status
 * byte, on the second the X offset, and on the 3rd the Y offset.
 */

#include <Drivers.h>
#include <string.h>
#include <malloc.h>

#include "kb_mouse_driver.h"
#include "packet_buffer.h"
#include "ps2_service.h"


#define MOUSE_HISTORY_SIZE	256
	// we record that many mouse packets before we start to drop them

typedef struct
{
	ps2_dev *		dev;
	
	sem_id			mouse_sem;
	packet_buffer *	mouse_buffer;
	bigtime_t		click_last_time;
	bigtime_t		click_speed;
	int				click_count;
	int				buttons_state;
	
	size_t			packet_size;
	size_t			packet_index;
	uint8			packet_buffer[PS2_MAX_PACKET_SIZE];

} mouse_cookie;


static status_t
ps2_reset_mouse(mouse_cookie *cookie)
{
	uint8 data[2];
	status_t status;
	
	TRACE(("ps2_reset_mouse()\n"));
	
	status = ps2_dev_command(cookie->dev, PS2_CMD_RESET, NULL, 0, data, 2);
		
	if (status == B_OK && data[0] != 0xAA && data[1] != 0x00) {
		TRACE(("reset mouse failed, response was: 0x%02x 0x%02x\n", data[0], data[1]));
		status = B_ERROR;
	} else if (status != B_OK) {
		TRACE(("reset mouse failed\n"));
	} else {
		TRACE(("reset mouse success\n"));
	}
	
	return status;
}


/** Set sampling rate of the ps2 port.
 */
static inline status_t
ps2_set_sample_rate(mouse_cookie *cookie, uint8 rate)
{
	return ps2_dev_command(cookie->dev, PS2_CMD_SET_SAMPLE_RATE, &rate, 1, NULL, 0);
}


/** Converts a packet received by the mouse to a "movement".
 */
static void
ps2_packet_to_movement(mouse_cookie *cookie, uint8 packet[], mouse_movement *pos)
{
	int buttons = packet[0] & 7;
	int xDelta = ((packet[0] & 0x10) ? 0xFFFFFF00 : 0) | packet[1];
	int yDelta = ((packet[0] & 0x20) ? 0xFFFFFF00 : 0) | packet[2];
	bigtime_t currentTime = system_time();
	int8 wheel_ydelta = 0;
	int8 wheel_xdelta = 0;
	
	if (buttons != 0 && cookie->buttons_state == 0) {
		if (cookie->click_last_time + cookie->click_speed > currentTime)
			cookie->click_count++;
		else
			cookie->click_count = 1;
	}

	cookie->click_last_time = currentTime;
	cookie->buttons_state = buttons;

	if (cookie->packet_size == PS2_PACKET_INTELLIMOUSE) { 
		switch (packet[3] & 0x0F) {
			case 0x01: wheel_ydelta = +1; break; // wheel 1 down
			case 0x0F: wheel_ydelta = -1; break; // wheel 1 up
			case 0x02: wheel_xdelta = +1; break; // wheel 2 down
			case 0x0E: wheel_xdelta = -1; break; // wheel 2 up
		}
	}

// 	dprintf("packet: %02x %02x %02x %02x: xd %d, yd %d, 0x%x (%d), w-xd %d, w-yd %d\n", 
//		packet[0], packet[1], packet[2], packet[3],
//		xDelta, yDelta, buttons, cookie->click_count, wheel_xdelta, wheel_ydelta);

	if (pos) {
		pos->xdelta = xDelta;
		pos->ydelta = yDelta;
		pos->buttons = buttons;
		pos->clicks = cookie->click_count;
		pos->modifiers = 0;
		pos->timestamp = currentTime;
		pos->wheel_ydelta = (int)wheel_ydelta;
		pos->wheel_xdelta = (int)wheel_xdelta;

		TRACE(("xdelta: %d, ydelta: %d, buttons %x, clicks: %ld, timestamp %Ld\n",
			xDelta, yDelta, buttons, cookie->click_count, currentTime));
	}
}


/** Read a mouse event from the mouse events chain buffer.
 */
static status_t
mouse_read_event(mouse_cookie *cookie, mouse_movement *movement)
{
	uint8 packet[PS2_MAX_PACKET_SIZE];
	status_t status;

	TRACE(("mouse_read_event()\n"));
	status = acquire_sem_etc(cookie->mouse_sem, 1, B_CAN_INTERRUPT, 0);
	if (status < B_OK)
		return status;

	if (!cookie->dev->active) {
		dprintf("ps2: mouse_read_event: Error device no longer active\n");
		return B_ERROR;
	}		

	if (packet_buffer_read(cookie->mouse_buffer, packet, cookie->packet_size) != cookie->packet_size) {
		TRACE(("error copying buffer\n"));
		return B_ERROR;
	}
	
	if (!(packet[0] & 8))
		panic("ps2_hid: got broken data from packet_buffer_read\n");

	ps2_packet_to_movement(cookie, packet, movement);
	return B_OK;
}


static void
ps2_mouse_disconnect(ps2_dev *dev)
{
	// the mouse device might not be opened at this point
	dprintf("ps2: ps2_mouse_disconnect %s\n", dev->name);
	if (dev->flags & PS2_FLAG_OPEN)
		release_sem(((mouse_cookie *)dev->cookie)->mouse_sem);
}


/** Interrupt handler for the mouse device. Called whenever the I/O
 *	controller generates an interrupt for the PS/2 mouse. Reads mouse
 *	information from the data port, and stores it, so it can be accessed
 *	by read() operations. The full data is obtained using 3 consecutive
 *	calls to the handler, each holds a different byte on the data port.
 */
int32
mouse_handle_int(ps2_dev *dev, uint8 data)
{
	mouse_cookie *cookie = dev->cookie;
	
	if (cookie->packet_index == 0 && !(data & 8)) {
		TRACE(("bad mouse data, trying resync\n"));
		return B_HANDLED_INTERRUPT;
	}

	cookie->packet_buffer[cookie->packet_index++] = data;

	if (cookie->packet_index != cookie->packet_size) {
		// packet not yet complete
		return B_HANDLED_INTERRUPT;
	}

	// complete packet is assembled
	
	cookie->packet_index = 0;
	
	if (packet_buffer_write(cookie->mouse_buffer, cookie->packet_buffer, cookie->packet_size) != cookie->packet_size) {
		// buffer is full, drop new data
		return B_HANDLED_INTERRUPT;
	}

	release_sem_etc(cookie->mouse_sem, 1, B_DO_NOT_RESCHEDULE);
	return B_INVOKE_SCHEDULER;
}


//	#pragma mark -


static status_t
probe_mouse(mouse_cookie *cookie, size_t *probed_packet_size)
{
	status_t status;
	uint8 deviceId = 0;
	
	
	status = ps2_reset_mouse(cookie);

	// get device id
	status = ps2_dev_command(cookie->dev, PS2_CMD_GET_DEVICE_ID, NULL, 0, &deviceId, 1);
	if (status != B_OK) {
		TRACE(("probe_mouse(): get device id failed\n"));
		return B_ERROR;
	}

	TRACE(("probe_mouse(): device id: %2x\n", deviceId));		

	// check for MS Intellimouse
	if (deviceId == 0) {
		uint8 alternate_device_id;
		status  = ps2_set_sample_rate(cookie, 200);
		status |= ps2_set_sample_rate(cookie, 100);
		status |= ps2_set_sample_rate(cookie, 80);
		status |= ps2_dev_command(cookie->dev, PS2_CMD_GET_DEVICE_ID, NULL, 0, &alternate_device_id, 1);
		if (status == 0) {
			TRACE(("probe_mouse(): alternate device id: %2x\n", alternate_device_id));		
			deviceId = alternate_device_id;
		}
	}

	if (deviceId == PS2_DEV_ID_STANDARD) {
		TRACE(("Standard PS/2 mouse found\n"));
		if (probed_packet_size)
			*probed_packet_size = PS2_PACKET_STANDARD;
	} else if (deviceId == PS2_DEV_ID_INTELLIMOUSE) {
		TRACE(("Extended PS/2 mouse found\n"));
		if (probed_packet_size)
			*probed_packet_size = PS2_PACKET_INTELLIMOUSE;
	} else {
		// Something's wrong. Better quit
		return B_ERROR;
	}

	return B_OK;
}


//	#pragma mark -
//	Device functions


static status_t 
mouse_open(const char *name, uint32 flags, void **_cookie)
{
	mouse_cookie *cookie;
	ps2_dev *dev;
	status_t status;
	int i;
	
	dprintf("ps2: mouse_open %s\n", name);

	for (dev = NULL, i = 0; i < PS2_DEVICE_COUNT; i++) {
		if (0 == strcmp(ps2_device[i].name, name)) {
			dev = &ps2_device[i];
			break;
		}
	}
	
	if (dev == NULL) {
		TRACE(("dev = NULL\n"));
		return B_ERROR;
	}
	
	if (atomic_or(&dev->flags, PS2_FLAG_OPEN) & PS2_FLAG_OPEN)
		return B_BUSY;
		
	cookie = malloc(sizeof(mouse_cookie));
	if (cookie == NULL)
		goto err1;
		
	*_cookie = cookie;
	memset(cookie, 0, sizeof(*cookie));

	cookie->dev = dev;
	dev->cookie = cookie;
	dev->disconnect = &ps2_mouse_disconnect;
		
	status = probe_mouse(cookie, &cookie->packet_size);
	if (status != B_OK) {
		TRACE(("probing mouse failed\n"));
		ps2_service_notify_device_removed(dev);
		goto err1;
	}

	cookie->mouse_buffer = create_packet_buffer(MOUSE_HISTORY_SIZE * cookie->packet_size);
	if (cookie->mouse_buffer == NULL) {
		TRACE(("can't allocate mouse actions buffer\n"));
		goto err2;
	}

	// create the mouse semaphore, used for synchronization between
	// the interrupt handler and the read operation
	cookie->mouse_sem = create_sem(0, "ps2_mouse_sem");
	if (cookie->mouse_sem < 0) {
		TRACE(("failed creating PS/2 mouse semaphore!\n"));
		goto err3;
	}

	status = ps2_dev_command(dev, PS2_CMD_ENABLE, NULL, 0, NULL, 0);
	if (status < B_OK) {
		TRACE(("mouse_open(): cannot enable PS/2 mouse\n"));	
		goto err4;
	}

	atomic_or(&dev->flags, PS2_FLAG_ENABLED);

	dprintf("ps2: mouse_open %s success\n", name);
	return B_OK;

err4:
	delete_sem(cookie->mouse_sem);
err3:
	delete_packet_buffer(cookie->mouse_buffer);
err2:
	free(cookie);
err1:
	atomic_and(&dev->flags, ~PS2_FLAG_OPEN);
	
	dprintf("ps2: mouse_open %s failed\n", name);
	return B_ERROR;
}


static status_t
mouse_close(void *_cookie)
{
	mouse_cookie *cookie = _cookie;

	dprintf("ps2: mouse_close %s\n", cookie->dev->name);

	ps2_dev_command(cookie->dev, PS2_CMD_DISABLE, NULL, 0, NULL, 0);

	delete_packet_buffer(cookie->mouse_buffer);
	delete_sem(cookie->mouse_sem);

	atomic_and(&cookie->dev->flags, ~PS2_FLAG_OPEN);
	atomic_and(&cookie->dev->flags, ~PS2_FLAG_ENABLED);

	return B_OK;
}


static status_t
mouse_freecookie(void *_cookie)
{
	mouse_cookie *cookie = _cookie;
	free(cookie);
	return B_OK;
}


static status_t
mouse_read(void *cookie, off_t pos, void *buffer, size_t *_length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


static status_t
mouse_write(void *cookie, off_t pos, const void *buffer, size_t *_length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


static status_t
mouse_ioctl(void *_cookie, uint32 op, void *buffer, size_t length)
{
	mouse_cookie *cookie = _cookie;
	
	switch (op) {
		case MS_NUM_EVENTS:
		{
			int32 count;
			TRACE(("MS_NUM_EVENTS\n"));
			get_sem_count(cookie->mouse_sem, &count);
			return count;
		}

		case MS_READ:
		{
			mouse_movement movement;
			status_t status;
			TRACE(("MS_READ\n"));
			if ((status = mouse_read_event(cookie, &movement)) < B_OK)
				return status;
			return user_memcpy(buffer, &movement, sizeof(movement));
		}

		case MS_SET_TYPE:
			TRACE(("MS_SET_TYPE not implemented\n"));
			return B_BAD_VALUE;

		case MS_SET_MAP:
			TRACE(("MS_SET_MAP (set mouse mapping) not implemented\n"));
			return B_BAD_VALUE;

		case MS_GET_ACCEL:
			TRACE(("MS_GET_ACCEL (get mouse acceleration) not implemented\n"));
			return B_BAD_VALUE;

		case MS_SET_ACCEL:
			TRACE(("MS_SET_ACCEL (set mouse acceleration) not implemented\n"));
			return B_BAD_VALUE;

		case MS_SET_CLICKSPEED:
			TRACE(("MS_SETCLICK (set click speed)\n"));
			return user_memcpy(&cookie->click_speed, buffer, sizeof(bigtime_t));

		default:
			TRACE(("unknown opcode: %ld\n", op));
			return B_BAD_VALUE;
	}
}

device_hooks gMouseDeviceHooks = {
	mouse_open,
	mouse_close,
	mouse_freecookie,
	mouse_ioctl,
	mouse_read,
	mouse_write,
};
