/*
 * Copyright 2001-2005 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * PS/2 mouse device driver
 *
 * Authors (in chronological order):
 * 		Elad Lahav (elad@eldarshany.com)
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
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

#include "kb_mouse_driver.h"
#include "common.h"
#include "packet_buffer.h"


#define MOUSE_HISTORY_SIZE	256
	// we record that many mouse packets before we start to drop them

static sem_id sMouseSem;
static struct packet_buffer *sMouseBuffer;
static int32 sOpenMask;

static bigtime_t sLastClickTime;
static bigtime_t sClickSpeed;
static int32 sClickCount;
static int sButtonsState;

static size_t sPacketSize;
static size_t sPacketIndex;
static uint8 sPacketBuffer[PS2_MAX_PACKET_SIZE];



static status_t
ps2_reset_mouse()
{
	uint8 read;
	status_t status;
	
	TRACE(("ps2_reset_mouse()\n"));
	
	status = ps2_mouse_command(PS2_CMD_RESET_MOUSE, NULL, 0, &read, 1);
		
	TRACE(("reset mouse: status 0x%08x, data 0x%02x\n", status, read));
	
	return B_OK;	
}



/** Set sampling rate of the ps2 port.
 */
 
static status_t
ps2_set_sample_rate(uint32 rate)
{
	int32 tries = 5;

	while (--tries > 0) {
		uint8 d = rate;
		status_t status = ps2_mouse_command(PS2_CMD_SET_SAMPLE_RATE, &d, 1, NULL, 0);

		if (status == B_OK)
			return B_OK;
	}

	return B_ERROR;
}


/** Converts a packet received by the mouse to a "movement".
 */  
 
static void
ps2_packet_to_movement(uint8 packet[], mouse_movement *pos)
{
	int buttons = packet[0] & 7;
	int xDelta = ((packet[0] & 0x10) ? 0xFFFFFF00 : 0) | packet[1];
	int yDelta = ((packet[0] & 0x20) ? 0xFFFFFF00 : 0) | packet[2];
	bigtime_t currentTime = system_time();
	int8 wheel_ydelta = 0;
	int8 wheel_xdelta = 0;
	
	if (buttons != 0 && sButtonsState == 0) {
		if (sLastClickTime + sClickSpeed > currentTime)
			sClickCount++;
		else
			sClickCount = 1;
	}

	sLastClickTime = currentTime;
	sButtonsState = buttons;

	if (sPacketSize == PS2_PACKET_INTELLIMOUSE) { 
		switch (packet[3] & 0x0F) {
			case 0x01: wheel_ydelta = +1; break; // wheel 1 down
			case 0x0F: wheel_ydelta = -1; break; // wheel 1 up
			case 0x02: wheel_xdelta = +1; break; // wheel 2 down
			case 0x0E: wheel_xdelta = -1; break; // wheel 2 up
		}
	}

// 	dprintf("packet: %02x %02x %02x %02x: xd %d, yd %d, 0x%x (%d), w-xd %d, w-yd %d\n", 
//		packet[0], packet[1], packet[2], packet[3],
//		xDelta, yDelta, buttons, sClickCount, wheel_xdelta, wheel_ydelta);

	if (pos) {
		pos->xdelta = xDelta;
		pos->ydelta = yDelta;
		pos->buttons = buttons;
		pos->clicks = sClickCount;
		pos->modifiers = 0;
		pos->timestamp = currentTime;
		pos->wheel_ydelta = (int)wheel_ydelta;
		pos->wheel_xdelta = (int)wheel_xdelta;

		TRACE(("xdelta: %d, ydelta: %d, buttons %x, clicks: %ld, timestamp %Ld\n",
			xDelta, yDelta, buttons, sClickCount, currentTime));
	}
}


/** Read a mouse event from the mouse events chain buffer.
 */

static status_t
mouse_read_event(mouse_movement *userMovement)
{
	uint8 packet[PS2_MAX_PACKET_SIZE];
	mouse_movement movement;
	status_t status;

	TRACE(("mouse_read_event()\n"));
	status = acquire_sem_etc(sMouseSem, 1, B_CAN_INTERRUPT, 0);
	if (status < B_OK)
		return status;

	if (packet_buffer_read(sMouseBuffer, packet, sPacketSize) != sPacketSize) {
		TRACE(("error copying buffer\n"));
		return B_ERROR;
	}
	
	if (!(packet[0] & 8))
		panic("ps2_hid: got broken data from packet_buffer_read\n");

	ps2_packet_to_movement(packet, &movement);

	return user_memcpy(userMovement, &movement, sizeof(mouse_movement));
}


/** Enables or disables mouse reporting for the PS/2 port.
 */

static status_t
set_mouse_enabled(bool enable)
{
	int32 tries = 5;

	while (true) {
		if (ps2_mouse_command(enable ? PS2_CMD_ENABLE_MOUSE : PS2_CMD_DISABLE_MOUSE, NULL, 0, NULL, 0) == B_OK)
			return B_OK;

		if (--tries <= 0)
			break;
	}

	return B_ERROR;
}


