/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009-13 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _USB_AUDIO_DRIVER_H_
#define _USB_AUDIO_DRIVER_H_


#include <Drivers.h>
#include <USB3.h>


#define DRIVER_NAME	"usb_audio"
#define MAX_DEVICES	8

const char* const kVersion = "ver.0.0.5";

// initial buffer size in samples
const uint32 kSamplesBufferSize = 1024;
// [sub]buffers count
const uint32 kSamplesBufferCount = 2;

// calculate count of array members
#ifdef _countof
	#warning "countof(...) WAS ALREADY DEFINED!!! Remove local definition!"
	#undef countof
#endif
#define _countof(array)(sizeof(array) / sizeof(array[0]))


extern usb_module_info* gUSBModule;

extern "C" status_t usb_audio_device_added(usb_device device, void** cookie);
extern "C" status_t usb_audio_device_removed(void* cookie);

extern "C" status_t init_hardware();
extern "C" void uninit_driver();

extern "C" const char** publish_devices();
extern "C" device_hooks *find_device(const char* name);


#endif // _USB_AUDIO_DRIVER_H_

