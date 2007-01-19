/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

// MountMenu implements a context menu used for mounting/unmounting volumes

#include <MenuItem.h>
#include <Mime.h>
#include <InterfaceDefs.h>
#include <VolumeRoster.h>
#include <Volume.h>
#include <fs_info.h>

#include "AutoMounter.h"
#include "Commands.h"
#include "MountMenu.h"
#include "IconMenuItem.h"
#include "Tracker.h"
#include "Bitmaps.h"

#ifdef __HAIKU__
#	include <DiskDevice.h>
#	include <DiskDeviceList.h>
#else
#	include "DeviceMap.h"
#endif

#define SHOW_NETWORK_VOLUMES


#ifdef __HAIKU__
//	#pragma mark - Haiku Disk Device API


class AddMenuItemVisitor : public BDiskDeviceVisitor {
	public:
		AddMenuItemVisitor(BMenu* menu);
		virtual ~AddMenuItemVisitor();

		virtual bool Visit(BDiskDevice *device);
		virtual bool Visit(BPartition *partition, int32 level);

	private:
		BMenu* fMenu;
};


AddMenuItemVisitor::AddMenuItemVisitor(BMenu* menu)
	:
	fMenu(menu)
{
}


AddMenuItemVisitor::~AddMenuItemVisitor()
{
}


bool
AddMenuItemVisitor::Visit(BDiskDevice *device)
{
	return Visit(device, 0);
}


bool
AddMenuItemVisitor::Visit(BPartition *partition, int32 level)
{
	if (!partition->ContainsFileSystem())
		return NULL;

	// get name (and eventually the type)
	BString name = partition->ContentName();
	if (name.Length() == 0) {
		name = partition->Name();
		if (name.Length() == 0) {
			const char *type = partition->ContentType();
			if (type == NULL)
				return NULL;

			name = "(unnamed ";
			name << type;
			name << ")";
		}
	}

	// get icon
	BBitmap *icon = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1),
		B_CMAP8);
	if (partition->GetIcon(icon, B_MINI_ICON) != B_OK) {
		delete icon;
		icon = NULL;
	}

	BMessage *message = new BMessage(partition->IsMounted() ?
		kUnmountVolume : kMountVolume);
	message->AddInt32("id", partition->ID());

	// TODO: for now, until we actually have disk device icons
	BMenuItem *item;
	if (icon != NULL)
		item = new IconMenuItem(name.String(), message, icon);
	else
		item = new BMenuItem(name.String(), message);
	if (partition->IsMounted()) {
		item->SetMarked(true);

		BVolume volume;
		if (partition->GetVolume(&volume) == B_OK) {
			BVolume bootVolume;
			BVolumeRoster().GetBootVolume(&bootVolume);
			if (volume == bootVolume)
				item->SetEnabled(false);
		}
	}

	fMenu->AddItem(item);
	return false;
}

#else	// !__HAIKU__
//	#pragma mark - R5 DeviceMap API


struct AddOneAsMenuItemParams {
	BMenu *mountMenu;
};


static Partition *
AddOnePartitionAsMenuItem(Partition *partition, void *castToParams)
{
	if (partition->Hidden())
		return NULL;

	AddOneAsMenuItemParams *params = (AddOneAsMenuItemParams *)castToParams;
	BBitmap *icon = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1),
		B_CMAP8);
	get_device_icon(partition->GetDevice()->Name(), icon->Bits(), B_MINI_ICON);	


	const char *name = partition->GetDevice()->DisplayName();

	if (!partition->GetDevice()->IsFloppy() ||
		partition->Mounted() == kMounted) {
		if (*partition->VolumeName())
			name = partition->VolumeName();
		else if (*partition->Type())
			name = partition->Type();
	}

	BMessage *message = new BMessage;

	if (partition->Mounted() == kMounted) {	
		message->what = kUnmountVolume;	
		message->AddInt32("device_id", partition->VolumeDeviceID());
	} else {
		message->what = kMountVolume;
		
		//
		//	Floppies have an ID of -1, because they don't have 
		//	partition (and hence no parititon ID).
		//
		if (partition->GetDevice()->IsFloppy())
			message->AddInt32("id", kFloppyID);
		else
			message->AddInt32("id", partition->UniqueID());
	}	

	BMenuItem *item = new IconMenuItem(name, message, icon);
	if (partition->Mounted() == kMounted)
		item->SetMarked(true);

	if (partition->Mounted() == kMounted) {
		BVolume partVolume(partition->VolumeDeviceID());

		BVolume bootVolume;
		BVolumeRoster().GetBootVolume(&bootVolume);
		if (partVolume == bootVolume)
			item->SetEnabled(false);
	}

	params->mountMenu->AddItem(item);

	return NULL;
}
#endif	// !__HAIKU__


