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

#include <Catalog.h>
#include <Debug.h>
#include <Locale.h>
#include <MenuItem.h>
#include <Mime.h>
#include <InterfaceDefs.h>
#include <VolumeRoster.h>
#include <Volume.h>
#include <fs_info.h>

#include "Commands.h"
#include "MountMenu.h"
#include "IconMenuItem.h"
#include "Tracker.h"
#include "Bitmaps.h"

#include <DiskDevice.h>
#include <DiskDeviceList.h>

#define SHOW_NETWORK_VOLUMES


class AddMenuItemVisitor : public BDiskDeviceVisitor {
	public:
		AddMenuItemVisitor(BMenu* menu);
		virtual ~AddMenuItemVisitor();

		virtual bool Visit(BDiskDevice* device);
		virtual bool Visit(BPartition* partition, int32 level);

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
AddMenuItemVisitor::Visit(BDiskDevice* device)
{
	return Visit(device, 0);
}


bool
AddMenuItemVisitor::Visit(BPartition* partition, int32 level)
{
	if (!partition->ContainsFileSystem())
		return false;

	// get name (and eventually the type)
	BString name = partition->ContentName();
	if (name.Length() == 0) {
		name = partition->Name();
		if (name.Length() == 0) {
			const char* type = partition->ContentType();
			if (type == NULL)
				return false;

			uint32 divisor = 1UL << 30;
			char unit = 'G';
			if (partition->Size() < divisor) {
				divisor = 1UL << 20;
				unit = 'M';
			}

			char* buffer = name.LockBuffer(256);
			snprintf(buffer, 256, "(%.1f %cB %s)",
				1.0 * partition->Size() / divisor, unit, type);

			name.UnlockBuffer();
		}
	}

	// get icon
	BBitmap* icon = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1),
		B_RGBA32);
	if (partition->GetIcon(icon, B_MINI_ICON) != B_OK) {
		delete icon;
		icon = NULL;
	}

	BMessage* message = new BMessage(partition->IsMounted() ?
		kUnmountVolume : kMountVolume);
	message->AddInt32("id", partition->ID());

	// TODO: for now, until we actually have disk device icons
	BMenuItem* item;
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


//	#pragma mark -


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MountMenu"

MountMenu::MountMenu(const char* name)
	: BMenu(name)
{
	SetFont(be_plain_font);
}


bool
MountMenu::AddDynamicItem(add_state)
{
	// remove old items
	for (;;) {
		BMenuItem* item = RemoveItem(0L);
		if (item == NULL)
			break;
		delete item;
	}

	BDiskDeviceList devices;
	status_t status = devices.Fetch();
	if (status == B_OK) {
		AddMenuItemVisitor visitor(this);
		devices.VisitEachPartition(&visitor);
	}

#ifdef SHOW_NETWORK_VOLUMES
	// iterate the volume roster and look for volumes with the
	// 'shared' attributes -- these same volumes will not be returned
	// by the autoMounter because they do not show up in the /dev tree
	BVolumeRoster volumeRoster;
	BVolume volume;
	while (volumeRoster.GetNextVolume(&volume) == B_OK) {
		if (volume.IsShared()) {
			BBitmap* icon = new BBitmap(BRect(0, 0, 15, 15), B_CMAP8);
			fs_info info;
			if (fs_stat_dev(volume.Device(), &info) != B_OK) {
				PRINT(("Cannot get mount menu item icon; bad device ID\n"));
				delete icon;
				continue;
			}
			// Use the shared icon instead of the device icon
			if (get_device_icon(info.device_name, icon->Bits(), B_MINI_ICON) != B_OK)
				GetTrackerResources()->GetIconResource(R_ShareIcon, B_MINI_ICON, icon);

			BMessage* message = new BMessage(kUnmountVolume);
			message->AddInt32("device_id", volume.Device());
			char volumeName[B_FILE_NAME_LENGTH];
			volume.GetName(volumeName);

			BMenuItem* item = new IconMenuItem(volumeName, message, icon);
			item->SetMarked(true);
			AddItem(item);
		}
	}
#endif	// SHOW_NETWORK_VOLUMES

	AddSeparatorItem();

	BMenuItem* mountAll = new BMenuItem(B_TRANSLATE("Mount all"),
		new BMessage(kMountAllNow));
	AddItem(mountAll);
	BMenuItem* mountSettings = new BMenuItem(
		B_TRANSLATE("Settings" B_UTF8_ELLIPSIS),
		new BMessage(kRunAutomounterSettings));
	AddItem(mountSettings);

	SetTargetForItems(be_app);

	return false;
}
