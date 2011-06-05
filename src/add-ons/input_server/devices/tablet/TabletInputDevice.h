/*
 * Copyright 2004-2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini
 *		Michael Lotz, mmlr@mlotz.ch
 */
#ifndef TABLET_INPUT_DEVICE_H
#define TABLET_INPUT_DEVICE_H


#include <InputServerDevice.h>
#include <InterfaceDefs.h>
#include <Locker.h>

#include <ObjectList.h>


class TabletDevice;

class TabletInputDevice : public BInputServerDevice {
public:
							TabletInputDevice();
	virtual					~TabletInputDevice();

	virtual status_t		InitCheck();

	virtual status_t		Start(const char* name, void* cookie);
	virtual status_t		Stop(const char* name, void* cookie);

	virtual status_t		Control(const char* name, void* cookie,
								uint32 command, BMessage* message);

private:
	friend class TabletDevice;
	// TODO: needed by the control thread to remove a dead device
	// find a better way...

			status_t		_HandleMonitor(BMessage* message);
			void			_RecursiveScan(const char* directory);

			TabletDevice*	_FindDevice(const char* path) const;
			status_t		_AddDevice(const char* path);
			status_t		_RemoveDevice(const char* path);

private:
			BObjectList<TabletDevice> fDevices;
			BLocker			fDeviceListLock;
};

extern "C" BInputServerDevice* instantiate_input_device();

#endif	// TABLET_INPUT_DEVICE_H
