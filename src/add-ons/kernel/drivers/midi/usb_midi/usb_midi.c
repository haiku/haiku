/*
 * midi usb driver
 * usb_midi.c
 *
 * Copyright 2006-2009 Haiku Inc.  All rights reserved.
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


/* #define DEBUG 1 */	/* Define this to enable DPRINTF_INFO statements */
#include "usb_midi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "usbdevs.h"
#include "usbdevs_data.h"


static int midi_device_number = 0;
const char* midi_base_name = "midi/usb/";

usbmidi_device_info* 
create_device(const usb_device* dev, const usb_interface_info* ii, uint16 ifno)
{
	usbmidi_device_info* my_dev = NULL;
	int number;
	area_id area;
	sem_id sem;
	char	area_name[32];
	const char* base_name;
	
	assert(usb != NULL && dev != NULL);

	number = midi_device_number++;
	base_name = midi_base_name;
		
	my_dev = malloc(sizeof(usbmidi_device_info));
	if (my_dev == NULL)
		return NULL;

	my_dev->sem_lock = sem = create_sem(1, DRIVER_NAME "_lock");
	if (sem < 0) {
		DPRINTF_ERR((MY_ID "create_sem() failed %d\n", (int)sem));
		free(my_dev);
		return NULL;
	}

	sprintf(area_name, DRIVER_NAME "_buffer%d", number);
	my_dev->buffer_area = area = create_area(area_name,
		(void**)&my_dev->buffer, B_ANY_KERNEL_ADDRESS,
		B_PAGE_SIZE, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if (area < 0) {
		DPRINTF_ERR((MY_ID "create_area() failed %d\n", (int)area));
		delete_sem(my_dev->sem_lock);
		free(my_dev);
		return NULL;
	}
	/* use half of reserved area for each of in and out buffers: */
	my_dev->out_buffer = (usb_midi_event_packet*)((uint8*)my_dev->buffer + B_PAGE_SIZE/2);
	my_dev->sem_send =  sem = create_sem(1, DRIVER_NAME "_send");
	if (sem < 0) {
		DPRINTF_ERR((MY_ID "create_sem() failed %d\n", (int)sem));
		delete_sem(my_dev->sem_lock);
		delete_area(area);
		free(my_dev);
		return NULL;
	}
	{
		int32 bc;
		get_sem_count(sem, &bc);
		DPRINTF_INFO((MY_ID "Allocated %ld write buffers\n", bc));
	}


	sprintf(my_dev->name, "%s%d", base_name, number);
	my_dev->dev = dev;
	my_dev->ifno = ifno;
	my_dev->open = 0;
	my_dev->open_fd = NULL;
	my_dev->active = true;
	my_dev->flags = 0;
	my_dev->rbuf = create_ring_buffer(1024);
	my_dev->buffer_size = B_PAGE_SIZE/2;
	DPRINTF_INFO((MY_ID "Created device %p\n", my_dev);)
	  	
	return my_dev;
}


void 
remove_device(usbmidi_device_info* my_dev)
{
	assert(my_dev != NULL);
	if (my_dev->rbuf != NULL) {
		delete_ring_buffer(my_dev->rbuf);
		my_dev->rbuf = NULL;
	}
	DPRINTF_INFO((MY_ID "remove_device %p\n", my_dev);)
  
	delete_area(my_dev->buffer_area);
	delete_sem(my_dev->sem_lock);
	delete_sem(my_dev->sem_send);
	free(my_dev);
}


/* driver cookie (per open -- but only one open allowed!) */

typedef struct driver_cookie
{
	struct driver_cookie	*next;
	usbmidi_device_info			*my_dev;
	sem_id sem_cb;
} driver_cookie;

	
/* NB global variables are valid only while driver is loaded */

_EXPORT int32	api_version = B_CUR_DRIVER_API_VERSION;

const char* usb_midi_driver_name = "usb_midi";

const int CINbytes[] = {	/* See USB-MIDI Spec */
	0,	/* 0x0 -- undefined Misc -- Reserved */
	0,	/* 0x1 -- undefined  Cable -- Reserved */
	2,	/* 0x2 -- 2-byte system common */
	3,	/* 0x3 -- 3-byte system common */
	3,	/* 0x4 -- SysEx start/continue */
	1,	/* 0x5 -- SysEx single-byte-end, or 1-byte Common */
	2,	/* 0x6 --  SysEx two-byte-end */
	3,	/* 0x7 --  SysEx three-byte-end */
	3,	/* 0x8 --  Note Off */
	3,	/* 0x9 --  Note On */
	3,	/* 0xA --  Poly KeyPress*/
	3,	/* 0xB --  Control Change */
	2,	/* 0xC --  Program Change */
	2,	/* 0xD --  Channel Pressure */
	3,	/* 0xE --  Pitch Bend */
	1,	/* 0xF --  Single Byte */
};

usb_module_info* usb;


static void
interpret_midi_buffer(usbmidi_device_info* my_dev)
{
	usb_midi_event_packet* packet = my_dev->buffer;
	size_t bytes_left = my_dev->actual_length;
	while (bytes_left) {	/* buffer may have several packets */
		int pktlen = CINbytes[packet->cin];
		DPRINTF_INFO((MY_ID "received packet %x:%d %x %x %x\n", packet->cin, packet->cn,
			packet->midi[0], packet->midi[1], packet->midi[2]));
		ring_buffer_write(my_dev->rbuf, packet->midi, pktlen);
		release_sem_etc(my_dev->open_fd->sem_cb, pktlen, B_DO_NOT_RESCHEDULE);
		packet++;
		bytes_left -= sizeof(usb_midi_event_packet);
	}
}


/*
	callback: got a report, issue next request
*/

static void 
midi_usb_read_callback(void* cookie, status_t status, 
	void* data, size_t actual_len)
{
	status_t st;
	usbmidi_device_info* my_dev = cookie;

	assert(cookie != NULL);
	DPRINTF_INFO((MY_ID "midi_usb_read_callback() -- packet length %ld\n", actual_len));

	acquire_sem(my_dev->sem_lock);
	my_dev->actual_length = actual_len;
	my_dev->bus_status = status;	/* B_USB_STATUS_* */
	if (status != B_OK) {
		/* request failed */
		DPRINTF_INFO((MY_ID "bus status %d\n", (int)status)); /* previously DPRINTF_ERR */
		if (status == B_CANCELED || !my_dev->active) {
			/* cancelled: device is unplugged */
			DPRINTF_INFO((MY_ID "midi_usb_read_callback: cancelled (status=%lx active=%d -- deleting sem_cb\n",
				status, my_dev->active));
			delete_sem(my_dev->open_fd->sem_cb);	/* done here to ensure read is freed */
			release_sem(my_dev->sem_lock);
			return;
		}
		release_sem(my_dev->sem_lock);
	} else {
		/* got a report */
#if 0
		uint32 i;
		char linbuf[256];
		uint8* buffer = my_dev->buffer;

		for (i = 0; i < 4; i++)
			sprintf=&linbuf[i*3], "%02X ", buffer[i]);
		DPRINTF_INFO((MY_ID "report: %s\n", linbuf));
#endif
		my_dev->timestamp = system_time();

		interpret_midi_buffer(my_dev);
		release_sem(my_dev->sem_lock);
	}

	/* issue next request */

	st = usb->queue_bulk(my_dev->ept_in->handle, my_dev->buffer,
		my_dev->buffer_size, (usb_callback_func)midi_usb_read_callback, my_dev);
	if (st != B_OK) {
		/* XXX probably endpoint stall */
		DPRINTF_ERR((MY_ID "queue_bulk() error %d\n", (int)st));
	}
}


