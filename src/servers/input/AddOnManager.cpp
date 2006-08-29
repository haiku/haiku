/*
 * Copyright 2004-2005, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marcus Overhagen, Axel Dörfler
 *		Jérôme Duval
 */


#include <Autolock.h>
#include <Deskbar.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>

#include <image.h>
#include <stdio.h>
#include <string.h>

#include "AddOnManager.h"
#include "InputServer.h"
#include "InputServerTypes.h"
#include "MethodReplicant.h"


class AddOnManager::InputServerMonitorHandler : public AddOnMonitorHandler {
	public:
		InputServerMonitorHandler(AddOnManager* manager)
		{
			fManager = manager;
		}

		virtual void
		AddOnCreated(const add_on_entry_info * entry_info)
		{
		}

		virtual void
		AddOnEnabled(const add_on_entry_info* entryInfo)
		{
			CALLED();
			entry_ref ref;
			make_entry_ref(entryInfo->dir_nref.device, entryInfo->dir_nref.node,
				entryInfo->name, &ref);
			BEntry entry(&ref, false);
			fManager->RegisterAddOn(entry);
		}

		virtual void
		AddOnDisabled(const add_on_entry_info* entryInfo)
		{
			CALLED();
			entry_ref ref;
			make_entry_ref(entryInfo->dir_nref.device, entryInfo->dir_nref.node,
				entryInfo->name, &ref);
			BEntry entry(&ref, false);
			fManager->UnregisterAddOn(entry);
		}

		virtual void
		AddOnRemoved(const add_on_entry_info* entryInfo)
		{
		}

	private:
		AddOnManager* fManager;
};


//	#pragma mark -


