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
#ifndef _HID_H_
#define _HID_H_

#include "hidparse.h"
#include "ring_buffer.h"

#include <Drivers.h>
#include <USB.h>
#include <usb/USB_hid.h>

#if DEBUG
	#define	DPRINTF_INFO(x)	dprintf x
	#define	DPRINTF_ERR(x)	dprintf x
#else
	#define DPRINTF_INFO(x)  
	#define DPRINTF_ERR(x)	dprintf x
#endif

/* driver specific definitions */

#define	DRIVER_NAME	"usb_hid"

#define	MY_ID	"\033[34m" DRIVER_NAME ":\033[m "
#define	MY_ERR	"\033[31merror:\033[m "
#define	MY_WARN	"\033[31mwarning:\033[m "
#define	assert(x) \
	((x) ? 0 : dprintf (MY_ID "assertion failed at " __FILE__ ", line %d\n", __LINE__))

/* 0-origin */
#define	DEFAULT_CONFIGURATION	0

#define	BUF_SIZ	B_PAGE_SIZE

struct driver_cookie;

struct hid_repeat_timer {
	struct timer timer;
	struct hid_device_info *device;
	uint32 key;
};

typedef struct hid_device_info {
	/* list structure */
	struct hid_device_info *next;

	/* maintain device */
	sem_id sem_cb;
	sem_id sem_lock;
	area_id buffer_area;
	void *buffer;
	
	uint8 last_buffer[32];
	const usb_device *dev;
	uint16 ifno;
	char name[30];
	
	struct ring_buffer *rbuf;

	bool active;
	int open;
	struct driver_cookie *open_fds;

	/* workarea for transfer */
	int usbd_status, bus_status, cmd_status;
	int actual_length;
	const usb_endpoint_info *ept;

	report_insn	*insns;
	size_t num_insns;
	size_t total_report_size;
	int num_buttons, num_axes, num_hats;
	bigtime_t timestamp;
	uint flags;
	bool is_keyboard;

	struct hid_repeat_timer repeat_timer;
	bigtime_t repeat_delay;
	bigtime_t repeat_rate;
} hid_device_info;

/* hid.c */

extern usb_module_info *usb;
extern const char *my_driver_name;
extern const char *keyboard_base_name;
extern const char *mouse_base_name;

hid_device_info *create_device(const usb_device *dev, const usb_interface_info *ii,
	uint16 ifno, bool is_keyboard);
void remove_device(hid_device_info *device);

/* devlist.c */

extern sem_id gDeviceListLock;
extern bool gDeviceListChanged;

void add_device_info(hid_device_info *device);
void remove_device_info(hid_device_info *device);
hid_device_info *search_device_info(const char *name);

extern char **gDeviceNames;

void alloc_device_names(void);
void free_device_names(void);
void rebuild_device_names(void);

#endif	// _HID_H_
