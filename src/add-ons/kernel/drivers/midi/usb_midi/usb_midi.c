/*****************************************************************************/
// midi usb driver
// Written by Jérôme Duval
//
// usb_midi.c
//
// Copyright (c) 2006 Haiku Project
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
#include <string.h>
#include <unistd.h>
#include "usb_midi.h"
#include "usbdevs.h"
#include "usbdevs_data.h"

static int midi_device_number = 0;
const char *midi_base_name = "midi/usb/";

my_device_info *
create_device(const usb_device *dev, const usb_interface_info *ii, uint16 ifno)
{
	my_device_info *my_dev = NULL;
	int number;
	area_id area;
	sem_id sem;
	char	area_name [32];
	const char *base_name;
	
	assert (usb != NULL && dev != NULL);

	number = midi_device_number++;
	base_name = midi_base_name;
		
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
	my_dev->flags = 0;
	my_dev->rbuf = create_ring_buffer(384);
	my_dev->total_report_size = sizeof(usb_midi_event_packet);
	  	
	return my_dev;
}

void 
remove_device (my_device_info *my_dev)
{
	assert (my_dev != NULL);
	if (my_dev->rbuf != NULL) {
		delete_ring_buffer(my_dev->rbuf);
		my_dev->rbuf = NULL;
	}
  
	delete_area (my_dev->buffer_area);
	delete_sem (my_dev->sem_cb);
	delete_sem (my_dev->sem_lock);
	free (my_dev);
}


/* driver cookie (per open) */

typedef struct driver_cookie
{
	struct driver_cookie	*next;
	my_device_info			*my_dev;
} driver_cookie;
	
/* NB global variables are valid only while driver is loaded */

_EXPORT int32	api_version = B_CUR_DRIVER_API_VERSION;

const char *usb_midi_driver_name = "usb_midi";

usb_module_info *usb;

static void
interpret_midi_buffer(my_device_info *my_dev)
{
	usb_midi_event_packet *packet = my_dev->buffer;
	
	ring_buffer_write(my_dev->rbuf, packet->midi, sizeof(packet->midi));
	release_sem_etc(my_dev->sem_cb, 3, B_DO_NOT_RESCHEDULE);
}


/*
	callback: got a report, issue next request
*/

static void 
midi_usb_callback(void *cookie, uint32 status, 
	void *data, uint32 actual_len)
{
	status_t st;
	my_device_info *my_dev = cookie;

	assert (cookie != NULL);
	DPRINTF_INFO ((MY_ID "midi_usb_callback()\n"));

	acquire_sem (my_dev->sem_lock);
	my_dev->actual_length = actual_len;
	my_dev->bus_status = status;	/* B_USB_STATUS_* */
	if (status != B_OK) {
		/* request failed */
		release_sem (my_dev->sem_lock);
		DPRINTF_ERR ((MY_ID "bus status %d\n", (int)status));
		if (status == B_CANCELED) {
			/* cancelled: device is unplugged */
			return;
		}
#if 0
		st = usb->clear_feature (my_dev->ept->handle, USB_FEATURE_ENDPOINT_HALT);
		if (st != B_OK)
			DPRINTF_ERR ((MY_ID "clear_feature() error %d\n", (int)st));
#endif
	} else {
		/* got a report */
//#if 0
		uint32 i;
		char linbuf [256];
		uint8 *buffer = my_dev->buffer;

		for (i = 0; i < my_dev->total_report_size; i++)
			sprintf (&linbuf[i*3], "%02X ", buffer [i]);
		DPRINTF_INFO ((MY_ID "report: %s\n", linbuf));
//#endif
		my_dev->timestamp = system_time ();
	
		interpret_midi_buffer(my_dev);
		release_sem (my_dev->sem_lock);
	}

	/* issue next request */

	st = usb->queue_bulk (my_dev->ept->handle, my_dev->buffer,
		my_dev->total_report_size, midi_usb_callback, my_dev);
	if (st != B_OK) {
		/* XXX probably endpoint stall */
		DPRINTF_ERR ((MY_ID "queue_interrupt() error %d\n", (int)st));
	}
}

/*
	USB specific device hooks
*/

static status_t 
usb_midi_added(const usb_device *dev, void **cookie)
{
	my_device_info *my_dev;
	const usb_device_descriptor *dev_desc;
	const usb_configuration_info *conf;
	const usb_interface_info *intf;
	status_t st;
	uint16 ifno, i;
	
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
		int alt;

		for (alt = 0; alt < conf->interface[ifno].alt_count; alt++) {
			intf = &conf->interface [ifno].alt[alt];
			class    = intf->descr->interface_class;
			subclass = intf->descr->interface_subclass;
			protocol = intf->descr->interface_protocol;
			DPRINTF_INFO ((MY_ID "interface %d, alt : %d: class %d, subclass %d, protocol %d\n",
                        	ifno, alt, class, subclass, protocol));
                        	
			if (class == USB_AUDIO_DEVICE_CLASS 
				&& subclass == USB_AUDIO_INTERFACE_MIDISTREAMING_SUBCLASS)
				goto got_one;
		}		
	}

	DPRINTF_INFO ((MY_ID "Midi interface not found\n"));
	return B_ERROR;
	
