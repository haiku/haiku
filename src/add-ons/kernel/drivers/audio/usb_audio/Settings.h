/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009,10,12 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _USB_AUDIO_SETTINGS_H_
#define _USB_AUDIO_SETTINGS_H_


#include <driver_settings.h>

#include "Driver.h"

void load_settings();
void release_settings();

void usb_audio_trace(bool force, const char *func, const char *fmt, ...);

#ifdef TRACE
#undef TRACE
#endif

#define TRACE(x...)			usb_audio_trace(false,	__func__, x)
#define TRACE_ALWAYS(x...)	usb_audio_trace(true,	__func__, x)

extern bool gTraceFlow;
#define TRACE_FLOW(x...)	usb_audio_trace(gTraceFlow, NULL, x)

#define TRACE_RET(result)	usb_audio_trace(false, __func__, \
									"Returns:%#010x\n", result);

#endif /*_USB_AUDIO_SETTINGS_H_*/

