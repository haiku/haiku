/*
 * Copyright 2004-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marcus Overhagen
 *		Axel Dörfler, axeld@pinc-software.de
 *		Jérôme Duval
 */


//! Manager for input_server add-ons (devices, filters, methods)


#include "AddOnManager.h"

#include <stdio.h>
#include <string.h>

#include <Autolock.h>
#include <Deskbar.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <image.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>

#include <PathMonitor.h>

#include "InputServer.h"
#include "InputServerTypes.h"
#include "MethodReplicant.h"


#undef TRACE
//#define TRACE_ADD_ON_MONITOR
#ifdef TRACE_ADD_ON_MONITOR
#	define TRACE(x...) debug_printf(x)
#	define ERROR(x...) debug_printf(x)
#else
#	define TRACE(x...) ;
// TODO: probably better to the syslog
#	define ERROR(x...) debug_printf(x)
#endif



class AddOnManager::MonitorHandler : public AddOnMonitorHandler {
public:
	MonitorHandler(AddOnManager* manager)
	{
		fManager = manager;
	}

	virtual void AddOnEnabled(const add_on_entry_info* entryInfo)
	{
		CALLED();
		entry_ref ref;
		make_entry_ref(entryInfo->dir_nref.device, entryInfo->dir_nref.node,
			entryInfo->name, &ref);
		BEntry entry(&ref, false);

		fManager->_RegisterAddOn(entry);
	}

	virtual void AddOnDisabled(const add_on_entry_info* entryInfo)
	{
		CALLED();
		entry_ref ref;
		make_entry_ref(entryInfo->dir_nref.device, entryInfo->dir_nref.node,
			entryInfo->name, &ref);
		BEntry entry(&ref, false);

		fManager->_UnregisterAddOn(entry);
	}

private:
	AddOnManager* fManager;
};


//	#pragma mark -


template<class T> T*
instantiate_add_on(image_id image, const char* path, const char* type)
{
	T* (*instantiateFunction)();

	BString functionName = "instantiate_input_";
	functionName += type;

	if (get_image_symbol(image, functionName.String(), B_SYMBOL_TYPE_TEXT,
			(void**)&instantiateFunction) < B_OK) {
		ERROR("AddOnManager::_RegisterAddOn(): can't find %s() in \"%s\"\n",
			functionName.String(), path);
		return NULL;
	}

	T* addOn = (*instantiateFunction)();
	if (addOn == NULL) {
		ERROR("AddOnManager::_RegisterAddOn(): %s() in \"%s\" returned "
			"NULL\n", functionName.String(), path);
		return NULL;
	}

	status_t status = addOn->InitCheck();
	if (status != B_OK) {
		ERROR("AddOnManager::_RegisterAddOn(): InitCheck() in \"%s\" "
			"returned %s\n", path, strerror(status));
		delete addOn;
		return NULL;
	}

	return addOn;
}


//	#pragma mark -


AddOnManager::AddOnManager(bool safeMode)
	:
	AddOnMonitor(),
	fHandler(new(std::nothrow) MonitorHandler(this)),
	fSafeMode(safeMode)
{
	SetHandler(fHandler);
}


AddOnManager::~AddOnManager()
{
	delete fHandler;
}


void
AddOnManager::MessageReceived(BMessage* message)
{
	CALLED();

	BMessage reply;
	status_t status;

	TRACE("%s what: %.4s\n", __PRETTY_FUNCTION__, (char*)&message->what);

	switch (message->what) {
		case IS_FIND_DEVICES:
			status = _HandleFindDevices(message, &reply);
			break;
		case IS_WATCH_DEVICES:
			status = _HandleWatchDevices(message, &reply);
			break;
		case IS_IS_DEVICE_RUNNING:
			status = _HandleIsDeviceRunning(message, &reply);
			break;
		case IS_START_DEVICE:
			status = _HandleStartStopDevices(message, &reply);
			break;
		case IS_STOP_DEVICE:
			status = _HandleStartStopDevices(message, &reply);
			break;
		case IS_CONTROL_DEVICES:
			status = _HandleControlDevices(message, &reply);
			break;
		case SYSTEM_SHUTTING_DOWN:
			status = _HandleSystemShuttingDown(message, &reply);
			break;
		case IS_METHOD_REGISTER:
			status = _HandleMethodReplicant(message, &reply);
			break;

		case B_PATH_MONITOR:
			_HandleDeviceMonitor(message);
			return;

		default:
			AddOnMonitor::MessageReceived(message);
			return;
	}

	reply.AddInt32("status", status);
	message->SendReply(&reply);
}


