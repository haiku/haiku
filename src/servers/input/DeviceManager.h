/*
 * Copyright 2004-2008, the Haiku project. All rights reserved.
 * Distributed under the terms of the Haiku License.
 *
 * Authors:
 *		Marcus Overhagen, Axel Dörfler
 *		Jérôme Duval
 */
#ifndef _DEVICE_MANAGER_H
#define _DEVICE_MANAGER_H

//! Manager for devices monitoring

#include <Handler.h>
#include <Looper.h>
#include <Locker.h>
#include <Node.h>

#include <InputServerDevice.h>
#include <InputServerFilter.h>
#include <InputServerMethod.h>

#include "TList.h"


class DeviceManager : public BLooper {
public:
							DeviceManager();
	virtual					~DeviceManager();

			void			LoadState();
			void			SaveState();

			status_t		StartMonitoringDevice(_BDeviceAddOn_ *addon,
								const char *device);
			status_t		StopMonitoringDevice(_BDeviceAddOn_ *addon,
								const char *device);
			_BDeviceAddOn_	*GetAddOn(int32 index)
								{ return (_BDeviceAddOn_*)
									fDeviceAddons.ItemAt(index); }

	virtual void			MessageReceived(BMessage *msg);

private:
			status_t		AddDirectory(const node_ref *nref,
								_BDeviceAddOn_ *addon);
			status_t		RemoveDirectory(const node_ref *nref,
								_BDeviceAddOn_ *addon);

			BList			fDeviceAddons;
			BLocker			fLock;
};

#endif	// _DEVICE_MANAGER_H
