/*
** Copyright 2004, the Haiku project. All rights reserved.
** Distributed under the terms of the MIT License.
**
** Author : Jérôme Duval
** Original authors: Marcus Overhagen, Axel Dörfler
*/
#ifndef _DEVICE_MANAGER_H
#define _DEVICE_MANAGER_H

// Manager for devices monitoring
#include <Handler.h>
#include <Node.h>
#include <Looper.h>
#include <Locker.h>


class DeviceManager : public BLooper {
	public:
		DeviceManager();
		~DeviceManager();

		void		LoadState();
		void		SaveState();
		
		status_t StartMonitoringDevice(const char* device);
		status_t StopMonitoringDevice(const char* device);

		void MessageReceived(BMessage *msg);
		
	private:
		status_t AddDirectory(node_ref* nref);
		status_t RemoveDirectory(node_ref* nref);
		status_t AddDevice(entry_ref* nref);
		
		BLocker fLock;
};

#endif // _DEVICE_MANAGER_H