got_one:

	for (i=0; i<sizeof(usb_knowndevs)/sizeof(struct usb_knowndev) ; i++) {
		if (usb_knowndevs[i].vendor == dev_desc->vendor_id
			&& usb_knowndevs[i].product == dev_desc->product_id) {
			DPRINTF_INFO ((MY_ID "vendor %s, product %s\n",
				usb_knowndevs[i].vendorname, usb_knowndevs[i].productname));
		}
	}

	if((st = usb->set_alt_interface(dev, intf)) != B_OK) {
		DPRINTF_ERR ((MY_ID "set_alt_interface() failed %d\n", (int)st));
		return B_ERROR;
	}
	
	/* configuration */
	if ((st = usb->set_configuration (dev, conf)) != B_OK) {
		DPRINTF_ERR ((MY_ID "set_configuration() failed %d\n", (int)st));
		return B_ERROR;
	}

	if ((my_dev = create_device (dev, intf, ifno)) == NULL) {
		return B_ERROR;
	}
	
	st = usb->queue_request(dev, 
		USB_REQTYPE_ENDPOINT_OUT | USB_REQTYPE_STANDARD,
		USB_REQUEST_CLEAR_FEATURE,
		USB_FEATURE_ENDPOINT_HALT, 1, my_dev->total_report_size, 
		my_dev->buffer, my_dev->total_report_size, midi_usb_callback, my_dev);
	if (st != B_OK) {
		DPRINTF_ERR ((MY_ID "queue_request() error %d\n", (int)st));
		return B_ERROR;
	}
	
	my_dev->timestamp = system_time ();

	/* issue bulk transfer */
	my_dev->ept = &intf->endpoint [0];		/* interrupt IN */
	st = usb->queue_bulk (my_dev->ept->handle, my_dev->buffer,
		my_dev->total_report_size, midi_usb_callback, my_dev);
	if (st != B_OK) {
		DPRINTF_ERR ((MY_ID "queue_bulk() error %d\n", (int)st));
		return B_ERROR;
	}

	/* create a port */
	add_device_info (my_dev);

	*cookie = my_dev;
	DPRINTF_INFO ((MY_ID "added %s\n", my_dev->name));

	return B_OK;
}

static status_t 
usb_midi_removed (void *cookie)
{
	my_device_info *my_dev = cookie;

	assert (cookie != NULL);

	DPRINTF_INFO ((MY_ID "device_removed(%s)\n", my_dev->name));
	usb->cancel_queued_transfers (my_dev->ept->handle);
	remove_device_info (my_dev);
	if (my_dev->open == 0) {
		remove_device (my_dev);
	} else {
		DPRINTF_INFO ((MY_ID "%s still open\n", my_dev->name));
		my_dev->active = false;
	}
	return B_OK;
}

static usb_notify_hooks my_notify_hooks =
{
	usb_midi_added, usb_midi_removed
};

#define	SUPPORTED_DEVICES	1
usb_support_descriptor my_supported_devices [SUPPORTED_DEVICES] =
{
	{ USB_AUDIO_DEVICE_CLASS, USB_AUDIO_INTERFACE_MIDISTREAMING_SUBCLASS, 0, 0, 0 },
};


/* ----------
	usb_midi_open - handle open() calls
----- */

static status_t 
usb_midi_open(const char *name, uint32 flags,
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
	usb_midi_read - handle read() calls
----- */

static status_t 
usb_midi_read(driver_cookie *cookie, off_t position,
	void *buf,	size_t *num_bytes)
{
	status_t err = B_ERROR;
	my_device_info *my_dev;

	assert (cookie != NULL);
	my_dev = cookie->my_dev;
	assert (my_dev != NULL);
	
	if (!my_dev->active)
		return B_ERROR;		/* already unplugged */
		
	err = acquire_sem_etc(my_dev->sem_cb, 1, B_CAN_INTERRUPT, 0LL);
	if (err != B_OK)
		return err;
	acquire_sem (my_dev->sem_lock);
	ring_buffer_user_read(my_dev->rbuf, buf, 1);
	release_sem (my_dev->sem_lock);
	*num_bytes = 1;
	return err;
}


/* ----------
	usb_midi_write - handle write() calls
----- */

static status_t 
usb_midi_write(driver_cookie *cookie, off_t position,
	const void *buf, size_t *num_bytes)
{
	return B_ERROR;
}

/* ----------
	usb_midi_control - handle ioctl calls
----- */

static status_t
usb_midi_control(void * cookie, uint32 iop,
	void * data, size_t len)
{
	return B_ERROR;
}

/* ----------
	usb_midi_close - handle close() calls
----- */

static status_t 
usb_midi_close(driver_cookie *cookie)
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
	usb_midi_free - called after the last device is closed, and after
	all i/o is complete.
----- */
static status_t 
usb_midi_free(driver_cookie *cookie)
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
		remove_device (my_dev);
	}

	return B_OK;
}


/* -----
	function pointers for the device hooks entry points
----- */

static device_hooks usb_midi_hooks = {
	(device_open_hook) usb_midi_open,
	(device_close_hook) usb_midi_close,
	(device_free_hook) usb_midi_free,
	(device_control_hook) usb_midi_control,
	(device_read_hook) usb_midi_read,
	(device_write_hook) usb_midi_write,
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

	usb->register_driver (usb_midi_driver_name, my_supported_devices, 
		SUPPORTED_DEVICES, NULL);
	usb->install_notify (usb_midi_driver_name, &my_notify_hooks);
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
	usb->uninstall_notify (usb_midi_driver_name);
	
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
	return &usb_midi_hooks;
}

