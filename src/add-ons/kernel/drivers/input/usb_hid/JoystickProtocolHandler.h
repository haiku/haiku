/*
 * Copyright 2008-2011 Michael Lotz <mmlr@mlotz.ch>
 * Distributed under the terms of the MIT license.
 */
#ifndef USB_JOYSTICK_PROTOCOL_HANDLER_H
#define USB_JOYSTICK_PROTOCOL_HANDLER_H


#include <InterfaceDefs.h>

#include "ProtocolHandler.h"

#include <joystick_driver.h>


class HIDCollection;
class HIDReportItem;


class JoystickProtocolHandler : public ProtocolHandler {
public:
									JoystickProtocolHandler(HIDReport &report);

	static	void					AddHandlers(HIDDevice &device,
										HIDCollection &collection,
										ProtocolHandler *&handlerList);

	virtual	status_t				Read(uint32 *cookie, off_t position,
										void *buffer, size_t *numBytes);
	virtual	status_t				Write(uint32 *cookie, off_t position,
										const void *buffer, size_t *numBytes);

	virtual	status_t				Control(uint32 *cookie, uint32 op,
										void *buffer, size_t length);

private:
			status_t				_ReadReport();

private:
			HIDReport &				fReport;

			HIDReportItem *			fAxis[MAX_AXES];
			HIDReportItem *			fButtons[MAX_BUTTONS];

			joystick_module_info 	fJoystickModuleInfo;
};

#endif // USB_JOYSTICK_PROTOCOL_HANDLER_H
