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

#include "AddOnManager.h"
#include "InputServer.h"


AddOnManager::AddOnManager()
	:
 	fLock("add-on manager")
{
}


AddOnManager::~AddOnManager()
{
}


void
AddOnManager::LoadState()
{
	RegisterAddOns();
}


void
AddOnManager::SaveState()
{
	CALLED();
	UnregisterAddOns();
}


status_t
AddOnManager::RegisterAddOn(BEntry &entry)
{
	BPath path(&entry);

	entry_ref ref;
	status_t status = entry.GetRef(&ref);
	if (status < B_OK)
		return status;

	printf("AddOnManager::RegisterAddOn(): trying to load \"%s\"\n", path.Path());

	image_id addon_image = load_add_on(path.Path());
	
	if (addon_image < B_OK)
		return addon_image;
		
	BEntry parent;
	entry.GetParent(&parent);
	BPath parentPath(&parent);
	BString pathString = parentPath.Path();
	
	if (pathString.FindFirst("input_server/devices")>0) {
		BInputServerDevice *(*instantiate_func)();

		if (get_image_symbol(addon_image, "instantiate_input_device",
				B_SYMBOL_TYPE_TEXT, (void **)&instantiate_func) < B_OK) {
			printf("AddOnManager::RegisterAddOn(): can't find instantiate_input_device in \"%s\"\n",
				path.Path());
			goto exit_error;
		}
	
		BInputServerDevice *isd = (*instantiate_func)();
		if (isd == NULL) {
			printf("AddOnManager::RegisterAddOn(): instantiate_input_device in \"%s\" returned NULL\n",
				path.Path());
			goto exit_error;
		}
		status_t status = isd->InitCheck();
		if (status != B_OK) {
			printf("AddOnManager::RegisterAddOn(): BInputServerDevice.InitCheck in \"%s\" returned %s\n",
				path.Path(), strerror(status));
			delete isd;
			goto exit_error;
		}
		
		RegisterDevice(isd, ref, addon_image);
		
		
	} else if (pathString.FindFirst("input_server/filters")>0) {
		BInputServerFilter *(*instantiate_func)();

		if (get_image_symbol(addon_image, "instantiate_input_filter",
				B_SYMBOL_TYPE_TEXT, (void **)&instantiate_func) < B_OK) {
			printf("AddOnManager::RegisterAddOn(): can't find instantiate_input_filter in \"%s\"\n",
				path.Path());
			goto exit_error;
		}
	
		BInputServerFilter *isf = (*instantiate_func)();
		if (isf == NULL) {
			printf("AddOnManager::RegisterAddOn(): instantiate_input_filter in \"%s\" returned NULL\n",
				path.Path());
			goto exit_error;
		}
		status_t status = isf->InitCheck();
		if (status != B_OK) {
			printf("AddOnManager::RegisterAddOn(): BInputServerFilter.InitCheck in \"%s\" returned %s\n",
				path.Path(), strerror(status));
			delete isf;
			goto exit_error;
		}
		
		RegisterFilter(isf, ref, addon_image);
	
	} else if (pathString.FindFirst("input_server/methods")>0) {
		BInputServerMethod *(*instantiate_func)();

		if (get_image_symbol(addon_image, "instantiate_input_method",
				B_SYMBOL_TYPE_TEXT, (void **)&instantiate_func) < B_OK) {
			printf("AddOnManager::RegisterAddOn(): can't find instantiate_input_method in \"%s\"\n",
				path.Path());
			goto exit_error;
		}
	
		BInputServerMethod *ism = (*instantiate_func)();
		if (ism == NULL) {
			printf("AddOnManager::RegisterAddOn(): instantiate_input_method in \"%s\" returned NULL\n",
				path.Path());
			goto exit_error;
		}
		status_t status = ism->InitCheck();
		if (status != B_OK) {
			printf("AddOnManager::RegisterAddOn(): BInputServerMethod.InitCheck in \"%s\" returned %s\n",
				path.Path(), strerror(status));
			delete ism;
			goto exit_error;
		}
		
		RegisterMethod(ism, ref, addon_image);
	
	}
	
	return B_OK;
exit_error:
	unload_add_on(addon_image);
	
	return status;
}


