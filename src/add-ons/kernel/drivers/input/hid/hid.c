/*****************************************************************************/
// HID usb driver
// Written by Jérôme Duval
//
// hid.c
//
// Copyright (c) 2004 Haiku Project
//
// 	Some portions of code are copyrighted by
//	USB Joystick driver for BeOS R5
//	Copyright 2000 (C) ITO, Takayuki
//	All rights reserved
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/
#include <support/Debug.h>
#include <stdlib.h>
#include <unistd.h>
#include "hid.h"
#include "kb_mouse_driver.h"

static int keyboard_device_number = 0;
static int mouse_device_number = 0;
const char *keyboard_base_name = "input/keyboard/usb/";
const char *mouse_base_name = "input/mouse/usb/";

my_device_info *
create_device(const usb_device *dev, const usb_interface_info *ii,
	uint16 ifno, bool is_keyboard)
{
	my_device_info *my_dev = NULL;
	int number;
	area_id area;
	sem_id sem;
	char	area_name [32];
	const char *base_name;
	
	assert (usb != NULL && dev != NULL);

	if (is_keyboard) {	
		number = keyboard_device_number++;
		base_name = keyboard_base_name;
	} else {
		number = mouse_device_number++;
		base_name = mouse_base_name;
	}
	
	my_dev = malloc (sizeof (my_device_info));
	if (my_dev == NULL)
		return NULL;

	my_dev->sem_cb = sem = create_sem (0, DRIVER_NAME "_cb");
	if (sem < 0) {
		DPRINTF_ERR ((MY_ID "create_sem() failed %d\n", (int) sem));
		free (my_dev);
		return NULL;
	}

	my_dev->sem_lock = sem = create_sem (1, DRIVER_NAME "_lock");
	if (sem < 0) {
		DPRINTF_ERR ((MY_ID "create_sem() failed %d\n", (int) sem));
		delete_sem (my_dev->sem_cb);
		free (my_dev);
		return NULL;
	}

	sprintf (area_name, DRIVER_NAME "_buffer%d", number);
	my_dev->buffer_area = area = create_area (area_name,
		(void **) &my_dev->buffer, B_ANY_KERNEL_ADDRESS,
		B_PAGE_SIZE, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if (area < 0) {
		DPRINTF_ERR ((MY_ID "create_area() failed %d\n", (int) area));
		delete_sem (my_dev->sem_cb);
		delete_sem (my_dev->sem_lock);
		free (my_dev);
		return NULL;
	}

	sprintf(my_dev->name, "%s%d", base_name, number);
	my_dev->dev = dev;
	my_dev->ifno = ifno;
	my_dev->open = 0;
	my_dev->open_fds = NULL;
	my_dev->active = true;
	my_dev->insns = NULL;
	my_dev->num_insns = 0;
	my_dev->flags = 0;
	my_dev->cbuf = cbuf_init(384);
	my_dev->is_keyboard = is_keyboard;
	  	
	return my_dev;
}

void 
remove_device (my_device_info *my_dev)
{
	assert (my_dev != NULL);
	if (my_dev->cbuf != NULL) {
		cbuf_delete(my_dev->cbuf);
		my_dev->cbuf = NULL;
	}
  
	delete_area (my_dev->buffer_area);
	delete_sem (my_dev->sem_cb);
	delete_sem (my_dev->sem_lock);
	free (my_dev);
}


/* gameport driver cookie (per open) */

typedef struct driver_cookie
{
	struct driver_cookie	*next;

	my_device_info			*my_dev;
	bool					enhanced;

} driver_cookie;
	
/* NB global variables are valid only while driver is loaded */

_EXPORT int32	api_version = B_CUR_DRIVER_API_VERSION;

const char *hid_driver_name = "hid";

usb_module_info *usb;

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

// here we don't need to follow a report descriptor for typical keyboards
// TODO : but we should for keypads for example because they aren't boot keyboard devices !
// see hidparse.c
static void
interpret_kb_buffer(my_device_info *my_dev)
{
	raw_key_info info;
	uint8 modifiers = ((uint8*)my_dev->buffer)[0];
	uint8 bits = my_dev->last_buffer[0] ^ modifiers;
	uint32 i,j;
	
	info.timestamp = my_dev->timestamp;
	
	if (bits)
		for (j=0; bits; j++,bits>>=1)
			if (bits & 1) {
				info.be_keycode = modifier_table[j];				
				info.is_keydown = (modifiers >> j) & 1;			
				cbuf_putn(my_dev->cbuf, &info, sizeof(info));
				release_sem_etc(my_dev->sem_cb, 1, B_DO_NOT_RESCHEDULE);
			}

	for (i=2; i<my_dev->total_report_size; i++)
		if (((uint8*)my_dev->buffer)[i] && ((uint8*)my_dev->last_buffer)[i]!=0x1) {
			bool found = false;
			for (j=2; j<my_dev->total_report_size; j++)
				if (((uint8*)my_dev->last_buffer)[j] 
					&& ((uint8*)my_dev->last_buffer)[j] == ((uint8*)my_dev->buffer)[i]) {
					found = true;
					break;
			}
			
			if (!found) {
				info.be_keycode = key_table[((uint8*)my_dev->buffer)[i]];
				if (info.be_keycode == KEY_Pause && modifiers & 1)
					info.be_keycode = KEY_Break;			
				if (info.be_keycode == 0xe && modifiers & 1)
					info.be_keycode = KEY_SysRq;			
				info.is_keydown = 1;			
				cbuf_putn(my_dev->cbuf, &info, sizeof(info));
				release_sem_etc(my_dev->sem_cb, 1, B_DO_NOT_RESCHEDULE);
			}			
		} else
			break;
	
	for (i=2; i<my_dev->total_report_size; i++)
		if (((uint8*)my_dev->last_buffer)[i] && ((uint8*)my_dev->last_buffer)[i]!=0x1) {
			bool found = false;
			for (j=2; j<my_dev->total_report_size; j++)
				if (((uint8*)my_dev->buffer)[j] 
					&& ((uint8*)my_dev->buffer)[j] == ((uint8*)my_dev->last_buffer)[i]) {
					found = true;
					break;
			}
			
			if (!found) {
				info.be_keycode = key_table[((uint8*)my_dev->last_buffer)[i]];				
				if (info.be_keycode == KEY_Pause && modifiers & 1)
					info.be_keycode = KEY_Break;			
				if (info.be_keycode == 0xe && modifiers & 1)
					info.be_keycode = KEY_SysRq;			
				info.is_keydown = 0;			
				cbuf_putn(my_dev->cbuf, &info, sizeof(info));
				release_sem_etc(my_dev->sem_cb, 1, B_DO_NOT_RESCHEDULE);
			}			
		} else
			break;
			
}


// TODO : here we don't follow a report descriptor but we actually should
// see hidparse.c
static void
set_leds(my_device_info *my_dev, uint8* data)
{
	status_t st;
	size_t actual = 1;
	uint8 leds = 0;
	int report_id = 0;
	if (data[0] == 1)
		leds |= (1 << 0); 
	if (data[1] == 1)
		leds |= (1 << 1); 
	if (data[2] == 1)
		leds |= (1 << 2);
	
	st = usb->send_request (my_dev->dev, 
		USB_REQTYPE_INTERFACE_OUT | USB_REQTYPE_CLASS,
		0x09,
		0x200 | report_id, my_dev->ifno, actual, 
		&leds, actual, &actual);
	DPRINTF_INFO ((MY_ID "set_leds: leds=0x%02x, st=%d, len=%d\n", 
		leds, (int) st, (int)actual));
}


/*
	callback: got a report, issue next request
*/

static void 
usb_callback(void *cookie, uint32 status, 
	void *data, uint32 actual_len)
{
	status_t st;
	my_device_info *my_dev = cookie;

	assert (cookie != NULL);

	acquire_sem (my_dev->sem_lock);
	my_dev->actual_length = actual_len;
	my_dev->bus_status = status;	/* B_USB_STATUS_* */
	if (status != B_USB_STATUS_SUCCESS) {
		/* request failed */
		release_sem (my_dev->sem_lock);
		DPRINTF_ERR ((MY_ID "bus status %d\n", (int)status));
		if (status == B_USB_STATUS_IRP_CANCELLED_BY_REQUEST) {
			/* cancelled: device is unplugged */
			return;
		}
#if 1
		st = usb->clear_feature (my_dev->ept->handle, USB_FEATURE_ENDPOINT_HALT);
		if (st != B_OK)
			DPRINTF_ERR ((MY_ID "clear_feature() error %d\n", (int)st));
#endif
	} else {
		/* got a report */
#if 0
		uint32 i;
		char linbuf [256];
		uint8 *buffer = my_dev->buffer;

		for (i = 0; i < my_dev->total_report_size; i++)
			sprintf (&linbuf[i*3], "%02X ", buffer [i]);
		DPRINTF_INFO ((MY_ID "input report: %s\n", linbuf));
#endif
		my_dev->timestamp = system_time ();
		
		if (my_dev->is_keyboard)
			interpret_kb_buffer(my_dev);
		
		memcpy(my_dev->last_buffer, my_dev->buffer, my_dev->total_report_size);
		
		release_sem (my_dev->sem_lock);
	}

	/* issue next request */

	st = usb->queue_interrupt (my_dev->ept->handle, my_dev->buffer,
		my_dev->total_report_size, usb_callback, my_dev);
	if (st != B_OK) {
		/* XXX probably endpoint stall */
		DPRINTF_ERR ((MY_ID "queue_interrupt() error %d\n", (int)st));
	}
}

/*
	USB specific device hooks
*/

static status_t 
kb_device_added(const usb_device *dev, void **cookie)
{
	my_device_info *my_dev;
	const usb_device_descriptor *dev_desc;
	const usb_configuration_info *conf;
	const usb_interface_info *intf;
	status_t st;
	usb_hid_descriptor *hid_desc;
	uint8 *rep_desc = NULL;
	size_t desc_len, actual;
	decomp_item *items;
	size_t num_items;
	int fd, report_id;
	uint16 ifno;
	bool is_keyboard;

	assert (dev != NULL && cookie != NULL);
	DPRINTF_INFO ((MY_ID "device_added()\n"));

	dev_desc = usb->get_device_descriptor (dev);
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
		class    = intf->descr->interface_class;
		subclass = intf->descr->interface_subclass;
		protocol = intf->descr->interface_protocol;
		DPRINTF_INFO ((MY_ID "interface %d: class %d, subclass %d, protocol %d\n",
			ifno, class, subclass, protocol));
		if (class == USB_CLASS_HID && subclass == 1)
			break;
	}

	if (ifno >= conf->interface_count) {
		DPRINTF_INFO ((MY_ID "Boot HID interface not found\n"));
		return B_ERROR;
	}

	/* read HID descriptor */

	desc_len = sizeof (usb_hid_descriptor);
	hid_desc = malloc (desc_len);
	st = usb->send_request (dev, 
		USB_REQTYPE_INTERFACE_IN | USB_REQTYPE_STANDARD,
		USB_REQUEST_GET_DESCRIPTOR,
		USB_DESCRIPTOR_HID << 8, ifno, desc_len, 
		hid_desc, desc_len, &desc_len);
	DPRINTF_INFO ((MY_ID "get_hid_desc: st=%d, len=%d\n", 
		(int) st, (int)desc_len));
	if (st != B_OK)
		desc_len = 256;		/* XXX */

	/* read report descriptor */

	desc_len = hid_desc->descriptor_info [0].descriptor_length;
	free (hid_desc);
	rep_desc = malloc (desc_len);
	assert (rep_desc != NULL);
	st = usb->send_request (dev, 
		USB_REQTYPE_INTERFACE_IN | USB_REQTYPE_STANDARD,
		USB_REQUEST_GET_DESCRIPTOR,
		USB_DESCRIPTOR_HID_REPORT << 8, ifno, desc_len, 
		rep_desc, desc_len, &desc_len);
	DPRINTF_INFO ((MY_ID "get_hid_rep_desc: st=%d, len=%d\n", 
		(int) st, (int)desc_len));
	if (st != B_OK) {
		free (rep_desc);
		return B_ERROR;
	}

	/* save report descriptor for troubleshooting */

	fd = open ("/tmp/rep_desc.bin", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd >= 0) {
		write (fd, rep_desc, desc_len);
		close (fd);
	}

	/* XXX check application type */
	/* Generic Desktop : Keyboard or Mouse */
	
	if (memcmp (rep_desc, "\x05\x01\x09\x06", 4) != 0 &&
	    memcmp (rep_desc, "\x05\x01\x09\x02", 4) != 0) {
		DPRINTF_INFO ((MY_ID "not a keyboard or a mouse %08lx\n", *(uint32*)rep_desc));
		free (rep_desc);
		return B_ERROR;
	}

	/* configuration */

	if ((st = usb->set_configuration (dev, conf)) != B_OK) {
		DPRINTF_ERR ((MY_ID "set_configuration() failed %d\n", (int)st));
		free (rep_desc);
		return B_ERROR;
	}

	is_keyboard = memcmp (rep_desc, "\x05\x01\x09\x06", 4) == 0;

	if ((my_dev = create_device (dev, intf, ifno, is_keyboard)) == NULL) {
		free (rep_desc);
		return B_ERROR;
	}

	/* decompose report descriptor */

	num_items = desc_len;	/* XXX */
	items = malloc (sizeof (decomp_item) * num_items);
	assert (items != NULL);
	decompose_report_descriptor (rep_desc, desc_len, items, &num_items);
	free (rep_desc);

	/* parse report descriptor */

	my_dev->num_insns = num_items;	/* XXX */
	my_dev->insns = malloc (sizeof (report_insn) * my_dev->num_insns);
	assert (my_dev->insns != NULL);
	parse_report_descriptor (items, num_items, my_dev->insns, 
		&my_dev->num_insns, &my_dev->total_report_size, &report_id);
	free (items);
	realloc (my_dev->insns, sizeof (report_insn) * my_dev->num_insns);
	DPRINTF_INFO ((MY_ID "%d items, %d insns, %d bytes\n", 
		(int)num_items, (int)my_dev->num_insns, (int)my_dev->total_report_size));

	/* count axes, hats and buttons */

	count_controls (my_dev->insns, my_dev->num_insns,
		&my_dev->num_axes, &my_dev->num_hats, &my_dev->num_buttons);
	DPRINTF_INFO ((MY_ID "%d axes, %d hats, %d buttons\n",
		my_dev->num_axes, my_dev->num_hats, my_dev->num_buttons));

	/* get initial state */
	
	
	st = usb->send_request (dev,
		USB_REQTYPE_INTERFACE_IN | USB_REQTYPE_CLASS,
		USB_REQUEST_HID_GET_REPORT,
		0x0100 | report_id, ifno, my_dev->total_report_size,
		my_dev->buffer, my_dev->total_report_size, &actual);
	if (st != B_OK)
		DPRINTF_ERR ((MY_ID "Get_Report failed %d\n", (int)st));
	my_dev->timestamp = system_time ();

	DPRINTF_INFO ((MY_ID "%08lx %08lx %08lx\n", *(((uint32*)my_dev->buffer)), *(((uint32*)my_dev->buffer)+1), *(((uint32*)my_dev->buffer)+2)));
	
	/* issue interrupt transfer */

	my_dev->ept = &intf->endpoint [0];		/* interrupt IN */
	st = usb->queue_interrupt (my_dev->ept->handle, my_dev->buffer,
		my_dev->total_report_size, usb_callback, my_dev);
	if (st != B_OK) {
		DPRINTF_ERR ((MY_ID "queue_interrupt() error %d\n", (int)st));
		return B_ERROR;
	}

	/* create a port */

	add_device_info (my_dev);

	*cookie = my_dev;
	DPRINTF_INFO ((MY_ID "added %s\n", my_dev->name));

	return B_OK;
}

