/*
 * Copyright 2004-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *
 * Some portions of code are copyrighted by
 * USB Joystick driver for BeOS R5
 * Copyright 2000 (C) ITO, Takayuki. All rights reserved
 */


#include "hid.h"
#include "kb_mouse_driver.h"
#include "usbdevs.h"

#include <Debug.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define MAX_BUTTONS 16

static status_t hid_device_added(const usb_device *dev, void **cookie);
static status_t hid_device_removed(void *cookie);


typedef struct driver_cookie {
	struct driver_cookie	*next;
	hid_device_info			*device;
} driver_cookie;

int32 api_version = B_CUR_DRIVER_API_VERSION;

static usb_notify_hooks sNotifyHooks = {
	hid_device_added, hid_device_removed
};

#define	SUPPORTED_DEVICES	1
static usb_support_descriptor sSupportedDevices[SUPPORTED_DEVICES] = {
	{ USB_HID_DEVICE_CLASS, 0, 0, 0, 0 },
};

const uint32 modifier_table[] = {
	KEY_ControlL,
	KEY_ShiftL,
	KEY_AltL,
	KEY_WinL,
	KEY_ControlR,
	KEY_ShiftR,
	KEY_AltR,
	KEY_WinR
};

const uint32 key_table[] = {
	0x00,	// ERROR
	0x00,	// ERROR
	0x00,	// ERROR
	0x00,	// ERROR
	0x3c,	// A
	0x50,	// B
	0x4e,	// C
	0x3e,	// D
	0x29, 	// E
	0x3f, 	// F
	0x40, 	// G
	0x41, 	// H
	0x2e, 	// I
	0x42,	// J
	0x43, 	// K
	0x44, 	// L
	0x52,	// M
	0x51, 	// N
	0x2f, 	// O
	0x30, 	// P
	0x27,	// Q
	0x2a,	// R
	0x3d,	// S
	0x2b,	// T
	0x2d,	// U
	0x4f,	// V
	0x28,	// W
	0x4d,	// X
	0x2c,	// Y
	0x4c, 	// Z
	0x12, 	// 1
	0x13,	// 2
	0x14,	// 3
	0x15,	// 4
	0x16, 	// 5
	0x17,	// 6
	0x18,	// 7
	0x19,	// 8
	0x1a,	// 9
	0x1b,	// 0
	0x47,	// enter
	0x01,	// Esc
	0x1e,	// Backspace
	0x26,	// Tab
	0x5e,	// Space
	0x1c,	// -
	0x1d,	// =
	0x31,	// [
	0x32,	// ]
	0x00, 	// unmapped
	0x33,	// \ 
	0x45,	// ;
	0x46,	// '
	0x11,	// `
	0x53,	// ,
	0x54,	// .
	0x55,	// /
	KEY_CapsLock,	// Caps
	0x02,	// F1
	0x03,	// F2
	0x04,	// F3
	0x05,	// F4
	0x06,	// F5
	0x07,	// F6
	0x08,	// F7
	0x09,	// F8
	0x0a,	// F9
	0x0b,	// F10
	0x0c,	// F11
	0x0d,	// F12
	0x0e,	// PrintScreen
	KEY_Scroll,	// Scroll Lock
	KEY_Pause, 	// Pause (0x7f with Ctrl)
	0x1f,	// Insert
	0x20,	// Home
	0x21,	// Page up
	0x34,	// Delete
	0x35,	// End
	0x36,	// Page down
	0x63,	// Right arrow
	0x61,	// Left arrow
	0x62,	// Down arrow
	0x57,	// Up arrow
	0x22,	// Num Lock
	0x23,	// Pad /
	0x24,	// Pad *
	0x25,	// Pad -
	0x3a,	// Pad +
	0x5b,	// Pad Enter
	0x58,	// Pad 1
	0x59, 	// Pad 2
	0x5a,	// Pad 3
	0x48,	// Pad 4
	0x49,	// Pad 5
	0x4a,	// Pad 6
	0x37,	// Pad 7
	0x38,	// Pad 8
	0x39,	// Pad 9
	0x64,	// Pad 0
	0x65,	// Pad .
	0x69, 	// <
	KEY_Menu,	// Menu
	KEY_Power,	// Power
	KEY_NumEqual,	// Pad =
	0x00,	// F13 unmapped
	0x00,	// F14 unmapped
	0x00,	// F15 unmapped
	0x00,	// F16 unmapped
	0x00,	// F17 unmapped
	0x00,	// F18 unmapped
	0x00,	// F19 unmapped
	0x00,	// F20 unmapped
	0x00,	// F21 unmapped
	0x00,	// F22 unmapped
	0x00,	// F23 unmapped
	0x00,	// F24 unmapped
	0x00, 	// Execute unmapped
	0x00, 	// Help unmapped
	0x00, 	// Menu unmapped
	0x00, 	// Select unmapped
	0x00, 	// Stop unmapped
	0x00, 	// Again unmapped
	0x00, 	// Undo unmapped
	0x00, 	// Cut unmapped
	0x00, 	// Copy unmapped
	0x00, 	// Paste unmapped
	0x00, 	// Find unmapped
	0x00, 	// Mute unmapped
	0x00, 	// Volume up unmapped
	0x00, 	// Volume down unmapped
	0x00, 	// CapsLock unmapped
	0x00, 	// NumLock unmapped
	0x00, 	// Scroll lock unmapped
	0x70,   // Keypad . on Brazilian ABNT2
	0x00, 	// = sign
	0x6b,   // Ro (\\ key, japanese)
	0x6e,   // Katakana/Hiragana, second key right to spacebar, japanese
	0x6a,   // Yen (macron key, japanese)
	0x6d,   // Henkan, first key right to spacebar, japanese
	0x6c,   // Muhenkan, key left to spacebar, japanese
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
	0x00,	// unmapped
};