AddOnManager::AddOnManager(bool safeMode)
	: BLooper("add-on manager"),
	fLock("add-on manager"),
	fSafeMode(safeMode)
{
	Run();
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
AddOnManager::RegisterAddOn(BEntry& entry)
{
	BPath path(&entry);

	entry_ref ref;
	status_t status = entry.GetRef(&ref);
	if (status < B_OK)
		return status;

	PRINT(("AddOnManager::RegisterAddOn(): trying to load \"%s\"\n", path.Path()));

	image_id addonImage = load_add_on(path.Path());
	
	if (addonImage < B_OK) {
		PRINT(("load addon %s failed\n", path.Path()));
		return addonImage;
	}

	BString pathString = path.Path();

	if (pathString.FindFirst("input_server/devices") > 0) {
		BInputServerDevice *(*instantiate_func)();

		if (get_image_symbol(addonImage, "instantiate_input_device",
				B_SYMBOL_TYPE_TEXT, (void **)&instantiate_func) < B_OK) {
			PRINTERR(("AddOnManager::RegisterAddOn(): can't find instantiate_input_device in \"%s\"\n",
				path.Path()));
			goto exit_error;
		}

		BInputServerDevice *device = (*instantiate_func)();
		if (device == NULL) {
			PRINTERR(("AddOnManager::RegisterAddOn(): instantiate_input_device in \"%s\" returned NULL\n",
				path.Path()));
			goto exit_error;
		}

		status_t status = device->InitCheck();
		if (status != B_OK) {
			PRINTERR(("AddOnManager::RegisterAddOn(): BInputServerDevice.InitCheck in \"%s\" returned %s\n",
				path.Path(), strerror(status)));
			delete device;
			goto exit_error;
		}

		RegisterDevice(device, ref, addonImage);
	} else if (pathString.FindFirst("input_server/filters") > 0) {
		BInputServerFilter *(*instantiate_func)();

		if (get_image_symbol(addonImage, "instantiate_input_filter",
				B_SYMBOL_TYPE_TEXT, (void **)&instantiate_func) < B_OK) {
			PRINTERR(("AddOnManager::RegisterAddOn(): can't find instantiate_input_filter in \"%s\"\n",
				path.Path()));
			goto exit_error;
		}

		BInputServerFilter *filter = (*instantiate_func)();
		if (filter == NULL) {
			PRINTERR(("AddOnManager::RegisterAddOn(): instantiate_input_filter in \"%s\" returned NULL\n",
				path.Path()));
			goto exit_error;
		}
		status_t status = filter->InitCheck();
		if (status != B_OK) {
			PRINTERR(("AddOnManager::RegisterAddOn(): BInputServerFilter.InitCheck in \"%s\" returned %s\n",
				path.Path(), strerror(status)));
			delete filter;
			goto exit_error;
		}

		RegisterFilter(filter, ref, addonImage);
	} else if (pathString.FindFirst("input_server/methods") > 0) {
		BInputServerMethod *(*instantiate_func)();

		if (get_image_symbol(addonImage, "instantiate_input_method",
				B_SYMBOL_TYPE_TEXT, (void **)&instantiate_func) < B_OK) {
			PRINTERR(("AddOnManager::RegisterAddOn(): can't find instantiate_input_method in \"%s\"\n",
				path.Path()));
			goto exit_error;
		}

		BInputServerMethod *method = (*instantiate_func)();
		if (method == NULL) {
			PRINTERR(("AddOnManager::RegisterAddOn(): instantiate_input_method in \"%s\" returned NULL\n",
				path.Path()));
			goto exit_error;
		}
		status_t status = method->InitCheck();
		if (status != B_OK) {
			PRINTERR(("AddOnManager::RegisterAddOn(): BInputServerMethod.InitCheck in \"%s\" returned %s\n",
				path.Path(), strerror(status)));
			delete method;
			goto exit_error;
		}

		RegisterMethod(method, ref, addonImage);
	} else {
		PRINTERR(("AddOnManager::RegisterAddOn(): addon type not found for \"%s\" \n", path.Path()));
		goto exit_error;
	}

	return B_OK;

exit_error:
	unload_add_on(addonImage);
	return status;
}


status_t
AddOnManager::UnregisterAddOn(BEntry& entry)
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

	if (pathString.FindFirst("input_server/devices") > 0) {
		device_info *pinfo;
		for (fDeviceList.Rewind(); fDeviceList.GetNext(&pinfo);) {
			if (!strcmp(pinfo->ref.name, ref.name)) {
				gInputServer->StartStopDevices(*pinfo->device, false);
				delete pinfo->device;
				if (pinfo->addon_image >= B_OK)
					unload_add_on(pinfo->addon_image);
				fDeviceList.RemoveCurrent();
				break;
			}
		}
	} else if (pathString.FindFirst("input_server/filters") > 0) {
		filter_info *pinfo;
		for (fFilterList.Rewind(); fFilterList.GetNext(&pinfo);) {
			if (!strcmp(pinfo->ref.name, ref.name)) {
				delete pinfo->filter;
				if (pinfo->addon_image >= B_OK)
					unload_add_on(pinfo->addon_image);
				fFilterList.RemoveCurrent();
				break;
			}
		}
	} else if (pathString.FindFirst("input_server/methods") > 0) {
		method_info *pinfo;
		for (fMethodList.Rewind(); fMethodList.GetNext(&pinfo);) {
			if (!strcmp(pinfo->ref.name, ref.name)) {
				delete pinfo->method;
				if (pinfo->addon_image >= B_OK)
					unload_add_on(pinfo->addon_image);
				fMethodList.RemoveCurrent();
				break;
			}
		}

		if (fMethodList.CountItems() <= 0) {
			// we remove the method replicant
			BDeskbar().RemoveItem(REPLICANT_CTL_NAME);
			gInputServer->SetMethodReplicant(NULL);
		} else {
			BMessage msg(IS_REMOVE_METHOD);
			msg.AddInt32("cookie", (uint32)pinfo->method);
			if (gInputServer->MethodReplicant())
				gInputServer->MethodReplicant()->SendMessage(&msg);
		}
	} 

	return B_OK;
}


