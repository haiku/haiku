/*
** Copyright 2004, the Haiku project. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Author : Jérôme Duval
** Original authors: Marcus Overhagen, Axel Dörfler
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

AddOnManager::AddOnManager(bool safeMode)
	: BLooper("addon_manager"),
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
AddOnManager::RegisterAddOn(BEntry &entry)
{
	BPath path(&entry);

	entry_ref ref;
	status_t status = entry.GetRef(&ref);
	if (status < B_OK)
		return status;

	PRINT(("AddOnManager::RegisterAddOn(): trying to load \"%s\"\n", path.Path()));

	image_id addon_image = load_add_on(path.Path());
	
	if (addon_image < B_OK) {
		PRINT(("load addon %s failed\n", path.Path()));
		return addon_image;
	}
	
	BEntry parent;
	entry.GetParent(&parent);
	BPath parentPath(&parent);
	BString pathString = parentPath.Path();
	
	if (pathString.FindFirst("input_server/devices")>0) {
		BInputServerDevice *(*instantiate_func)();

		if (get_image_symbol(addon_image, "instantiate_input_device",
				B_SYMBOL_TYPE_TEXT, (void **)&instantiate_func) < B_OK) {
			PRINTERR(("AddOnManager::RegisterAddOn(): can't find instantiate_input_device in \"%s\"\n",
				path.Path()));
			goto exit_error;
		}
	
		BInputServerDevice *isd = (*instantiate_func)();
		if (isd == NULL) {
			PRINTERR(("AddOnManager::RegisterAddOn(): instantiate_input_device in \"%s\" returned NULL\n",
				path.Path()));
			goto exit_error;
		}
		status_t status = isd->InitCheck();
		if (status != B_OK) {
			PRINTERR(("AddOnManager::RegisterAddOn(): BInputServerDevice.InitCheck in \"%s\" returned %s\n",
				path.Path(), strerror(status)));
			delete isd;
			goto exit_error;
		}
		
		RegisterDevice(isd, ref, addon_image);
		
		
	} else if (pathString.FindFirst("input_server/filters")>0) {
		BInputServerFilter *(*instantiate_func)();

		if (get_image_symbol(addon_image, "instantiate_input_filter",
				B_SYMBOL_TYPE_TEXT, (void **)&instantiate_func) < B_OK) {
			PRINTERR(("AddOnManager::RegisterAddOn(): can't find instantiate_input_filter in \"%s\"\n",
				path.Path()));
			goto exit_error;
		}
	
		BInputServerFilter *isf = (*instantiate_func)();
		if (isf == NULL) {
			PRINTERR(("AddOnManager::RegisterAddOn(): instantiate_input_filter in \"%s\" returned NULL\n",
				path.Path()));
			goto exit_error;
		}
		status_t status = isf->InitCheck();
		if (status != B_OK) {
			PRINTERR(("AddOnManager::RegisterAddOn(): BInputServerFilter.InitCheck in \"%s\" returned %s\n",
				path.Path(), strerror(status)));
			delete isf;
			goto exit_error;
		}
		
		RegisterFilter(isf, ref, addon_image);
	
	} else if (pathString.FindFirst("input_server/methods")>0) {
		BInputServerMethod *(*instantiate_func)();

		if (get_image_symbol(addon_image, "instantiate_input_method",
				B_SYMBOL_TYPE_TEXT, (void **)&instantiate_func) < B_OK) {
			PRINTERR(("AddOnManager::RegisterAddOn(): can't find instantiate_input_method in \"%s\"\n",
				path.Path()));
			goto exit_error;
		}
	
		BInputServerMethod *ism = (*instantiate_func)();
		if (ism == NULL) {
			PRINTERR(("AddOnManager::RegisterAddOn(): instantiate_input_method in \"%s\" returned NULL\n",
				path.Path()));
			goto exit_error;
		}
		status_t status = ism->InitCheck();
		if (status != B_OK) {
			PRINTERR(("AddOnManager::RegisterAddOn(): BInputServerMethod.InitCheck in \"%s\" returned %s\n",
				path.Path(), strerror(status)));
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
				if (pinfo->addon_image >= B_OK)
					unload_add_on(pinfo->addon_image);
				fMethodList.RemoveCurrent();
				break;
			}
		}

		if (fMethodList.CountItems()<=0) {
			// we remove the method replicant
			BDeskbar().RemoveItem(REPLICANT_CTL_NAME);
			((InputServer*)be_app)->SetMethodReplicant(NULL);
		} else {
			BMessage msg(IS_REMOVE_METHOD);
			msg.AddInt32("cookie", (uint32)pinfo->ism);
			if (((InputServer*)be_app)->MethodReplicant())
				((InputServer*)be_app)->MethodReplicant()->SendMessage(&msg);
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
		B_BEOS_ADDONS_DIRECTORY
	};
	const char subDirectories[][24] = {
		"input_server/devices",
		"input_server/filters",
		"input_server/methods"
	};
	fHandler = new IAHandler(this);
	fAddOnMonitor = new AddOnMonitor(fHandler);

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
	
	fMethodList.Insert(info);
	
	BAutolock lock2(InputServer::gInputMethodListLocker);
	
	InputServer::gInputMethodList.AddItem(method);
	
	if (((InputServer*)be_app)->MethodReplicant() == NULL) {
		app_info info;
        	be_app->GetAppInfo(&info);

        	status_t err = BDeskbar().AddItem(&info.ref);
        	if (err!=B_OK) {
                	PRINTERR(("Deskbar refuses to add method replicant\n"));
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
				if (GetReplicantName(status, uid, &rep_info) != B_OK) {
					continue;
				}
				const char *name;
				if ((rep_info.FindString("result", &name) == B_OK) 
					&& (strcmp(name, REPLICANT_CTL_NAME)==0)) {
					BMessage rep_view;
					if (GetReplicantView(status, uid, &rep_view)==0) {
						BMessenger result;
						if (rep_view.FindMessenger("result", &result) == B_OK) {
							((InputServer*)be_app)->SetMethodReplicant(new BMessenger(result));
						}
					} 
				}
			}
		}

		if (((InputServer*)be_app)->MethodReplicant()) {
			_BMethodAddOn_ *addon = InputServer::gKeymapMethod.fOwner;
                	addon->AddMethod();
		}
	}

	if (((InputServer*)be_app)->MethodReplicant()) {
		_BMethodAddOn_ *addon = method->fOwner;
		addon->AddMethod();
	}
}




//
int32 
AddOnManager::GetReplicantAt(BMessenger target, int32 index) const
{
	/*
	 So here we want to get the Unique ID of the replicant at the given index
	 in the target Shelf.
	 */
 
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


