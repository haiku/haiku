/*
 * Copyright 2004-2010 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors (in chronological order):
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 *      Marcus Overhagen <marcus@overhagen.de>
 */


/*! PS/2 keyboard device driver */


#include <string.h>

#include <new>

#include <debug.h>
#include <debugger_keymaps.h>
#include <lock.h>
#include <util/AutoLock.h>

#include "ATKeymap.h"
#include "ps2_service.h"
#include "keyboard_mouse_driver.h"
#include "packet_buffer.h"


#define KEY_BUFFER_SIZE 100
	// we will buffer 100 key strokes before we start dropping them

enum {
	LED_SCROLL 	= 1,
	LED_NUM 	= 2,
	LED_CAPS	= 4
};

enum {
	EXTENDED_KEY	= 0xe0,

	LEFT_ALT_KEY	= 0x38,
	RIGHT_ALT_KEY	= 0xb8,
	SYS_REQ_KEY		= 0x54
};


struct keyboard_cookie {
	bool is_reader;
	bool is_debugger;
};


static mutex sInitializeLock = MUTEX_INITIALIZER("keyboard init");
static int32 sKeyboardOpenCount = 0;
static bool sHasKeyboardReader = false;
static bool sHasDebugReader = false;
static sem_id sKeyboardSem;
static struct packet_buffer *sKeyBuffer;
static bool sIsExtended = false;

static int32 sKeyboardRepeatRate;
static bigtime_t sKeyboardRepeatDelay;
static uint8 sKeyboardIds[2];


static status_t
set_leds(led_info *ledInfo)
{
	uint8 leds = 0;

	TRACE("ps2: set keyboard LEDs\n");

	if (ledInfo->scroll_lock)
		leds |= LED_SCROLL;
	if (ledInfo->num_lock)
		leds |= LED_NUM;
	if (ledInfo->caps_lock)
		leds |= LED_CAPS;

	return ps2_dev_command(&ps2_device[PS2_DEVICE_KEYB],
		PS2_CMD_KEYBOARD_SET_LEDS, &leds, 1, NULL, 0);
}


static status_t
set_typematic(int32 rate, bigtime_t delay)
{
	uint8 value;

	TRACE("ps2: set_typematic rate %" B_PRId32 ", delay %" B_PRId64 "\n",
		rate, delay);

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

	return ps2_dev_command(&ps2_device[PS2_DEVICE_KEYB],
		PS2_CMD_KEYBOARD_SET_TYPEMATIC, &value, 1, NULL, 0);
}


static int32
keyboard_handle_int(ps2_dev *dev)
{
	enum emergency_keys {
		EMERGENCY_LEFT_ALT	= 0x01,
		EMERGENCY_RIGHT_ALT	= 0x02,
		EMERGENCY_SYS_REQ	= 0x04,
	};
	static int emergencyKeyStatus = 0;
	raw_key_info keyInfo;
	uint8 scancode = dev->history[0].data;

	if (atomic_get(&sKeyboardOpenCount) == 0)
		return B_HANDLED_INTERRUPT;

	// TODO: Handle braindead "pause" key special case

	if (scancode == EXTENDED_KEY) {
		sIsExtended = true;
//		TRACE("Extended key\n");
		return B_HANDLED_INTERRUPT;
	}

//	TRACE("scancode: %x\n", scancode);

	if ((scancode & 0x80) != 0) {
		keyInfo.is_keydown = false;
		scancode &= 0x7f;
	} else
		keyInfo.is_keydown = true;

	if (sIsExtended) {
		scancode |= 0x80;
		sIsExtended = false;
	}

	// Handle emergency keys
	if (scancode == LEFT_ALT_KEY || scancode == RIGHT_ALT_KEY) {
		// left or right alt-key pressed
		if (keyInfo.is_keydown) {
			emergencyKeyStatus |= scancode == LEFT_ALT_KEY
				? EMERGENCY_LEFT_ALT : EMERGENCY_RIGHT_ALT;
		} else {
			emergencyKeyStatus &= ~(scancode == LEFT_ALT_KEY
				? EMERGENCY_LEFT_ALT : EMERGENCY_RIGHT_ALT);
		}
	} else if (scancode == SYS_REQ_KEY) {
		if (keyInfo.is_keydown)
			emergencyKeyStatus |= EMERGENCY_SYS_REQ;
		else
			emergencyKeyStatus &= EMERGENCY_SYS_REQ;
	} else if (emergencyKeyStatus > EMERGENCY_SYS_REQ
		&& debug_emergency_key_pressed(kUnshiftedKeymap[scancode])) {
		static const int kKeys[] = {LEFT_ALT_KEY, RIGHT_ALT_KEY, SYS_REQ_KEY};

		// we probably have lost some keys, so reset our key states
		emergencyKeyStatus = 0;

		// send key ups for alt-sysreq
		keyInfo.timestamp = system_time();
		keyInfo.is_keydown = false;
		for (size_t i = 0; i < sizeof(kKeys) / sizeof(kKeys[0]); i++) {
			keyInfo.keycode = kATKeycodeMap[kKeys[i] - 1];
			if (packet_buffer_write(sKeyBuffer, (uint8 *)&keyInfo,
					sizeof(keyInfo)) != 0)
				release_sem_etc(sKeyboardSem, 1, B_DO_NOT_RESCHEDULE);
		}

		return B_HANDLED_INTERRUPT;
	}

	keyInfo.timestamp = dev->history[0].time;
	keyInfo.keycode = kATKeycodeMap[scancode - 1];

	if (packet_buffer_write(sKeyBuffer, (uint8 *)&keyInfo,
			sizeof(keyInfo)) == 0) {
		// If there is no space left in the buffer, we drop this key stroke. We
		// avoid dropping old key strokes, to not destroy what already was
		// typed.
		return B_HANDLED_INTERRUPT;
	}

	release_sem_etc(sKeyboardSem, 1, B_DO_NOT_RESCHEDULE);

	return B_INVOKE_SCHEDULER;
}