static void 
midi_usb_write_callback(void* cookie, status_t status, 
	void* data, size_t actual_len)
{
	usbmidi_device_info* my_dev = cookie;
#ifdef DEBUG
	usb_midi_event_packet* pkt = data;
#endif

	assert(cookie != NULL);
	DPRINTF_INFO((MY_ID "midi_usb_write_callback() status %ld length %ld  pkt %p cin %x\n",
		status, actual_len, pkt, pkt->cin));
	release_sem(my_dev->sem_send); /* done with buffer */
}


/*
	USB specific device hooks
*/

static status_t 
usb_midi_added(const usb_device* dev, void** cookie)
{
	usbmidi_device_info* my_dev;
	const usb_device_descriptor* dev_desc;
	const usb_configuration_info* conf;
	const usb_interface_info* intf;
	status_t st;
	uint16 ifno, i;
	int alt;
	
	assert(dev != NULL && cookie != NULL);
	DPRINTF_INFO((MY_ID "usb_midi_added(%p, %p)\n", dev, cookie));

	dev_desc = usb->get_device_descriptor(dev);

	DPRINTF_INFO((MY_ID "vendor ID 0x%04X, product ID 0x%04X\n",
		dev_desc->vendor_id, dev_desc->product_id));

	/* check interface class */

	if ((conf = usb->get_nth_configuration(dev, DEFAULT_CONFIGURATION)) == NULL) {
		DPRINTF_ERR((MY_ID "cannot get default configuration\n"));
		return B_ERROR;
	}

	for (ifno = 0; ifno < conf->interface_count; ifno++) {
		/* This is C; I can use "class" :-> */
		int class, subclass, protocol;

		for (alt = 0; alt < conf->interface[ifno].alt_count; alt++) {
			intf = &conf->interface[ifno].alt[alt];
			class    = intf->descr->interface_class;
			subclass = intf->descr->interface_subclass;
			protocol = intf->descr->interface_protocol;
			DPRINTF_INFO((MY_ID "interface %d, alt : %d: class %d, subclass %d, protocol %d\n",
                        	ifno, alt, class, subclass, protocol));
                        	
			if (class == USB_AUDIO_DEVICE_CLASS 
				&& subclass == USB_AUDIO_INTERFACE_MIDISTREAMING_SUBCLASS)
				goto got_one;
		}		
	}

	DPRINTF_INFO((MY_ID "Midi interface not found\n"));
	return B_ERROR;
	
got_one:

	for (i=0; i < sizeof(usb_knowndevs)/sizeof(struct usb_knowndev); i++) {
		if (usb_knowndevs[i].vendor == dev_desc->vendor_id
			&& usb_knowndevs[i].product == dev_desc->product_id) {
			DPRINTF_INFO((MY_ID "vendor %s, product %s\n",
				usb_knowndevs[i].vendorname, usb_knowndevs[i].productname));
		}
	}

		
	/* configuration */
	if ((st = usb->set_configuration(dev, conf)) != B_OK) {
		DPRINTF_ERR((MY_ID "set_configuration() failed %d\n", (int)st));
		return B_ERROR;
	}

	if ((my_dev = create_device(dev, intf, ifno)) == NULL) {
		return B_ERROR;
	}
	
	DPRINTF_INFO((MY_ID "my_dev = %p endpoint count = %ld\n",
		my_dev, intf->endpoint_count));
	DPRINTF_INFO((MY_ID " input endpoint = %p\n",
		&intf->endpoint[0]));
	DPRINTF_INFO((MY_ID " output endpoint = %p\n",
		&intf->endpoint[1]));
	usb->clear_feature(intf->endpoint[0].handle, USB_FEATURE_ENDPOINT_HALT);
	/* This may need more thought...  */
	my_dev->ept_in = &intf->endpoint[0];		/* bulk IN */
	my_dev->ept_out = &intf->endpoint[1];		/* OUT */
	
	my_dev->timestamp = system_time();

	/* issue bulk transfer */
	DPRINTF_INFO((MY_ID "queueing bulk xfer ep 0\n"));
	st = usb->queue_bulk(my_dev->ept_in->handle, my_dev->buffer,
		my_dev->buffer_size, (usb_callback_func)midi_usb_read_callback, my_dev);
	if (st != B_OK) {
		DPRINTF_ERR((MY_ID "queue_bulk() error %d\n", (int)st));
		return B_ERROR;
	}

	/* create a port */
	add_device_info(my_dev);

	*cookie = my_dev;
	DPRINTF_INFO((MY_ID "usb_midi_added: added %s\n", my_dev->name));

	return B_OK;
}


