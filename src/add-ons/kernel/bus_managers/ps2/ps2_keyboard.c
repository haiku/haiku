/*
 * Copyright 2004-2005 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * PS/2 keyboard device driver
 *
 * Authors (in chronological order):
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "ps2_common.h"
#include "kb_mouse_driver.h"
#include "packet_buffer.h"

#include <string.h>


#define KEY_BUFFER_SIZE 100
	// we will buffer 100 key strokes before we start dropping them

enum {
	LED_SCROLL 	= 1,
	LED_NUM 	= 2,
	LED_CAPS	= 4
} leds_status;

enum {
	PS2_DATA_SET_LEDS		= 0xed,
	PS2_ENABLE_KEYBOARD		= 0xf4,
	PS2_DISABLE_KEYBOARD	= 0xf5,

	EXTENDED_KEY = 0xe0,
};


static int32 sKeyboardOpenMask;
static sem_id sKeyboardSem;
static struct packet_buffer *sKeyBuffer;
static bool sIsExtended = false;


static status_t
set_leds(led_info *ledInfo)
{
	uint8 leds = 0;

	TRACE(("ps2_hid: set keyboard LEDs\n"));

	if (ledInfo->scroll_lock)
		leds |= LED_SCROLL;
	if (ledInfo->num_lock)
		leds |= LED_NUM;
	if (ledInfo->caps_lock)
		leds |= LED_CAPS;

	return ps2_keyboard_command(PS2_DATA_SET_LEDS, &leds, 1, NULL, 0);
}


int32 keyboard_handle_int(uint8 data)
{
	at_kbd_io keyInfo;
	uint8 scancode;

	if (atomic_and(&sKeyboardOpenMask, 1) == 0)
		return B_HANDLED_INTERRUPT;

	// TODO: Handle braindead "pause" key special case

	if (data == EXTENDED_KEY) {
		sIsExtended = true;
		TRACE(("Extended key\n"));
		return B_HANDLED_INTERRUPT;
	} 

	scancode = data;

	TRACE(("scancode: %x\n", scancode));

	// For now, F12 enters the kernel debugger
	// ToDo: remove me later :-)
	if (scancode == 88)
		panic("keyboard requested halt.\n");

	if (scancode & 0x80) {
		keyInfo.is_keydown = false;
		scancode -= 0x80;	
	} else
		keyInfo.is_keydown = true;

	if (sIsExtended) {
		scancode |= 0x80;
		sIsExtended = false;
	}

	keyInfo.timestamp = system_time();
	keyInfo.scancode = scancode;

	if (packet_buffer_write(sKeyBuffer, (uint8 *)&keyInfo, sizeof(keyInfo)) == 0) {
		// If there is no space left in the buffer, we drop this key stroke. We avoid
		// dropping old key strokes, to not destroy what already was typed.
		return B_HANDLED_INTERRUPT;
	}

	release_sem_etc(sKeyboardSem, 1, B_DO_NOT_RESCHEDULE);

	return B_INVOKE_SCHEDULER;
}


static status_t
read_keyboard_packet(at_kbd_io *userBuffer)
{
	at_kbd_io packet;
	status_t status;

	TRACE(("read_keyboard_packet()\n"));

	status = acquire_sem_etc(sKeyboardSem, 1, B_CAN_INTERRUPT, 0);
	if (status < B_OK)
		return status;

	if (packet_buffer_read(sKeyBuffer, (uint8 *)&packet, sizeof(at_kbd_io)) == 0) {
		TRACE(("read_keyboard_packet(): error reading packet: %s\n", strerror(status)));
		return B_ERROR;
	}

	TRACE(("scancode: %x, keydown: %s\n", packet.scancode, packet.is_keydown ? "true" : "false"));

	return user_memcpy(userBuffer, &packet, sizeof(at_kbd_io));
}


static status_t
enable_keyboard(void)
{
	uint32 tries = 3;

	while (tries-- > 0) {
//		keyboard_empty_data();

		if (ps2_keyboard_command(PS2_ENABLE_KEYBOARD, NULL, 0, NULL, 0) == B_OK)
			return B_OK;
	}

	dprintf("enable_keyboard() failed\n");	
	return B_ERROR;
}


//	#pragma mark -


status_t
probe_keyboard(void)
{
	int32 tries;

	// ToDo: for now there just is a keyboard ready to be used...
	
	// Keyboard detection does not seem to be working always correctly

	// not sure if this is the correct way
	ps2_keyboard_command(PS2_CTRL_KEYBOARD_SELF_TEST, NULL, 0, NULL, 0);
	ps2_keyboard_command(PS2_CTRL_KEYBOARD_SELF_TEST, NULL, 0, NULL, 0);

#if 0
	// Keyboard self-test

	if (ps2_write_ctrl(PS2_CTRL_KEYBOARD_SELF_TEST) != B_OK)
		return B_ERROR;

	tries = 3;
	while (tries-- > 0) {
		uint8 acknowledged;
		if (ps2_read_data(&acknowledged) == B_OK && acknowledged == PS2_REPLY_ACK)
			break;
	}

	// This selftest appears to fail quite frequently we'll just disable it
	/*if (tries < 0)
		return B_ERROR;*/

	// Activate keyboard

	if (ps2_write_ctrl(PS2_CTRL_KEYBOARD_ACTIVATE) != B_OK)
		return B_ERROR;

	// Enable keyboard

	return enable_keyboard();
