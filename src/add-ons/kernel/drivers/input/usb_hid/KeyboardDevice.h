/*
	Driver for USB Human Interface Devices.
	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
	Distributed under the terms of the MIT license.
*/
#ifndef _USB_KEYBOARD_DEVICE_H_
#define _USB_KEYBOARD_DEVICE_H_

#include "HIDDevice.h"

class KeyboardDevice : public HIDDevice {
public:
								KeyboardDevice(usb_device device,
									usb_pipe interruptPipe,
									size_t interfaceIndex,
									report_insn *instructions,
									size_t instructionCount,
									size_t totalReportSize);
virtual							~KeyboardDevice();

virtual	status_t				Control(uint32 op, void *buffer, size_t length);

private:
		void					_WriteKey(uint32 key, bool down);
		status_t				_SetLEDs(uint8 *data);
		status_t				_InterpretBuffer();

		bigtime_t				fRepeatDelay;
		bigtime_t				fRepeatRate;
		bigtime_t				fCurrentRepeatDelay;
		uint32					fCurrentRepeatKey;

		uint8 *					fLastTransferBuffer;
};

#endif // _USB_KEYBOARD_DEVICE_H_
