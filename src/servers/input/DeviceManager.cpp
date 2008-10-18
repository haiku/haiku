/*
 * Copyright 2004-2008, the Haiku project. All rights reserved.
 * Distributed under the terms of the Haiku License.
 *
 * Authors:
 *		Marcus Overhagen, Axel Dörfler
 *		Jérôme Duval
 */

#include "DeviceManager.h"

#include <stdio.h>
#include <string.h>

#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <image.h>
#include <Path.h>
#include <String.h>

#include "InputServer.h"


void
DeviceManager::MessageReceived(BMessage *msg)
{
	CALLED();
	if (msg->what == B_NODE_MONITOR) {
		int32 opcode;
		if (msg->FindInt32("opcode", &opcode) == B_OK) {
			switch (opcode) {
				case B_ENTRY_CREATED:
				case B_ENTRY_REMOVED:
				case B_ENTRY_MOVED:
				{
					node_ref dir_nref;
					if (msg->FindInt32("device", &dir_nref.device) != B_OK
						|| msg->FindInt64("directory", &dir_nref.node) != B_OK)
						return;

					_BDeviceAddOn_ *addon = NULL;
					int32 i = 0;
					while ((addon = GetAddOn(i++)) !=NULL) {
						int32 j=0;
						node_ref *dnref = NULL;
						while ((dnref = (node_ref *)addon->fMonitoredRefs.ItemAt(j++)) != NULL) {
							if (*dnref == dir_nref) {
								addon->fDevice->Control(NULL, NULL, msg->what,
									msg);
							}
						}
					}
					break;
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
DeviceManager::AddDirectory(const node_ref *nref, _BDeviceAddOn_ *addon)
{
	CALLED();
	BDirectory directory(nref);
	status_t status = directory.InitCheck();
	if (status != B_OK)
		return status;

	status = watch_node(nref, B_WATCH_DIRECTORY, this);
	if (status != B_OK)
		return status;

	BEntry entry;
	while (directory.GetNextEntry(&entry, true) == B_OK) {
		entry_ref ref;
		entry.GetRef(&ref);
		BMessage msg(B_NODE_MONITOR);
		msg.AddInt32("opcode", B_ENTRY_CREATED);
		msg.AddInt32("device", nref->device);
		msg.AddInt64("directory", nref->node);
		msg.AddString("name", ref.name);
		addon->fDevice->Control(NULL, NULL, msg.what, &msg);
	}

	return B_OK;
}


status_t
DeviceManager::RemoveDirectory(const node_ref *nref, _BDeviceAddOn_ *addon)
{
	CALLED();
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
		addon->fDevice->Control(NULL, NULL, msg.what, &msg);
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
	_BDeviceAddOn_ *addon = NULL;
	while ((addon = (_BDeviceAddOn_ *)fDeviceAddons.RemoveItem((int32)0)) != NULL)
		delete addon;
}


void
DeviceManager::LoadState()
{
	CALLED();
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
DeviceManager::StartMonitoringDevice(_BDeviceAddOn_ *addon, const char *device)
{
	CALLED();
	status_t err;
	node_ref nref;
	BDirectory directory;
	BPath path("/dev");
	if ((err = path.Append(device)) != B_OK) {
		PRINTERR(("DeviceManager::StartMonitoringDevice BPath::Append() error %s: %s\n", path.Path(), strerror(err)));
		return err;
	}

	if ((err = directory.SetTo(path.Path())) != B_OK) {
		if (err != B_ENTRY_NOT_FOUND) {
			PRINTERR(("DeviceManager::StartMonitoringDevice SetTo error %s: %s\n", path.Path(), strerror(err)));
			return err;
		}
		if ((err = create_directory(path.Path(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) != B_OK
			|| (err = directory.SetTo(path.Path())) != B_OK) {
			PRINTERR(("DeviceManager::StartMonitoringDevice CreateDirectory error %s: %s\n", path.Path(), strerror(err)));
			return err;
		}
	}

	if ((err = directory.GetNodeRef(&nref)) != B_OK) {
		PRINTERR(("DeviceManager::StartMonitoringDevice GetNodeRef error %s: %s\n", path.Path(), strerror(err)));
		return err;
	}

	// test if already monitored
	bool alreadyMonitored = false;
	_BDeviceAddOn_ *tmpaddon = NULL;
	int32 i = 0;
	while ((tmpaddon = (_BDeviceAddOn_ *)fDeviceAddons.ItemAt(i++)) !=NULL) {
		int32 j=0;
		node_ref *dnref = NULL;
		while ((dnref = (node_ref *)tmpaddon->fMonitoredRefs.ItemAt(j++)) != NULL) {
			if (*dnref == nref) {
				PRINT(("StartMonitoringDevice already monitored\n"));
				alreadyMonitored = true;
				break;
			}
		}
		if (alreadyMonitored)
			break;
	}

	// monitor if needed
	if (!alreadyMonitored) {
		if ((err = AddDirectory(&nref, addon)) != B_OK)
			return err;
	}

	// add addon in list
	if (!fDeviceAddons.HasItem(addon))
		fDeviceAddons.AddItem(addon);

	// add dir ref in list
	int32 j=0;
	node_ref *dnref = NULL;
	alreadyMonitored = false;
	while ((dnref = (node_ref *)addon->fMonitoredRefs.ItemAt(j++)) != NULL) {
		if (*dnref == nref) {
			alreadyMonitored = true;
			break;
		}
	}

	if (!alreadyMonitored)
		addon->fMonitoredRefs.AddItem(new node_ref(nref));

	return B_OK;
}


status_t
DeviceManager::StopMonitoringDevice(_BDeviceAddOn_ *addon, const char *device)
{
	CALLED();
	status_t err;
	node_ref nref;
	BDirectory directory;
	BPath path("/dev");
	if ((err = path.Append(device)) != B_OK
		|| (err = directory.SetTo(path.Path())) != B_OK
		|| (err = directory.GetNodeRef(&nref)) != B_OK)
		return err;

	// test if still monitored
	bool stillMonitored = false;
	_BDeviceAddOn_ *tmpaddon = NULL;
	int32 i = 0;
	while ((tmpaddon = (_BDeviceAddOn_ *)fDeviceAddons.ItemAt(i++)) != NULL) {
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
	int32 j = 0;
	while ((dnref = (node_ref *)addon->fMonitoredRefs.ItemAt(j)) != NULL) {
		if (*dnref == nref) {
			addon->fMonitoredRefs.RemoveItem(j);
			delete dnref;
			break;
		}
		j++;
	}

	if (addon->fMonitoredRefs.IsEmpty())
		fDeviceAddons.RemoveItem(addon);

	// stop monitoring if needed
	if (!stillMonitored) {
		if ((err = RemoveDirectory(&nref, addon)) != B_OK)
			return err;
	}

	return B_OK;
}
