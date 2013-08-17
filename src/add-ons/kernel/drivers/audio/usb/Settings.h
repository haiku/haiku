/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009-13 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _USB_AUDIO_SETTINGS_H_
#define _USB_AUDIO_SETTINGS_H_

#include <SupportDefs.h>

enum {
	ERR = 0x00000001,
	INF = 0x00000002,
	MIX = 0x00000004,
	API = 0x00000008,
	DTA = 0x00000010,
	ISO = 0x00000020,
	UAC = 0x00000040
};

void load_settings();
void release_settings();

void usb_audio_trace(uint32 bits, const char* func, const char* fmt, ...);

#define TRACE_USB_AUDIO

#ifdef TRACE
#undef TRACE
#endif

#ifdef TRACE_USB_AUDIO
#define TRACE(__mask__, x...) usb_audio_trace(__mask__, __func__, x)
#else
#define TRACE(__mask__, x...) // nothing
#endif

#endif // _USB_AUDIO_SETTINGS_H_