static status_t
read_keyboard_packet(raw_key_info *packet, bool isDebugger)
{
	status_t status;

	TRACE("ps2: read_keyboard_packet: enter\n");

	while (true) {
		status = acquire_sem_etc(sKeyboardSem, 1, B_CAN_INTERRUPT, 0);
		if (status != B_OK)
			return status;

		if (!ps2_device[PS2_DEVICE_KEYB].active) {
			TRACE("ps2: read_keyboard_packet, Error device no longer active\n");
			return B_ERROR;
		}

		if (isDebugger || !sHasDebugReader)
			break;

		// Give the debugger a chance to read this packet
		release_sem(sKeyboardSem);
		snooze(100000);
	}

	if (packet_buffer_read(sKeyBuffer, (uint8 *)packet, sizeof(*packet)) == 0) {
		TRACE("ps2: read_keyboard_packet, Error reading packet: %s\n",
			strerror(status));
		return B_ERROR;
	}

	TRACE("ps2: read_keyboard_packet: keycode: %" B_PRIx32 ", keydown: %s\n",
		packet->keycode, packet->is_keydown ? "true" : "false");

	return B_OK;
}


static void
ps2_keyboard_disconnect(ps2_dev *dev)
{
	// the keyboard might not be opened at this point
	INFO("ps2: ps2_keyboard_disconnect %s\n", dev->name);
	if (atomic_get(&sKeyboardOpenCount) != 0)
		release_sem(sKeyboardSem);
}


//	#pragma mark -


status_t
probe_keyboard(void)
{
	uint8 data;
	status_t status;

//  This test doesn't work relyable on some notebooks (it reports 0x03)
//	status = ps2_command(PS2_CTRL_KEYBOARD_TEST, NULL, 0, &data, 1);
//	if (status != B_OK || data != 0x00) {
//		INFO("ps2: keyboard test failed, status 0x%08lx, data 0x%02x\n", status, data);
//		return B_ERROR;
//	}

	status = ps2_dev_command(&ps2_device[PS2_DEVICE_KEYB], PS2_CMD_RESET, NULL,
		0, &data, 1);
	if (status != B_OK || data != 0xaa) {
		INFO("ps2: keyboard reset failed, status 0x%08" B_PRIx32 ", data 0x%02x"
			"\n", status, data);
		return B_ERROR;
	}

	// default settings after keyboard reset: delay = 0x01 (500 ms),
	// rate = 0x0b (10.9 chr/sec)
	sKeyboardRepeatRate = ((31 - 0x0b) * 280) / 31 + 20;
	sKeyboardRepeatDelay = 500000;

//	status = ps2_dev_command(&ps2_device[PS2_DEVICE_KEYB], PS2_ENABLE_KEYBOARD, NULL, 0, NULL, 0);

//  On my notebook, the keyboard controller does NACK the echo command.
//	status = ps2_dev_command(&ps2_device[PS2_DEVICE_KEYB], PS2_CMD_ECHO, NULL, 0, &data, 1);
//	if (status != B_OK || data != 0xee) {
//		INFO("ps2: keyboard echo test failed, status 0x%08lx, data 0x%02x\n", status, data);
//		return B_ERROR;
//	}

// Some controllers set the disble keyboard command bit to "on" after resetting
// the keyboard device. Read #7973 #6313 for more details.
// So check the command byte now and re-enable the keyboard if it is the case.
	uint8 cmdbyte = 0;
	status = ps2_command(PS2_CTRL_READ_CMD, NULL, 0, &cmdbyte, 1);

	if (status != B_OK) {
		INFO("ps2: cannot read CMD byte on kbd probe:%#08" B_PRIx32 "\n",
			status);
	} else
	if ((cmdbyte & PS2_BITS_KEYBOARD_DISABLED) == PS2_BITS_KEYBOARD_DISABLED) {
		cmdbyte &= ~PS2_BITS_KEYBOARD_DISABLED;
		status = ps2_command(PS2_CTRL_WRITE_CMD, &cmdbyte, 1, NULL, 0);
		if (status != B_OK) {
			INFO("ps2: cannot write 0x%02x to CMD byte on kbd probe:%#08"
				B_PRIx32 "\n", cmdbyte, status);
		}
	}
	
	status = ps2_dev_command(&ps2_device[PS2_DEVICE_KEYB],
			PS2_CMD_GET_DEVICE_ID, NULL, 0, sKeyboardIds, sizeof(sKeyboardIds));
	
	if (status != B_OK) {
		INFO("ps2: cannot read keyboard device id:%#08" B_PRIx32 "\n", status);
	}

	return B_OK;
}


