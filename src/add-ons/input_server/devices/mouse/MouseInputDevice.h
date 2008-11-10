/*
 * Copyright 2004-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini
 */
#ifndef MOUSE_INPUT_DEVICE_H
#define MOUSE_INPUT_DEVICE_H


#include <InputServerDevice.h>
#include <InterfaceDefs.h>
#include <Locker.h>

#include <ObjectList.h>


class MouseDevice;

class MouseInputDevice : public BInputServerDevice {
public:
							MouseInputDevice();
	virtual					~MouseInputDevice();

	virtual status_t		InitCheck();

	virtual status_t		Start(const char* name, void* cookie);
	virtual status_t		Stop(const char* name, void* cookie);

	virtual status_t		Control(const char* name, void* cookie,
								uint32 command, BMessage* message);

private:
	friend class MouseDevice;
	// TODO: needed by the control thread to remove a dead device
	// find a better way...

			status_t		_HandleMonitor(BMessage* message);
			void			_RecursiveScan(const char* directory);

			MouseDevice*	_FindDevice(const char* path) const;
			status_t		_AddDevice(const char* path);
			status_t		_RemoveDevice(const char* path);

private:
			BObjectList<MouseDevice> fDevices;
			BLocker			fDeviceListLock;
};

extern "C" BInputServerDevice* instantiate_input_device();

#endif	// MOUSE_INPUT_DEVICE_H