//	#pragma mark -


MountMenu::MountMenu(const char *name)
	: BMenu(name)
{
	SetFont(be_plain_font);
}


bool
MountMenu::AddDynamicItem(add_state)
{
	// remove old items
	for (;;) {
		BMenuItem *item = RemoveItem(0L);
		if (item == NULL)
			break;
		delete item;
	}

#ifdef __HAIKU__
	BDiskDeviceList devices;
	status_t status = devices.Fetch();
	if (status == B_OK) {
		AddMenuItemVisitor visitor(this);
		devices.VisitEachPartition(&visitor);
	}
#else
	AddOneAsMenuItemParams params;
	params.mountMenu = this;

	AutoMounter *autoMounter = dynamic_cast<TTracker *>(be_app)->
		AutoMounterLoop();

	autoMounter->CheckVolumesNow();
	autoMounter->EachPartition(&AddOnePartitionAsMenuItem, &params);
#endif

#ifdef SHOW_NETWORK_VOLUMES
	// iterate the volume roster and look for volumes with the
	// 'shared' attributes -- these same volumes will not be returned
	// by the autoMounter because they do not show up in the /dev tree
	BVolumeRoster volumeRoster;
	BVolume volume;
	bool needSeparator = false;
	while (volumeRoster.GetNextVolume(&volume) == B_OK) {
		if (volume.IsShared()) {
			needSeparator = true;
			BBitmap *icon = new BBitmap(BRect(0, 0, 15, 15), B_CMAP8);
			fs_info info;
			if (fs_stat_dev(volume.Device(), &info) != B_OK) {
				PRINT(("Cannot get mount menu item icon; bad device ID\n"));
				delete icon;
				continue;
			}
			// Use the shared icon instead of the device icon
			if (get_device_icon(info.device_name, icon->Bits(), B_MINI_ICON) != B_OK)
				GetTrackerResources()->GetIconResource(kResShareIcon, B_MINI_ICON, icon);
			
			BMessage *message = new BMessage(kUnmountVolume);
			message->AddInt32("device_id", volume.Device());
			char volumeName[B_FILE_NAME_LENGTH];
			volume.GetName(volumeName);
			
			BMenuItem *item = new IconMenuItem(volumeName, message, icon);
			item->SetMarked(true);
			AddItem(item);
		}
	}
#endif	// SHOW_NETWORK_VOLUMES

	AddSeparatorItem();

#ifndef __HAIKU__
	// add an option to rescan the scsii bus, etc.
	BMenuItem *rescanItem = NULL;
	if (modifiers() & B_SHIFT_KEY) {
		rescanItem = new BMenuItem("Rescan Devices", new BMessage(kAutomounterRescan));
		AddItem(rescanItem);
	}
#endif

	BMenuItem *mountAll = new BMenuItem("Mount All", new BMessage(kMountAllNow));
	AddItem(mountAll);
	BMenuItem *mountSettings = new BMenuItem("Settings"B_UTF8_ELLIPSIS, 
		new BMessage(kRunAutomounterSettings));
	AddItem(mountSettings);

	SetTargetForItems(be_app);
	
#ifndef __HAIKU__
	if (rescanItem)
		rescanItem->SetTarget(autoMounter);
#endif

	return false;
}
