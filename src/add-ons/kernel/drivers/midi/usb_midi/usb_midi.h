/*
 * midi usb driver
 * usb_midi.h
 *
 * Copyright 2009 Haiku Inc.  All rights reserved.
 * Distributed under tthe terms of the MIT Licence.
 *
 * Authors:
 *		Jérôme Duval
 *		Pete Goodeve, pete.goodeve@computer.org
 *
 *		Some portions of this code were originally derived from
 *		USB Joystick driver for BeOS R5
 *		Copyright 2000 (C) ITO, Takayuki
 *		All rights reserved
 *
 */


#include <Drivers.h>
#include <USB.h>
#include <usb/USB_midi.h>

#include "ring_buffer.h"

/* #define DEBUG 1 -- more convenient to define in the code file when needed */
#define DEBUG_ERR 1

#if DEBUG
	#define	DPRINTF_INFO(x)	dprintf x
#else
	#define DPRINTF_INFO(x) 
#endif
#if DEBUG_ERR
	#define	DPRINTF_ERR(x)	dprintf x
#else
	#define DPRINTF_ERR(x)
#endif

/* For local finer debugging control: */
#define	DPRINTF_INFOX(x)	dprintf x
#define DPRINTF_INFOZ(x) 

/* driver specific definitions */

#define	DRIVER_NAME	"usb_midi"

#define	MY_ID	"\033[34m" DRIVER_NAME ":\033[m "
#define	MY_ERR	"\033[31merror:\033[m "
#define	MY_WARN	"\033[31mwarning:\033[m "
#define	assert(x) \
	((x) ? 0 : dprintf(MY_ID "assertion failed at " __FILE__ ", line %d\n", __LINE__))

#define	DEFAULT_CONFIGURATION	0

struct driver_cookie;


typedef struct usbmidi_device_info
{
	/* list structure */
	struct usbmidi_device_info* next;

	/* maintain device */
	sem_id sem_cb;
	sem_id sem_lock;
	sem_id sem_send;
	area_id buffer_area;
	usb_midi_event_packet* buffer;	/* input buffer & base of area */
	usb_midi_event_packet* out_buffer;	/* above input buff */
	
	const usb_device* dev;
	uint16 ifno;
	char name[30];
	
	struct ring_buffer* rbuf;

	bool active;
	int open;
	struct driver_cookie* open_fds;

	/* work area for transfer */
	int usbd_status, bus_status, cmd_status;
	int actual_length;
	const usb_endpoint_info* ept_in;
	const usb_endpoint_info* ept_out;

	size_t buffer_size;
	bigtime_t timestamp;
	uint flags;
} usbmidi_device_info;


/* usb_midi.c */

extern usb_module_info* usb;
extern const char* usb_midi_base_name;

usbmidi_device_info* 
create_device(const usb_device* dev, const usb_interface_info* ii, uint16 ifno);

void 
remove_device(usbmidi_device_info* my_dev);


/* devlist.c */

extern sem_id usbmidi_device_list_lock;
extern bool usbmidi_device_list_changed;

void add_device_info(usbmidi_device_info* my_dev);
void remove_device_info(usbmidi_device_info* my_dev);
usbmidi_device_info* search_device_info(const char* name);

extern char** usbmidi_device_names;

void alloc_device_names(void);
void free_device_names(void);
void rebuild_device_names(void);