status_t
AddOnManager::UnregisterAddOn(BEntry &entry)
{
	BPath path(&entry);

	entry_ref ref;
	status_t status = entry.GetRef(&ref);
	if (status < B_OK)
		return status;

	PRINT(("AddOnManager::UnregisterAddOn(): trying to unload \"%s\"\n", path.Path()));

	BEntry parent;
	entry.GetParent(&parent);
	BPath parentPath(&parent);
	BString pathString = parentPath.Path();
	
	BAutolock locker(fLock);
	
	if (pathString.FindFirst("input_server/devices")>0) {
		device_info *pinfo;
		for (fDeviceList.Rewind(); fDeviceList.GetNext(&pinfo);) {
			if (!strcmp(pinfo->ref.name, ref.name)) {
				InputServer::StartStopDevices(pinfo->isd, false);
				delete pinfo->isd;
				delete pinfo->addon;
				if (pinfo->addon_image >= B_OK)
					unload_add_on(pinfo->addon_image);
				fDeviceList.RemoveCurrent();
				break;
			}
		}
	} else if (pathString.FindFirst("input_server/filters")>0) {
		filter_info *pinfo;
		for (fFilterList.Rewind(); fFilterList.GetNext(&pinfo);) {
			if (!strcmp(pinfo->ref.name, ref.name)) {
				delete pinfo->isf;
				if (pinfo->addon_image >= B_OK)
					unload_add_on(pinfo->addon_image);
				fFilterList.RemoveCurrent();
				break;
			}
		}
	} else if (pathString.FindFirst("input_server/methods")>0) {
		method_info *pinfo;
		for (fMethodList.Rewind(); fMethodList.GetNext(&pinfo);) {
			if (!strcmp(pinfo->ref.name, ref.name)) {
				delete pinfo->ism;
				delete pinfo->addon;
				if (pinfo->addon_image >= B_OK)
					unload_add_on(pinfo->addon_image);
				fMethodList.RemoveCurrent();
				break;
			}
		}
	} 

	return B_OK;
}

void
AddOnManager::RegisterAddOns()
{
	class IAHandler : public AddOnMonitorHandler {
	private:
		AddOnManager * fManager;
	public:
		IAHandler(AddOnManager * manager) {
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
			fManager->RegisterAddOn(entry);
		}
		virtual void	AddOnDisabled(const add_on_entry_info * entry_info) {
			CALLED();
			entry_ref ref;
			make_entry_ref(entry_info->dir_nref.device, entry_info->dir_nref.node,
			               entry_info->name, &ref);
			BEntry entry(&ref, false);
			fManager->UnregisterAddOn(entry);
		}
		virtual void	AddOnRemoved(const add_on_entry_info * entry_info) {
		}
	};

	const directory_which directories[] = {
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
//		B_BEOS_ADDONS_DIRECTORY,
	};
	const char subDirectories[][24] = {
		"input_server/devices",
		"input_server/filters",
		"input_server/methods",
	};
	fHandler = new IAHandler(this);
	fAddOnMonitor = new AddOnMonitor(fHandler);

	node_ref nref;
	BDirectory directory;
	BPath path;
	for (uint i = 0 ; i < sizeof(directories) / sizeof(directory_which) ; i++)
		for (uint j = 0 ; j < sizeof(subDirectories) / sizeof(char[24]) ; j++) {
			if ((find_directory(directories[i], &path) == B_OK)
				&& (path.Append(subDirectories[j]) == B_OK)
				&& (directory.SetTo(path.Path()) == B_OK) 
				&& (directory.GetNodeRef(&nref) == B_OK)) {
				fHandler->AddDirectory(&nref);
			}
		}

	// ToDo: this is for our own convenience only, and should be removed
	//	in the final release
	//if ((directory.SetTo("/boot/home/develop/openbeos/current/distro/x86.R1/beos/system/add-ons/media/plugins") == B_OK)
	//	&& (directory.GetNodeRef(&nref) == B_OK)) {
	//	fHandler->AddDirectory(&nref);
	//}
}


