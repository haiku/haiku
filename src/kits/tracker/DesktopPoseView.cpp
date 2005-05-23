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

//	DesktopPoseView adds support for displaying integrated desktops
//	from multiple volumes to BPoseView
//
//	Used by the Desktop window and by the root view in file panels

#include <NodeMonitor.h>
#include <Path.h>
#include <VolumeRoster.h>
#include <Volume.h>

#include "Commands.h"
#include "DesktopPoseView.h"
#include "FSUtils.h"
#include "PoseList.h"
#include "Tracker.h"
#include "TrackerSettings.h"
#include "TrackerString.h"


namespace BPrivate {

bool
ShouldShowDesktopPose(dev_t device, const Model *model, const PoseInfo *)
{
	if (model->NodeRef()->device != device) {
		// avoid having more than one Trash
		BDirectory remoteTrash;
		if (FSGetTrashDir(&remoteTrash, model->NodeRef()->device) == B_OK) {
			node_ref remoteTrashNodeRef;
			remoteTrash.GetNodeRef(&remoteTrashNodeRef);
			if (remoteTrashNodeRef == *model->NodeRef())
				return false;
		}
	}
	return true;
}

}	// namespace BPrivate


DesktopEntryListCollection::DesktopEntryListCollection()
{
}


//	#pragma mark -


DesktopPoseView::DesktopPoseView(Model *model, BRect frame, uint32 viewMode,
	uint32 resizeMask)
	:	BPoseView(model, frame, viewMode, resizeMask)
{
}


EntryListBase *
DesktopPoseView::InitDesktopDirentIterator(BPoseView *nodeMonitoringTarget,
	const entry_ref *ref)
{
	// the desktop dirent iterator knows how to iterate over all the volumes,
	// integrated onto the desktop

	Model sourceModel(ref, false, true);
	if (sourceModel.InitCheck() != B_OK)
		return NULL;

	CachedEntryIteratorList *result = new DesktopEntryListCollection();
	
	ASSERT(!sourceModel.IsQuery());
	ASSERT(sourceModel.Node());
	BDirectory *sourceDirectory = dynamic_cast<BDirectory *>(sourceModel.Node());

	dev_t sourceDevice = sourceModel.NodeRef()->device;
	
	ASSERT(sourceDirectory);
	
	// build an iterator list, start with boot
	EntryListBase *perDesktopIterator = new CachedDirectoryEntryList(
		*sourceDirectory);

	result->AddItem(perDesktopIterator);
	if (nodeMonitoringTarget) {
		TTracker::WatchNode(sourceModel.NodeRef(),
				B_WATCH_DIRECTORY | B_WATCH_NAME | B_WATCH_STAT | B_WATCH_ATTR,
				nodeMonitoringTarget);
	}

	// add the other volumes

	BVolumeRoster roster;
	roster.Rewind();
	BVolume volume;
	while (roster.GetNextVolume(&volume) == B_OK) {
		if (volume.Device() == sourceDevice)
			// got that already
			continue;

		if (!DesktopPoseView::ShouldIntegrateDesktop(volume))
			continue;

		BDirectory remoteDesktop;
		if (FSGetDeskDir(&remoteDesktop, volume.Device()) < B_OK)
			continue;

		BDirectory root;
		if (volume.GetRootDirectory(&root) == B_OK) {
			perDesktopIterator = new CachedDirectoryEntryList(remoteDesktop);
			result->AddItem(perDesktopIterator);

			node_ref nodeRef;
			remoteDesktop.GetNodeRef(&nodeRef);

			if (nodeMonitoringTarget) {
				TTracker::WatchNode(&nodeRef,
						B_WATCH_DIRECTORY | B_WATCH_NAME | B_WATCH_STAT | B_WATCH_ATTR,
						nodeMonitoringTarget);
			}
		}
	}
	
	if (result->Rewind() != B_OK) {
		delete result;
		if (nodeMonitoringTarget)
			nodeMonitoringTarget->HideBarberPole();

		return NULL;
	}

	return result;
}


EntryListBase *
DesktopPoseView::InitDirentIterator(const entry_ref *ref)
{
	return InitDesktopDirentIterator(this, ref);
}


