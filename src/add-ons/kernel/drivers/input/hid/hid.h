/*****************************************************************************/
// HID usb driver
// Written by Jérôme Duval
//
// hid.h
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

#include <drivers/Drivers.h>
#include <drivers/USB.h>
#include "hidparse.h"

#if DEBUG
	#define	DPRINTF_INFO(x)	dprintf x
	#define	DPRINTF_ERR(x)	dprintf x
#else
	#define DPRINTF_INFO(x)  
	#define DPRINTF_ERR(x)	dprintf x
#endif


/* Undocumented kernel cbuf_* functions */
struct cbuffer_t;
typedef struct cbuffer_t cbuffer;

size_t cbuf_getn_no_lock(cbuffer *, char *, size_t);
size_t cbuf_putn_no_lock(cbuffer *, char *, size_t);
cbuffer *cbuf_init(size_t size);
void cbuf_delete(cbuffer *buffer);
char cbuf_get(cbuffer *);
bool cbuf_mt(cbuffer *);
bool cbuf_full(cbuffer *);
status_t cbuf_put(cbuffer *, char);
status_t cbuf_unput(cbuffer *);
void cbuf_flush(cbuffer *);
size_t cbuf_size(cbuffer *);
size_t cbuf_avail(cbuffer *);
size_t cbuf_free(cbuffer *);
size_t cbuf_putn(cbuffer *, void *, size_t num_bytes);
size_t cbuf_getn(cbuffer *, void *, size_t num_bytes);
cpu_status cbuf_lock(cbuffer *);
void cbuf_unlock(cbuffer *, cpu_status);

/* HID class-specific definitions */

#define	USB_CLASS_HID	3

#define	USB_DESCRIPTOR_HID			0x21
#define	USB_DESCRIPTOR_HID_REPORT	0x22

#define	USB_REQUEST_HID_GET_REPORT	0x01

typedef struct
{
	uint8	length;
	uint8	descriptor_type;
	uint16	hid_version;
	uint8	country_code;
	uint8	num_descriptors;
	struct
	{
		uint8	descriptor_type;
		uint16	descriptor_length;
	} _PACKED descriptor_info [1];
} _PACKED usb_hid_descriptor;

/* driver specific definitions */

#define	DRIVER_NAME	"hid"

#define	MY_ID	"\033[34m" DRIVER_NAME ":\033[m "
#define	MY_ERR	"\033[31merror:\033[m "
#define	MY_WARN	"\033[31mwarning:\033[m "
#define	assert(x) \
	((x) ? 0 : dprintf (MY_ID "assertion failed at " __FILE__ ", line %d\n", __LINE__))

/* 0-origin */
#define	DEFAULT_CONFIGURATION	0

#define	BUF_SIZ	B_PAGE_SIZE

struct driver_cookie;

typedef struct my_device_info
{
	/* list structure */
	struct my_device_info *next;

	/* maintain device */
	sem_id sem_cb;
	sem_id sem_lock;
	area_id buffer_area;
	void *buffer;
	
	uint8 last_buffer[32];
	const usb_device *dev;
	char name[30];
	
	cbuffer *cbuf;

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
} my_device_info;

/* driver.c */

extern usb_module_info *usb;
extern const char *my_driver_name;
extern const char *keyboard_base_name;
extern const char *mouse_base_name;

/* devmgmt.c */

my_device_info *
create_device (const usb_device *dev, const usb_interface_info *ii, bool is_keyboard);

void 
remove_device (my_device_info *my_dev);

/* devlist.c */

extern sem_id my_device_list_lock;
extern bool my_device_list_changed;

void add_device_info (my_device_info *my_dev);
void remove_device_info (my_device_info *my_dev);
my_device_info *search_device_info (const char *name);

extern char **my_device_names;

void alloc_device_names (void);
void free_device_names (void);
void rebuild_device_names (void);