void
AddOnManager::UnregisterAddOns()
{
	BMessenger messenger(fAddOnMonitor);
	messenger.SendMessage(B_QUIT_REQUESTED);
	int32 exit_value;
	wait_for_thread(fAddOnMonitor->Thread(), &exit_value);
	delete fHandler;
	
	BAutolock locker(fLock);
	
	// we have to stop manually the addons because the monitor doesn't disable them on exit
	
	{
		device_info *pinfo;
		for (fDeviceList.Rewind(); fDeviceList.GetNext(&pinfo);) {
			InputServer::StartStopDevices(pinfo->isd, false);
			delete pinfo->isd;
			delete pinfo->addon;
			if (pinfo->addon_image >= B_OK)
				unload_add_on(pinfo->addon_image);
			fDeviceList.RemoveCurrent();
		}
	}
	
	{
		filter_info *pinfo;
		for (fFilterList.Rewind(); fFilterList.GetNext(&pinfo);) {
			delete pinfo->isf;
			if (pinfo->addon_image >= B_OK)
				unload_add_on(pinfo->addon_image);
			fFilterList.RemoveCurrent();
		}
	}
	
	{
		method_info *pinfo;
		for (fMethodList.Rewind(); fMethodList.GetNext(&pinfo);) {
				delete pinfo->ism;
				delete pinfo->addon;
				if (pinfo->addon_image >= B_OK)
					unload_add_on(pinfo->addon_image);
				fMethodList.RemoveCurrent();
		}
	}
}


void
AddOnManager::RegisterDevice(BInputServerDevice *device, const entry_ref &ref, image_id addon_image)
{
	BAutolock locker(fLock);

	device_info *pinfo;
	for (fDeviceList.Rewind(); fDeviceList.GetNext(&pinfo);) {
		if (!strcmp(pinfo->ref.name, ref.name)) {
			// we already know this device
			return;
		}
	}

	PRINT(("AddOnManager::RegisterDevice, name %s\n", ref.name));

	device_info info;
	info.ref = ref;
	info.addon_image = addon_image;
	info.isd = device;
	info.addon = new _BDeviceAddOn_;
	
	fDeviceList.Insert(info);
}


void
AddOnManager::RegisterFilter(BInputServerFilter *filter, const entry_ref &ref, image_id addon_image)
{
	BAutolock locker(fLock);

	filter_info *pinfo;
	for (fFilterList.Rewind(); fFilterList.GetNext(&pinfo);) {
		if (!strcmp(pinfo->ref.name, ref.name)) {
			// we already know this ref
			return;
		}
	}

	PRINT(("%s, name %s\n", __PRETTY_FUNCTION__, ref.name));

	filter_info info;
	info.ref = ref;
	info.addon_image = addon_image;
	info.isf = filter;

	fFilterList.Insert(info);
	
	BAutolock lock2(InputServer::gInputFilterListLocker);
	
	InputServer::gInputFilterList.AddItem(filter);
}


void
AddOnManager::RegisterMethod(BInputServerMethod *method, const entry_ref &ref, image_id addon_image)
{
	BAutolock locker(fLock);

	method_info *pinfo;
	for (fMethodList.Rewind(); fMethodList.GetNext(&pinfo);) {
		if (!strcmp(pinfo->ref.name, ref.name)) {
			// we already know this ref
			return;
		}
	}

	PRINT(("%s, name %s\n", __PRETTY_FUNCTION__, ref.name));

	method_info info;
	info.ref = ref;
	info.addon_image = addon_image;
	info.ism = method;
	info.addon = new _BMethodAddOn_;

	fMethodList.Insert(info);
	
	BAutolock lock2(InputServer::gInputMethodListLocker);
	
	InputServer::gInputMethodList.AddItem(method);
}


