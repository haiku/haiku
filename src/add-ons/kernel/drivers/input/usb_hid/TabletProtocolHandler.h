/*
 * Copyright 2013 Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2010-2011 Enrique Medina Gremaldos <quiqueiii@gmail.com>
 * Copyright 2008-2011 Michael Lotz <mmlr@mlotz.ch>
 * Distributed under the terms of the MIT license.
 */
#ifndef USB_TABLET_PROTOCOL_HANDLER_H
#define USB_TABLET_PROTOCOL_HANDLER_H


#include <InterfaceDefs.h>

#include "ProtocolHandler.h"


class HIDCollection;
class HIDReportItem;


#ifndef B_MAX_MOUSE_BUTTONS
#	define B_MAX_MOUSE_BUTTONS		8
#endif

#ifndef B_MAX_DIGITIZER_SWITCHES
	#define B_MAX_DIGITIZER_SWITCHES	5
#endif

class TabletProtocolHandler : public ProtocolHandler {
public:
								TabletProtocolHandler(HIDReport &report,
									HIDReportItem &xAxis,
									HIDReportItem &yAxis);

	static	void				AddHandlers(HIDDevice &device,
									HIDCollection &collection,
									ProtocolHandler *&handlerList);

	virtual	status_t			Control(uint32 *cookie, uint32 op, void *buffer,
									size_t length);

private:
			status_t			_ReadReport(void *buffer, uint32 *cookie);

			HIDReport &			fReport;

			HIDReportItem &		fXAxis;
			HIDReportItem &		fYAxis;
			HIDReportItem *		fWheel;
			HIDReportItem *		fButtons[B_MAX_MOUSE_BUTTONS];
			HIDReportItem *		fSwitches[B_MAX_DIGITIZER_SWITCHES];

			HIDReportItem *		fPressure;
			HIDReportItem *		fInRange;
			HIDReportItem *		fTip;
			HIDReportItem *		fBarrelSwitch;
			HIDReportItem *		fEraser;
			HIDReportItem *		fXTilt;
			HIDReportItem *		fYTilt;

			uint32				fLastButtons;
			uint32				fLastSwitches;
			uint32				fClickCount;
			bigtime_t			fLastClickTime;
			bigtime_t			fClickSpeed;
};

#endif // USB_TABLET_PROTOCOL_HANDLER_H