bool
DesktopPoseView::FSNotification(const BMessage *message)
{
	switch (message->FindInt32("opcode")) {
		case B_DEVICE_MOUNTED:
		{
			dev_t device;
			if (message->FindInt32("new device", &device) != B_OK)
				break;

			ASSERT(TargetModel());
			TrackerSettings settings;

			BVolume volume(device);
			if (volume.InitCheck() != B_OK)
				break;

			if (settings.MountVolumesOntoDesktop()
				&& (!volume.IsShared() || settings.MountSharedVolumesOntoDesktop())) {
				// place an icon for the volume onto the desktop
				CreateVolumePose(&volume, true);
			}

			if (!ShouldIntegrateDesktop(volume))
				break;

			BDirectory otherDesktop;
			BEntry entry;
			if (FSGetDeskDir(&otherDesktop, volume.Device()) == B_OK
				&& otherDesktop.GetEntry(&entry) == B_OK) {
				// place desktop items from the mounted volume onto the desktop
				Model model(&entry);
				if (model.InitCheck() == B_OK) 
					AddPoses(&model);
			}
		}
		break;
	}

	return _inherited::FSNotification(message);
}


bool
DesktopPoseView::AddPosesThreadValid(const entry_ref *) const
{
	return true;
}


bool
DesktopPoseView::ShouldShowPose(const Model *model, const PoseInfo *poseInfo)
{
	ASSERT(TargetModel());
	if (!ShouldShowDesktopPose(TargetModel()->NodeRef()->device, model, poseInfo))
		return false;

	return _inherited::ShouldShowPose(model, poseInfo);
}


bool
DesktopPoseView::Represents(const node_ref *ref) const
{
	//	When the Tracker is set up to integrate non-boot beos volumes,
	//	it represents the home/Desktop folders of all beos volumes

	if (TrackerSettings().IntegrateNonBootBeOSDesktops()) {
		BDirectory deviceDesktop;
		FSGetDeskDir(&deviceDesktop, ref->device);
		node_ref nref;
		deviceDesktop.GetNodeRef(&nref);
		return nref == *ref;
	}
	
	return _inherited::Represents(ref);
}


bool
DesktopPoseView::Represents(const entry_ref *ref) const
{
	BEntry entry(ref);
	node_ref nref;
	entry.GetNodeRef(&nref);
	return Represents(&nref);
}


void
DesktopPoseView::ShowVolumes(bool visible, bool showShared)
{
	if (LockLooper()) {
		if (!visible)
			RemoveRootPoses();
		else
			AddRootPoses(true, showShared);	
		UnlockLooper();
	}
}
 

void
DesktopPoseView::RemoveNonBootItems()
{
	AutoLock<BWindow> lock(Window());
	if (!lock)
		return;

	EachPoseAndModel(fPoseList, &RemoveNonBootDesktopModels, (BPoseView*)this, (dev_t)0);
}


void
DesktopPoseView::AddNonBootItems()
{
	AutoLock<BWindow> lock(Window());
	if (!lock)
		return;

	BVolumeRoster volumeRoster;
 
	BVolume boot;
	volumeRoster.GetBootVolume(&boot);

 	BVolume volume;
	while (volumeRoster.GetNextVolume(&volume) == B_OK) {
		if (volume == boot || !ShouldIntegrateDesktop(volume))
			continue;

		BDirectory otherDesktop;
		BEntry entry;

		if (FSGetDeskDir(&otherDesktop, volume.Device()) == B_OK
			&& otherDesktop.GetEntry(&entry) == B_OK) {
			// place desktop items from the mounted volume onto the desktop
			Model model(&entry);
			if (model.InitCheck() == B_OK) 
				AddPoses(&model);
		}	
	}
}


void
DesktopPoseView::StartSettingsWatch()
{
	be_app->LockLooper();
	be_app->StartWatching(this, kShowDisksIconChanged);
	be_app->StartWatching(this, kVolumesOnDesktopChanged);
	be_app->StartWatching(this, kDesktopIntegrationChanged);
	be_app->UnlockLooper();
}


void
DesktopPoseView::StopSettingsWatch()
{
	be_app->LockLooper();
	be_app->StopWatching(this, kShowDisksIconChanged);
	be_app->StopWatching(this, kVolumesOnDesktopChanged);
	be_app->StopWatching(this, kDesktopIntegrationChanged);
	be_app->UnlockLooper();
}