//	#pragma mark -


static status_t
keyboard_open(const char *name, uint32 flags, void **_cookie)
{
	TRACE("ps2: keyboard_open %s\n", name);

	keyboard_cookie* cookie = new(std::nothrow) keyboard_cookie();
	if (cookie == NULL)
		return B_NO_MEMORY;

	cookie->is_reader = false;
	cookie->is_debugger = false;

	MutexLocker locker(sInitializeLock);

	if (atomic_get(&sKeyboardOpenCount) == 0) {
		status_t status = probe_keyboard();
		if (status != B_OK) {
			INFO("ps2: keyboard probing failed\n");
			ps2_service_notify_device_removed(&ps2_device[PS2_DEVICE_KEYB]);
			delete cookie;
			return status;
		}

		INFO("ps2: keyboard found\n");

		sKeyboardSem = create_sem(0, "keyboard_sem");
		if (sKeyboardSem < 0) {
			delete cookie;
			return sKeyboardSem;
		}

		sKeyBuffer
			= create_packet_buffer(KEY_BUFFER_SIZE * sizeof(raw_key_info));
		if (sKeyBuffer == NULL) {
			delete_sem(sKeyboardSem);
			delete cookie;
			return B_NO_MEMORY;
		}

		ps2_device[PS2_DEVICE_KEYB].disconnect = &ps2_keyboard_disconnect;
		ps2_device[PS2_DEVICE_KEYB].handle_int = &keyboard_handle_int;

		atomic_or(&ps2_device[PS2_DEVICE_KEYB].flags, PS2_FLAG_ENABLED);
	}

	atomic_add(&sKeyboardOpenCount, 1);
	*_cookie = cookie;

	TRACE("ps2: keyboard_open %s success\n", name);
	return B_OK;
}


static status_t
keyboard_close(void *_cookie)
{
	keyboard_cookie *cookie = (keyboard_cookie *)_cookie;

	TRACE("ps2: keyboard_close enter\n");

	if (atomic_add(&sKeyboardOpenCount, -1) == 1) {
		delete_packet_buffer(sKeyBuffer);
		delete_sem(sKeyboardSem);

		atomic_and(&ps2_device[PS2_DEVICE_KEYB].flags, ~PS2_FLAG_ENABLED);

		sKeyboardIds[0] = sKeyboardIds[1] = 0;
	}

	if (cookie->is_reader)
		sHasKeyboardReader = false;
	if (cookie->is_debugger)
		sHasDebugReader = false;

	TRACE("ps2: keyboard_close done\n");
	return B_OK;
}


static status_t
keyboard_freecookie(void *cookie)
{
	delete (keyboard_cookie*)cookie;
	return B_OK;
}


static status_t
keyboard_read(void *cookie, off_t pos, void *buffer, size_t *_length)
{
	TRACE("ps2: keyboard read\n");
	*_length = 0;
	return B_NOT_ALLOWED;
}


static status_t
keyboard_write(void *cookie, off_t pos, const void *buffer,  size_t *_length)
{
	TRACE("ps2: keyboard write\n");
	*_length = 0;
	return B_NOT_ALLOWED;
}


