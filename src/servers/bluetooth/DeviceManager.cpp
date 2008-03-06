/*
** Copyright 2004, the Haiku project. All rights reserved.
** Distributed under the terms of the Haiku License.
**
**
** Author : Jérôme Duval
** Original authors: Marcus Overhagen, Axel Dörfler
**
** Bluetooth adaptation: Oliver Ruiz Dorantes
**
*/


#include <Application.h>
#include <Autolock.h>
#include <String.h>

#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>
#include <NodeMonitor.h>


#include <image.h>
#include <stdio.h>
#include <string.h>

#include "DeviceManager.h"
#include "LocalDeviceImpl.h"

#include "Output.h"
#include "BluetoothServer.h"

#include <bluetoothserver_p.h>


void
DeviceManager::MessageReceived(BMessage * msg)
{
	if (msg->what == B_NODE_MONITOR) {
		int32 opcode;
		if (msg->FindInt32("opcode", &opcode) == B_OK) {
			switch (opcode) {
			case B_ENTRY_CREATED:
			case B_ENTRY_REMOVED:
			case B_ENTRY_MOVED:
			{
				node_ref dir_nref;
				const char *name;				
				BDirectory dir;

				(Output::Instance()->Post("Bus changing ...\n", BLACKBOARD_DEVICEMANAGER));	
				
				if ((msg->FindInt32("device", &dir_nref.device)!=B_OK)
					|| (msg->FindInt64("directory", &dir_nref.node)!=B_OK)
					|| (msg->FindString("name", &name) != B_OK))
					return;
					
				// Check if the entry is a File or a directory
				if (dir.SetTo(&dir_nref) == B_OK) {				    
					(Output::Instance()->Post(name, BLACKBOARD_DEVICEMANAGER));	            
					(Output::Instance()->Post(" adding folder...\n", BLACKBOARD_DEVICEMANAGER));	            
					AddDirectory(&dir_nref);
			    } else {
			        // Then it is a device!
					// post it to server			        
//			        msg->what = BT_MSG_ADD_DEVICE;			        			     
//			        be_app_messenger.SendMessage(msg);
//					(Output::Instance()->Post("New bluetooth device on bus ...\n", BLACKBOARD_DEVICEMANAGER));	
			    }
			}
			case B_STAT_CHANGED:
			case B_ATTR_CHANGED:
			case B_DEVICE_MOUNTED:
			case B_DEVICE_UNMOUNTED:
			default:
				BLooper::MessageReceived(msg);
				break;
			}
		}
	}
}


status_t
DeviceManager::AddDirectory(node_ref *nref)
{
	BDirectory directory(nref);
	status_t status = directory.InitCheck();
	if (status != B_OK) {
		(Output::Instance()->Post("AddDirectory::Initcheck Failed\n", BLACKBOARD_DEVICEMANAGER));
		return status;
	}

	status = watch_node(nref, B_WATCH_DIRECTORY, this);
	if (status != B_OK) {
		(Output::Instance()->Post("AddDirectory::watch_node Failed\n", BLACKBOARD_DEVICEMANAGER));	
		return status;
	}
	
	BEntry entry;
	while (directory.GetNextEntry(&entry, true) == B_OK) {
	    
	    // its suposed to be devices ...
		entry_ref ref;
		entry.GetRef(&ref);
		BPath path(new BEntry(&ref));
		BString* str = new BString(path.Path());
		
		BMessage* msg = new BMessage(BT_MSG_ADD_DEVICE);
		msg->AddInt32("opcode", B_ENTRY_CREATED);
		msg->AddInt32("device", nref->device);
		msg->AddInt64("directory", nref->node);
		msg->AddString("name", *str );
        
        be_app_messenger.SendMessage(msg);


		(Output::Instance()->Post( path.Path(), BLACKBOARD_DEVICEMANAGER));
		(Output::Instance()->Post(" Entry added\n", BLACKBOARD_DEVICEMANAGER));
	
	}
	(Output::Instance()->Post("Finished exploring entries\n", BLACKBOARD_DEVICEMANAGER));

	return B_OK;
}


status_t
DeviceManager::RemoveDirectory(node_ref* nref)
{
	BDirectory directory(nref);
	status_t status = directory.InitCheck();
	if (status != B_OK)
		return status;

	status = watch_node(nref, B_STOP_WATCHING, this);
	if (status != B_OK)
		return status;

	BEntry entry;
	while (directory.GetNextEntry(&entry, true) == B_OK) {
		entry_ref ref;
		entry.GetRef(&ref);
		BMessage msg(B_NODE_MONITOR);
		msg.AddInt32("opcode", B_ENTRY_REMOVED);
		msg.AddInt32("device", nref->device);
		msg.AddInt64("directory", nref->node);
		msg.AddString("name", ref.name);
		//addon->fDevice->Control(NULL, NULL, msg.what, &msg);
	}

	return B_OK;
}


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

	if (!Lock())
		return;
	Run();
	Unlock();
}


