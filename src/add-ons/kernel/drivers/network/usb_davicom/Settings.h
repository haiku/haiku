/*
 *	ASIX AX88172/AX88772/AX88178 USB 2.0 Ethernet Driver.
 *	Copyright (c) 2008 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *	
 *	Heavily based on code of the 
 *	Driver for USB Ethernet Control Model devices
 *	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
 *	Distributed under the terms of the MIT license.
 *
 */

#ifndef _USB_DAVICOM_SETTINGS_H_ 
  #define _USB_DAVICOM_SETTINGS_H_

#include <driver_settings.h> 

#include "Driver.h"

void load_settings();
void release_settings();

void usb_davicom_trace(bool force, const char *func, const char *fmt, ...);

#define	TRACE(x...)			usb_davicom_trace(false, __func__, x)
#define TRACE_ALWAYS(x...)	usb_davicom_trace(true,  __func__, x)

extern bool gTraceState;
#define TRACE_STATE(x...)	usb_davicom_trace(gTraceState, NULL, x)

extern bool gTraceRX;
#define TRACE_RX(x...)	usb_davicom_trace(gTraceRX, NULL, x)

extern bool gTraceTX;
#define TRACE_TX(x...)	usb_davicom_trace(gTraceTX, NULL, x)

#define TRACE_RET(result)	usb_davicom_trace(false, __func__, \
									"Returns:%#010x\n", result);

#endif /*_USB_DAVICOM_SETTINGS_H_*/ 
