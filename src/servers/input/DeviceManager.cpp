/*
** Copyright 2004, the OpenBeOS project. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
**
** Author : Jérôme Duval
** Original authors: Marcus Overhagen, Axel Dörfler
*/


#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>

#include <image.h>
#include <stdio.h>
#include <string.h>

#include "DeviceManager.h"
#include "InputServer.h"


DeviceManager::DeviceManager()
	:
 	fLock("device manager")
{
}


DeviceManager::~DeviceManager()
{
}


void
DeviceManager::LoadState()
{
	class IAHandler : public AddOnMonitorHandler {
	private:
		DeviceManager * fManager;
	public:
		IAHandler(DeviceManager * manager) {
			fManager = manager;
		}
		virtual void	AddOnCreated(const add_on_entry_info * entry_info) {
		}
		virtual void	AddOnEnabled(const add_on_entry_info * entry_info) {
			CALLED();
			entry_ref ref;
			make_entry_ref(entry_info->dir_nref.device, entry_info->dir_nref.node,
			               entry_info->name, &ref);
			BEntry entry(&ref, false);
			//fManager->RegisterAddOn(entry);
		}
		virtual void	AddOnDisabled(const add_on_entry_info * entry_info) {
			CALLED();
			entry_ref ref;
			make_entry_ref(entry_info->dir_nref.device, entry_info->dir_nref.node,
			               entry_info->name, &ref);
			BEntry entry(&ref, false);
			//fManager->UnregisterAddOn(entry);
		}
		virtual void	AddOnRemoved(const add_on_entry_info * entry_info) {
		}
	};

	fHandler = new IAHandler(this);
	fAddOnMonitor = new AddOnMonitor(fHandler);
}


void
DeviceManager::SaveState()
{
	CALLED();
	BMessenger messenger(fAddOnMonitor);
	messenger.SendMessage(B_QUIT_REQUESTED);
	int32 exit_value;
	wait_for_thread(fAddOnMonitor->Thread(), &exit_value);
	delete fHandler;
}


status_t 
DeviceManager::StartMonitoringDevice(_BDeviceAddOn_ *addon, 
				const char *device)
{
	status_t err;
	node_ref nref;
	BDirectory directory;
	BPath path("/dev");
	if (((err = path.Append(device)) == B_OK)
		&& ((err = directory.SetTo(path.Path())) == B_OK) 
		&& ((err = directory.GetNodeRef(&nref)) == B_OK)
		&& ((err = fHandler->AddDirectory(&nref)) == B_OK) ) {
		return B_OK;
	} else
		return err;
}


status_t 
DeviceManager::StopMonitoringDevice(_BDeviceAddOn_ *addon, 
				const char *device)
{
	status_t err;
	node_ref nref;
	BDirectory directory;
	BPath path("/dev");
	if (((err = path.Append(device)) == B_OK)
		&& ((err = directory.SetTo(path.Path())) == B_OK) 
		&& ((err = directory.GetNodeRef(&nref)) == B_OK)
		/*&& ((err = fHandler->RemoveDirectory(&nref)) ==B_OK)*/) {
		return B_OK;
	} else
		return err;
}