void
AddOnManager::LoadState()
{
	_RegisterAddOns();
}


void
AddOnManager::SaveState()
{
	CALLED();
	_UnregisterAddOns();
}


status_t
AddOnManager::StartMonitoringDevice(DeviceAddOn* addOn, const char* device)
{
	CALLED();

	BString path;
	if (device[0] != '/')
		path = "/dev/";
	path += device;

	TRACE("AddOnMonitor::StartMonitoringDevice(%s)\n", path.String());

	bool newPath;
	status_t status = _AddDevicePath(addOn, path.String(), newPath);
	if (status == B_OK && newPath) {
		status = BPathMonitor::StartWatching(path.String(), B_ENTRY_CREATED
			| B_ENTRY_REMOVED | B_ENTRY_MOVED | B_WATCH_FILES_ONLY
			| B_WATCH_RECURSIVELY, this);
		if (status != B_OK) {
			bool lastPath;
			_RemoveDevicePath(addOn, path.String(), lastPath);
		}
	}

	return status;
}


status_t
AddOnManager::StopMonitoringDevice(DeviceAddOn* addOn, const char *device)
{
	CALLED();

	BString path;
	if (device[0] != '/')
		path = "/dev/";
	path += device;

	TRACE("AddOnMonitor::StopMonitoringDevice(%s)\n", path.String());

	bool lastPath;
	status_t status = _RemoveDevicePath(addOn, path.String(), lastPath);
	if (status == B_OK && lastPath)
		BPathMonitor::StopWatching(path.String(), this);

	return status;
}


// #pragma mark -


void
AddOnManager::_RegisterAddOns()
{
	CALLED();
	BAutolock locker(this);

	const directory_which directories[] = {
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_SYSTEM_ADDONS_DIRECTORY
	};
	const char* subDirectories[] = {
		"input_server/devices",
		"input_server/filters",
		"input_server/methods"
	};
	int32 subDirectoryCount = sizeof(subDirectories) / sizeof(const char*);

	node_ref nref;
	BDirectory directory;
	BPath path;
	// when safemode, only B_SYSTEM_ADDONS_DIRECTORY is used
	for (uint32 i = fSafeMode ? 2 : 0;
			i < sizeof(directories) / sizeof(directory_which); i++) {
		for (int32 j = 0; j < subDirectoryCount; j++) {
			if (find_directory(directories[i], &path) == B_OK
				&& path.Append(subDirectories[j]) == B_OK
				&& directory.SetTo(path.Path()) == B_OK
				&& directory.GetNodeRef(&nref) == B_OK) {
				fHandler->AddDirectory(&nref);
			}
		}
	}
}


void
AddOnManager::_UnregisterAddOns()
{
	BAutolock locker(this);

	// We have to stop manually the add-ons because the monitor doesn't
	// disable them on exit

	while (device_info* info = fDeviceList.RemoveItemAt(0)) {
		gInputServer->StartStopDevices(*info->add_on, false);
		delete info;
	}

	// TODO: what about the filters/methods lists in the input_server?

	while (filter_info* info = fFilterList.RemoveItemAt(0)) {
		delete info;
	}

	while (method_info* info = fMethodList.RemoveItemAt(0)) {
		delete info;
	}
}


bool
AddOnManager::_IsDevice(const char* path) const
{
	return strstr(path, "input_server/devices") != 0;
}


bool
AddOnManager::_IsFilter(const char* path) const
{
	return strstr(path, "input_server/filters") != 0;
}


bool
AddOnManager::_IsMethod(const char* path) const
{
	return strstr(path, "input_server/methods") != 0;
}


status_t
AddOnManager::_RegisterAddOn(BEntry& entry)
{
	BPath path(&entry);

	entry_ref ref;
	status_t status = entry.GetRef(&ref);
	if (status < B_OK)
		return status;

	TRACE("AddOnManager::RegisterAddOn(): trying to load \"%s\"\n",
		path.Path());

	image_id image = load_add_on(path.Path());
	if (image < B_OK) {
		ERROR("load addon %s failed\n", path.Path());
		return image;
	}

	status = B_ERROR;

	if (_IsDevice(path.Path())) {
		BInputServerDevice* device = instantiate_add_on<BInputServerDevice>(
			image, path.Path(), "device");
		if (device != NULL)
			status = _RegisterDevice(device, ref, image);
	} else if (_IsFilter(path.Path())) {
		BInputServerFilter* filter = instantiate_add_on<BInputServerFilter>(
			image, path.Path(), "filter");
		if (filter != NULL)
			status = _RegisterFilter(filter, ref, image);
	} else if (_IsMethod(path.Path())) {
		BInputServerMethod* method = instantiate_add_on<BInputServerMethod>(
			image, path.Path(), "method");
		if (method != NULL)
			status = _RegisterMethod(method, ref, image);
	} else {
		ERROR("AddOnManager::_RegisterAddOn(): addon type not found for "
			"\"%s\" \n", path.Path());
	}

	if (status != B_OK)
		unload_add_on(image);

	return status;
}


