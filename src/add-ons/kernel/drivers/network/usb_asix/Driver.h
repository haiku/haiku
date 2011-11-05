/*
 *	ASIX AX88172/AX88772/AX88178 USB 2.0 Ethernet Driver.
 *	Copyright (c) 2008, 2011 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 *	Heavily based on code of the
 *	Driver for USB Ethernet Control Model devices
 *	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _USB_ASIX_DRIVER_H_
#define _USB_ASIX_DRIVER_H_


#include <Drivers.h>
#include <USB3.h>


#define DRIVER_NAME	"usb_asix"
#define MAX_DEVICES	8


const uint8 kInvalidRequest = 0xff;
const char* const kVersion = "ver.0.9.1";
extern usb_module_info *gUSBModule;


extern "C" {
status_t	usb_asix_device_added(usb_device device, void **cookie);
status_t	usb_asix_device_removed(void *cookie);

status_t	init_hardware();
void		uninit_driver();

const char **publish_devices();
device_hooks *find_device(const char *name);
}


#endif // _USB_ASIX_DRIVER_H_