void
AddOnManager::RegisterAddOns()
{
	CALLED();
	status_t err;

	fHandler = new InputServerMonitorHandler(this);
	fAddOnMonitor = new AddOnMonitor(fHandler);

#ifndef APPSERVER_TEST_MODE
	err = fAddOnMonitor->InitCheck();
	if (err != B_OK) {
		PRINTERR(("AddOnManager::RegisterAddOns(): fAddOnMonitor->InitCheck() returned %s\n",
			strerror(err)));
		return;
	}

	const directory_which directories[] = {
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_BEOS_ADDONS_DIRECTORY
	};
	const char subDirectories[][24] = {
		"input_server/devices",
		"input_server/filters",
		"input_server/methods"
	};

	node_ref nref;
	BDirectory directory;
	BPath path;
	// when safemode, only B_BEOS_ADDONS_DIRECTORY is used
	for (uint32 i = fSafeMode ? 2 : 0 ; i < sizeof(directories) / sizeof(directory_which) ; i++)
		for (uint32 j = 0 ; j < sizeof(subDirectories) / sizeof(char[24]) ; j++) {
			if ((find_directory(directories[i], &path) == B_OK)
				&& (path.Append(subDirectories[j]) == B_OK)
				&& (directory.SetTo(path.Path()) == B_OK) 
				&& (directory.GetNodeRef(&nref) == B_OK)) {
				fHandler->AddDirectory(&nref);
			}
		}
#else
	BEntry entry("/boot/home/svnhaiku/trunk/tests/servers/input/view_input_device/input_server/devices/ViewInputDevice");
	RegisterAddOn(entry);
#endif
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
		device_info *info;
		for (fDeviceList.Rewind(); fDeviceList.GetNext(&info);) {
			gInputServer->StartStopDevices(*info->device, false);
			delete info->device;
			if (info->addon_image >= B_OK)
				unload_add_on(info->addon_image);
			fDeviceList.RemoveCurrent();
		}
	}

	{
		filter_info *info;
		for (fFilterList.Rewind(); fFilterList.GetNext(&info);) {
			delete info->filter;
			if (info->addon_image >= B_OK)
				unload_add_on(info->addon_image);
			fFilterList.RemoveCurrent();
		}
	}

	{
		method_info *info;
		for (fMethodList.Rewind(); fMethodList.GetNext(&info);) {
			delete info->method;
			if (info->addon_image >= B_OK)
				unload_add_on(info->addon_image);
			fMethodList.RemoveCurrent();
		}
	}
}


void
AddOnManager::RegisterDevice(BInputServerDevice* device, const entry_ref& ref,
	image_id addonImage)
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
	info.addon_image = addonImage;
	info.device = device;

	fDeviceList.Insert(info);
}


void
AddOnManager::RegisterFilter(BInputServerFilter* filter, const entry_ref& ref,
	image_id addonImage)
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
	info.addon_image = addonImage;
	info.filter = filter;

	fFilterList.Insert(info);

	BAutolock lock2(InputServer::gInputFilterListLocker);
	InputServer::gInputFilterList.AddItem(filter);
}


void
AddOnManager::RegisterMethod(BInputServerMethod* method, const entry_ref& ref,
	image_id addonImage)
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
	info.addon_image = addonImage;
	info.method = method;

	fMethodList.Insert(info);

	BAutolock lock2(InputServer::gInputMethodListLocker);
	InputServer::gInputMethodList.AddItem(method);

	if (gInputServer->MethodReplicant() == NULL) {
		LoadReplicant();

		if (gInputServer->MethodReplicant()) {
			_BMethodAddOn_ *addon = InputServer::gKeymapMethod.fOwner;
			addon->AddMethod();
		}
	}

	if (gInputServer->MethodReplicant()) {
		_BMethodAddOn_ *addon = method->fOwner;
		addon->AddMethod();
	}
}


