/*
 * Copyright 2004-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */
#ifndef TOUCHPAD_INPUT_DEVICE_H
#define TOUCHPAD_INPUT_DEVICE_H


#include <InputServerDevice.h>
#include <InterfaceDefs.h>

#include <ObjectList.h>

#include <stdio.h>


class TouchpadDevice;

class TouchpadInputDevice : public BInputServerDevice {
public:
								TouchpadInputDevice();
	virtual						~TouchpadInputDevice();

	virtual	status_t			InitCheck();

	virtual	status_t			Start(const char* name, void* cookie);
	virtual	status_t			Stop(const char* name, void* cookie);

	virtual status_t			Control(const char* name, void* cookie,
									uint32 command, BMessage* message);

	private:
	friend class TouchpadDevice;
	// TODO: needed by the control thread to remove a dead device
	// find a better way...

			status_t			_HandleMonitor(BMessage* message);
			void				_RecursiveScan(const char* directory);

			TouchpadDevice*		_FindDevice(const char* path);
			status_t			_AddDevice(const char* path);
			status_t			_RemoveDevice(const char* path);

			BObjectList<TouchpadDevice> fDevices;
};

extern "C" BInputServerDevice* instantiate_input_device();

#endif	// TOUCHPAD_INPUT_DEVICE_H
