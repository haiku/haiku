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


IAHandler::IAHandler(DeviceManager * manager) 
	: BHandler ("DeviceManager") 
{
	fManager = manager;
}


void
IAHandler::MessageReceived(BMessage * msg)
{
	if (msg->what == B_NODE_MONITOR) {
		int32 opcode;
		if (msg->FindInt32("opcode", &opcode) == B_OK) {
			switch (opcode) {
			case B_ENTRY_CREATED:
				
				
				break;
			case B_ENTRY_REMOVED:
				
				break;
			case B_ENTRY_MOVED:
			case B_STAT_CHANGED:
			case B_ATTR_CHANGED:
			case B_DEVICE_MOUNTED:
			case B_DEVICE_UNMOUNTED:
			default:
				BHandler::MessageReceived(msg);
				break;
			}
		}
	}
}


status_t
IAHandler::AddDirectory(const node_ref * nref)
{
	BDirectory directory(nref);
	status_t status = directory.InitCheck();
	if (status != B_OK)
		return status;

	status = watch_node(nref, B_WATCH_DIRECTORY, this);
	if (status != B_OK)
		return status;

	BEntry entry;
	while (directory.GetNextEntry(&entry, true) == B_OK) {
		add_on_entry_info entry_info;
		if (entry.GetName(entry_info.name) != B_OK) {
			continue; // discard and proceed
		}
		if (entry.GetNodeRef(&entry_info.nref) != B_OK) {
			continue; // discard and proceed
		}
		entry_info.dir_nref = *nref;
		
		// TODO : handle refs
	}

	return B_OK;
}


status_t
IAHandler::RemoveDirectory(const node_ref * nref)
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
		add_on_entry_info entry_info;
		if (entry.GetName(entry_info.name) != B_OK) {
			continue; // discard and proceed
		}
		if (entry.GetNodeRef(&entry_info.nref) != B_OK) {
			continue; // discard and proceed
		}
		entry_info.dir_nref = *nref;
		
		// TODO : handle refs
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
	fHandler = new IAHandler(this);
}


void
DeviceManager::SaveState()
{
	CALLED();
	delete fHandler;
}


status_t 
DeviceManager::StartMonitoringDevice(_BDeviceAddOn_ *addon, 
				const char *device)
{
	// test if already monitored

	// monitor if needed
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
		
	// add in list
}


status_t 
DeviceManager::StopMonitoringDevice(_BDeviceAddOn_ *addon, 
				const char *device)
{
	// test if still monitored
	// remove from list
	
	// stop monitoring if needed
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
