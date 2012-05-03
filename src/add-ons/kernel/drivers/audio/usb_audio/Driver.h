/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009,10,12 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _USB_AUDIO_DRIVER_H_
#define _USB_AUDIO_DRIVER_H_


#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <USB3.h>
#include <hmulti_audio.h>


#define DRIVER_NAME	"usb_audio"
#define MAX_DEVICES	8

const char* const kVersion = "ver.0.0.4";

const uint32 kSamplesBufferSize = 1024;
const uint32 kSamplesBufferCount = 2;


// calculate count of array members
#ifdef _countof
	#warning "countof(...) WAS ALREADY DEFINED!!! Remove local definition!"
	#undef countof
#endif
#define _countof(array)(sizeof(array) / sizeof(array[0]))


extern usb_module_info *gUSBModule;


extern "C" {

	status_t	usb_audio_device_added(usb_device device, void **cookie);
	status_t	usb_audio_device_removed(void *cookie);

	status_t	init_hardware();
	void		uninit_driver();

	const char **publish_devices();
	device_hooks *find_device(const char *name);

}


#endif // _USB_AUDIO_DRIVER_H_