static status_t 
kb_device_removed (void *cookie)
{
	my_device_info *my_dev = cookie;

	assert (cookie != NULL);

	DPRINTF_INFO ((MY_ID "device_removed(%s)\n", my_dev->name));
	usb->cancel_queued_transfers (my_dev->ept->handle);
	remove_device_info (my_dev);
	if (my_dev->open == 0) {
		if (my_dev->insns != NULL)
			free (my_dev->insns);
		remove_device (my_dev);
	} else {
		DPRINTF_INFO ((MY_ID "%s still open\n", my_dev->name));
		my_dev->active = false;
	}
	return B_OK;
}

static usb_notify_hooks my_notify_hooks =
{
	kb_device_added, kb_device_removed
};

#define	SUPPORTED_DEVICES	1
usb_support_descriptor my_supported_devices [SUPPORTED_DEVICES] =
{
	{ USB_CLASS_HID, 0, 0, 0, 0 },
};


/* ----------
	my_device_open - handle open() calls
----- */

static status_t 
my_device_open(const char *name, uint32 flags,
	driver_cookie **out_cookie)
{
	driver_cookie *cookie;
	my_device_info *my_dev;

	assert (name != NULL);
	assert (out_cookie != NULL);
	DPRINTF_INFO ((MY_ID "open(%s)\n", name));

	if ((my_dev = search_device_info (name)) == NULL)
		return B_ENTRY_NOT_FOUND;
	if ((cookie = malloc (sizeof (driver_cookie))) == NULL)
		return B_NO_MEMORY;

	acquire_sem (my_dev->sem_lock);
	cookie->enhanced = false;
	cookie->my_dev = my_dev;
	cookie->next = my_dev->open_fds;
	my_dev->open_fds = cookie;
	my_dev->open++;
	release_sem (my_dev->sem_lock);

	*out_cookie = cookie;
	DPRINTF_INFO ((MY_ID "device %s open (%d)\n", name, my_dev->open));
	return B_OK;
}


