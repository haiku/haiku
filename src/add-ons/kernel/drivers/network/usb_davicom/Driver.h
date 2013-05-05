/*
 *	Davicom DM9601 USB 1.1 Ethernet Driver.
 *	Copyright (c) 2008, 2011 Siarzhuk Zharski <imker@gmx.li>
 *	Copyright (c) 2009 Adrien Destugues <pulkomandy@gmail.com>
 *	Distributed under the terms of the MIT license.
 *
 *	Heavily based on code of the
 *	Driver for USB Ethernet Control Model devices
 *	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
 *	Distributed under the terms of the MIT license.
 */
#ifndef _USB_DAVICOM_DRIVER_H_
#define _USB_DAVICOM_DRIVER_H_


#include <Drivers.h>
#include <USB3.h>


// extra tracing in debug mode
//#define UDAV_TRACE

#define DRIVER_NAME	"usb_davicom"
#define MAX_DEVICES	8


const char* const kVersion = "ver.0.9.5";
extern usb_module_info *gUSBModule;


extern "C" {

status_t	usb_davicom_device_added(usb_device device, void **cookie);
status_t	usb_davicom_device_removed(void *cookie);

status_t	init_hardware();
void		uninit_driver();

const char **publish_devices();
device_hooks *find_device(const char *name);

}


#endif // _USB_DAVICOM_DRIVER_H_