#endif
	return B_OK;
}


//	#pragma mark -


status_t
keyboard_open(const char *name, uint32 flags, void **_cookie)
{
	status_t status;

	TRACE(("keyboard open()\n"));

	if (atomic_or(&sKeyboardOpenMask, 1) != 0)
		return B_BUSY;

	sKeyboardSem = create_sem(0, "keyboard_sem");
	if (sKeyboardSem < 0) {
		status = sKeyboardSem;
		goto err2;
	}

	sKeyBuffer = create_packet_buffer(KEY_BUFFER_SIZE * sizeof(at_kbd_io));
	if (sKeyBuffer == NULL) {
		status = B_NO_MEMORY;
		goto err3;
	}

	atomic_or(&ps2_device[PS2_DEVICE_KEYB].flags, PS2_FLAG_ENABLED);

	*_cookie = NULL;

	TRACE(("keyboard_open(): done.\n"));
	return B_OK;

err4:
	delete_packet_buffer(sKeyBuffer);
err3:
	delete_sem(sKeyboardSem);
err2:
err1:
	atomic_and(&sKeyboardOpenMask, 0);

	return status;
}


status_t
keyboard_close(void *cookie)
{
	TRACE(("keyboard_close()\n"));


	delete_packet_buffer(sKeyBuffer);
	delete_sem(sKeyboardSem);

	atomic_and(&sKeyboardOpenMask, 0);

	return B_OK;
}


status_t
keyboard_freecookie(void *cookie)
{
	return B_OK;
}


status_t
keyboard_read(void *cookie, off_t pos, void *buffer, size_t *_length)
{
	TRACE(("keyboard read()\n"));
	*_length = 0;
	return B_NOT_ALLOWED;
}


status_t
keyboard_write(void *cookie, off_t pos, const void *buffer,  size_t *_length)
{
	TRACE(("keyboard write()\n"));
	*_length = 0;
	return B_NOT_ALLOWED;
}


status_t
keyboard_ioctl(void *cookie, uint32 op, void *buffer, size_t length)
{
	TRACE(("keyboard ioctl()\n"));
	switch (op) {
		case KB_READ:
			TRACE(("KB_READ\n"));
			return read_keyboard_packet((at_kbd_io *)buffer);

		case KB_SET_LEDS:
		{
			led_info info;
			if (user_memcpy(&info, buffer, sizeof(led_info)) < B_OK)
				return B_BAD_ADDRESS;

			TRACE(("KB_SET_LEDS\n"));
			return set_leds(&info);
		}

		case KB_SET_KEY_REPEATING:
		case KB_SET_KEY_NONREPEATING:
			TRACE(("ps2_hid: ioctl 0x%lx not implemented yet, returning B_OK\n", op));
			return B_OK;

		default:
			TRACE(("ps2_hid: invalid ioctl 0x%lx\n", op));
			return EINVAL;
	}
}
