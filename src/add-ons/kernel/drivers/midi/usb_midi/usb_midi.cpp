/*
 * midi usb driver
 * usb_midi.c
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


/* #define DEBUG 1 */	/* Define this to enable DPRINTF_DEBUG statements */
/* (Other categories of printout set in usb_midi.h) */

#include "usb_midi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


const char* midi_base_name = "midi/usb/";


usbmidi_port_info*
create_usbmidi_port(usbmidi_device_info* devinfo,
	int cable, bool has_in, bool has_out)
{
	usbmidi_port_info* port = NULL;
	assert(usb != NULL && devinfo != NULL);

	port = (usbmidi_port_info*)malloc(sizeof(usbmidi_port_info));
	if (port == NULL)
		return NULL;

	sprintf(port->name, "%s-%d", devinfo->name, cable);
	port->device = devinfo;
	port->cable = cable;
	port->next = NULL;
	port->open = 0;
	port->open_fd = NULL;
	port->has_in = has_in;
	port->has_out = has_out;
	port->rbuf = create_ring_buffer(1024);

	devinfo->ports[cable] = port;

	DPRINTF_INFO((MY_ID "Created port %p cable %d: %s\n",
		port, cable, port->name));

	return port;
}


void
remove_port(usbmidi_port_info* port)
{
	assert(port != NULL);
	if (port->rbuf != NULL) {
		delete_ring_buffer(port->rbuf);
		port->rbuf = NULL;
	}
	DPRINTF_INFO((MY_ID "remove_port %p done\n", port));

	free(port);
}


