/*
 * Copyright 2004-2006 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * PS/2 keyboard device driver
 *
 * Authors (in chronological order):
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 *      Marcus Overhagen <marcus@overhagen.de>
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
	EXTENDED_KEY = 0xe0,
};


static int32 sKeyboardOpenMask;
static sem_id sKeyboardSem;
static struct packet_buffer *sKeyBuffer;
static bool sIsExtended = false;

static int32		sKeyboardRepeatRate;
static bigtime_t	sKeyboardRepeatDelay;

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

	return ps2_dev_command(&ps2_device[PS2_DEVICE_KEYB], PS2_CMD_KEYBOARD_SET_LEDS, &leds, 1, NULL, 0);
}


static status_t
set_typematic(int32 rate, bigtime_t delay)
{
	uint8 value;
	
	dprintf("ps2: set_typematic rate %ld, delay %Ld\n", rate, delay);

	// input server and keyboard preferences *seem* to use a range of 20-300
	if (rate < 20)
		rate = 20;
	if (rate > 300)
		rate = 300;
		
	// map this into range 0-31
	rate = ((rate - 20) * 31) / (300 - 20);

	// keyboard uses 0 == fast, 31 == slow
	value = 31 - rate;
	
	if (delay >= 875000)
		value |= 3 << 5;
	else if (delay >= 625000)
		value |= 2 << 5;
	else if (delay >= 375000)
		value |= 1 << 5;
	else 
		value |= 0 << 5;
		
	return ps2_dev_command(&ps2_device[PS2_DEVICE_KEYB], PS2_CMD_KEYBOARD_SET_TYPEMATIC, &value, 1, NULL, 0);
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
read_keyboard_packet(at_kbd_io *packet)
{
	status_t status;

	TRACE(("read_keyboard_packet()\n"));

	status = acquire_sem_etc(sKeyboardSem, 1, B_CAN_INTERRUPT, 0);
	if (status < B_OK)
		return status;

	if (packet_buffer_read(sKeyBuffer, (uint8 *)packet, sizeof(*packet)) == 0) {
		TRACE(("read_keyboard_packet(): error reading packet: %s\n", strerror(status)));
		return B_ERROR;
	}

	TRACE(("scancode: %x, keydown: %s\n", packet->scancode, packet->is_keydown ? "true" : "false"));
	return B_OK;
}


//	#pragma mark -


status_t
probe_keyboard(void)
{
	uint8 data;
	status_t status;
	
//	status = ps2_command(PS2_CTRL_KEYBOARD_ACTIVATE, const uint8 *out, int out_count, uint8 *in, int in_count)

	status = ps2_command(PS2_CTRL_KEYBOARD_TEST, NULL, 0, &data, 1);
	if (status != B_OK || data != 0x00) {
		dprintf("ps2: keyboard test failed, status 0x%08lx, data 0x%02x\n", status, data);
		return B_ERROR;
	}

	status = ps2_dev_command(&ps2_device[PS2_DEVICE_KEYB], PS2_CMD_RESET, NULL, 0, &data, 1);
	if (status != B_OK || data != 0xaa) {
		dprintf("ps2: keyboard reset failed, status 0x%08lx, data 0x%02x\n", status, data);
		return B_ERROR;
	}

	// default settings after keyboard reset: delay = 0x01 (500 ms), rate = 0x0b (10.9 chr/sec)
	sKeyboardRepeatRate = ((31 - 0x0b) * 280) / 31 + 20;
	sKeyboardRepeatDelay = 500000;

//	status = ps2_dev_command(&ps2_device[PS2_DEVICE_KEYB], PS2_ENABLE_KEYBOARD, NULL, 0, NULL, 0);

//  On my notebook, the keyboard controller does NACK the echo command.
//	status = ps2_dev_command(&ps2_device[PS2_DEVICE_KEYB], PS2_CMD_ECHO, NULL, 0, &data, 1);
//	if (status != B_OK || data != 0xee) {
//		dprintf("ps2: keyboard echo test failed, status 0x%08lx, data 0x%02x\n", status, data);
//		return B_ERROR;
//	}

	return B_OK;
}


//	#pragma mark -


static status_t
keyboard_open(const char *name, uint32 flags, void **_cookie)
{
	status_t status;

	dprintf("ps2: keyboard_open %s\n", name);

	if (atomic_or(&sKeyboardOpenMask, 1) != 0)
		return B_BUSY;
		
	status = probe_keyboard();
	if (status != B_OK) {
		dprintf("ps2: keyboard probing failed\n");
		goto err1;
	}

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

	dprintf("ps2: keyboard_open %s success\n", name);
	return B_OK;

err4:
	delete_packet_buffer(sKeyBuffer);
err3:
	delete_sem(sKeyboardSem);
err2:
err1:
	atomic_and(&sKeyboardOpenMask, 0);

	dprintf("ps2: keyboard_open %s failed\n", name);
	return status;
}


static status_t
keyboard_close(void *cookie)
{
	TRACE(("keyboard_close()\n"));

	delete_packet_buffer(sKeyBuffer);
	delete_sem(sKeyboardSem);

	atomic_and(&ps2_device[PS2_DEVICE_KEYB].flags, ~PS2_FLAG_ENABLED);
	
	atomic_and(&sKeyboardOpenMask, 0);

	return B_OK;
}


static status_t
keyboard_freecookie(void *cookie)
{
	return B_OK;
}


static status_t
keyboard_read(void *cookie, off_t pos, void *buffer, size_t *_length)
{
	TRACE(("keyboard read()\n"));
	*_length = 0;
	return B_NOT_ALLOWED;
}


static status_t
keyboard_write(void *cookie, off_t pos, const void *buffer,  size_t *_length)
{
	TRACE(("keyboard write()\n"));
	*_length = 0;
	return B_NOT_ALLOWED;
}


static status_t
keyboard_ioctl(void *cookie, uint32 op, void *buffer, size_t length)
{
	TRACE(("keyboard ioctl()\n"));
	switch (op) {
		case KB_READ:
		{
			at_kbd_io packet;
			TRACE(("KB_READ\n"));
			if (read_keyboard_packet(&packet) < B_OK)
				return B_ERROR;
			return user_memcpy(buffer, &packet, sizeof(packet));
		}

		case KB_SET_LEDS:
		{
			led_info info;
			TRACE(("KB_SET_LEDS\n"));
			if (user_memcpy(&info, buffer, sizeof(led_info)) < B_OK)
				return B_BAD_ADDRESS;
			return set_leds(&info);
		}

		case KB_SET_KEY_REPEATING:
		{
			// 0xFA (Set All Keys Typematic/Make/Break) - Keyboard responds with "ack" (0xFA).
			return ps2_dev_command(&ps2_device[PS2_DEVICE_KEYB], 0xfa, NULL, 0, NULL, 0);
		}

		case KB_SET_KEY_NONREPEATING:
		{
			// 0xF8 (Set All Keys Make/Break) - Keyboard responds with "ack" (0xFA).
			return ps2_dev_command(&ps2_device[PS2_DEVICE_KEYB], 0xf8, NULL, 0, NULL, 0);
		}
		
		case KB_SET_KEY_REPEAT_RATE:
		{
			int32 key_repeat_rate;
			if (user_memcpy(&key_repeat_rate, buffer, sizeof(key_repeat_rate)) < B_OK)
				return B_BAD_ADDRESS;
			dprintf("ps2: KB_SET_KEY_REPEAT_RATE %ld\n", key_repeat_rate);
			if (set_typematic(key_repeat_rate, sKeyboardRepeatDelay) < B_OK)
				return B_ERROR;
			sKeyboardRepeatRate = key_repeat_rate;
			return B_OK;
		}
		
		case KB_GET_KEY_REPEAT_RATE:
		{
			return user_memcpy(buffer, &sKeyboardRepeatRate, sizeof(sKeyboardRepeatRate));
		}

		case KB_SET_KEY_REPEAT_DELAY:
		{
			bigtime_t key_repeat_delay;
			if (user_memcpy(&key_repeat_delay, buffer, sizeof(key_repeat_delay)) < B_OK)
				return B_BAD_ADDRESS;
			dprintf("ps2: KB_SET_KEY_REPEAT_DELAY %Ld\n", key_repeat_delay);
			if (set_typematic(sKeyboardRepeatRate, key_repeat_delay) < B_OK)
				return B_ERROR;
			sKeyboardRepeatDelay = key_repeat_delay;
			return B_OK;
				
		}

		case KB_GET_KEY_REPEAT_DELAY:
		{
			return user_memcpy(buffer, &sKeyboardRepeatDelay, sizeof(sKeyboardRepeatDelay));
		}

		case KB_GET_KEYBOARD_ID:
		case KB_SET_CONTROL_ALT_DEL_TIMEOUT:
		case KB_CANCEL_CONTROL_ALT_DEL:
		case KB_DELAY_CONTROL_ALT_DEL:
			TRACE(("ps2_hid: ioctl 0x%lx not implemented yet, returning B_OK\n", op));
			return B_OK;

		default:
			TRACE(("ps2_hid: invalid ioctl 0x%lx\n", op));
			return EINVAL;
	}
}

device_hooks gKeyboardDeviceHooks = {
	keyboard_open,
	keyboard_close,
	keyboard_freecookie,
	keyboard_ioctl,
	keyboard_read,
	keyboard_write,
};
