/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HYPERV_HID_DRIVER_H_
#define _HYPERV_HID_DRIVER_H_

#include <Drivers.h>
#include <KernelExport.h>
#include <OS.h>
#include <util/kernel_cpp.h>

#define DRIVER_NAME			"hyperv_hid"
#define DEVICE_PATH_SUFFIX	"hyperv"
#define DEVICE_NAME			"Hyper-V HID"

//#define TRACE_HYPERV_HID
#ifdef TRACE_HYPERV_HID
#	define TRACE(x...) dprintf("\33[94mhyperv_hid:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[94mhyperv_hid:\33[0m " x)
#define ERROR(x...)			dprintf("\33[94mhyperv_hid:\33[0m " x)
#define CALLED(x...)		TRACE("CALLED %s\n", __PRETTY_FUNCTION__)

#define HYPERV_HID_DRIVER_MODULE_NAME		"drivers/input/hyperv_hid/driver_v1"
#define HYPERV_HID_DEVICE_MODULE_NAME		"drivers/input/hyperv_hid/device_v1"


#endif // _HYPERV_HID_DRIVER_H_
