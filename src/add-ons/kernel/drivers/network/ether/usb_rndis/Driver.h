/*
	Driver for USB RNDIS network devices
	Copyright (C) 2022 Adrien Destugues <pulkomandy@pulkomandy.tk>
	Distributed under the terms of the MIT license.
*/
#ifndef _USB_RNDIS_DRIVER_H_
#define _USB_RNDIS_DRIVER_H_

#include <Drivers.h>
#include <KernelExport.h>
#include <OS.h>
#include <USB3.h>
#include <drivers/usb/USB_cdc.h>

#include <util/kernel_cpp.h>

#define DRIVER_NAME	"usb_rndis"
#define MAX_DEVICES	8

extern usb_module_info *gUSBModule;

extern "C" {
status_t	usb_rndis_device_added(usb_device device, void **cookie);
status_t	usb_rndis_device_removed(void *cookie);

status_t	init_hardware();
void		uninit_driver();

const char **publish_devices();
device_hooks *find_device(const char *name);
}

#if TRACE_RNDIS
#define TRACE(x...)			dprintf(DRIVER_NAME ": " x)
#else
#define TRACE(x...)
#endif
#define TRACE_ALWAYS(x...)	dprintf(DRIVER_NAME ": " x)

#endif //_USB_RNDIS_DRIVER_H_