status_t
AddOnManager::_UnregisterAddOn(BEntry& entry)
{
	BPath path(&entry);

	entry_ref ref;
	status_t status = entry.GetRef(&ref);
	if (status < B_OK)
		return status;

	TRACE("AddOnManager::UnregisterAddOn(): trying to unload \"%s\"\n",
		path.Path());

	BAutolock _(this);

	if (_IsDevice(path.Path())) {
		for (int32 i = fDeviceList.CountItems(); i-- > 0;) {
			device_info* info = fDeviceList.ItemAt(i);
			if (!strcmp(info->ref.name, ref.name)) {
				gInputServer->StartStopDevices(*info->add_on, false);
				delete fDeviceList.RemoveItemAt(i);
				break;
			}
		}
	} else if (_IsFilter(path.Path())) {
		for (int32 i = fFilterList.CountItems(); i-- > 0;) {
			filter_info* info = fFilterList.ItemAt(i);
			if (!strcmp(info->ref.name, ref.name)) {
				BAutolock locker(InputServer::gInputFilterListLocker);
				InputServer::gInputFilterList.RemoveItem(info->add_on);
				delete fFilterList.RemoveItemAt(i);
				break;
			}
		}
	} else if (_IsMethod(path.Path())) {
		BInputServerMethod* method = NULL;

		for (int32 i = fMethodList.CountItems(); i-- > 0;) {
			method_info* info = fMethodList.ItemAt(i);
			if (!strcmp(info->ref.name, ref.name)) {
				BAutolock locker(InputServer::gInputMethodListLocker);
				InputServer::gInputMethodList.RemoveItem(info->add_on);
				method = info->add_on;
					// this will only be used as a cookie, and not referenced
					// anymore
				delete fMethodList.RemoveItemAt(i);
				break;
			}
		}

		if (fMethodList.CountItems() <= 0) {
			// we remove the method replicant
			BDeskbar().RemoveItem(REPLICANT_CTL_NAME);
			gInputServer->SetMethodReplicant(NULL);
		} else if (method != NULL) {
			BMessage msg(IS_REMOVE_METHOD);
			msg.AddInt32("cookie", (uint32)method);
			if (gInputServer->MethodReplicant())
				gInputServer->MethodReplicant()->SendMessage(&msg);
		}
	}

	return B_OK;
}


//!	Takes over ownership of the \a device, regardless of success.
status_t
AddOnManager::_RegisterDevice(BInputServerDevice* device, const entry_ref& ref,
	image_id addOnImage)
{
	BAutolock locker(this);

	for (int32 i = fDeviceList.CountItems(); i-- > 0;) {
		device_info* info = fDeviceList.ItemAt(i);
		if (!strcmp(info->ref.name, ref.name)) {
			// we already know this device
			delete device;
			return B_NAME_IN_USE;
		}
	}

	TRACE("AddOnManager::RegisterDevice, name %s\n", ref.name);

	device_info* info = new(std::nothrow) device_info;
	if (info == NULL) {
		delete device;
		return B_NO_MEMORY;
	}

	info->ref = ref;
	info->add_on = device;

	if (!fDeviceList.AddItem(info)) {
		delete info;
		return B_NO_MEMORY;
	}

	info->image = addOnImage;

	return B_OK;
}


