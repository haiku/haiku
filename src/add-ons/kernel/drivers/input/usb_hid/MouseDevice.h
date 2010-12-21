/*
 * Copyright 2008-2009 Michael Lotz <mmlr@mlotz.ch>
 * Distributed under the terms of the MIT license.
 */
#ifndef USB_MOUSE_DEVICE_H
#define USB_MOUSE_DEVICE_H


#include <InterfaceDefs.h>

#include "ProtocolHandler.h"


class HIDReportItem;


#ifndef B_MAX_MOUSE_BUTTONS
#	define B_MAX_MOUSE_BUTTONS		8
#endif


class MouseDevice : public ProtocolHandler {
public:
								MouseDevice(HIDReport *report,
									HIDReportItem *xAxis, HIDReportItem *yAxis);

	static	ProtocolHandler *	AddHandler(HIDDevice *device,
									HIDReport *report);

	virtual	status_t			Control(uint32 *cookie, uint32 op, void *buffer,
									size_t length);

private:
			status_t			_ReadReport();

private:
			HIDReport *			fReport;

			HIDReportItem *		fXAxis;
			HIDReportItem *		fYAxis;
			HIDReportItem *		fWheel;
			HIDReportItem *		fButtons[B_MAX_MOUSE_BUTTONS];

			uint32				fLastButtons;
			uint32				fClickCount;
			bigtime_t			fLastClickTime;
			bigtime_t			fClickSpeed;
};

#endif // USB_MOUSE_DEVICE_H