/** Interrupt handler for the mouse device. Called whenever the I/O
 *	controller generates an interrupt for the PS/2 mouse. Reads mouse
 *	information from the data port, and stores it, so it can be accessed
 *	by read() operations. The full data is obtained using 3 consecutive
 *	calls to the handler, each holds a different byte on the data port.
 */

int32 mouse_handle_int(uint8 data)
{
	if (atomic_and(&sOpenMask, 1) == 0)
		return B_HANDLED_INTERRUPT;

	if (sPacketIndex == 0 && !(data & 8)) {
		TRACE(("bad mouse data, trying resync\n"));
		return B_HANDLED_INTERRUPT;
	}

	sPacketBuffer[sPacketIndex++] = data;

	if (sPacketIndex != sPacketSize) {
		// packet not yet complete
		return B_HANDLED_INTERRUPT;
	}

	// complete packet is assembled
	
	sPacketIndex = 0;
	
	if (packet_buffer_write(sMouseBuffer, sPacketBuffer, sPacketSize) != sPacketSize) {
		// buffer is full, drop new data
		return B_HANDLED_INTERRUPT;
	}

	release_sem_etc(sMouseSem, 1, B_DO_NOT_RESCHEDULE);
	return B_INVOKE_SCHEDULER;
}


//	#pragma mark -


status_t
probe_mouse(size_t *probed_packet_size)
{
	uint8 deviceId = 0;

	// get device id
	ps2_mouse_command(PS2_CMD_GET_DEVICE_ID, NULL, 0, &deviceId, 1);

	TRACE(("probe_mouse(): device id: %2x\n", deviceId));		

	if (deviceId == 0) {
		int32 tries = 5;

		while (--tries > 0) {
			// try to switch to intellimouse mode
			if (ps2_set_sample_rate(200) == B_OK
				&& ps2_set_sample_rate(100) == B_OK
				&& ps2_set_sample_rate(80) == B_OK) {
				// get device id, again
				ps2_mouse_command(PS2_CMD_GET_DEVICE_ID, NULL, 0, &deviceId, 1);
				break;
			}
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


status_t 
mouse_open(const char *name, uint32 flags, void **_cookie)
{
	status_t status;	
	int8 commandByte;

	TRACE(("mouse_open()\n"));	

	if (atomic_or(&sOpenMask, 1) != 0)
		return B_BUSY;

	acquire_sem(gDeviceOpenSemaphore);

	status = probe_mouse(&sPacketSize);
	if (status != B_OK)
		goto err1;
		
	sPacketIndex = 0;

	sMouseBuffer = create_packet_buffer(MOUSE_HISTORY_SIZE * sPacketSize);
	if (sMouseBuffer == NULL) {
		TRACE(("can't allocate mouse actions buffer\n"));
		status = B_NO_MEMORY;
		goto err2;
	}

	// create the mouse semaphore, used for synchronization between
	// the interrupt handler and the read operation
	sMouseSem = create_sem(0, "ps2_mouse_sem");
	if (sMouseSem < 0) {
		TRACE(("failed creating PS/2 mouse semaphore!\n"));
		status = sMouseSem;
		goto err3;
	}

	*_cookie = NULL;

	status = set_mouse_enabled(true);
	if (status < B_OK) {
		TRACE(("mouse_open(): cannot enable PS/2 mouse\n"));	
		goto err4;
	}

	release_sem(gDeviceOpenSemaphore);

	TRACE(("mouse_open(): mouse succesfully enabled\n"));
	return B_OK;

err4:
	delete_sem(sMouseSem);
err3:
	delete_packet_buffer(sMouseBuffer);
err2:
err1:
	atomic_and(&sOpenMask, 0);
	release_sem(gDeviceOpenSemaphore);

	return status;
}


status_t
mouse_close(void *cookie)
{
	TRACE(("mouse_close()\n"));

	set_mouse_enabled(false);

	delete_packet_buffer(sMouseBuffer);
	delete_sem(sMouseSem);

	atomic_and(&sOpenMask, 0);

	return B_OK;
}


status_t
mouse_freecookie(void *cookie)
{
	return B_OK;
}


status_t
mouse_read(void *cookie, off_t pos, void *buffer, size_t *_length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


status_t
mouse_write(void *cookie, off_t pos, const void *buffer, size_t *_length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


status_t
mouse_ioctl(void *cookie, uint32 op, void *buffer, size_t length)
{
	switch (op) {
		case MS_NUM_EVENTS:
		{
			int32 count;
			TRACE(("MS_NUM_EVENTS\n"));
			get_sem_count(sMouseSem, &count);
			return count;
		}

		case MS_READ:
			TRACE(("MS_READ\n"));	
			return mouse_read_event((mouse_movement *)buffer);

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
			return user_memcpy(&sClickSpeed, buffer, sizeof(bigtime_t));

		default:
			TRACE(("unknown opcode: %ld\n", op));
			return B_BAD_VALUE;
	}
}