/* ----------
	my_device_read - handle read() calls
----- */

static status_t 
my_device_read(driver_cookie *cookie, off_t position,
	void *buf,	size_t *num_bytes)
{
	return B_ERROR;
}


/* ----------
	my_device_write - handle write() calls
----- */

static status_t 
my_device_write(driver_cookie *cookie, off_t position,
	const void *buf, size_t *num_bytes)
{
	return B_ERROR;
}

/* ----------
	my_device_control - handle ioctl calls
----- */

static status_t 
my_device_control(driver_cookie *cookie, uint32 op,
		void *arg, size_t len)
{
	status_t err = B_ERROR;
	my_device_info *my_dev;

	assert (cookie != NULL);
	my_dev = cookie->my_dev;
	assert (my_dev != NULL);
	DPRINTF_INFO ((MY_ID "ioctl(0x%x)\n", (int)op));

	if (!my_dev->active)
		return B_ERROR;		/* already unplugged */

	if (my_dev->is_keyboard) {
		switch (op) {
			case 0x270f:
	    		err = acquire_sem_etc(my_dev->sem_cb, 1, B_CAN_INTERRUPT, 0LL);
	    		if (err != B_OK)
					return err;
				cbuf_getn(my_dev->cbuf, arg, sizeof(raw_key_info));
				return err;
				break;
	    
			case 0x2711:
				set_leds(my_dev, (uint8 *)arg);
				break; 
	
			default:
				/* not implemented */
				break;
		}

	} else {
	
	}
	return err;
}