void
DesktopPoseView::AdaptToVolumeChange(BMessage *message)
{
	TTracker *tracker = dynamic_cast<TTracker *>(be_app);
	if (!tracker)
		return;

	bool showDisksIcon = false;
	bool mountVolumesOnDesktop = true;
	bool mountSharedVolumesOntoDesktop = false;

	message->FindBool("ShowDisksIcon", &showDisksIcon);
	message->FindBool("MountVolumesOntoDesktop", &mountVolumesOnDesktop);
	message->FindBool("MountSharedVolumesOntoDesktop", &mountSharedVolumesOntoDesktop);

	BEntry entry("/");
	Model model(&entry);
	if (model.InitCheck() == B_OK) {
		BMessage entryMessage;
		entryMessage.what = B_NODE_MONITOR;
		
		if (showDisksIcon)
			entryMessage.AddInt32("opcode", B_ENTRY_CREATED);
		else {
			entryMessage.AddInt32("opcode", B_ENTRY_REMOVED);
			entry_ref ref;
			if (entry.GetRef(&ref) == B_OK) {
				BContainerWindow *disksWindow = tracker->FindContainerWindow(&ref);
				if (disksWindow) {
					disksWindow->Lock();
					disksWindow->Close();
				}
			}
		}
		entryMessage.AddInt32("device", model.NodeRef()->device);
		entryMessage.AddInt64("node", model.NodeRef()->node);
		entryMessage.AddInt64("directory", model.EntryRef()->directory);
		entryMessage.AddString("name", model.EntryRef()->name);
		BContainerWindow *deskWindow = dynamic_cast<BContainerWindow*>(Window());
		if (deskWindow)
			deskWindow->PostMessage(&entryMessage, deskWindow->PoseView());
	}

	ShowVolumes(mountVolumesOnDesktop, mountSharedVolumesOntoDesktop);
}


void
DesktopPoseView::AdaptToDesktopIntegrationChange(BMessage *message)
{
	bool mountVolumesOnDesktop = true;
	bool mountSharedVolumesOntoDesktop = true;
	bool integrateNonBootBeOSDesktops = true;
	
	message->FindBool("MountVolumesOntoDesktop", &mountVolumesOnDesktop);
	message->FindBool("MountSharedVolumesOntoDesktop", &mountSharedVolumesOntoDesktop);
	message->FindBool("IntegrateNonBootBeOSDesktops", &integrateNonBootBeOSDesktops);

	ShowVolumes(false, mountSharedVolumesOntoDesktop);
	ShowVolumes(mountVolumesOnDesktop, mountSharedVolumesOntoDesktop);

	UpdateNonBootDesktopPoses(integrateNonBootBeOSDesktops);
}


void
DesktopPoseView::UpdateNonBootDesktopPoses(bool integrateNonBootBeOSDesktops)
{
	static bool nonBootDesktopPosesAlreadyAdded = false;

	BVolumeRoster volumeRoster;
	BVolume	bootVolume;
	volumeRoster.GetBootVolume(&bootVolume);

	dev_t bootDevice = bootVolume.Device();

	int32 poseCount = CountItems();

	if (!integrateNonBootBeOSDesktops) {
		for (int32 index = 0; index < poseCount; index++) {
			Model *model = PoseAtIndex(index)->TargetModel();
			if (!model->IsVolume() && model->NodeRef()->device != bootDevice){
				DeletePose(model->NodeRef());
				index--;
				poseCount--;
			}
		}
		nonBootDesktopPosesAlreadyAdded = false;
	} else if (!nonBootDesktopPosesAlreadyAdded) {
		for (int32 index = 0; index < poseCount; index++) {
			Model *model = PoseAtIndex(index)->TargetModel();

			if (model->IsVolume()) {
				BDirectory remoteDesktop;
				BEntry entry;
				BVolume volume(model->NodeRef()->device);

				if (ShouldIntegrateDesktop(volume)
					&& FSGetDeskDir(&remoteDesktop, volume.Device()) == B_OK
					&& remoteDesktop.GetEntry(&entry) == B_OK
					&& volume != bootVolume) {
					// place desktop items from the volume onto the desktop
					Model model(&entry);
					if (model.InitCheck() == B_OK) 
						AddPoses(&model);
				}
			}
		}
		nonBootDesktopPosesAlreadyAdded = true;
	}
}