static status_t 
usb_midi_removed(void* cookie)
{
	usbmidi_device_info* my_dev = cookie;

	assert(cookie != NULL);

	DPRINTF_INFO((MY_ID "usb_midi_removed(%s)\n", my_dev->name));
	acquire_sem(usbmidi_device_list_lock);	/* convenient mutex for safety */
	my_dev->active = false;
	if (my_dev->open_fd) {
		my_dev->open_fd->my_dev = NULL;
		delete_sem(my_dev->open_fd->sem_cb);	/* done here to ensure read is freed */
	}
	release_sem(usbmidi_device_list_lock);
	usb->cancel_queued_transfers(my_dev->ept_in->handle);
	usb->cancel_queued_transfers(my_dev->ept_out->handle);
	DPRINTF_INFO((MY_ID "usb_midi_removed: removing info & device: %s\n", my_dev->name));
	remove_device_info(my_dev);
	remove_device(my_dev);
	return B_OK;
}


static usb_notify_hooks my_notify_hooks =
{
	usb_midi_added, usb_midi_removed
};

#define	SUPPORTED_DEVICES	1
usb_support_descriptor my_supported_devices[SUPPORTED_DEVICES] =
{
	{ USB_AUDIO_DEVICE_CLASS, USB_AUDIO_INTERFACE_MIDISTREAMING_SUBCLASS, 0, 0, 0 },
};


