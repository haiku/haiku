/*
	Copyright (C) 2008-2009 Michael Lotz <mmlr@mlotz.ch>
	Distributed under the terms of the MIT license.
*/
#ifndef USB_KEYBOARD_DEVICE_H
#define USB_KEYBOARD_DEVICE_H

#include "ProtocolHandler.h"

#define MAX_MODIFIERS	16
#define MAX_KEYS		16
#define MAX_LEDS		3

class HIDReportItem;

class KeyboardDevice : public ProtocolHandler {
public:
								KeyboardDevice(HIDReport *inputReport,
									HIDReport *outputReport);
virtual							~KeyboardDevice();

static	ProtocolHandler *		AddHandler(HIDDevice *device);

virtual	status_t				Open(uint32 flags);
virtual	status_t				Control(uint32 op, void *buffer, size_t length);

private:
		void					_WriteKey(uint32 key, bool down);
		status_t				_SetLEDs(uint8 *data);
		status_t				_ReadReport(bigtime_t timeout);

		HIDReport *				fInputReport;
		HIDReport *				fOutputReport;

		HIDReportItem *			fModifiers[MAX_MODIFIERS];
		HIDReportItem *			fKeys[MAX_KEYS];
		HIDReportItem *			fLEDs[MAX_LEDS];

		bigtime_t				fRepeatDelay;
		bigtime_t				fRepeatRate;
		bigtime_t				fCurrentRepeatDelay;
		uint32					fCurrentRepeatKey;

		uint32					fKeyCount;
		uint32					fModifierCount;

		uint8					fLastModifiers;
		uint16 *				fCurrentKeys;
		uint16 *				fLastKeys;
};

#endif // USB_KEYBOARD_DEVICE_H