/* ----------
	my_device_close - handle close() calls
----- */

static status_t 
my_device_close(driver_cookie *cookie)
{
	my_device_info *my_dev;

	assert (cookie != NULL && cookie->my_dev != NULL);
	my_dev = cookie->my_dev;
	DPRINTF_INFO ((MY_ID "close(%s)\n", my_dev->name));

	/* detach the cookie from list */

	acquire_sem (my_dev->sem_lock);
	if (my_dev->open_fds == cookie)
		my_dev->open_fds = cookie->next;
	else {
		driver_cookie *p;
		for (p = my_dev->open_fds; p != NULL; p = p->next) {
			if (p->next == cookie) {
				p->next = cookie->next;
				break;
			}
		}
	}
	--my_dev->open;
	release_sem (my_dev->sem_lock);

	return B_OK;
}


/* -----
	my_device_free - called after the last device is closed, and after
	all i/o is complete.
----- */
static status_t 
my_device_free(driver_cookie *cookie)
{
	my_device_info *my_dev;

	assert (cookie != NULL && cookie->my_dev != NULL);
	my_dev = cookie->my_dev;
	DPRINTF_INFO ((MY_ID "free(%s)\n", my_dev->name));

	free (cookie);
	if (my_dev->open > 0)
		DPRINTF_INFO ((MY_ID "%d opens left\n", my_dev->open));
	else if (!my_dev->active) {
		DPRINTF_INFO ((MY_ID "removed %s\n", my_dev->name));
		if (my_dev->insns != NULL)
			free (my_dev->insns);
		remove_device (my_dev);
	}

	return B_OK;
}