/*
	usb_midi_open - handle open() calls
 */

static status_t 
usb_midi_open(const char* name, uint32 flags,
	driver_cookie** out_cookie)
{
	driver_cookie* cookie;
	usbmidi_device_info* my_dev;

	assert(name != NULL);
	assert(out_cookie != NULL);
	DPRINTF_INFO((MY_ID "usb_midi_open(%s)\n", name));

	if ((my_dev = search_device_info(name)) == NULL)
		return B_ENTRY_NOT_FOUND;
	if (my_dev->open_fd != NULL)
		return B_BUSY;	/* there can only be one open channel to the device */
	if ((cookie = malloc(sizeof(driver_cookie))) == NULL)
		return B_NO_MEMORY;

	cookie->sem_cb = create_sem(0, DRIVER_NAME "_cb");
	if (cookie->sem_cb < 0) {
		DPRINTF_ERR((MY_ID "create_sem() failed %d\n", (int)cookie->sem_cb));
		free(cookie);
		return B_ERROR;
	}

	acquire_sem(usbmidi_device_list_lock);	/* use global mutex now */
	cookie->my_dev = my_dev;
	my_dev->open_fd = cookie;
	my_dev->open++;
	release_sem(usbmidi_device_list_lock);

	*out_cookie = cookie;
	DPRINTF_INFO((MY_ID "usb_midi_open: device %s open (%d)\n", name, my_dev->open));
	return B_OK;
}


/*
	usb_midi_read - handle read() calls
 */

static status_t 
usb_midi_read(driver_cookie* cookie, off_t position,
	void* buf,	size_t* num_bytes)
{
	status_t err = B_ERROR;
	usbmidi_device_info* my_dev;

	assert(cookie != NULL);
	my_dev = cookie->my_dev;
	
	if (!my_dev || !my_dev->active)
		return B_ERROR;	/* already unplugged */
		
	DPRINTF_INFO((MY_ID "usb_midi_read: (%ld byte buffer at %ld cookie %p)\n",
		*num_bytes, (int32)position, cookie));
	while (my_dev && my_dev->active) {
		DPRINTF_INFOZ((MY_ID "waiting on acquire_sem_etc\n");)
		err = acquire_sem_etc(cookie->sem_cb, 1,
			 B_RELATIVE_TIMEOUT, 1000000);
		if (err == B_TIMED_OUT) {
			DPRINTF_INFOZ((MY_ID "acquire_sem_etc timed out\n");)
			continue;	/* see if we're still active */
		}
		if (err != B_OK) {
			*num_bytes = 0;
			DPRINTF_INFO((MY_ID "acquire_sem_etc aborted\n");)
			break;
		}
		DPRINTF_INFO((MY_ID "reading from ringbuffer\n");)
		acquire_sem(my_dev->sem_lock);
		ring_buffer_user_read(my_dev->rbuf, buf, 1);
		release_sem(my_dev->sem_lock);
		*num_bytes = 1;
		DPRINTF_INFO((MY_ID "read byte %x -- cookie %p)\n", *(uint8*)buf, cookie));
		return B_OK;
	}
	DPRINTF_INFO((MY_ID "usb_midi_read: loop terminated -- Device no longer active\n");)
	return B_CANCELED;
}