void
DeviceManager::SaveState()
{
}


status_t 
DeviceManager::StartMonitoringDevice(const char *device)
{

	status_t err;
	node_ref nref;
	BDirectory directory;
	BPath path("/dev");

	/* Build the path */
	if ((err = path.Append(device)) != B_OK) {
		printf("DeviceManager::StartMonitoringDevice BPath::Append() error %s: %s\n", path.Path(), strerror(err));
		return err;
	}

	/* Check the path */
	if ((err = directory.SetTo(path.Path())) != B_OK) {
		/* Entry not there ... */	
		if (err != B_ENTRY_NOT_FOUND) { // something else we cannot handle
			printf("DeviceManager::StartMonitoringDevice SetTo error %s: %s\n", path.Path(), strerror(err));
			return err;
		}
		/* Create it */
		if ((err = create_directory(path.Path(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) != B_OK
			|| (err = directory.SetTo(path.Path())) != B_OK) {
			printf("DeviceManager::StartMonitoringDevice CreateDirectory error %s: %s\n", path.Path(), strerror(err));
			return err;
		}
	}
 
	/* get noderef */
	if ((err = directory.GetNodeRef(&nref)) != B_OK) {
		printf("DeviceManager::StartMonitoringDevice GetNodeRef error %s: %s\n", path.Path(), strerror(err));
		return err;
	}
	
	// test if already monitored in any add-on

	(Output::Instance()->Post(device, BLACKBOARD_DEVICEMANAGER));
	(Output::Instance()->Post(" Being monitorized\n", BLACKBOARD_DEVICEMANAGER));	
		
	bool alreadyMonitored = false;
#if 0		
	HCIDelegate *tmphd = NULL;
	int32 i = 0;

	// TODO!! ask the server if this needs to be monitored

	while ((tmphd = (HCIDelegate *)fDelegatesList.ItemAt(i++)) !=NULL) {

		/* Find out the reference*/
		node_ref *dnref = (node_ref *)tmphd->fMonitoredRefs ;
		if (*dnref == nref) {
			printf("StartMonitoringDevice already monitored\n");
			alreadyMonitored = true;
			break;
		}
		
	}
#endif
	
	// monitor if needed
	if (!alreadyMonitored) {
		if ((err = AddDirectory(&nref)) != B_OK) 
			return err;
	}

/*	// add addon in list
	if (!fDeviceAddons.HasItem(addon))
		fDeviceAddons.AddItem(addon);
	
	// add dir ref in list
	int32 j=0;
	node_ref *dnref = NULL;
	alreadyMonitored = false; // why rechecking?
	while ((dnref = (node_ref *)addon->fMonitoredRefs.ItemAt(j++)) != NULL) {
		if (*dnref == nref) {
			alreadyMonitored = true;
			break;
		}
	}
	
	if (!alreadyMonitored) {
		addon->fMonitoredRefs.AddItem(new node_ref(nref));
	}
*/
	return B_OK;
}


status_t 
DeviceManager::StopMonitoringDevice(const char *device)
{
	status_t err;
	node_ref nref;
	BDirectory directory;
	BPath path("/dev");
	if (((err = path.Append(device)) != B_OK)
		|| ((err = directory.SetTo(path.Path())) != B_OK) 
		|| ((err = directory.GetNodeRef(&nref)) != B_OK))
		return err;
		
	// test if still monitored
	bool stillMonitored = false;
/*
	int32 i = 0;
	while ((tmpaddon = (_BDeviceAddOn_ *)fDeviceAddons.ItemAt(i++)) !=NULL) {
		if (addon == tmpaddon)
			continue;
			
		int32 j=0;
		node_ref *dnref = NULL;
		while ((dnref = (node_ref *)tmpaddon->fMonitoredRefs.ItemAt(j++)) != NULL) {
			if (*dnref == nref) {
				stillMonitored = true;
				break;
			}
		}
		if (stillMonitored)
			break;				
	}
	
	// remove from list
	node_ref *dnref = NULL;
	int32 j=0;
	while ((dnref = (node_ref *)addon->fMonitoredRefs.ItemAt(j)) != NULL) {
		if (*dnref == nref) {
			addon->fMonitoredRefs.RemoveItem(j);
			delete dnref;
			break;
		}
		j++;
	}
	
	// stop monitoring if needed
	if (!stillMonitored) {
		if ((err = RemoveDirectory(&nref, addon)) != B_OK)
			return err;
	} 
*/		
	return B_OK;
}