usbmidi_device_info*
create_device(const usb_device* dev, uint16 ifno)
{
	usbmidi_device_info* midiDevice = NULL;
	int number;
	area_id area;
	sem_id sem;
	char area_name[32];

	assert(usb != NULL && dev != NULL);

	number = find_free_device_number();

	midiDevice = (usbmidi_device_info*)malloc(sizeof(usbmidi_device_info));
	if (midiDevice == NULL)
		return NULL;

	midiDevice->sem_lock = sem = create_sem(1, DRIVER_NAME "_lock");
	if (sem < 0) {
		DPRINTF_ERR((MY_ID "create_sem() failed 0x%lx\n", sem));
		free(midiDevice);
		return NULL;
	}

	sprintf(area_name, DRIVER_NAME "_buffer%d", number);
	midiDevice->buffer_area = area = create_area(area_name,
		(void**)&midiDevice->buffer, B_ANY_KERNEL_ADDRESS,
		B_PAGE_SIZE, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if (area < 0) {
		DPRINTF_ERR((MY_ID "create_area() failed 0x%lx\n", area));
		delete_sem(midiDevice->sem_lock);
		free(midiDevice);
		return NULL;
	}
	/* use half of reserved area for each of in and out buffers: */
	midiDevice->out_buffer = 
		(usb_midi_event_packet*)((uint8*)midiDevice->buffer + B_PAGE_SIZE / 2);
	midiDevice->sem_send =  sem = create_sem(1, DRIVER_NAME "_send");
	if (sem < 0) {
		DPRINTF_ERR((MY_ID "create_sem() failed 0x%lx\n", sem));
		delete_sem(midiDevice->sem_lock);
		delete_area(area);
		free(midiDevice);
		return NULL;
	}
	{
		int32 bc;
		get_sem_count(sem, &bc);
		DPRINTF_DEBUG((MY_ID "Allocated %ld write buffers\n", bc));
	}


	sprintf(midiDevice->name, "%s%d", midi_base_name, number);
	midiDevice->dev = dev;
	midiDevice->devnum = number;
	midiDevice->ifno = ifno;
	midiDevice->active = true;
	midiDevice->flags = 0;
	memset(midiDevice->ports, 0, sizeof(midiDevice->ports));
	midiDevice->buffer_size = B_PAGE_SIZE / 2;
	DPRINTF_INFO((MY_ID "Created device %p\n", midiDevice));

	return midiDevice;
}


void
remove_device(usbmidi_device_info* midiDevice)
{
	assert(midiDevice != NULL);
	DPRINTF_INFO((MY_ID "remove_device %p\n", midiDevice));

	delete_area(midiDevice->buffer_area);
	delete_sem(midiDevice->sem_lock);
	delete_sem(midiDevice->sem_send);
	free(midiDevice);
}


/* driver cookie (per open -- but only one open per port allowed!) */

typedef struct driver_cookie
{
	struct driver_cookie*	next;
	usbmidi_device_info*	device;	/* a bit redundant, but convenient */
	usbmidi_port_info*		port;
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
interpret_midi_buffer(usbmidi_device_info* midiDevice)
{
	usb_midi_event_packet* packet = midiDevice->buffer;
	size_t bytes_left = midiDevice->actual_length;
	while (bytes_left) {	/* buffer may have several packets */
		int pktlen = CINbytes[packet->cin];
		usbmidi_port_info* port = midiDevice->ports[packet->cn];

		DPRINTF_DEBUG((MY_ID "received packet %x:%d %x %x %x\n",
			packet->cin, packet->cn,
			packet->midi[0], packet->midi[1], packet->midi[2]));

		/* port matching 'cable number' */
		if (port == NULL) {
			DPRINTF_ERR((MY_ID "no port matching cable number %d!\n",
				packet->cn));
		} else {
			ring_buffer_write(port->rbuf, packet->midi, pktlen);
			release_sem_etc(port->open_fd->sem_cb, pktlen,
				B_DO_NOT_RESCHEDULE);
		}

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
	usbmidi_device_info* midiDevice = (usbmidi_device_info*)cookie;

	assert(cookie != NULL);
	DPRINTF_DEBUG((MY_ID "midi_usb_read_callback() -- packet length %ld\n",
		actual_len));

	acquire_sem(midiDevice->sem_lock);
	midiDevice->actual_length = actual_len;
	midiDevice->bus_status = status;	/* B_USB_STATUS_* */
	if (status != B_OK) {
		/* request failed */
		DPRINTF_DEBUG((MY_ID "bus status 0x%lx\n", status));
		if (status == B_CANCELED || !midiDevice->active) {
			/* cancelled: device is unplugged */
			DPRINTF_DEBUG((MY_ID "midi_usb_read_callback: cancelled"
				"(status=%lx active=%d -- deleting sem_cbs\n",
				status, midiDevice->active));

			// Free any read() still blocked on semaphore
			for (int cable = 0; cable < 16; cable++) {
				usbmidi_port_info* port = midiDevice->ports[cable];
				if (port == NULL)
					break;
				if (port->open_fd != NULL)
					delete_sem(port->open_fd->sem_cb);
			}
			release_sem(midiDevice->sem_lock);
			return;
		}
		release_sem(midiDevice->sem_lock);
	} else {
		/* got a report */
		midiDevice->timestamp = system_time();	/* not used... */

		interpret_midi_buffer(midiDevice);
		release_sem(midiDevice->sem_lock);
	}

	/* issue next request */
	st = usb->queue_bulk(midiDevice->ept_in->handle,
		midiDevice->buffer, midiDevice->buffer_size,
		(usb_callback_func)midi_usb_read_callback, midiDevice);
	if (st != B_OK) {
		/* probably endpoint stall */
		DPRINTF_ERR((MY_ID "queue_bulk() error 0x%lx\n", st));
	}
}


static void
midi_usb_write_callback(void* cookie, status_t status,
	void* data, size_t actual_len)
{
	usbmidi_device_info* midiDevice = (usbmidi_device_info*)cookie;
#ifdef DEBUG
	usb_midi_event_packet* pkt = data;
#endif

	assert(cookie != NULL);
	DPRINTF_DEBUG((MY_ID "midi_usb_write_callback()"
		" status %ld length %ld  pkt %p cin %x\n",
		status, actual_len, pkt, pkt->cin));
	release_sem(midiDevice->sem_send); /* done with buffer */
}


/*
	USB specific device hooks
*/

static status_t
usb_midi_added(const usb_device* dev, void** cookie)
{
	/* This seems overcomplicated, but endpoints can be in either order...
	and could possibly have different number of connectors! */
	int in_cables = 0, out_cables = 0;
	int cable_count[2] = {0, 0};
	int iep = 0;
	status_t status;

	assert(dev != NULL && cookie != NULL);
	DPRINTF_INFO((MY_ID "usb_midi_added(%p, %p)\n", dev, cookie));

	const usb_device_descriptor* dev_desc = usb->get_device_descriptor(dev);

	DPRINTF_INFO((MY_ID "vendor ID 0x%04X, product ID 0x%04X\n",
		dev_desc->vendor_id, dev_desc->product_id));

	/* check interface class */
	const usb_configuration_info* conf;
	if ((conf = usb->get_nth_configuration(dev, DEFAULT_CONFIGURATION))
		== NULL) {
		DPRINTF_ERR((MY_ID "cannot get default configuration\n"));
		return B_ERROR;
	}
	DPRINTF_INFO((MY_ID "Interface count = %ld\n", conf->interface_count));

	uint16 alt, ifno;
	const usb_interface_info* intf;
	for (ifno = 0; ifno < conf->interface_count; ifno++) {
		int devclass, subclass, protocol;

		for (alt = 0; alt < conf->interface[ifno].alt_count; alt++) {
			intf = &conf->interface[ifno].alt[alt];
			devclass    = intf->descr->interface_class;
			subclass = intf->descr->interface_subclass;
			protocol = intf->descr->interface_protocol;
			DPRINTF_INFO((
				MY_ID "interface %d, alt : %d: class %d,"
				" subclass %d, protocol %d\n",
                ifno, alt, devclass, subclass, protocol));

			if (devclass == USB_AUDIO_DEVICE_CLASS
				&& subclass == USB_AUDIO_INTERFACE_MIDISTREAMING_SUBCLASS)
				goto got_one;
		}
	}

	DPRINTF_INFO((MY_ID "Midi interface not found\n"));
	return B_ERROR;

got_one:

	if ((status = usb->set_configuration(dev, conf)) != B_OK) {
		DPRINTF_ERR((MY_ID "set_configuration() failed 0x%lx\n", status));
		return B_ERROR;
	}

	usbmidi_device_info* midiDevice;
	if ((midiDevice = create_device(dev, ifno)) == NULL) {
		return B_ERROR;
	}

	/* get the actual number of  ports in and out */
	for (uint16 i = 0; i < intf->generic_count; i++) {
		usb_generic_descriptor *generic = &intf->generic[i]->generic;
		DPRINTF_DEBUG((MY_ID "descriptor %d: type %x sub %x\n",
			i, generic->descriptor_type, generic->data[0]));
		if (generic->descriptor_type == USB_DESCRIPTOR_CS_ENDPOINT
			&& generic->data[0] == USB_MS_GENERAL_DESCRIPTOR) {
			/* These *better* be in the same order as the endpoints! */
			cable_count[iep] = generic->data[1];
			iep = 1;
		}
	}

	DPRINTF_DEBUG((MY_ID "midiDevice = %p endpoint count = %ld\n",
		midiDevice, intf->endpoint_count));
	midiDevice->ept_in = midiDevice->ept_out = NULL;

	for (uint16 i = 0; i < intf->endpoint_count && i < 2; i++) {
		/* we are actually assuming max one IN, one OUT endpoint... */
		DPRINTF_INFO((MY_ID "endpoint %d = %p  %s\n",
			i, &intf->endpoint[i],
			(intf->endpoint[i].descr->endpoint_address & 0x80) != 0
			 ? "IN" : "OUT"));
		if ((intf->endpoint[i].descr->endpoint_address & 0x80) != 0) {
			if (midiDevice->ept_in == NULL) {
				midiDevice->ept_in = &intf->endpoint[i];
				in_cables = cable_count[i];
			}
		} else if (midiDevice->ept_out == NULL) {
			midiDevice->ept_out = &intf->endpoint[i];
			out_cables = cable_count[i];
		}
	}

	midiDevice->timestamp = system_time();	/* This never seems to be used */

	/* Create the actual device ports */
	usbmidi_port_info* port;
	for (uint16 i = 0; in_cables > 0 || out_cables > 0; i++) {
		port = create_usbmidi_port(midiDevice, i,
			(bool)in_cables, (bool)out_cables);
		midiDevice->ports[i] = port;
		if (in_cables)
			in_cables--;
		if (out_cables)
			out_cables--;
		add_port_info(port);
	}

	/* issue bulk transfer */
	DPRINTF_DEBUG((MY_ID "queueing bulk xfer IN endpoint\n"));
	status = usb->queue_bulk(midiDevice->ept_in->handle, midiDevice->buffer,
		midiDevice->buffer_size,
		(usb_callback_func)midi_usb_read_callback, midiDevice);
	if (status != B_OK) {
		DPRINTF_ERR((MY_ID "queue_bulk() error 0x%lx\n", status));
		return B_ERROR;
	}

	*cookie = midiDevice;
	DPRINTF_INFO((MY_ID "usb_midi_added: %s\n", midiDevice->name));

	return B_OK;
}


static status_t
usb_midi_removed(void* cookie)
{
	usbmidi_device_info* midiDevice = (usbmidi_device_info*)cookie;
	
	assert(cookie != NULL);

	DPRINTF_INFO((MY_ID "usb_midi_removed(%s)\n", midiDevice->name));
	midiDevice->active = false;
	for (int cable = 0; cable < 16; cable++) {
		usbmidi_port_info* port = midiDevice->ports[cable];
		if (port == NULL)
			break;
		DPRINTF_DEBUG((MY_ID "removing port %d\n", cable));
		if (port->open_fd != NULL) {
			remove_port_info(port);
			port->open_fd->port = NULL;
			port->open_fd->device = NULL;
			delete_sem(port->open_fd->sem_cb);
				/* done here to ensure read is freed */
		}
		remove_port(port);
	}
	usb->cancel_queued_transfers(midiDevice->ept_in->handle);
	usb->cancel_queued_transfers(midiDevice->ept_out->handle);
	DPRINTF_DEBUG((MY_ID "usb_midi_removed: doing remove: %s\n",
		midiDevice->name));
	remove_device(midiDevice);
	return B_OK;
}


static usb_notify_hooks my_notify_hooks =
{
	usb_midi_added, usb_midi_removed
};

#define	SUPPORTED_DEVICES	1
usb_support_descriptor my_supported_devices[SUPPORTED_DEVICES] =
{
	{
		USB_AUDIO_DEVICE_CLASS,
		USB_AUDIO_INTERFACE_MIDISTREAMING_SUBCLASS,
		0, 0, 0
	},
};


/*
	Device Driver Hook Functions
	-- open, read, write, close, and free
 */

static status_t
usb_midi_open(const char* name, uint32 flags,
	driver_cookie** out_cookie)
{
	driver_cookie* cookie;
	usbmidi_port_info* port;
	int mode = flags & O_RWMASK;

	assert(name != NULL);
	assert(out_cookie != NULL);
	DPRINTF_INFO((MY_ID "usb_midi_open(%s) flags=%lx\n", name, flags));

	if ((port = search_port_info(name)) == NULL)
		return B_ENTRY_NOT_FOUND;

	if (!port->has_in && mode != O_RDONLY)
		return B_PERMISSION_DENIED;	 /* == EACCES */
	else if (!port->has_out && mode != O_WRONLY)
		return B_PERMISSION_DENIED;

	if ((cookie = (driver_cookie*)malloc(sizeof(driver_cookie))) == NULL)
		return B_NO_MEMORY;

	cookie->sem_cb = create_sem(0, DRIVER_NAME "_cb");
	if (cookie->sem_cb < 0) {
		DPRINTF_ERR((MY_ID "create_sem() failed 0x%lx\n", cookie->sem_cb));
		free(cookie);
		return B_ERROR;
	}

	cookie->port = port;
	cookie->device = port->device;

	acquire_sem(usbmidi_port_list_lock);
	if (port->open_fd != NULL) {
		/* there can only be one open channel to the device */
		delete_sem(cookie->sem_cb);
		free(cookie);
		release_sem(usbmidi_port_list_lock);
		return B_BUSY;
	}
	port->open_fd = cookie;
	port->open++;
	release_sem(usbmidi_port_list_lock);

	*out_cookie = cookie;
	DPRINTF_INFO((MY_ID "usb_midi_open: device %s open (%d)\n",
		name, port->open));
	return B_OK;
}


static status_t
usb_midi_read(driver_cookie* cookie, off_t position,
	void* buf,	size_t* num_bytes)
{
	assert(cookie != NULL);
	status_t err = B_ERROR;
	usbmidi_port_info* port = cookie->port;
	usbmidi_device_info* midiDevice = cookie->device;

	if (midiDevice == NULL || !midiDevice->active)
		return B_ERROR;	/* already unplugged */

	DPRINTF_DEBUG((MY_ID "usb_midi_read: (%ld byte buffer at %lld cookie %p)"
		"\n", *num_bytes, position, cookie));
	while (midiDevice && midiDevice->active) {
		DPRINTF_DEBUG((MY_ID "waiting on acquire_sem_etc\n"));
		err = acquire_sem_etc(cookie->sem_cb, 1,
			 B_RELATIVE_TIMEOUT, 1000000);
		if (err == B_TIMED_OUT) {
			DPRINTF_DEBUG((MY_ID "acquire_sem_etc timed out\n"));
			continue;	/* see if we're still active */
		}
		if (err != B_OK) {
			*num_bytes = 0;
			DPRINTF_DEBUG((MY_ID "acquire_sem_etc aborted\n"));
			break;
		}
		DPRINTF_DEBUG((MY_ID "reading from ringbuffer\n"));
		acquire_sem(midiDevice->sem_lock);
			/* a global semaphore -- OK, I think */
		ring_buffer_user_read(port->rbuf, (uint8*)buf, 1);
		release_sem(midiDevice->sem_lock);
		*num_bytes = 1;
		DPRINTF_DEBUG((MY_ID "read byte %x -- cookie %p)\n",
			*(uint8*)buf, cookie));
		return B_OK;
	}
	DPRINTF_INFO((MY_ID "usb_midi_read: loop terminated"
		" -- Device no longer active\n"));
	return B_CANCELED;
}


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
	uint8* midiseq = (uint8*)buf;
	uint8 midicode = midiseq[0];	/* preserved for reference */
	status_t status;
	size_t buff_lim;
	uint8 cin = ((midicode & 0xF0) == 0xF0) ? CINcode[midicode & 0x0F]
		 : (midicode >> 4);

	assert(cookie != NULL);
	usbmidi_port_info* port = cookie->port;
	usbmidi_device_info* midiDevice = cookie->device;

	if (!midiDevice || !midiDevice->active)
		return B_ERROR;		/* already unplugged */

	buff_lim = midiDevice->buffer_size * 3 / 4;
		/* max MIDI bytes buffer space */

	DPRINTF_DEBUG((MY_ID "MIDI write (%ld bytes at %lld)\n",
		*num_bytes, position));
	if (*num_bytes > 3 && midicode != 0xF0) {
		DPRINTF_ERR((MY_ID "Non-SysEx packet of %ld bytes"
			" -- too big to handle\n", *num_bytes));
		return B_ERROR;
	}

	size_t bytes_left = *num_bytes;
	while (bytes_left) {
		size_t xfer_bytes = (bytes_left < buff_lim) ?  bytes_left : buff_lim;
		usb_midi_event_packet* pkt = midiDevice->out_buffer;
		int packet_count = 0;

		status = acquire_sem_etc(midiDevice->sem_send,
			1, B_RELATIVE_TIMEOUT, 2000000LL);
		if (status != B_OK)
			return status;

		while (xfer_bytes) {
			uint8 pkt_bytes = CINbytes[cin];
			memset(pkt, 0, sizeof(usb_midi_event_packet));
			pkt->cin = cin;
			pkt->cn = port->cable;
			DPRINTF_DEBUG((MY_ID "using packet data (code %x -- %d bytes)"
				" %x %x %x\n", pkt->cin, CINbytes[pkt->cin],
				midiseq[0], midiseq[1], midiseq[2]));
			memcpy(pkt->midi, midiseq, pkt_bytes);
			DPRINTF_DEBUG((MY_ID "built packet %p %x:%d %x %x %x\n",
				pkt, pkt->cin, pkt->cn,
				pkt->midi[0], pkt->midi[1], pkt->midi[2]));
			xfer_bytes -= pkt_bytes;
			bytes_left -= pkt_bytes;
			midiseq += pkt_bytes;
			packet_count++;
			pkt++;
			if (midicode == 0xF0 && bytes_left < 4) cin = 4 + bytes_left;
				/* see USB-MIDI Spec */
		}
		status = usb->queue_bulk(midiDevice->ept_out->handle,
			midiDevice->out_buffer,	sizeof(usb_midi_event_packet)
			* packet_count, (usb_callback_func)midi_usb_write_callback,
			midiDevice);
		if (status != B_OK) {
			DPRINTF_ERR((MY_ID "midi write queue_bulk() error 0x%lx\n",
				status));
			return B_ERROR;
		}
	}
	return B_OK;
}


static status_t
usb_midi_control(void* cookie, uint32 iop, void* data, size_t len)
{
	return B_ERROR;
}


static status_t
usb_midi_close(driver_cookie* cookie)
{
	assert(cookie != NULL);
	delete_sem(cookie->sem_cb);
	
	usbmidi_port_info* port = cookie->port;
	usbmidi_device_info* midiDevice = cookie->device;
	
	DPRINTF_INFO((MY_ID "usb_midi_close(%p device=%p port=%p)\n",
		cookie, midiDevice, port));

	acquire_sem(usbmidi_port_list_lock);
	if (port != NULL) {
		/* detach the cookie from port */
		port->open_fd = NULL;
		--port->open;
	}
	release_sem(usbmidi_port_list_lock);
	DPRINTF_DEBUG((MY_ID "usb_midi_close: complete\n"));

	return B_OK;
}


static status_t
usb_midi_free(driver_cookie* cookie)
{
	assert(cookie != NULL);
	usbmidi_device_info* midiDevice = cookie->device;
	DPRINTF_INFO((MY_ID "usb_midi_free(%p device=%p)\n", cookie, midiDevice));

	free(cookie);

	return B_OK;
}


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
	Driver Registration
 */

_EXPORT status_t
init_hardware(void)
{
	DPRINTF_DEBUG((MY_ID "init_hardware() version:"
		__DATE__ " " __TIME__ "\n"));
	return B_OK;
}


_EXPORT status_t
init_driver(void)
{
	DPRINTF_INFO((MY_ID "init_driver() version:" __DATE__ " " __TIME__ "\n"));

	if (get_module(B_USB_MODULE_NAME, (module_info**)&usb) != B_OK)
		return B_ERROR;

	if ((usbmidi_port_list_lock = create_sem(1, "dev_list_lock")) < 0) {
		put_module(B_USB_MODULE_NAME);
		return usbmidi_port_list_lock;		/* error code */
	}

	usb->register_driver(usb_midi_driver_name, my_supported_devices,
		SUPPORTED_DEVICES, NULL);
	usb->install_notify(usb_midi_driver_name, &my_notify_hooks);
	DPRINTF_INFO((MY_ID "init_driver() OK\n"));

	return B_OK;
}


_EXPORT void
uninit_driver(void)
{
	DPRINTF_INFO((MY_ID "uninit_driver()\n"));
	usb->uninstall_notify(usb_midi_driver_name);

	delete_sem(usbmidi_port_list_lock);
	put_module(B_USB_MODULE_NAME);
	free_port_names();
	DPRINTF_INFO((MY_ID "uninit complete\n"));
}


_EXPORT const char**
publish_devices(void)
{
	DPRINTF_INFO((MY_ID "publish_devices()\n"));

	if (usbmidi_port_list_changed) {
		free_port_names();
		alloc_port_names();
		if (usbmidi_port_names != NULL)
			rebuild_port_names();
		usbmidi_port_list_changed = false;
	}
	assert(usbmidi_port_names != NULL);
	return (const char**)usbmidi_port_names;
}


_EXPORT device_hooks*
find_device(const char* name)
{
	assert(name != NULL);
	DPRINTF_INFO((MY_ID "find_device(%s)\n", name));
	if (search_port_info(name) == NULL)
		return NULL;
	return &usb_midi_hooks;
}

