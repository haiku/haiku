/*
** Copyright 2004, the Haiku project. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Author : Jérôme Duval
** Original authors: Marcus Overhagen, Axel Dörfler
*/
#ifndef _DEVICE_MANAGER_H
#define _DEVICE_MANAGER_H

// Manager for devices monitoring

#include <Handler.h>
#include <Locker.h>
#include <InputServerDevice.h>
#include <InputServerFilter.h>
#include <InputServerMethod.h>
#include "TList.h"

class DeviceManager;

class IAHandler : public BHandler {
	public:
		IAHandler(DeviceManager * manager);
		void MessageReceived(BMessage * msg);
		status_t AddDirectory(const node_ref * nref, _BDeviceAddOn_ *addon);
		status_t RemoveDirectory(const node_ref * nref, _BDeviceAddOn_ *addon);
	private:
		DeviceManager * fManager;
		
};

class DeviceManager : public BLooper {
	public:
		DeviceManager();
		~DeviceManager();

		void		LoadState();
		void		SaveState();
		
		status_t StartMonitoringDevice(_BDeviceAddOn_ *addon, 
				const char *device);
		status_t StopMonitoringDevice(_BDeviceAddOn_ *addon, 
				const char *device);
		_BDeviceAddOn_ *GetAddOn(int32 index) {
			return (_BDeviceAddOn_*)fDeviceAddons.ItemAt(index);
		}
	private:
		BList 	fDeviceAddons;
		BLocker fLock;
		IAHandler	*fHandler;
};

#endif // _DEVICE_MANAGER_H
