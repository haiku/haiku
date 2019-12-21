/*
 * Copyright 2004-2013, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Jérôme Duval
 *		Marcus Overhagen
 *		John Scipione, jscipione@gmail.com
 */
#ifndef ADD_ON_MANAGER_H
#define ADD_ON_MANAGER_H


#include <InputServerDevice.h>
#include <InputServerFilter.h>
#include <InputServerMethod.h>
#include <Locker.h>
#include <Looper.h>

#include <AddOnMonitor.h>
#include <AddOnMonitorHandler.h>

#include "PathList.h"


using namespace BPrivate;

class AddOnManager : public AddOnMonitor {
public:
								AddOnManager();
								~AddOnManager();

	virtual	void 				MessageReceived(BMessage* message);

			void				LoadState();
			void				SaveState();

			status_t			StartMonitoringDevice(DeviceAddOn* addOn,
									const char* device);
			status_t			StopMonitoringDevice(DeviceAddOn* addOn,
									const char* device);

private:
			void				_RegisterAddOns();
			void				_UnregisterAddOns();

			status_t			_RegisterAddOn(BEntry& entry);
			status_t			_UnregisterAddOn(BEntry& entry);

			bool				_IsDevice(const char* path) const;
			bool				_IsFilter(const char* path) const;
			bool				_IsMethod(const char* path) const;

			status_t			_RegisterDevice(BInputServerDevice* device,
									const entry_ref& ref, image_id image);
			status_t			_RegisterFilter(BInputServerFilter* filter,
									const entry_ref& ref, image_id image);
			status_t			_RegisterMethod(BInputServerMethod* method,
									const entry_ref& ref, image_id image);

			status_t			_HandleFindDevices(BMessage* message,
									BMessage* reply);
			status_t			_HandleWatchDevices(BMessage* message,
									BMessage* reply);
			status_t			_HandleNotifyDevice(BMessage* message,
									BMessage* reply);
			status_t			_HandleIsDeviceRunning(BMessage* message,
									BMessage* reply);
			status_t			_HandleStartStopDevices(BMessage* message,
									BMessage* reply);
			status_t			_HandleControlDevices(BMessage* message,
									BMessage* reply);
			status_t			_HandleSystemShuttingDown(BMessage* message,
									BMessage* reply);
			status_t			_HandleMethodReplicant(BMessage* message,
									BMessage* reply);
			void				_HandleDeviceMonitor(BMessage* message);

			void				_LoadReplicant();
			void				_UnloadReplicant();
			int32				_GetReplicantAt(BMessenger target,
									int32 index) const;
			status_t			_GetReplicantName(BMessenger target,
									int32 uid, BMessage* reply) const;
			status_t			_GetReplicantView(BMessenger target, int32 uid,
									BMessage* reply) const;

			status_t			_AddDevicePath(DeviceAddOn* addOn,
									const char* path, bool& newPath);
			status_t			_RemoveDevicePath(DeviceAddOn* addOn,
									const char* path, bool& lastPath);

private:
	class MonitorHandler;
	friend class MonitorHandler;

	template<typename T> struct add_on_info {
		add_on_info()
			:
			image(-1), add_on(NULL)
		{
		}

		~add_on_info()
		{
			delete add_on;
			if (image >= 0)
				unload_add_on(image);
		}

		entry_ref				ref;
		image_id				image;
		T*						add_on;
	};
	typedef struct add_on_info<BInputServerDevice> device_info;
	typedef struct add_on_info<BInputServerFilter> filter_info;
	typedef struct add_on_info<BInputServerMethod> method_info;

			BObjectList<device_info> fDeviceList;
			BObjectList<filter_info> fFilterList;
			BObjectList<method_info> fMethodList;

			BObjectList<DeviceAddOn> fDeviceAddOns;
			PathList			fDevicePaths;

			MonitorHandler*		fHandler;
			BMessenger			fWatcherMessenger;

			bool				fSafeMode;
};

#endif	// ADD_ON_MANAGER_H