static status_t
keyboard_ioctl(void *_cookie, uint32 op, void *buffer, size_t length)
{
	keyboard_cookie *cookie = (keyboard_cookie *)_cookie;

	switch (op) {
		case KB_READ:
		{
			if (!sHasKeyboardReader && !cookie->is_debugger) {
				cookie->is_reader = true;
				sHasKeyboardReader = true;
			} else if (!cookie->is_debugger && !cookie->is_reader)
				return B_BUSY;

			raw_key_info packet;
			status_t status = read_keyboard_packet(&packet,
				cookie->is_debugger);
			TRACE("ps2: ioctl KB_READ: %s\n", strerror(status));
			if (status != B_OK)
				return status;

			return user_memcpy(buffer, &packet, sizeof(packet));
		}

		case KB_SET_LEDS:
		{
			led_info info;
			TRACE("ps2: ioctl KB_SET_LEDS\n");
			if (user_memcpy(&info, buffer, sizeof(led_info)) < B_OK)
				return B_BAD_ADDRESS;
			return set_leds(&info);
		}

		case KB_SET_KEY_REPEATING:
		{
			TRACE("ps2: ioctl KB_SET_KEY_REPEATING\n");
			// 0xFA (Set All Keys Typematic/Make/Break) - Keyboard responds
			// with "ack" (0xFA).
			return ps2_dev_command(&ps2_device[PS2_DEVICE_KEYB], 0xfa, NULL, 0,
				NULL, 0);
		}

		case KB_SET_KEY_NONREPEATING:
		{
			TRACE("ps2: ioctl KB_SET_KEY_NONREPEATING\n");
			// 0xF8 (Set All Keys Make/Break) - Keyboard responds with "ack"
			// (0xFA).
			return ps2_dev_command(&ps2_device[PS2_DEVICE_KEYB], 0xf8, NULL, 0,
				NULL, 0);
		}

		case KB_SET_KEY_REPEAT_RATE:
		{
			int32 key_repeat_rate;
			TRACE("ps2: ioctl KB_SET_KEY_REPEAT_RATE\n");
			if (user_memcpy(&key_repeat_rate, buffer, sizeof(key_repeat_rate))
					!= B_OK)
				return B_BAD_ADDRESS;
			if (set_typematic(key_repeat_rate, sKeyboardRepeatDelay) != B_OK)
				return B_ERROR;
			sKeyboardRepeatRate = key_repeat_rate;
			return B_OK;
		}

		case KB_GET_KEY_REPEAT_RATE:
		{
			TRACE("ps2: ioctl KB_GET_KEY_REPEAT_RATE\n");
			return user_memcpy(buffer, &sKeyboardRepeatRate,
				sizeof(sKeyboardRepeatRate));
		}

		case KB_SET_KEY_REPEAT_DELAY:
		{
			bigtime_t key_repeat_delay;
			TRACE("ps2: ioctl KB_SET_KEY_REPEAT_DELAY\n");
			if (user_memcpy(&key_repeat_delay, buffer, sizeof(key_repeat_delay))
					!= B_OK)
				return B_BAD_ADDRESS;
			if (set_typematic(sKeyboardRepeatRate, key_repeat_delay) != B_OK)
				return B_ERROR;
			sKeyboardRepeatDelay = key_repeat_delay;
			return B_OK;

		}

		case KB_GET_KEY_REPEAT_DELAY:
		{
			TRACE("ps2: ioctl KB_GET_KEY_REPEAT_DELAY\n");
			return user_memcpy(buffer, &sKeyboardRepeatDelay,
				sizeof(sKeyboardRepeatDelay));
		}

		case KB_GET_KEYBOARD_ID:
		{
			TRACE("ps2: ioctl KB_GET_KEYBOARD_ID\n");
			uint16 keyboardId = sKeyboardIds[1] << 8 | sKeyboardIds[0];
			return user_memcpy(buffer, &keyboardId, sizeof(keyboardId));
		}

		case KB_SET_CONTROL_ALT_DEL_TIMEOUT:
		case KB_CANCEL_CONTROL_ALT_DEL:
		case KB_DELAY_CONTROL_ALT_DEL:
			INFO("ps2: ioctl 0x%" B_PRIx32 " not implemented yet, returning "
				"B_OK\n", op);
			return B_OK;

		case KB_SET_DEBUG_READER:
			if (sHasDebugReader)
				return B_BUSY;

			cookie->is_debugger = true;
			sHasDebugReader = true;
			return B_OK;

		default:
			INFO("ps2: invalid ioctl 0x%" B_PRIx32 "\n", op);
			return B_DEV_INVALID_IOCTL;
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