/*
	usb_midi_write - handle write() calls
 */

const uint8 CINcode[] = {	/* see USB-MIDI Spec */
	0x4,	/* 0x0 - sysex start */
	0,	/* 0x1 -- undefined */
	0x3,	/* 0x2  -- song pos */
	0x2,	/* 0x3 -- song select */
	0,	/* 0x4 -- undefined */
	0,	/* 0x5 -- undefined */
	0x2,	/* 0x6  -- tune request */
	0x5,	/* 0x7 -- sysex end */
	0x5,	/* 0x8 -- clock */
	0,	/* 0x9 -- undefined */
	0x5,	/* 0xA -- start */
	0x5,	/* 0xB -- continue */
	0x5,	/* 0xC -- stop */
	0,	/* 0xD -- undefined */
	0x5,	/* 0xE -- active sensing */
	0x5,	/* 0x0 -- system reset */
};


static status_t 
usb_midi_write(driver_cookie* cookie, off_t position,
	const void* buf, size_t* num_bytes)
{
	usbmidi_device_info* my_dev;
	uint8* midiseq = (uint8*)buf;
	uint8 midicode = midiseq[0];	/* preserved for reference */
	status_t st;
	size_t bytes_left = *num_bytes;
	size_t buff_lim;
	uint8 cin = ((midicode & 0xF0) == 0xF0) ? CINcode[midicode & 0x0F]
		 : (midicode >> 4);

	assert(cookie != NULL);
	my_dev = cookie->my_dev;

	if (!my_dev || !my_dev->active)
		return B_ERROR;		/* already unplugged */
		
	buff_lim = my_dev->buffer_size * 3 / 4;	/* max MIDI bytes buffer space */
	
	DPRINTF_INFO((MY_ID "MIDI write (%ld bytes at %Ld)\n", *num_bytes, position));
	if (*num_bytes > 3 && midicode != 0xF0) {
		DPRINTF_ERR((MY_ID "Non-SysEx packet of %ld bytes -- too big to handle\n",
			*num_bytes));
		return B_ERROR;
	}

	while (bytes_left) {
		size_t xfer_bytes = (bytes_left < buff_lim)?  bytes_left : buff_lim;
		usb_midi_event_packet* pkt = my_dev->out_buffer;
		int packet_count = 0;

		st = acquire_sem_etc(my_dev->sem_send, 1, B_RELATIVE_TIMEOUT, 2000000LL);
		if (st != B_OK)
			return st;

		while (xfer_bytes) {
			uint8 pkt_bytes = CINbytes[cin];
			memset(pkt, 0, sizeof(usb_midi_event_packet));
			pkt->cin = cin;
			DPRINTF_INFO((MY_ID "using packet data (code %x -- %d bytes) %x %x %x\n", pkt->cin,
			  CINbytes[pkt->cin], midiseq[0], midiseq[1], midiseq[2]));
			memcpy(pkt->midi, midiseq, pkt_bytes);
			DPRINTF_INFO((MY_ID "built packet %p %x:%d %x %x %x\n", pkt, pkt->cin, pkt->cn,
				pkt->midi[0], pkt->midi[1], pkt->midi[2]));
			xfer_bytes -= pkt_bytes;
			bytes_left -= pkt_bytes;
			midiseq += pkt_bytes;
			packet_count++;
			pkt++;
			if (midicode == 0xF0 && bytes_left < 4) cin = 4 + bytes_left;	/* see USB-MIDI Spec */
		}
		st = usb->queue_bulk(my_dev->ept_out->handle, my_dev->out_buffer,
			sizeof(usb_midi_event_packet) * packet_count,
			(usb_callback_func)midi_usb_write_callback, my_dev);
		if (st != B_OK) {
			DPRINTF_ERR((MY_ID "midi write queue_bulk() error %d\n", (int)st));
			return B_ERROR;
		}
	}
	return B_OK;
}


/*
	usb_midi_control - handle ioctl calls
 */

