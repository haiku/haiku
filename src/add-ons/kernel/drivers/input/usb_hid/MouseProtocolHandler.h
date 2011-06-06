/*
 * Copyright 2008-2011 Michael Lotz <mmlr@mlotz.ch>
 * Distributed under the terms of the MIT license.
 */
#ifndef USB_MOUSE_PROTOCOL_HANDLER_H
#define USB_MOUSE_PROTOCOL_HANDLER_H


#include <InterfaceDefs.h>

#include "ProtocolHandler.h"


class HIDCollection;
class HIDReportItem;


#ifndef B_MAX_MOUSE_BUTTONS
#	define B_MAX_MOUSE_BUTTONS		8
#endif


class MouseProtocolHandler : public ProtocolHandler {
public:
								MouseProtocolHandler(HIDReport &report,
									bool tablet, HIDReportItem &xAxis,
									HIDReportItem &yAxis);

	static	void				AddHandlers(HIDDevice &device,
									HIDCollection &collection,
									ProtocolHandler *&handlerList);

	virtual	status_t			Control(uint32 *cookie, uint32 op, void *buffer,
									size_t length);

private:
			status_t			_ReadReport(void *buffer);

			HIDReport &			fReport;
			bool				fTablet;

			HIDReportItem &		fXAxis;
			HIDReportItem &		fYAxis;
			HIDReportItem *		fWheel;
			HIDReportItem *		fButtons[B_MAX_MOUSE_BUTTONS];

			uint32				fLastButtons;
			uint32				fClickCount;
			bigtime_t			fLastClickTime;
			bigtime_t			fClickSpeed;
};

#endif // USB_MOUSE_PROTOCOL_HANDLER_H
