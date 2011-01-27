/*
 *	Ubee WiMax USB Driver.
 *	Copyright (c) 2010 Alexander von Gluck <kallisti5@unixzen.com>
 *	Distributed under the terms of the MIT license.
 *	
 *	Heavily based on code of :
 *	Driver for USB Ethernet Control Model devices
 *	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
 *	Distributed under the terms of the MIT license.
 *
 */

#ifndef _USB_BECEEM_SETTINGS_H_ 
#define _USB_BECEEM_SETTINGS_H_

#include <driver_settings.h> 

#include "Driver.h"

void load_settings();
void release_settings();

void usb_beceem_trace(bool force, const char *func, const char *fmt, ...);

#define	TRACE(x...)			usb_beceem_trace(false, __func__, x)
#define TRACE_ALWAYS(x...)	usb_beceem_trace(true,  __func__, x)

extern bool gTraceFlow;
#define TRACE_FLOW(x...)	usb_beceem_trace(gTraceFlow, NULL, x)

#define TRACE_RET(result)	usb_beceem_trace(false, __func__, \
									"Returns:%#010x\n", result);

#endif /*_USB_BECEEM_SETTINGS_H_*/ 

