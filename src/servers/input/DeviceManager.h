/*
** Copyright 2004, the OpenBeOS project. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
**
** Author : Jérôme Duval
** Original authors: Marcus Overhagen, Axel Dörfler
*/
#ifndef _DEVICE_MANAGER_H
#define _DEVICE_MANAGER_H

// Manager for devices monitoring

#include <Locker.h>
#include <InputServerDevice.h>
#include <InputServerFilter.h>
#include <InputServerMethod.h>
#include "AddOnMonitor.h"
#include "AddOnMonitorHandler.h"
#include "TList.h"

class DeviceManager {
	public:
		DeviceManager();
		~DeviceManager();

		void		LoadState();
		void		SaveState();
		
		status_t StartMonitoringDevice(_BDeviceAddOn_ *addon, 
				const char *device);
		status_t StopMonitoringDevice(_BDeviceAddOn_ *addon, 
				const char *device);

	private:
		
	private:
		
		BLocker fLock;
		
		AddOnMonitorHandler	*fHandler;
		AddOnMonitor		*fAddOnMonitor;
};

#endif // _DEVICE_MANAGER_H
