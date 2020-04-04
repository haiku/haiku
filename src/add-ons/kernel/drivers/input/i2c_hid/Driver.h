/*
	Driver for I2C Human Interface Devices.
	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
	Distributed under the terms of the MIT license.
*/
#ifndef _I2C_HID_DRIVER_H_
#define _I2C_HID_DRIVER_H_

#include <Drivers.h>
#include <KernelExport.h>
#include <OS.h>
#include <util/kernel_cpp.h>

#include "DeviceList.h"

#define DRIVER_NAME	"i2c_hid"
#define DEVICE_PATH_SUFFIX	"i2c"


extern DeviceList *gDeviceList;


//#define TRACE_I2C_HID
#ifdef TRACE_I2C_HID
#	define TRACE(x...) dprintf(DRIVER_NAME ": " x)
#else
#	define TRACE(x...)
#endif
#define ERROR(x...) dprintf(DRIVER_NAME ": " x)
#define TRACE_ALWAYS(x...)	dprintf(DRIVER_NAME ": " x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)

#endif //_I2C_HID_DRIVER_H_
