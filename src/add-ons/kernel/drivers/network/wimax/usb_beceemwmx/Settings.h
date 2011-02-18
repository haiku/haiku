/*
 *	Beceem WiMax USB Driver
 *	Copyright 2010-2011 Haiku, Inc. All rights reserved.
 *	Distributed under the terms of the MIT license.
 *
 *	Authors:
 *		Alexander von Gluck, <kallisti5@unixzen.com>
 *
 *	Partially using:
 *		USB Ethernet Control Model devices
 *			(c) 2008 by Michael Lotz, <mmlr@mlotz.ch>
 *		ASIX AX88172/AX88772/AX88178 USB 2.0 Ethernet Driver
 *			(c) 2008 by S.Zharski, <imker@gmx.li>
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