//
status_t 
AddOnManager::GetReplicantName(BMessenger target, int32 uid, BMessage *reply) const
{
	/*
	 We send a message to the target shelf, asking it for the Name of the 
	 replicant with the given unique id.
	 */
 
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

//
status_t 
AddOnManager::GetReplicantView(BMessenger target, int32 uid, BMessage *reply) const
{
	/*
	 We send a message to the target shelf, asking it for the Name of the 
	 replicant with the given unique id.
	 */
 
	BMessage request(B_GET_PROPERTY);
	BMessage uid_specifier(B_ID_SPECIFIER);// specifying via ID
	status_t err;
	status_t e;

	request.AddSpecifier("View");// ask for the Name of the replicant

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


/*
 *  Method: AddOnManager::MessageReceived()
 *   Descr: 
 */
void
AddOnManager::MessageReceived(BMessage *message)
{
	CALLED();
	
	BMessage reply;
	status_t status = B_OK;

	PRINT(("%s what:%c%c%c%c\n", __PRETTY_FUNCTION__, message->what>>24, message->what>>16, message->what>>8, message->what));
	
	switch(message->what)
	{
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
		default:
		{
			return;		
		}
	}
	
	reply.AddInt32("status", status);
	message->SendReply(&reply);
}


/*
 *  Method: AddOnManager::HandleStartStopDevices()
 *   Descr: 
 */
status_t
AddOnManager::HandleStartStopDevices(BMessage *message,
                                     BMessage *reply)
{
	const char *name = NULL;
	int32 type = 0;
	if (! ((message->FindInt32("type", &type)!=B_OK) ^ (message->FindString("device", &name)!=B_OK)))
		return B_ERROR;
		
	return InputServer::StartStopDevices(name, (input_device_type)type, message->what == IS_START_DEVICE);
}


/*
 *  Method: AddOnManager::HandleFindDevices()
 *   Descr: 
 */
status_t
AddOnManager::HandleFindDevices(BMessage *message,
                                     BMessage *reply)
{
	CALLED();
	const char *name = NULL;
	message->FindString("device", &name);
	
	for (int i = InputServer::gInputDeviceList.CountItems() - 1; i >= 0; i--) {
		InputDeviceListItem* item = (InputDeviceListItem*)InputServer::gInputDeviceList.ItemAt(i);
		if (!item)
			continue;
			
		if (!name || strcmp(name, item->mDev.name) == 0) {
			reply->AddString("device", item->mDev.name);
			reply->AddInt32("type", item->mDev.type);
			if (name)
				return B_OK;	
		}
	}

	return B_OK;
}


/*
 *  Method: AddOnManager::HandleWatchDevices()
 *   Descr: 
 */
status_t
AddOnManager::HandleWatchDevices(BMessage *message,
                                     BMessage *reply)
{
	// TODO
	return B_OK;
}


/*
 *  Method: AddOnManager::HandleIsDeviceRunning()
 *   Descr: 
 */
status_t
AddOnManager::HandleIsDeviceRunning(BMessage *message,
                                     BMessage *reply)
{
	const char *name = NULL;
	if (message->FindString("device", &name)!=B_OK)
		return B_ERROR;
	
	for (int i = InputServer::gInputDeviceList.CountItems() - 1; i >= 0; i--) {
		InputDeviceListItem* item = (InputDeviceListItem*)InputServer::gInputDeviceList.ItemAt(i);
		if (!item)
			continue;
			
		if (strcmp(name, item->mDev.name) == 0)
			return (item->mStarted) ? B_OK : B_ERROR;
	}

	return B_ERROR;
}


/*
 *  Method: AddOnManager::HandleControlDevices()
 *   Descr: 
 */
status_t
AddOnManager::HandleControlDevices(BMessage *message,
                                     BMessage *reply)
{
	CALLED();
	const char *name = NULL;
	int32 type = 0;
	if (! ((message->FindInt32("type", &type)!=B_OK) ^ (message->FindString("device", &name)!=B_OK)))
		return B_ERROR;
	
	uint32 code = 0;
	BMessage msg;
	bool isMessage = true;
	if (message->FindInt32("code", (int32*)&code)!=B_OK)
		return B_ERROR;
	if (message->FindMessage("message", &msg)!=B_OK)
		isMessage = false;
		
	return InputServer::ControlDevices(name, (input_device_type)type, code, isMessage ? &msg : NULL);
}


/*
 *  Method: AddOnManager::HandleSystemShuttingDown()
 *   Descr: 
 */
status_t
AddOnManager::HandleSystemShuttingDown(BMessage *message,
                                     BMessage *reply)
{
	// TODO
	return B_OK;
}