/* -----
	function pointers for the device hooks entry points
----- */

static device_hooks my_device_hooks = {
	(device_open_hook)    my_device_open,
	(device_close_hook)   my_device_close,
	(device_free_hook)    my_device_free,
	(device_control_hook) my_device_control,
	(device_read_hook)    my_device_read,
	(device_write_hook)   my_device_write,
	NULL, NULL, NULL, NULL
};


/* ----------
	init_hardware - called once the first time the driver is loaded
----- */
_EXPORT status_t 
init_hardware (void)
{
	DPRINTF_INFO ((MY_ID "init_hardware() " __DATE__ " " __TIME__ "\n"));
	return B_OK;
}

/* ----------
	init_driver - optional function - called every time the driver
	is loaded.
----- */
_EXPORT status_t 
init_driver (void)
{

	DPRINTF_INFO ((MY_ID "init_driver() " __DATE__ " " __TIME__ "\n"));

	if (get_module (B_USB_MODULE_NAME, (module_info **) &usb) != B_OK)
		return B_ERROR;

	if ((my_device_list_lock = create_sem (1, "dev_list_lock")) < 0) {
		put_module (B_USB_MODULE_NAME);
		return my_device_list_lock;		/* error code */
	}

	usb->register_driver (hid_driver_name, my_supported_devices, 
		SUPPORTED_DEVICES, NULL);
	usb->install_notify (hid_driver_name, &my_notify_hooks);
	DPRINTF_INFO ((MY_ID "init_driver() OK\n"));

	return B_OK;
}


/* ----------
	uninit_driver - optional function - called every time the driver
	is unloaded
----- */
_EXPORT void 
uninit_driver (void)
{
	DPRINTF_INFO ((MY_ID "uninit_driver()\n"));
	usb->uninstall_notify (hid_driver_name);
	
	delete_sem (my_device_list_lock);
	put_module (B_USB_MODULE_NAME);
	free_device_names ();
}

/*
	publish_devices
	device names are generated dynamically
*/

_EXPORT const char **
publish_devices (void)
{
	DPRINTF_INFO ((MY_ID "publish_devices()\n"));

	if (my_device_list_changed) {
		free_device_names ();
		alloc_device_names ();
		if (my_device_names != NULL)
			rebuild_device_names ();
		my_device_list_changed = false;
	}
	assert (my_device_names != NULL);
	return (const char **) my_device_names;
}

/* ----------
	find_device - return ptr to device hooks structure for a
	given device name
----- */

_EXPORT device_hooks *
find_device(const char *name)
{
	assert (name != NULL);
	DPRINTF_INFO ((MY_ID "find_device(%s)\n", name));
	if (search_device_info(name) == NULL)
		return NULL;
	return &my_device_hooks;
}

