/*
 * Copyright 2004 Haiku, Inc.
 * Distributed under the terms of the Haiku License.
 *
 * keyboard.c:
 * PS/2 keyboard device driver
 * Authors (in chronological order):
 *		Stefano Ceccherini (burton666@libero.it)
 */

// TODO: Uncomment locking code 
// We probably need to lock to avoid misbehaving on SMP machines,
// although the kb_mouse replacement driver never locks.

#include "common.h"
#ifdef COMPILE_FOR_R5
#include "cbuf_adapter.h"
#else
#include "cbuf.h"
#endif

#include <string.h>
//#include <lock.h>

#include "kb_mouse_driver.h"


enum {
	LED_SCROLL 	= 1,
	LED_NUM 	= 2,
	LED_CAPS	= 4
} leds_status;


enum {
	PS2_SET_LEDS = 0xed
} commands;


static sem_id sKeyboardSem;
//static mutex keyboard_read_mutex;
static cbuf *keyboard_buf;
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
	if (scancode & 0x80) {
		keyInfo.is_keydown = false;
		scancode -= 0x80;	
	} else
		keyInfo.is_keydown = true;

	if (sIsExtended)
		scancode |= 0x80;
	
	keyInfo.timestamp = system_time();
	keyInfo.scancode = scancode;
	
	// TODO: Check return value
	cbuf_memcpy_to_chain(keyboard_buf, 0, (void *)&keyInfo, sizeof(keyInfo));
	release_sem_etc(sKeyboardSem, 1, B_DO_NOT_RESCHEDULE);
	
	sIsExtended = false;
		
	return B_INVOKE_SCHEDULER;
}


static status_t
read_keyboard_packet(at_kbd_io *buffer)
{
	status_t status;
	status = acquire_sem_etc(sKeyboardSem, 1, B_CAN_INTERRUPT, 0);
	if (status < B_OK)
		return status;
	
	status = cbuf_memcpy_from_chain((void *)buffer, keyboard_buf, 0, sizeof(at_kbd_io));
	if (status < B_OK)
		TRACE(("read_keyboard_packet(): error reading packet: %s\n", strerror(status)));

	TRACE(("scancode: %x, keydown: %s\n", buffer->scancode, buffer->is_keydown ? "true" : "false"));
	return status;
}


status_t 
keyboard_open(const char *name, uint32 flags, void **cookie)
{
	status_t status;
	
	TRACE(("keyboard open()\n"));
	if (atomic_or(&gKeyboardOpenMask, 1) != 0)
		return B_BUSY;

	sKeyboardSem = create_sem(0, "keyboard_sem");
	if (sKeyboardSem < 0) {
		atomic_and(&gKeyboardOpenMask, 0);
		status = sKeyboardSem;
		return status;
	}
	
	//if (mutex_init(&keyboard_read_mutex, "keyboard_read_mutex") < 0)
	//	panic("could not create keyboard read mutex!\n");

	keyboard_buf = cbuf_get_chain(1024);
	if (keyboard_buf == NULL)
		panic("could not create keyboard cbuf chain!\n");

	status = install_io_interrupt_handler(1, &handle_keyboard_interrupt, NULL, 0);
	
	*cookie = NULL;
	
	return status;
}


status_t 
keyboard_close(void *cookie)
{
	remove_io_interrupt_handler(1, &handle_keyboard_interrupt, NULL);

	cbuf_free_chain(keyboard_buf);
	delete_sem(sKeyboardSem);
	//mutex_destroy(&keyboard_read_mutex);
	
	atomic_and(&gKeyboardOpenMask, 0);
	
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
	return EROFS;
}


status_t 
keyboard_write(void *cookie, off_t pos, const void *buf,  size_t *len)
{
	TRACE(("keyboard write()\n"));
	*len = 0;
	return EROFS;
}


status_t 
keyboard_ioctl(void *cookie, uint32 op, void *buf, size_t len)
{
	TRACE(("keyboard ioctl()\n"));
	switch (op) {
		case KB_READ:
			TRACE(("KB_READ\n"));
			return read_keyboard_packet((at_kbd_io *)buf);
		case KB_SET_LEDS:
			set_leds((led_info *)buf);
			TRACE(("KB_SET_LEDS\n"));
			return B_OK;
		case KB_SET_KEY_REPEATING:
		case KB_SET_KEY_NONREPEATING:
			TRACE(("ioctl 0x%x not implemented yet, returning B_OK\n", op));
			return B_OK;
		default:
			TRACE(("invalid ioctl 0x%x\n", op));
			return EINVAL;
	}
}