//!	Takes over ownership of the \a filter, regardless of success.
status_t
AddOnManager::_RegisterFilter(BInputServerFilter* filter, const entry_ref& ref,
	image_id addOnImage)
{
	BAutolock _(this);

	for (int32 i = fFilterList.CountItems(); i-- > 0;) {
		filter_info* info = fFilterList.ItemAt(i);
		if (!strcmp(info->ref.name, ref.name)) {
			// we already know this ref
			delete filter;
			return B_NAME_IN_USE;
		}
	}

	TRACE("%s, name %s\n", __PRETTY_FUNCTION__, ref.name);

	filter_info* info = new(std::nothrow) filter_info;
	if (info == NULL) {
		delete filter;
		return B_NO_MEMORY;
	}

	info->ref = ref;
	info->add_on = filter;

	if (!fFilterList.AddItem(info)) {
		delete info;
		return B_NO_MEMORY;
	}

	BAutolock locker(InputServer::gInputFilterListLocker);
	if (!InputServer::gInputFilterList.AddItem(filter)) {
		fFilterList.RemoveItem(info);
		delete info;
		return B_NO_MEMORY;
	}

	info->image = addOnImage;

	return B_OK;
}


//!	Takes over ownership of the \a method, regardless of success.
status_t
AddOnManager::_RegisterMethod(BInputServerMethod* method, const entry_ref& ref,
	image_id addOnImage)
{
	BAutolock _(this);

	for (int32 i = fMethodList.CountItems(); i-- > 0;) {
		method_info* info = fMethodList.ItemAt(i);
		if (!strcmp(info->ref.name, ref.name)) {
			// we already know this ref
			delete method;
			return B_NAME_IN_USE;
		}
	}

	TRACE("%s, name %s\n", __PRETTY_FUNCTION__, ref.name);

	method_info* info = new(std::nothrow) method_info;
	if (info == NULL) {
		delete method;
		return B_NO_MEMORY;
	}

	info->ref = ref;
	info->add_on = method;

	if (!fMethodList.AddItem(info)) {
		delete info;
		return B_NO_MEMORY;
	}

	BAutolock locker(InputServer::gInputMethodListLocker);
	if (!InputServer::gInputMethodList.AddItem(method)) {
		fMethodList.RemoveItem(info);
		delete info;
		return B_NO_MEMORY;
	}

	info->image = addOnImage;

	if (gInputServer->MethodReplicant() == NULL) {
		_LoadReplicant();

		if (gInputServer->MethodReplicant()) {
			_BMethodAddOn_ *addon = InputServer::gKeymapMethod.fOwner;
			addon->AddMethod();
		}
	}

	if (gInputServer->MethodReplicant() != NULL) {
		_BMethodAddOn_ *addon = method->fOwner;
		addon->AddMethod();
	}

	return B_OK;
}


// #pragma mark -


void
AddOnManager::_UnloadReplicant()
{
	BDeskbar().RemoveItem(REPLICANT_CTL_NAME);
}


void
AddOnManager::_LoadReplicant()
{
	CALLED();
	app_info info;
	be_app->GetAppInfo(&info);

	status_t err = BDeskbar().AddItem(&info.ref);
	if (err != B_OK)
		ERROR("Deskbar refuses to add method replicant: %s\n", strerror(err));

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

	if (to.SendMessage(&request, &reply) == B_OK
		&& reply.FindMessenger("result", &status) == B_OK) {
		// enum replicant in Status view
		int32 index = 0;
		int32 uid;
		while ((uid = _GetReplicantAt(status, index++)) >= B_OK) {
			BMessage replicantInfo;
			if (_GetReplicantName(status, uid, &replicantInfo) != B_OK)
				continue;

			const char *name;
			if (replicantInfo.FindString("result", &name) == B_OK
				&& !strcmp(name, REPLICANT_CTL_NAME)) {
				BMessage replicant;
				if (_GetReplicantView(status, uid, &replicant) == B_OK) {
					BMessenger result;
					if (replicant.FindMessenger("result", &result) == B_OK) {
						gInputServer->SetMethodReplicant(new BMessenger(result));
					}
				}
			}
		}
	}

	if (!gInputServer->MethodReplicant()) {
		ERROR("LoadReplicant(): Method replicant not found!\n");
	}
}


