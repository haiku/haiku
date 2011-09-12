/*
 *	Davicom DM9601 USB 1.1 Ethernet Driver.
 *	Copyright (c) 2008, 2011 Siarzhuk Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 */
#ifndef _USB_DAVICOM_SETTINGS_H_
#define _USB_DAVICOM_SETTINGS_H_


#ifdef _countof
#warning "_countof(...) WAS ALREADY DEFINED!!! Remove local definition!"
#undef _countof
#endif
#define _countof(array)(sizeof(array) / sizeof(array[0]))


void load_settings();
void release_settings();
void usb_davicom_trace(bool force, const char *func, const char *fmt, ...);


#define	TRACE(x...)			usb_davicom_trace(false, __func__, x)
#define TRACE_ALWAYS(x...)	usb_davicom_trace(true,  __func__, x)


#ifdef UDAV_TRACE

extern bool gTraceState;
extern bool gTraceRX;
extern bool gTraceTX;
extern bool gTraceStats;
#define TRACE_STATE(x...)	usb_davicom_trace(gTraceState, NULL, x)
#define TRACE_STATS(x...)	usb_davicom_trace(gTraceStats, NULL, x)
#define TRACE_RX(x...)	usb_davicom_trace(gTraceRX, NULL, x)
#define TRACE_TX(x...)	usb_davicom_trace(gTraceTX, NULL, x)

#else 

#define TRACE_STATE(x...)
#define TRACE_STATS(x...)
#define TRACE_RX(x...)
#define TRACE_TX(x...)

#endif


#endif // _USB_DAVICOM_SETTINGS_H_