void
AddOnManager::LoadReplicant()
{
	CALLED();
	app_info info;
	be_app->GetAppInfo(&info);

	status_t err = BDeskbar().AddItem(&info.ref);
	if (err != B_OK) {
		PRINTERR(("Deskbar refuses to add method replicant: %s\n", strerror(err)));
	}
	BMessage request(B_GET_PROPERTY);
	BMessenger to;
	BMessenger status;

	request.AddSpecifier("Messenger");
	request.AddSpecifier("Shelf");

	// In the Deskbar the Shelf is in the View "Status" in Window "Deskbar"
	request.AddSpecifier("View", "Status");
	request.AddSpecifier("Window", "Deskbar");
	to = BMessenger("application/x-vnd.Be-TSKB", -1);

	BMessage reply;

	if ((to.SendMessage(&request, &reply) == B_OK) 
		&& (reply.FindMessenger("result", &status) == B_OK)) {
		// enum replicant in Status view
		int32 index = 0;
		int32 uid;
		while ((uid = GetReplicantAt(status, index++)) >= B_OK) {
			BMessage rep_info;
			if (GetReplicantName(status, uid, &rep_info) != B_OK)
				continue;

			const char *name;
			if (rep_info.FindString("result", &name) == B_OK
				&& !strcmp(name, REPLICANT_CTL_NAME)) {
				BMessage replicant;
				if (GetReplicantView(status, uid, &replicant) == B_OK) {
					BMessenger result;
					if (replicant.FindMessenger("result", &result) == B_OK) {
						gInputServer->SetMethodReplicant(new BMessenger(result));
					}
				} 
			}
		}
	}
}


int32
AddOnManager::GetReplicantAt(BMessenger target, int32 index) const
{
	// So here we want to get the Unique ID of the replicant at the given index
	// in the target Shelf.

	BMessage request(B_GET_PROPERTY);// We're getting the ID property
	BMessage reply;
	status_t err;

	request.AddSpecifier("ID");// want the ID
	request.AddSpecifier("Replicant", index);// of the index'th replicant

	if ((err = target.SendMessage(&request, &reply)) != B_OK)
		return err;

	int32 uid;
	if ((err = reply.FindInt32("result", &uid)) != B_OK) 
		return err;

	return uid;
}


status_t 
AddOnManager::GetReplicantName(BMessenger target, int32 uid, BMessage *reply) const
{
	// We send a message to the target shelf, asking it for the Name of the 
	// replicant with the given unique id.

	BMessage request(B_GET_PROPERTY);
	BMessage uid_specifier(B_ID_SPECIFIER);// specifying via ID
	status_t err;
	status_t e;

	request.AddSpecifier("Name");// ask for the Name of the replicant

	// IDs are specified using code like the following 3 lines:
	uid_specifier.AddInt32("id", uid);
	uid_specifier.AddString("property", "Replicant");
	request.AddSpecifier(&uid_specifier);

	if ((err = target.SendMessage(&request, reply)) != B_OK)
		return err;

	if (((err = reply->FindInt32("error", &e)) != B_OK) || (e != B_OK))
		return err ? err : e;

	return B_OK;
}


status_t
AddOnManager::GetReplicantView(BMessenger target, int32 uid,
	BMessage* reply) const
{
	// We send a message to the target shelf, asking it for the Name of the 
	// replicant with the given unique id.

	BMessage request(B_GET_PROPERTY);
	BMessage uid_specifier(B_ID_SPECIFIER);
		// specifying via ID
	status_t err;
	status_t e;

	request.AddSpecifier("View");
		// ask for the Name of the replicant

	// IDs are specified using code like the following 3 lines:
	uid_specifier.AddInt32("id", uid);
	uid_specifier.AddString("property", "Replicant");
	request.AddSpecifier(&uid_specifier);

	if ((err = target.SendMessage(&request, reply)) != B_OK)
		return err;

	if (((err = reply->FindInt32("error", &e)) != B_OK) || (e != B_OK))
		return err ? err : e;

	return B_OK;
}


