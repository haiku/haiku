/*
 * midi usb driver
 * usb_midi.h
 *
 * Copyright 2006-2011 Haiku Inc.  All rights reserved.
 * Distributed under the terms of the MIT Licence.
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

/* Three levels of printout for convenience: */
/* #define DEBUG 1 -- more convenient to define in the code file when needed */
#define DEBUG_INFO 1
#define DEBUG_ERR 1

/* Normally leave this enabled to leave a record in syslog */
#if DEBUG_ERR
	#define	DPRINTF_ERR(x)	dprintf x
#else
	#define DPRINTF_ERR(x)
#endif

/* Use this for initialization etc. messages -- nothing repetitive: */
#if DEBUG_INFO
	#define	DPRINTF_INFO(x)	dprintf x
#else
	#define DPRINTF_INFO(x)
#endif

/* Enable this to record detailed stuff: */
#if DEBUG
	#define	DPRINTF_DEBUG(x)	dprintf x
#else
	#define DPRINTF_DEBUG(x)
#endif


/* driver specific definitions */

#define	DRIVER_NAME	"usb_midi"

#define	MY_ID	"\033[34m" DRIVER_NAME ":\033[m "
#define	MY_ERR	"\033[31merror:\033[m "
#define	MY_WARN	"\033[31mwarning:\033[m "
#define	assert(x) \
	((x) ? 0 : dprintf(MY_ID "assertion failed at " \
	 __FILE__ ", line %d\n", __LINE__))

#define	DEFAULT_CONFIGURATION	0

struct driver_cookie;


typedef struct usbmidi_device_info
{
	/* list structure */ /* should not be needed eventually */
	struct usbmidi_device_info* next;

	/* Set of actual ports ("cables" -- one or more) */
	struct usbmidi_port_info* ports[16];

	/* maintain device  (common for all ports) */
	sem_id sem_lock;
	sem_id sem_send;
	area_id buffer_area;
	usb_midi_event_packet* buffer;	/* input buffer & base of area */
	usb_midi_event_packet* out_buffer;	/* above input buffer */
	size_t buffer_size;		/* for each of in and out buffers */

	const usb_device* dev;
	uint16 ifno;
	int devnum;	/* unique device number */
	char name[20];
		/* = "/dev/midi/usb/n" --port number will be appended to this */

	bool active;

	/* work area for transfer */
	int bus_status;
	int actual_length;
	const usb_endpoint_info* ept_in;
	const usb_endpoint_info* ept_out;

	bigtime_t timestamp;	/* Is this needed? Currently set but never read */
	uint flags;				/* set to 0 but never used */
} usbmidi_device_info;


typedef struct usbmidi_port_info
{
	/* list structure for manager */
	struct usbmidi_port_info* next;

	/* Common device that does the work */
	usbmidi_device_info* device;

	/* Port-specific variables */
	char name[40];	/* complete pathname of this port */
	struct ring_buffer* rbuf;

	int cable;	/* index of this port */
	bool has_in, has_out;

	int open;
	struct driver_cookie* open_fd;

} usbmidi_port_info;


/*
 usb_midi.c
*/

extern usb_module_info* usb;
extern const char* usb_midi_base_name;

extern usbmidi_port_info* create_usbmidi_port(usbmidi_device_info* devinfo,
	int cable, bool has_in, bool has_out);
extern void remove_port(usbmidi_port_info* port);

extern usbmidi_device_info* create_device(const usb_device* dev, uint16 ifno);
extern void remove_device(usbmidi_device_info* my_dev);


/*
 devlist.c
*/

extern sem_id usbmidi_port_list_lock;
extern bool usbmidi_port_list_changed;

extern void add_port_info(usbmidi_port_info* port);
extern void remove_port_info(usbmidi_port_info* port);

extern usbmidi_port_info* search_port_info(const char* name);

extern int find_free_device_number(void);

extern char** usbmidi_port_names;

extern void alloc_port_names(void);
extern void free_port_names(void);
extern void rebuild_port_names(void);
