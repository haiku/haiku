/*
	Driver for USB Human Interface Devices.
	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
	Distributed under the terms of the MIT license.
*/
#ifndef _USB_MOUSE_DEVICE_H_
#define _USB_MOUSE_DEVICE_H_

#include "HIDDevice.h"

class MouseDevice : public HIDDevice {
public:
								MouseDevice(usb_device device,
									usb_pipe interruptPipe,
									size_t interfaceIndex,
									report_insn *instructions,
									size_t instructionCount,
									size_t totalReportSize);

virtual	status_t				Control(uint32 op, void *buffer, size_t length);

private:
		status_t				_InterpretBuffer();

		uint32					fLastButtons;
		uint32					fClickCount;
		bigtime_t				fLastClickTime;
		bigtime_t				fClickSpeed;
		uint32					fMaxButtons;
};

#endif // _USB_MOUSE_DEVICE_H_