void
AddOnManager::MessageReceived(BMessage *message)
{
	CALLED();

	BMessage reply;
	status_t status;

	PRINT(("%s what:%c%c%c%c\n", __PRETTY_FUNCTION__, message->what >> 24,
		message->what >> 16, message->what >> 8, message->what));

	switch (message->what) {
		case IS_FIND_DEVICES:
			status = HandleFindDevices(message, &reply);
			break;
		case IS_WATCH_DEVICES:
			status = HandleWatchDevices(message, &reply);
			break;
		case IS_IS_DEVICE_RUNNING:
			status = HandleIsDeviceRunning(message, &reply);
			break;
		case IS_START_DEVICE:
			status = HandleStartStopDevices(message, &reply);
			break;
		case IS_STOP_DEVICE:
			status = HandleStartStopDevices(message, &reply);
			break;
		case IS_CONTROL_DEVICES:
			status = HandleControlDevices(message, &reply);
			break;
		case SYSTEM_SHUTTING_DOWN:
			status = HandleSystemShuttingDown(message, &reply);
			break;
		case IS_METHOD_REGISTER:
			status = HandleMethodReplicant(message, &reply);
			break;

		default:
			return;
	}

	reply.AddInt32("status", status);
	message->SendReply(&reply);
}


status_t
AddOnManager::HandleStartStopDevices(BMessage* message,
	BMessage* reply)
{
	const char *name = NULL;
	int32 type = 0;
	if (!((message->FindInt32("type", &type) != B_OK)
			^ (message->FindString("device", &name) != B_OK)))
		return B_ERROR;

	return gInputServer->StartStopDevices(name, (input_device_type)type,
		message->what == IS_START_DEVICE);
}


status_t
AddOnManager::HandleFindDevices(BMessage* message, BMessage* reply)
{
	CALLED();
	const char *name = NULL;
	input_device_type type;
	if (message->FindString("device", &name) == B_OK) {
		if (gInputServer->GetDeviceInfo(name, &type) != B_OK)
			return B_NAME_NOT_FOUND;
		reply->AddString("device", name);
		reply->AddInt32("type", type);
	} else {
		gInputServer->GetDeviceInfos(reply);
	}
	return B_OK;
}


status_t
AddOnManager::HandleWatchDevices(BMessage* message, BMessage* reply)
{
	// TODO
	return B_OK;
}


status_t
AddOnManager::HandleIsDeviceRunning(BMessage* message, BMessage* reply)
{
	const char *name;
	bool running;
	if (message->FindString("device", &name) != B_OK
		|| gInputServer->GetDeviceInfo(name, NULL, &running) != B_OK)
		return B_NAME_NOT_FOUND;

	return running ? B_OK : B_ERROR;
}


status_t
AddOnManager::HandleControlDevices(BMessage* message, BMessage* reply)
{
	CALLED();
	const char *name = NULL;
	int32 type = 0;
	if (!((message->FindInt32("type", &type) != B_OK)
			^ (message->FindString("device", &name) != B_OK)))
		return B_ERROR;

	uint32 code = 0;
	BMessage controlMessage;
	bool hasMessage = true;
	if (message->FindInt32("code", (int32*)&code) != B_OK)
		return B_ERROR;
	if (message->FindMessage("message", &controlMessage) != B_OK)
		hasMessage = false;

	return gInputServer->ControlDevices(name, (input_device_type)type,
		code, hasMessage ? &controlMessage : NULL);
}


status_t
AddOnManager::HandleSystemShuttingDown(BMessage* message,
	BMessage* reply)
{
	CALLED();

	// TODO
	return B_OK;
}


status_t
AddOnManager::HandleMethodReplicant(BMessage* message, BMessage* reply)
{
	CALLED();
	LoadReplicant();
	
	BAutolock lock(InputServer::gInputMethodListLocker);
	
	if (gInputServer->MethodReplicant()) {
		_BMethodAddOn_ *addon = InputServer::gKeymapMethod.fOwner;
		addon->AddMethod();
		
		for (int32 i=0; i<InputServer::gInputMethodList.CountItems(); i++) {
			BInputServerMethod *method = 
				(BInputServerMethod *)InputServer::gInputMethodList.ItemAt(i);
			_BMethodAddOn_ *addon = method->fOwner;
			addon->AddMethod();
		}
	}
	
	return B_OK;
}
