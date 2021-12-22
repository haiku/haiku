/*
 * Copyright 2008-2011 Michael Lotz <mmlr@mlotz.ch>
 * Distributed under the terms of the MIT license.
 */
#ifndef USB_KEYBOARD_PROTOCOL_HANDLER_H
#define USB_KEYBOARD_PROTOCOL_HANDLER_H


#include "ProtocolHandler.h"

#include <lock.h>


class HIDReportItem;
class HIDCollection;


#define MAX_MODIFIERS	16
#define MAX_KEYS		128
#define MAX_LEDS		3


class KeyboardProtocolHandler : public ProtocolHandler {
public:
								KeyboardProtocolHandler(HIDReport &inputReport,
									HIDReport *outputReport);
	virtual						~KeyboardProtocolHandler();

	static	void				AddHandlers(HIDDevice &device,
									HIDCollection &collection,
									ProtocolHandler *&handlerList);

	virtual	status_t			Open(uint32 flags, uint32 *cookie);
	virtual	status_t			Close(uint32 *cookie);

	virtual	status_t			Control(uint32 *cookie, uint32 op, void *buffer,
									size_t length);

private:
			void				_WriteKey(uint32 key, bool down);
			status_t			_SetLEDs(uint8 *data);
			status_t			_ReadReport(bigtime_t timeout, uint32 *cookie);

private:
			mutex				fLock;

			HIDReport &			fInputReport;
			HIDReport *			fOutputReport;

			HIDReportItem *		fModifiers[MAX_MODIFIERS];
			HIDReportItem *		fKeys[MAX_KEYS];
			HIDReportItem *		fLEDs[MAX_LEDS];

			bigtime_t			fRepeatDelay;
			bigtime_t			fRepeatRate;
			bigtime_t			fCurrentRepeatDelay;
			uint32				fCurrentRepeatKey;

			uint32				fKeyCount;
			uint32				fModifierCount;

			uint8				fLastModifiers;
			uint16 *			fCurrentKeys;
			uint16 *			fLastKeys;

			int32				fHasReader;
			bool				fHasDebugReader;
};


#endif // USB_KEYBOARD_PROTOCOL_HANDLER_H
