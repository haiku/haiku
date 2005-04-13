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


#include "common.h"
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
	PS2_SET_LEDS = 0xed
} commands;


static int32 sKeyboardOpenMask;
static sem_id sKeyboardSem;
static struct packet_buffer *sKeyBuffer;
static bool sIsExtended = false;


static void 
set_leds(led_info *ledInfo)
{
	int leds = 0;
	if (ledInfo->scroll_lock)
		leds |= LED_SCROLL;
	if (ledInfo->num_lock)
		leds |= LED_NUM;
	if (ledInfo->caps_lock)
		leds |= LED_CAPS;

	wait_write_data();
	gIsa->write_io_8(PS2_PORT_DATA, PS2_SET_LEDS);
	wait_write_data();
	gIsa->write_io_8(PS2_PORT_DATA, leds);
}


static int32 
handle_keyboard_interrupt(void *data)
{
	// TODO: Handle braindead "pause" key special case
	at_kbd_io keyInfo;
	unsigned char read, scancode;

	read = gIsa->read_io_8(PS2_PORT_DATA);
	TRACE(("handle_keyboard_interrupt: read = 0x%x\n", read));

	if (read == 0xE0) {
		sIsExtended = true;
		TRACE(("Extended key\n"));
		return B_HANDLED_INTERRUPT;
	} 

	scancode = read;

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

	while (packet_buffer_write(sKeyBuffer, (uint8 *)&keyInfo, sizeof(keyInfo)) == 0) {
		// if there is no space left in the buffer, we start dropping old key strokes
		packet_buffer_flush(sKeyBuffer, sizeof(keyInfo));
	}
	
	release_sem_etc(sKeyboardSem, 1, B_DO_NOT_RESCHEDULE);

	return B_INVOKE_SCHEDULER;
}


static status_t
read_keyboard_packet(at_kbd_io *userBuffer)
{
	at_kbd_io packet;

	status_t status = acquire_sem_etc(sKeyboardSem, 1, B_CAN_INTERRUPT, 0);
	if (status < B_OK)
		return status;

	if (packet_buffer_read(sKeyBuffer, (uint8 *)&packet, sizeof(at_kbd_io)) == 0) {
		TRACE(("read_keyboard_packet(): error reading packet: %s\n", strerror(status)));
		return B_ERROR;
	}

	TRACE(("scancode: %x, keydown: %s\n", packet.scancode, packet.is_keydown ? "true" : "false"));

	return user_memcpy(userBuffer, &packet, sizeof(at_kbd_io));
}


//	#pragma mark -


status_t 
keyboard_open(const char *name, uint32 flags, void **_cookie)
{
	TRACE(("keyboard open()\n"));

	if (atomic_or(&sKeyboardOpenMask, 1) != 0)
		return B_BUSY;

	sKeyboardSem = create_sem(0, "keyboard_sem");
	if (sKeyboardSem < 0) {
		atomic_and(&sKeyboardOpenMask, 0);
		return sKeyboardSem;
	}

	sKeyBuffer = create_packet_buffer(KEY_BUFFER_SIZE * sizeof(at_kbd_io));
	if (sKeyBuffer == NULL) {
		panic("could not create keyboard packet buffer!\n");
		atomic_and(&sKeyboardOpenMask, 0);
		return B_NO_MEMORY;
	}

	*_cookie = NULL;

	return install_io_interrupt_handler(INT_PS2_KEYBOARD, &handle_keyboard_interrupt, NULL, 0);
}


status_t 
keyboard_close(void *cookie)
{
	remove_io_interrupt_handler(INT_PS2_KEYBOARD, &handle_keyboard_interrupt, NULL);

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
			set_leds(&info);
			return B_OK;
		}

		case KB_SET_KEY_REPEATING:
		case KB_SET_KEY_NONREPEATING:
			TRACE(("ioctl 0x%x not implemented yet, returning B_OK\n", op));
			return B_OK;

		default:
			TRACE(("invalid ioctl 0x%x\n", op));
			return EINVAL;
	}
}