static status_t
usb_midi_control(void*  cookie, uint32 iop,
	void*  data, size_t len)
{
	return B_ERROR;
}


/*
	usb_midi_close - handle close() calls
 */

static status_t 
usb_midi_close(driver_cookie* cookie)
{
	usbmidi_device_info* my_dev;

	assert(cookie != NULL);
	delete_sem(cookie->sem_cb);
	my_dev = cookie->my_dev;
	DPRINTF_INFO((MY_ID "usb_midi_close(%p device=%p)\n", cookie, my_dev));


	acquire_sem(usbmidi_device_list_lock);
	if (my_dev) {
		/* detach the cookie from device */
		my_dev->open_fd = NULL;
		--my_dev->open;
	}
	release_sem(usbmidi_device_list_lock);
	DPRINTF_INFO((MY_ID "usb_midi_close: complete\n");)

	return B_OK;
}


/*
	usb_midi_free - called after the last device is closed, and after
	all i/o is complete.
 */

static status_t 
usb_midi_free(driver_cookie* cookie)
{
	usbmidi_device_info* my_dev;

	assert(cookie != NULL);
	my_dev = cookie->my_dev;
	DPRINTF_INFO((MY_ID "usb_midi_free(%p device=%p)\n", cookie, my_dev));

	free(cookie);

	return B_OK;
}


/*
	function pointers for the device hooks entry points
 */

static device_hooks usb_midi_hooks = {
	(device_open_hook)usb_midi_open,
	(device_close_hook)usb_midi_close,
	(device_free_hook)usb_midi_free,
	(device_control_hook)usb_midi_control,
	(device_read_hook)usb_midi_read,
	(device_write_hook)usb_midi_write,
	NULL, NULL, NULL, NULL
};


/*
	init_hardware - called once the first time the driver is loaded
 */

_EXPORT status_t 
init_hardware(void)
{
	DPRINTF_INFO((MY_ID "init_hardware() " __DATE__ " " __TIME__ "\n"));
	return B_OK;
}


/*
	init_driver - optional function - called every time the driver
	is loaded.
 */

_EXPORT status_t 
init_driver(void)
{

	DPRINTF_INFO((MY_ID "init_driver() " __DATE__ " " __TIME__ "\n"));

	if (get_module(B_USB_MODULE_NAME, (module_info**)&usb) != B_OK)
		return B_ERROR;

	if ((usbmidi_device_list_lock = create_sem(1, "dev_list_lock")) < 0) {
		put_module(B_USB_MODULE_NAME);
		return usbmidi_device_list_lock;		/* error code */
	}

	usb->register_driver(usb_midi_driver_name, my_supported_devices, 
		SUPPORTED_DEVICES, NULL);
	usb->install_notify(usb_midi_driver_name, &my_notify_hooks);
	DPRINTF_INFO((MY_ID "init_driver() OK\n"));

	return B_OK;
}


/*
	uninit_driver - optional function - called every time the driver
	is unloaded
 */

_EXPORT void 
uninit_driver(void)
{
	DPRINTF_INFO((MY_ID "uninit_driver()\n"));
	usb->uninstall_notify(usb_midi_driver_name);
	
	delete_sem(usbmidi_device_list_lock);
	put_module(B_USB_MODULE_NAME);
	free_device_names();
	DPRINTF_INFO((MY_ID "uninit complete\n"));
}


/*
	publish_devices
	device names are generated dynamically
*/

_EXPORT const char**
publish_devices(void)
{
	DPRINTF_INFO((MY_ID "publish_devices()\n"));

	if (usbmidi_device_list_changed) {
		free_device_names();
		alloc_device_names();
		if (usbmidi_device_names != NULL)
			rebuild_device_names();
		usbmidi_device_list_changed = false;
	}
	assert(usbmidi_device_names != NULL);
	return (const char**)usbmidi_device_names;
}


/*
	find_device - return ptr to device hooks structure for a
	given device name
 */

_EXPORT device_hooks* 
find_device(const char* name)
{
	assert(name != NULL);
	DPRINTF_INFO((MY_ID "find_device(%s)\n", name));
	if (search_device_info(name) == NULL)
		return NULL;
	return &usb_midi_hooks;
}

