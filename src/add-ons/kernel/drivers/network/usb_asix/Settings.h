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
#ifndef _USB_ASIX_SETTINGS_H_
#define _USB_ASIX_SETTINGS_H_


#include <driver_settings.h>

#include "Driver.h"


#ifdef _countof
#warning "_countof(...) WAS ALREADY DEFINED!!! Remove local definition!"
#undef _countof
#endif
#define _countof(array)(sizeof(array) / sizeof(array[0]))


void load_settings();
void release_settings();

void usb_asix_trace(bool force, const char *func, const char *fmt, ...);


#define	TRACE(x...)			usb_asix_trace(false, __func__, x)
#define TRACE_ALWAYS(x...)	usb_asix_trace(true,  __func__, x)

extern bool gTraceFlow;
#define TRACE_FLOW(x...)	usb_asix_trace(gTraceFlow, NULL, x)

#define TRACE_RET(result)	usb_asix_trace(false, __func__, \
									"Returns:%#010x\n", result);


#endif // _USB_ASIX_SETTINGS_H_