int32
AddOnManager::_GetReplicantAt(BMessenger target, int32 index) const
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
AddOnManager::_GetReplicantName(BMessenger target, int32 uid,
	BMessage* reply) const
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
AddOnManager::_GetReplicantView(BMessenger target, int32 uid,
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


status_t
AddOnManager::_HandleStartStopDevices(BMessage* message, BMessage* reply)
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
AddOnManager::_HandleFindDevices(BMessage* message, BMessage* reply)
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
AddOnManager::_HandleWatchDevices(BMessage* message, BMessage* reply)
{
	// TODO
	return B_OK;
}


status_t
AddOnManager::_HandleIsDeviceRunning(BMessage* message, BMessage* reply)
{
	const char* name;
	bool running;
	if (message->FindString("device", &name) != B_OK
		|| gInputServer->GetDeviceInfo(name, NULL, &running) != B_OK)
		return B_NAME_NOT_FOUND;

	return running ? B_OK : B_ERROR;
}


status_t
AddOnManager::_HandleControlDevices(BMessage* message, BMessage* reply)
{
	CALLED();
	const char *name = NULL;
	int32 type = 0;
	if (!((message->FindInt32("type", &type) != B_OK)
			^ (message->FindString("device", &name) != B_OK)))
		return B_BAD_VALUE;

	uint32 code = 0;
	BMessage controlMessage;
	bool hasMessage = true;
	if (message->FindInt32("code", (int32*)&code) != B_OK)
		return B_BAD_VALUE;
	if (message->FindMessage("message", &controlMessage) != B_OK)
		hasMessage = false;

	return gInputServer->ControlDevices(name, (input_device_type)type,
		code, hasMessage ? &controlMessage : NULL);
}


status_t
AddOnManager::_HandleSystemShuttingDown(BMessage* message, BMessage* reply)
{
	CALLED();

	for (int32 i = 0; i < fDeviceList.CountItems(); i++) {
		device_info* info = fDeviceList.ItemAt(i);
		info->add_on->SystemShuttingDown();
	}

	return B_OK;
}


status_t
AddOnManager::_HandleMethodReplicant(BMessage* message, BMessage* reply)
{
	CALLED();

	if (InputServer::gInputMethodList.CountItems() == 0) {
		_UnloadReplicant();
		return B_OK;
	}

	_LoadReplicant();

	BAutolock lock(InputServer::gInputMethodListLocker);

	if (gInputServer->MethodReplicant()) {
		_BMethodAddOn_* addon = InputServer::gKeymapMethod.fOwner;
		addon->AddMethod();

		for (int32 i = 0; i < InputServer::gInputMethodList.CountItems(); i++) {
			BInputServerMethod* method
				= (BInputServerMethod*)InputServer::gInputMethodList.ItemAt(i);

			_BMethodAddOn_* addon = method->fOwner;
			addon->AddMethod();
		}
	}

	return B_OK;
}


void
AddOnManager::_HandleDeviceMonitor(BMessage* message)
{
	int32 opcode;
	if (message->FindInt32("opcode", &opcode) != B_OK)
		return;

	switch (opcode) {
		case B_ENTRY_CREATED:
		case B_ENTRY_REMOVED:
		{
			const char* path;
			const char* watchedPath;
			if (message->FindString("watched_path", &watchedPath) != B_OK
				|| message->FindString("path", &path) != B_OK) {
#if DEBUG
				char string[1024];
				sprintf(string, "message does not contain all fields - "
					"watched_path: %d, path: %d\n",
					message->HasString("watched_path"),
					message->HasString("path"));
				debugger(string);
#endif
				return;
			}

			// Notify all watching devices

			for (int32 i = 0; i < fDeviceAddOns.CountItems(); i++) {
				DeviceAddOn* addOn = fDeviceAddOns.ItemAt(i);
				if (!addOn->HasPath(watchedPath))
					continue;

				addOn->Device()->Control(NULL, NULL, B_NODE_MONITOR, message);
			}
			break;
		}
	}
}


status_t
AddOnManager::_AddDevicePath(DeviceAddOn* addOn, const char* path,
	bool& newPath)
{
	newPath = !fDevicePaths.HasPath(path);

	status_t status = fDevicePaths.AddPath(path);
	if (status == B_OK) {
		status = addOn->AddPath(path);
		if (status == B_OK) {
			if (!fDeviceAddOns.HasItem(addOn)
				&& !fDeviceAddOns.AddItem(addOn)) {
				addOn->RemovePath(path);
				status = B_NO_MEMORY;
			}
		} else
			fDevicePaths.RemovePath(path);
	}

	return status;
}


status_t
AddOnManager::_RemoveDevicePath(DeviceAddOn* addOn, const char* path,
	bool& lastPath)
{
	if (!fDevicePaths.HasPath(path) || !addOn->HasPath(path))
		return B_ENTRY_NOT_FOUND;

	fDevicePaths.RemovePath(path);

	lastPath = !fDevicePaths.HasPath(path);

	addOn->RemovePath(path);
	if (addOn->CountPaths() == 0)
		fDeviceAddOns.RemoveItem(addOn);

	return B_OK;
}