usb_module_info *usb;

static const char *kDriverName = DRIVER_NAME;

static int sKeyboardDeviceNumber = 0;
static int sMouseDeviceNumber = 0;
static const char *sKeyboardBaseName = "input/keyboard/usb/";
static const char *sMouseBaseName = "input/mouse/usb/";


hid_device_info *
create_device(const usb_device *dev, const usb_interface_info *ii,
	uint16 ifno, bool isKeyboard)
{
	hid_device_info *device = NULL;
	int number;
	area_id area;
	sem_id sem;
	char area_name[32];
	const char *base_name;
	
	assert (usb != NULL && dev != NULL);

	if (isKeyboard) {	
		number = sKeyboardDeviceNumber++;
		base_name = sKeyboardBaseName;
	} else {
		number = sMouseDeviceNumber++;
		base_name = sMouseBaseName;
	}
	
	device = malloc(sizeof(hid_device_info));
	if (device == NULL)
		return NULL;

	device->sem_cb = sem = create_sem(0, DRIVER_NAME "_cb");
	if (sem < B_OK) {
		DPRINTF_ERR((MY_ID "create_sem() failed %d\n", (int)sem));
		free(device);
		return NULL;
	}

	device->sem_lock = sem = create_sem(1, DRIVER_NAME "_lock");
	if (sem < B_OK) {
		DPRINTF_ERR((MY_ID "create_sem() failed %d\n", (int)sem));
		delete_sem(device->sem_cb);
		free(device);
		return NULL;
	}

	sprintf(area_name, DRIVER_NAME "_buffer%d", number);
	device->buffer_area = area = create_area(area_name,
		(void **) &device->buffer, B_ANY_KERNEL_ADDRESS,
		B_PAGE_SIZE, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if (area < B_OK) {
		DPRINTF_ERR((MY_ID "create_area() failed %d\n", (int)area));
		delete_sem(device->sem_cb);
		delete_sem(device->sem_lock);
		free(device);
		return NULL;
	}

	sprintf(device->name, "%s%d", base_name, number);
	device->dev = dev;
	device->ifno = ifno;
	device->open = 0;
	device->open_fds = NULL;
	device->active = true;
	device->insns = NULL;
	device->num_insns = 0;
	device->flags = 0;
	device->rbuf = create_ring_buffer(384);
	device->is_keyboard = isKeyboard;

	// default values taken from the PS/2 driver
	device->repeat_rate = 35000;
	device->repeat_delay = 300000;
	device->repeat_timer.device = device;

	return device;
}


void
remove_device(hid_device_info *device)
{
	assert(device != NULL);
	cancel_timer(&device->repeat_timer.timer);

	if (device->rbuf != NULL) {
		delete_ring_buffer(device->rbuf);
		device->rbuf = NULL;
	}

	delete_area(device->buffer_area);
	delete_sem(device->sem_cb);
	delete_sem(device->sem_lock);
	free(device);
}


static void
write_key(hid_device_info *device, uint32 key, bool down)
{
	raw_key_info raw;
	raw.be_keycode = key;
	raw.is_keydown = down;
	raw.timestamp = system_time();

	ring_buffer_write(device->rbuf, (const uint8*)&raw, sizeof(raw_key_info));
	release_sem_etc(device->sem_cb, 1, B_DO_NOT_RESCHEDULE);
}


static int32
timer_repeat_hook(struct timer *timer)
{
	hid_device_info *device = ((struct hid_repeat_timer *)timer)->device;

	write_key(device, device->repeat_timer.key, true);
	return B_HANDLED_INTERRUPT;
}


static int32
timer_delay_hook(struct timer *timer)
{
	hid_device_info *device = ((struct hid_repeat_timer *)timer)->device;

	add_timer(&device->repeat_timer.timer, timer_repeat_hook, device->repeat_rate,
		B_PERIODIC_TIMER);
	return B_HANDLED_INTERRUPT;
}


// here we don't need to follow a report descriptor for typical keyboards
// TODO : but we should for keypads for example because they aren't boot keyboard devices !
// see hidparse.c
static void
interpret_kb_buffer(hid_device_info *device)
{
	uint8 modifiers = ((uint8*)device->buffer)[0];
	uint8 bits = device->last_buffer[0] ^ modifiers;
	uint32 i, j;

	if (bits) {
		for (j = 0; bits; j++, bits >>= 1) {
			if (bits & 1)
				write_key(device, modifier_table[j], (modifiers >> j) & 1);
		}
	}

	// key down

	for (i = 2; i < device->total_report_size; i++) {
		if (((uint8*)device->buffer)[i] && ((uint8*)device->last_buffer)[i] != 0x1) {
			bool found = false;
			for (j = 2; j < device->total_report_size; j++) {
				if (((uint8*)device->last_buffer)[j]
					&& ((uint8*)device->last_buffer)[j] == ((uint8*)device->buffer)[i]) {
					found = true;
					break;
				}
			}

			if (!found) {
				uint32 key = key_table[((uint8*)device->buffer)[i]];
				if (key == KEY_Pause && modifiers & 1)
					key = KEY_Break;
				else if (key == 0xe && modifiers & 1)
					key = KEY_SysRq;
				else if (key == 0) {
					// unmapped key
					key = 0x200000 + ((uint8*)device->buffer)[i];
				}

				write_key(device, key, true);

				// repeat handling
				cancel_timer(&device->repeat_timer.timer);
				device->repeat_timer.key = key;
				add_timer(&device->repeat_timer.timer, timer_delay_hook,
					device->repeat_delay, B_ONE_SHOT_RELATIVE_TIMER);
			}
		} else
			break;
	}

	// key up
	// TODO: merge this...

	for (i = 2; i < device->total_report_size; i++) {
		if (((uint8*)device->last_buffer)[i] && ((uint8*)device->last_buffer)[i] != 0x1) {
			bool found = false;
			for (j = 2; j < device->total_report_size; j++) {
				if (((uint8*)device->buffer)[j]
					&& ((uint8*)device->buffer)[j] == ((uint8*)device->last_buffer)[i]) {
					found = true;
					break;
				}
			}

			if (!found) {
				uint32 key = key_table[((uint8*)device->last_buffer)[i]];
				if (key == KEY_Pause && modifiers & 1)
					key = KEY_Break;
				else if (key == 0xe && modifiers & 1)
					key = KEY_SysRq;
				else if (key == 0) {
					// unmapped key
					key = 0x200000 + ((uint8*)device->last_buffer)[i];
				}

				write_key(device, key, false);
				cancel_timer(&device->repeat_timer.timer);
			}
		} else
			break;
	}
}


// TODO : here we don't follow a report descriptor but we actually should
// see hidparse.c
static void
set_leds(hid_device_info *device, uint8* data)
{
	status_t status;
	size_t actual = 1;
	uint8 leds = 0;
	int report_id = 0;
	if (data[0] == 1)
		leds |= (1 << 0); 
	if (data[1] == 1)
		leds |= (1 << 1); 
	if (data[2] == 1)
		leds |= (1 << 2);
	
	status = usb->send_request (device->dev, 
		USB_REQTYPE_INTERFACE_OUT | USB_REQTYPE_CLASS,
		USB_REQUEST_HID_SET_REPORT,
		0x200 | report_id, device->ifno, actual, 
		&leds, actual, &actual);
	DPRINTF_INFO ((MY_ID "set_leds: leds=0x%02x, status=%d, len=%d\n", 
		leds, (int) status, (int)actual));
}


static int 
sign_extend(int value, int size)
{
	if (value & (1 << (size - 1)))
		return value | (UINT_MAX << size);

	return value;
}


static void
interpret_mouse_buffer(hid_device_info *device)
{
	mouse_movement info;
	uint8 *report = (uint8*)device->buffer;
	uint32 i;
	
	info.buttons = 0;
	info.clicks = 0;
	for (i = 0; i < device->num_insns; i++) {
		const report_insn *insn = &device->insns [i];
		int32 value = 
			(((report [insn->byte_idx + 1] << 8) | 
			   report [insn->byte_idx]) >> insn->bit_pos) 
			& ((1 << insn->num_bits) - 1);

		if (insn->usage_page == USAGE_PAGE_BUTTON) {
			if ((insn->usage_id - 1) < MAX_BUTTONS) {
				info.buttons |= (value & 1) << (insn->usage_id - 1);
				info.clicks = 1;
			}	
		} else if (insn->usage_page == USAGE_PAGE_GENERIC_DESKTOP) {
			if (insn->is_phy_signed)
				value = sign_extend(value, insn->num_bits);

			switch (insn->usage_id) {
				case USAGE_ID_X:
					info.xdelta = value;
					break;
				case USAGE_ID_Y:
					info.ydelta = -value;
					break;
				case USAGE_ID_WHEEL:
					info.wheel_ydelta = -value;
					break;
			}
		}
	}

	info.modifiers = 0;
	info.timestamp = device->timestamp;

	ring_buffer_write(device->rbuf, (const uint8*)&info, sizeof(info));
	release_sem_etc(device->sem_cb, 1, B_DO_NOT_RESCHEDULE);
}


/*!
	callback: got a report, issue next request
*/
static void 
usb_callback(void *cookie, uint32 busStatus,
	void *data, uint32 actualLength)
{
	hid_device_info *device = cookie;
	status_t status;

	if (device == NULL)
		return;

	acquire_sem(device->sem_lock);
	device->actual_length = actualLength;
	device->bus_status = busStatus;	/* B_USB_STATUS_* */
	if (busStatus != B_USB_STATUS_SUCCESS) {
		/* request failed */
		release_sem(device->sem_lock);
		DPRINTF_ERR((MY_ID "bus status %d\n", (int)busStatus));
		if (busStatus == B_USB_STATUS_IRP_CANCELLED_BY_REQUEST) {
			/* cancelled: device is unplugged */
			return;
		}
#if 1
		status = usb->clear_feature(device->ept->handle, USB_FEATURE_ENDPOINT_HALT);
		if (status != B_OK)
			DPRINTF_ERR((MY_ID "clear_feature() error %d\n", (int)status));
#endif
	} else {
		/* got a report */
#if 0
		uint32 i;
		char linbuf [256];
		uint8 *buffer = device->buffer;

		for (i = 0; i < device->total_report_size; i++)
			sprintf (&linbuf[i*3], "%02X ", buffer [i]);
		DPRINTF_INFO ((MY_ID "input report: %s\n", linbuf));
#endif
		device->timestamp = system_time ();
		
		if (device->is_keyboard) {
			interpret_kb_buffer(device);
			memcpy(device->last_buffer, device->buffer, device->total_report_size);
		} else
			interpret_mouse_buffer(device);
		
		release_sem(device->sem_lock);
	}

	/* issue next request */

	status = usb->queue_interrupt(device->ept->handle, device->buffer,
		device->total_report_size, usb_callback, device);
	if (status != B_OK) {
		/* XXX probably endpoint stall */
		DPRINTF_ERR ((MY_ID "queue_interrupt() error %d\n", (int)status));
	}
}


//	#pragma mark - device hooks


static status_t
hid_device_added(const usb_device *dev, void **cookie)
{
	hid_device_info *device;
	const usb_device_descriptor *dev_desc;
	const usb_configuration_info *conf;
	const usb_interface_info *intf;
	status_t status;
	usb_hid_descriptor *hid_desc;
	uint8 *rep_desc = NULL;
	size_t desc_len, actual;
	decomp_item *items;
	size_t num_items;
	int fd, report_id;
	uint16 ifno;
	bool is_keyboard;

	assert(dev != NULL && cookie != NULL);
	DPRINTF_INFO ((MY_ID "device_added()\n"));

	dev_desc = usb->get_device_descriptor(dev);

	if (dev_desc->vendor_id == USB_VENDOR_WACOM) {
		DPRINTF_INFO ((MY_ID "vendor ID 0x%04X, product ID 0x%04X\n",
			dev_desc->vendor_id, dev_desc->product_id));
		return B_ERROR;
	}

	DPRINTF_INFO ((MY_ID "vendor ID 0x%04X, product ID 0x%04X\n",
		dev_desc->vendor_id, dev_desc->product_id));

	/* check interface class */

	if ((conf = usb->get_nth_configuration(dev, DEFAULT_CONFIGURATION)) == NULL) {
		DPRINTF_ERR ((MY_ID "cannot get default configuration\n"));
		return B_ERROR;
	}

	for (ifno = 0; ifno < conf->interface_count; ifno++) {
		/* This is C; I can use "class" :-> */
		int class, subclass, protocol;

		intf = conf->interface [ifno].active;
		class = intf->descr->interface_class;
		subclass = intf->descr->interface_subclass;
		protocol = intf->descr->interface_protocol;
		DPRINTF_INFO ((MY_ID "interface %d: class %d, subclass %d, protocol %d\n",
			ifno, class, subclass, protocol));
		if (class == USB_HID_DEVICE_CLASS 
			&& subclass == USB_HID_INTERFACE_BOOT_SUBCLASS)
			break;
	}

	if (ifno >= conf->interface_count) {
		DPRINTF_INFO ((MY_ID "Boot HID interface not found\n"));
		return B_ERROR;
	}

	/* read HID descriptor */

	desc_len = sizeof (usb_hid_descriptor);
	hid_desc = malloc (desc_len);
	status = usb->send_request(dev, 
		USB_REQTYPE_INTERFACE_IN | USB_REQTYPE_STANDARD,
		USB_REQUEST_GET_DESCRIPTOR,
		USB_HID_DESCRIPTOR_HID << 8, ifno, desc_len, 
		hid_desc, desc_len, &desc_len);
	DPRINTF_INFO ((MY_ID "get_hid_desc: status=%d, len=%d\n", 
		(int) status, (int)desc_len));
	if (status != B_OK)
		desc_len = 256;		/* XXX */

	/* read report descriptor */

	desc_len = hid_desc->descriptor_info[0].descriptor_length;
	free(hid_desc);
	rep_desc = malloc(desc_len);
	if (rep_desc == NULL)
		return B_NO_MEMORY;

	status = usb->send_request(dev,
		USB_REQTYPE_INTERFACE_IN | USB_REQTYPE_STANDARD,
		USB_REQUEST_GET_DESCRIPTOR,
		USB_HID_DESCRIPTOR_REPORT << 8, ifno, desc_len, 
		rep_desc, desc_len, &desc_len);
	DPRINTF_INFO((MY_ID "get_hid_rep_desc: status=%d, len=%d\n", 
		(int) status, (int)desc_len));
	if (status != B_OK) {
		free(rep_desc);
		return B_ERROR;
	}

	/* save report descriptor for troubleshooting */

	fd = open ("/tmp/rep_desc.bin", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd >= 0) {
		write(fd, rep_desc, desc_len);
		close(fd);
	}

	/* Generic Desktop : Keyboard or Mouse */
	
	if (memcmp(rep_desc, "\x05\x01\x09\x06", 4) != 0 &&
	    memcmp(rep_desc, "\x05\x01\x09\x02", 4) != 0) {
		DPRINTF_INFO((MY_ID "not a keyboard or a mouse %08lx\n", *(uint32*)rep_desc));
		free(rep_desc);
		return B_ERROR;
	}

	/* configuration */

	if ((status = usb->set_configuration (dev, conf)) != B_OK) {
		DPRINTF_ERR((MY_ID "set_configuration() failed %d\n", (int)status));
		free (rep_desc);
		return B_ERROR;
	}

	is_keyboard = memcmp(rep_desc, "\x05\x01\x09\x06", 4) == 0;

	if ((device = create_device(dev, intf, ifno, is_keyboard)) == NULL) {
		free(rep_desc);
		return B_ERROR;
	}

	/* decompose report descriptor */

	num_items = desc_len;	/* XXX */
	items = malloc(sizeof (decomp_item) * num_items);
	if (items == NULL) {
		// TODO: free device
		free(rep_desc);
		return B_ERROR;
	}

	decompose_report_descriptor(rep_desc, desc_len, items, &num_items);
	free(rep_desc);

	/* parse report descriptor */

	device->num_insns = num_items;	/* XXX */
	device->insns = malloc (sizeof (report_insn) * device->num_insns);
	assert (device->insns != NULL);
	parse_report_descriptor (items, num_items, device->insns, 
		&device->num_insns, &device->total_report_size, &report_id);
	free(items);
	realloc(device->insns, sizeof (report_insn) * device->num_insns);
	DPRINTF_INFO ((MY_ID "%d items, %d insns, %d bytes\n", 
		(int)num_items, (int)device->num_insns, (int)device->total_report_size));

	/* count axes, hats and buttons */

	/*count_controls (device->insns, device->num_insns,
		&device->num_axes, &device->num_hats, &device->num_buttons);
	DPRINTF_INFO ((MY_ID "%d axes, %d hats, %d buttons\n",
		device->num_axes, device->num_hats, device->num_buttons));*/

	/* get initial state */

	status = usb->send_request(dev,
		USB_REQTYPE_INTERFACE_IN | USB_REQTYPE_CLASS,
		USB_REQUEST_HID_GET_REPORT,
		0x0100 | report_id, ifno, device->total_report_size,
		device->buffer, device->total_report_size, &actual);
	if (status != B_OK)
		DPRINTF_ERR ((MY_ID "Get_Report failed %d\n", (int)status));
	device->timestamp = system_time ();

	DPRINTF_INFO ((MY_ID "%08lx %08lx %08lx\n", *(((uint32*)device->buffer)), *(((uint32*)device->buffer)+1), *(((uint32*)device->buffer)+2)));

	/* issue interrupt transfer */

	device->ept = &intf->endpoint[0];		/* interrupt IN */
	status = usb->queue_interrupt(device->ept->handle, device->buffer,
		device->total_report_size, usb_callback, device);
	if (status != B_OK) {
		DPRINTF_ERR ((MY_ID "queue_interrupt() error %d\n", (int)status));
		return B_ERROR;
	}

	/* create a port */

	add_device_info(device);

	*cookie = device;
	DPRINTF_INFO ((MY_ID "added %s\n", device->name));
	return B_OK;
}


static status_t
hid_device_removed(void *cookie)
{
	hid_device_info *device = cookie;

	assert (cookie != NULL);

	DPRINTF_INFO((MY_ID "device_removed(%s)\n", device->name));
	usb->cancel_queued_transfers (device->ept->handle);
	remove_device_info(device);

	if (device->open == 0) {
		if (device->insns != NULL)
			free(device->insns);
		remove_device(device);
	} else {
		DPRINTF_INFO((MY_ID "%s still open\n", device->name));
		device->active = false;
	}

	return B_OK;
}


static status_t 
hid_device_open(const char *name, uint32 flags,
	driver_cookie **out_cookie)
{
	driver_cookie *cookie;
	hid_device_info *device;

	assert (name != NULL);
	assert (out_cookie != NULL);
	DPRINTF_INFO ((MY_ID "open(%s)\n", name));

	if ((device = search_device_info (name)) == NULL)
		return B_ENTRY_NOT_FOUND;
	if ((cookie = malloc (sizeof (driver_cookie))) == NULL)
		return B_NO_MEMORY;

	acquire_sem(device->sem_lock);
	cookie->device = device;
	cookie->next = device->open_fds;
	device->open_fds = cookie;
	device->open++;
	release_sem(device->sem_lock);

	*out_cookie = cookie;
	DPRINTF_INFO ((MY_ID "device %s open (%d)\n", name, device->open));
	return B_OK;
}


static status_t 
hid_device_read(driver_cookie *cookie, off_t position,
	void *buf,	size_t *num_bytes)
{
	return B_ERROR;
}


static status_t 
hid_device_write(driver_cookie *cookie, off_t position,
	const void *buf, size_t *num_bytes)
{
	return B_ERROR;
}


static status_t 
hid_device_control(driver_cookie *cookie, uint32 op,
		void *arg, size_t len)
{
	status_t err = B_ERROR;
	hid_device_info *device;

	assert (cookie != NULL);
	device = cookie->device;
	assert (device != NULL);
	DPRINTF_INFO ((MY_ID "ioctl(0x%x)\n", (int)op));

	if (!device->active)
		return B_ERROR;		/* already unplugged */

	if (device->is_keyboard)
		switch (op) {
			case KB_READ:
	    		err = acquire_sem_etc(device->sem_cb, 1, B_CAN_INTERRUPT, 0LL);
	    		if (err != B_OK)
					return err;
				acquire_sem(device->sem_lock);
				ring_buffer_user_read(device->rbuf, arg, sizeof(raw_key_info));
				release_sem(device->sem_lock);
				return err;
				break;
	    
			case KB_SET_LEDS:
				set_leds(device, (uint8 *)arg);
				break; 
		} 
	else
		switch (op) {
			case MS_READ:
				err = acquire_sem_etc(device->sem_cb, 1, B_CAN_INTERRUPT, 0LL);
	    		if (err != B_OK)
					return err;
				acquire_sem(device->sem_lock);
				ring_buffer_user_read(device->rbuf, arg, sizeof(mouse_movement));
				release_sem(device->sem_lock);
				return err;
				break;
	    	case MS_NUM_EVENTS:
			{
				int32 count;
				get_sem_count(device->sem_cb, &count);
        		return count;
      			break;
			}
			default:
				/* not implemented */
				break;
		}

	return err;
}


static status_t 
hid_device_close(driver_cookie *cookie)
{
	hid_device_info *device;

	assert (cookie != NULL && cookie->device != NULL);
	device = cookie->device;
	DPRINTF_INFO ((MY_ID "close(%s)\n", device->name));

	/* detach the cookie from list */

	acquire_sem(device->sem_lock);
	if (device->open_fds == cookie)
		device->open_fds = cookie->next;
	else {
		driver_cookie *p;
		for (p = device->open_fds; p != NULL; p = p->next) {
			if (p->next == cookie) {
				p->next = cookie->next;
				break;
			}
		}
	}
	--device->open;
	release_sem(device->sem_lock);

	return B_OK;
}


static status_t 
hid_device_free(driver_cookie *cookie)
{
	hid_device_info *device;

	assert(cookie != NULL && cookie->device != NULL);
	device = cookie->device;
	DPRINTF_INFO((MY_ID "free(%s)\n", device->name));

	free(cookie);
	if (device->open > 0)
		DPRINTF_INFO((MY_ID "%d opens left\n", device->open));
	else if (!device->active) {
		DPRINTF_INFO((MY_ID "removed %s\n", device->name));
		if (device->insns != NULL)
			free (device->insns);
		remove_device (device);
	}

	return B_OK;
}


//	#pragma mark - driver API


status_t 
init_hardware(void)
{
	DPRINTF_INFO((MY_ID "init_hardware() " __DATE__ " " __TIME__ "\n"));
	return B_OK;
}


status_t 
init_driver(void)
{
	DPRINTF_INFO((MY_ID "init_driver() " __DATE__ " " __TIME__ "\n"));

	if (get_module(B_USB_MODULE_NAME, (module_info **)&usb) != B_OK)
		return B_ERROR;

	gDeviceListLock = create_sem(1, "dev_list_lock");
	if (gDeviceListLock < B_OK) {
		put_module(B_USB_MODULE_NAME);
		return gDeviceListLock;
	}

	usb->register_driver(kDriverName, sSupportedDevices, 
		SUPPORTED_DEVICES, NULL);
	usb->install_notify(kDriverName, &sNotifyHooks);
	DPRINTF_INFO((MY_ID "init_driver() OK\n"));

	return B_OK;
}


void
uninit_driver(void)
{
	DPRINTF_INFO((MY_ID "uninit_driver()\n"));
	usb->uninstall_notify(kDriverName);

	delete_sem(gDeviceListLock);
	put_module(B_USB_MODULE_NAME);
	free_device_names();
}

/*!
	device names are generated dynamically
*/
const char **
publish_devices(void)
{
	DPRINTF_INFO((MY_ID "publish_devices()\n"));

	if (gDeviceListChanged) {
		free_device_names();
		alloc_device_names();
		if (gDeviceNames != NULL)
			rebuild_device_names();
		gDeviceListChanged = false;
	}
	assert(gDeviceNames != NULL);
	return (const char **)gDeviceNames;
}


device_hooks *
find_device(const char *name)
{
	static device_hooks hooks = {
		(device_open_hook)hid_device_open,
		(device_close_hook)hid_device_close,
		(device_free_hook)hid_device_free,
		(device_control_hook)hid_device_control,
		(device_read_hook)hid_device_read,
		(device_write_hook)hid_device_write,
		NULL
	};

	assert(name != NULL);
	DPRINTF_INFO((MY_ID "find_device(%s)\n", name));

	if (search_device_info(name) == NULL)
		return NULL;

	return &hooks;
}

