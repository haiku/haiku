/*
 * Copyright 2008-2011 Michael Lotz <mmlr@mlotz.ch>
 * Distributed under the terms of the MIT license.
 */
#ifndef USB_JOYSTICK_PROTOCOL_HANDLER_H
#define USB_JOYSTICK_PROTOCOL_HANDLER_H


#include <InterfaceDefs.h>

#include "ProtocolHandler.h"

#include <joystick_driver.h>
#include <lock.h>


class HIDCollection;
class HIDReportItem;


class JoystickProtocolHandler : public ProtocolHandler {
public:
									JoystickProtocolHandler(HIDReport &report);
	virtual							~JoystickProtocolHandler();

	static	void					AddHandlers(HIDDevice &device,
										HIDCollection &collection,
										ProtocolHandler *&handlerList);

	virtual	status_t				Open(uint32 flags, uint32 *cookie);
	virtual	status_t				Close(uint32 *cookie);

	virtual	status_t				Read(uint32 *cookie, off_t position,
										void *buffer, size_t *numBytes);
	virtual	status_t				Write(uint32 *cookie, off_t position,
										const void *buffer, size_t *numBytes);

	virtual	status_t				Control(uint32 *cookie, uint32 op,
										void *buffer, size_t length);

private:
	static	int32					_UpdateThread(void *data);
			status_t				_Update();

			HIDReport &				fReport;

			uint32					fAxisCount;
			HIDReportItem **		fAxis;
			uint32					fHatCount;
			HIDReportItem **		fHats;
			uint32					fButtonCount;
			uint32					fMaxButton;
			HIDReportItem **		fButtons;

			joystick_module_info 	fJoystickModuleInfo;
			variable_joystick		fCurrentValues;

			mutex					fUpdateLock;
			thread_id				fUpdateThread;
};

#endif // USB_JOYSTICK_PROTOCOL_HANDLER_H
