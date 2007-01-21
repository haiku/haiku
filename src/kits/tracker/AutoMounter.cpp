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

#include "AutoMounter.h"

#include "AutoLock.h"
#include "AutoMounterSettings.h"
#include "Commands.h"
#include "FSUtils.h"
#include "Tracker.h"
#include "TrackerSettings.h"

#ifdef __HAIKU__
#	include <DiskDeviceRoster.h>
#	include <DiskDevice.h>
#	include <DiskDeviceList.h>
#	include <fs_volume.h>
#endif

#include <Alert.h>
#include <fs_info.h>
#include <Message.h>
#include <Node.h>
#include <NodeMonitor.h>
#include <String.h>
#include <VolumeRoster.h>

#include <string.h>


static const char *kAutoMounterSettings = "automounter_settings";


#ifdef __HAIKU__
//	#pragma mark - Haiku Disk Device API

static const uint32 kMsgInitialScan = 'insc';


AutoMounter::AutoMounter()
	: BLooper("AutoMounter", B_LOW_PRIORITY),
	fNormalMode(kRestorePreviousVolumes),
	fRemovableMode(kAllVolumes)
{
	if (!BootedInSafeMode()) {
		_ReadSettings();
	} else {
		// defeat automounter in safe mode, don't even care about the settings
		fNormalMode = kNoVolumes;
		fRemovableMode = kNoVolumes;
	}

	if (PostMessage(kMsgInitialScan) != B_OK)
		debug_printf("AutoMounter: OH NO!\n");
}


AutoMounter::~AutoMounter()
{
}


void
AutoMounter::_MountVolumes(mount_mode normal, mount_mode removable)
{
	if (normal == kNoVolumes && removable == kNoVolumes)
		return;

	class InitialMountVisitor : public BDiskDeviceVisitor {
		public:
			InitialMountVisitor(mount_mode normalMode, mount_mode removableMode,
					BMessage& previous)
				:
				fNormalMode(normalMode),
				fRemovableMode(removableMode),
				fPrevious(previous)
			{
			}

			virtual
			~InitialMountVisitor()
			{
			}

			virtual bool
			Visit(BDiskDevice* device)
			{
				return Visit(device, 0);
			}

			virtual bool
			Visit(BPartition* partition, int32 level)
			{
				mount_mode mode = partition->Device()->IsRemovableMedia()
					? fRemovableMode : fNormalMode;
				if (mode == kNoVolumes
					|| partition->IsMounted()
					|| !partition->ContainsFileSystem())
					return false;

				if (mode == kRestorePreviousVolumes) {
					// mount all volumes that were stored in the settings file
					const char *volumeName;
					BPath path;
					if (partition->GetPath(&path) != B_OK
						|| partition->ContentName() == NULL
						|| fPrevious.FindString(path.Path(), &volumeName) != B_OK
						|| strcmp(volumeName, partition->ContentName()))
						return false;
				} else if (mode == kOnlyBFSVolumes) {
					if (partition->ContentType() == NULL
						|| strcmp(partition->ContentType(), "BFS"))
						return false;
				}

				partition->Mount();
				return false;
			}

		private:
			mount_mode	fNormalMode;
			mount_mode	fRemovableMode;
			BMessage&	fPrevious;
	} visitor(normal, removable, fSettings);

	BDiskDeviceList devices;
	status_t status = devices.Fetch();
	if (status == B_OK)
		devices.VisitEachPartition(&visitor);
}


void
AutoMounter::_MountVolume(BMessage *message)
{
	int32 id;
	if (message->FindInt32("id", &id) != B_OK)
		return;

	BDiskDeviceRoster roster;
	BPartition *partition;
	BDiskDevice device;
	if (roster.GetPartitionWithID(id, &device, &partition) != B_OK)
		return;

	status_t status = partition->Mount();
	if (status < B_OK) {
		BString string;
		string << "Error mounting volume. (" << strerror(status) << ")";
			(new BAlert("", string.String(), "OK"))->Go(0);
	}
}


bool
AutoMounter::_ForceUnmount(const char* name, status_t error)
{
	BString text;
	text << "Could not unmount disk \"" << name << "\":\n\t" << strerror(error);
	text << "\n\nShould I force unmounting the disk?\n\n"
		"Note: if an application is currently writing to the volume, unmounting"
		" it now might result in loss of data.\n";

	int32 choice = (new BAlert("", text.String(), "Cancel", "Force Unmount", NULL,
		B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();

	return choice == 1;
}


void
AutoMounter::_ReportUnmountError(const char* name, status_t error)
{
	BString text;
	text << "Could not unmount disk \"" << name << "\":\n\t" << strerror(error);

	(new BAlert("", text.String(), "OK", NULL, NULL, B_WIDTH_AS_USUAL, 
		B_WARNING_ALERT))->Go(NULL);
}


void
AutoMounter::_UnmountAndEjectVolume(BMessage *message)
{
	int32 id;
	if (message->FindInt32("id", &id) == B_OK) {
		BDiskDeviceRoster roster;
		BPartition *partition;
		BDiskDevice device;
		if (roster.GetPartitionWithID(id, &device, &partition) != B_OK)
			return;

		status_t status = partition->Unmount();
		if (status < B_OK) {
			if (!_ForceUnmount(partition->ContentName(), status))
				return;

			status = partition->Unmount(B_FORCE_UNMOUNT);
		}

		if (status < B_OK)
			_ReportUnmountError(partition->ContentName(), status);

		return;
	}

	// see if we got a dev_t

	dev_t device;
	if (message->FindInt32("device_id", &device) != B_OK)
		return;

	BVolume volume(device);
	status_t status = volume.InitCheck();

	char name[B_FILE_NAME_LENGTH];
	if (status == B_OK)
		status = volume.GetName(name);
	if (status < B_OK)
		snprintf(name, sizeof(name), "device:%ld", device);

	BDirectory mountPoint;
	if (status == B_OK)
		status = volume.GetRootDirectory(&mountPoint);

	BPath path;
	if (status == B_OK)
		status = path.SetTo(&mountPoint, ".");

	if (status == B_OK) {
		status = fs_unmount_volume(path.Path(), 0);
		if (status < B_OK) {
			if (!_ForceUnmount(name, status))
				return;

			status = fs_unmount_volume(path.Path(), B_FORCE_UNMOUNT);
		}
	}

	if (status == B_OK)
		rmdir(path.Path());

	if (status < B_OK)
		_ReportUnmountError(name, status);
}


void
AutoMounter::_FromMode(mount_mode mode, bool& all, bool& bfs, bool& restore)
{
	all = bfs = restore = false;

	switch (mode) {
		case kAllVolumes:
			all = true;
			break;
		case kOnlyBFSVolumes:
			bfs = true;
			break;
		case kRestorePreviousVolumes:
			restore = true;
			break;

		default:
			break;
	}
}


AutoMounter::mount_mode
AutoMounter::_ToMode(bool all, bool bfs, bool restore)
{
	if (all)
		return kAllVolumes;
	if (bfs)
		return kOnlyBFSVolumes;
	if (restore)
		return kRestorePreviousVolumes;

	return kNoVolumes;
}


void
AutoMounter::_ReadSettings()
{
	BPath directoryPath;
	if (FSFindTrackerSettingsDir(&directoryPath) != B_OK)
		return;

	BPath path(directoryPath);
	path.Append(kAutoMounterSettings);
	fPrefsFile.SetTo(path.Path(), O_RDWR);

	if (fPrefsFile.InitCheck() != B_OK) {
		// no prefs file yet, create a new one

		BDirectory dir(directoryPath.Path());
		dir.CreateFile(kAutoMounterSettings, &fPrefsFile);
		return;
	}

	ssize_t settingsSize = (ssize_t)fPrefsFile.Seek(0, SEEK_END);
	if (settingsSize == 0)
		return;

	ASSERT(settingsSize != 0);
	char *buffer = new char[settingsSize];

	fPrefsFile.Seek(0, 0);
	if (fPrefsFile.Read(buffer, (size_t)settingsSize) != settingsSize) {
		PRINT(("error reading automounter settings\n"));
		delete [] buffer;
		return;
	}

	BMessage message('stng');
	status_t result = message.Unflatten(buffer);
	if (result != B_OK) {
		PRINT(("error %s unflattening settings, size %d\n", strerror(result), 
			settingsSize));
		delete [] buffer;
		return;
	}

	delete [] buffer;

	_UpdateSettingsFromMessage(&message);
	GetSettings(&fSettings);
}


void
AutoMounter::_WriteSettings()
{
	if (fPrefsFile.InitCheck() != B_OK)
		return;

	BMessage message('stng');
	GetSettings(&message);

	ssize_t settingsSize = message.FlattenedSize();

	char *buffer = new char[settingsSize];
	status_t result = message.Flatten(buffer, settingsSize);

	fPrefsFile.Seek(0, 0);
	result = fPrefsFile.Write(buffer, (size_t)settingsSize);
	if (result != settingsSize)
		PRINT(("error writing settings, %s\n", strerror(result)));

	delete [] buffer;
}


void
AutoMounter::_UpdateSettingsFromMessage(BMessage* message)
{
	// auto mounter settings

	bool all, bfs, restore;
	if (message->FindBool("autoMountAll", &all) != B_OK)
		all = true;
	if (message->FindBool("autoMountAllBFS", &bfs) != B_OK)
		bfs = false;

	fRemovableMode = _ToMode(all, bfs, false);

	// initial mount settings

	if (message->FindBool("initialMountAll", &all) != B_OK)
		all = false;
	if (message->FindBool("initialMountAllBFS", &bfs) != B_OK)
		bfs = false;
	if (message->FindBool("initialMountRestore", &restore) != B_OK)
		restore = true;

	fNormalMode = _ToMode(all, bfs, restore);
}


void
AutoMounter::GetSettings(BMessage *message)
{
	message->MakeEmpty();

	bool all, bfs, restore;

	_FromMode(fNormalMode, all, bfs, restore);
	message->AddBool("initialMountAll", all);
	message->AddBool("initialMountAllBFS", bfs);
	message->AddBool("initialMountRestore", restore);

	_FromMode(fRemovableMode, all, bfs, restore);
	message->AddBool("autoMountAll", all);
	message->AddBool("autoMountAllBFS", bfs);

	// Save mounted volumes so we can optionally mount them on next
	// startup
	BVolumeRoster volumeRoster;
	BVolume volume;
	while (volumeRoster.GetNextVolume(&volume) == B_OK) {
        fs_info info;
        if (fs_stat_dev(volume.Device(), &info) == 0
			&& info.flags & (B_FS_IS_REMOVABLE | B_FS_IS_PERSISTENT))
			message->AddString(info.device_name, info.volume_name);
	}
}


void
AutoMounter::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgInitialScan:
			_MountVolumes(fNormalMode, fRemovableMode);
			break;

		case kMountVolume:
			_MountVolume(message);
			break;

		case kUnmountVolume:
			_UnmountAndEjectVolume(message);
			break;

		case kSetAutomounterParams:
		{
			bool rescanNow = false;
			message->FindBool("rescanNow", &rescanNow);

			_UpdateSettingsFromMessage(message);
			GetSettings(&fSettings);
			_WriteSettings();

			if (rescanNow)
				_MountVolumes(fNormalMode, fRemovableMode);
			break;
		}

		case kMountAllNow:
			_MountVolumes(kAllVolumes, kAllVolumes);
			break;

#if 0
		case B_NODE_MONITOR:
		{
			int32 opcode;
			if (message->FindInt32("opcode", &opcode) != B_OK)
				break;

			switch (opcode) {
				//	The name of a mount point has changed
				case B_ENTRY_MOVED: {
					WRITELOG(("*** Received Mount Point Renamed Notification"));

					const char *newName;
					if (message->FindString("name", &newName) != B_OK) {
						WRITELOG(("ERROR: Couldn't find name field in update message"));
						PRINT_OBJECT(*message);
						break ;
					}

					//
					// When the node monitor reports a move, it gives the
					// parent device and inode that moved.  The problem is 
					// that  the inode is the inode of root *in* the filesystem,
					// which is generally always the same number for every 
					// filesystem of a type.
					//
					// What we'd really like is the device that the moved	
					// volume is mounted on.  Find this by using the 
					// *new* name and directory, and then stat()ing that to
					// find the device.
					//
					dev_t parentDevice;
					if (message->FindInt32("device", &parentDevice) != B_OK) {
						WRITELOG(("ERROR: Couldn't find 'device' field in update"
							" message"));
						PRINT_OBJECT(*message);
						break;
					}

					ino_t toDirectory;	
					if (message->FindInt64("to directory", &toDirectory)!=B_OK){
						WRITELOG(("ERROR: Couldn't find 'to directory' field in update"
						  "message"));
						PRINT_OBJECT(*message);
						break;
					}

					entry_ref root_entry(parentDevice, toDirectory, newName);

					BNode entryNode(&root_entry);
					if (entryNode.InitCheck() != B_OK) {
						WRITELOG(("ERROR: Couldn't create mount point entry node: %s/n", 
							strerror(entryNode.InitCheck())));
						break;
					}	

					node_ref mountPointNode;
					if (entryNode.GetNodeRef(&mountPointNode) != B_OK) {
						WRITELOG(("ERROR: Couldn't get node ref for new mount point"));
						break;
					}

					WRITELOG(("Attempt to rename device %li to %s", mountPointNode.device,
						newName));

					Partition *partition = FindPartition(mountPointNode.device);
					if (partition != NULL) {
						WRITELOG(("Found device, changing name."));

						BVolume mountVolume(partition->VolumeDeviceID());
						BDirectory mountDir;
						mountVolume.GetRootDirectory(&mountDir);
						BPath dirPath(&mountDir, 0);

						partition->SetMountedAt(dirPath.Path());
						partition->SetVolumeName(newName);					
						break;
					} else {
						WRITELOG(("ERROR: Device %li does not appear to be present",
							mountPointNode.device));
					}
				}
			}
			break;
		}
#endif

		default:
			BLooper::MessageReceived(message);
			break;
	}
}


bool
AutoMounter::QuitRequested()
{
	if (!BootedInSafeMode()) {
		// don't write out settings in safe mode - this would overwrite the
		// normal, non-safe mode settings
		_WriteSettings();
	}

	return true;
}


#else	// !__HAIKU__
//	#pragma mark - R5 DeviceMap API

static const uint32 kStartPolling = 'strp';

static BMessage gSettingsMessage;
static bool gSilentAutoMounter;

struct OneMountFloppyParams {
	status_t result;
};

struct MountPartitionParams {
	int32 uniqueID;
	status_t result;
};


#if xDEBUG
static Partition *
DumpPartition(Partition *partition, void*)
{
	partition->Dump();
	return 0;
}
#endif


/** Sets the Tracker Shell's AutoMounter to monitor a node.
 *  n.b. Get's the one AutoMounter and uses Tracker's _special_ WatchNode.
 *
 *  @param nodeToWatch (node_ref const * const) The Node to monitor.
 *  @param flags (uint32) watch_node flags from NodeMonitor.
 *  @return (status_t) watch_node status or B_BAD_TYPE if not a TTracker app.
 */

static status_t
AutoMounterWatchNode(const node_ref *nodeRef, uint32 flags)
{
	ASSERT(nodeRef != NULL);

	TTracker *tracker = dynamic_cast<TTracker *>(be_app);
	if (tracker != NULL)
		return TTracker::WatchNode(nodeRef, flags, BMessenger(0, tracker->AutoMounterLoop()));

	return B_BAD_TYPE;
}


/** Tries to mount the partition and if it can it watches mount point.
 *
 *  @param partition (Partition * const) The partition to mount.
 */

static status_t
MountAndWatch(Partition *partition)
{
	ASSERT(partition != NULL);

	status_t status = partition->Mount();
	if (status != B_OK)
		return status;

	// Start watching this mount point
	node_ref nodeToWatch;
	status = partition->GetMountPointNodeRef(&nodeToWatch);
	if (status != B_OK) {
		PRINT(("Couldn't get mount point node ref: %s\n", strerror(status)));
		return status;
	}

	return AutoMounterWatchNode(&nodeToWatch, B_WATCH_NAME);
}


static Partition *
TryMountingEveryOne(Partition *partition, void *castToParams)
{
	MountPartitionParams *params = (MountPartitionParams *)castToParams;

	if (partition->Mounted() == kMounted) {
		if (!gSilentAutoMounter)
			PRINT(("%s already mounted\n", partition->VolumeName()));
	} else {
		status_t result = MountAndWatch(partition);
		// return error if caller asked for it
		if (params)
			params->result = result;

		if (!gSilentAutoMounter) {
			if (result == B_OK)
				PRINT(("%s mounted OK\n", partition->VolumeName()));
			else
				PRINT(("Error '%s' mounting %s\n",
					strerror(result), partition->VolumeName()));
		}

		if (params && result != B_OK)
			// signal an error
			return partition;
	}
	return NULL;
}


static Partition *
OneTryMountingFloppy(Partition *partition, void *castToParams)
{
	OneMountFloppyParams *params = (OneMountFloppyParams *)castToParams;
	if (partition->GetDevice()->IsFloppy()){

		status_t result = MountAndWatch(partition);
		// return error if caller asked for it
		if (params)
			params->result = result;

		return partition;
	}
	return 0;
}


static Partition *
OneMatchFloppy(Partition *partition, void *)
{
	if (partition->GetDevice()->IsFloppy())
		return partition;
	
	return 0;
}


static Partition *
TryMountingBFSOne(Partition *partition, void *params)
{	
	if (strcmp(partition->FileSystemShortName(), "bfs") == 0) 
		return TryMountingEveryOne(partition, params);

	return NULL;
}


static Partition *
TryMountingRestoreOne(Partition *partition, void *params)
{
	Session *session = partition->GetSession();
	Device *device = session->GetDevice();

	// create the name for the virtual device
	char path[B_FILE_NAME_LENGTH];
	int len = (int)strlen(device->Name()) - (int)strlen("/raw");
	if (session->CountPartitions() != 1)
		sprintf(path, "%.*s/%ld_%ld", len, device->Name(), session->Index(), partition->Index());
	else
		sprintf(path, "%s", device->Name());

	// Find the name of the current device/volume in the saved settings
	// and open if found.
	const char *volumename;
	if (gSettingsMessage.FindString(path, &volumename) == B_OK
		&& strcmp(volumename, partition->VolumeName()) == 0)
		return TryMountingEveryOne(partition, params);

	return NULL;
}


static Partition *
TryMountingHFSOne(Partition *partition, void *params)
{	
	if (strcmp(partition->FileSystemShortName(), "hfs") == 0)
		return TryMountingEveryOne(partition, params);
	
	return NULL;
}


struct FindPartitionByDeviceIDParams {
	dev_t dev;
};

static Partition *
FindPartitionByDeviceID(Partition *partition, void *castToParams)
{
	FindPartitionByDeviceIDParams *params = (FindPartitionByDeviceIDParams*) castToParams;
	if (params->dev == partition->VolumeDeviceID())
		return partition;

	return 0;	
}


static Partition *
TryWatchMountPoint(Partition *partition, void *)
{
	node_ref nodeRef;
	if (partition->GetMountPointNodeRef(&nodeRef) == B_OK)
		AutoMounterWatchNode(&nodeRef, B_WATCH_NAME);

	return 0;
}


static Partition *
TryMountVolumeByID(Partition *partition, void *params)
{
	PRINT(("Try mounting partition %i\n", partition->UniqueID()));
	if (!partition->Hidden() && partition->UniqueID() == 
		((MountPartitionParams *)params)->uniqueID) {
		Partition *result = TryMountingEveryOne(partition, params);
		if (result)
			return result;

		return partition;
	}
	return NULL;
}


static Partition *
AutomountOne(Partition *partition, void *castToParams)
{
	PRINT(("Partition %s not mounted\n", partition->Name()));
	AutomountParams *params = (AutomountParams *)castToParams;

	if (params->mountRemovableDisksOnly
		&& (!partition->GetDevice()->NoMedia() 
			&& !partition->GetDevice()->Removable())) 
		// not removable, don't mount it
		return NULL;

	if (params->mountAllFS)
		return TryMountingEveryOne(partition, NULL);
	if (params->mountBFS)
		return TryMountingBFSOne(partition, NULL);
	if (params->mountHFS)
		return TryMountingHFSOne(partition, NULL);
	
	return NULL;
}


struct UnmountDeviceParams {
	dev_t device;
	status_t result;
};


static Partition *
UnmountIfMatchingID(Partition *partition, void *castToParams)
{
	UnmountDeviceParams *params = (UnmountDeviceParams *)castToParams;

	if (partition->VolumeDeviceID() == params->device) {
		TTracker *tracker = dynamic_cast<TTracker *>(be_app);
		if (tracker && tracker->QueryActiveForDevice(params->device)) {
			BString text;
			text << "To unmount " << partition->VolumeName() << " some query "
			"windows have to be closed. Would you like to close the query "
			"windows?";
			if ((new BAlert("", text.String(), "Cancel", "Close and unmount", NULL,
				B_WIDTH_FROM_LABEL))->Go() == 0)
				return partition;
			tracker->CloseActiveQueryWindows(params->device);
		}

		params->result = partition->Unmount();
		Device *device = partition->GetDevice();
		bool deviceHasMountedPartitions = false;

		if (params->result == B_OK && device->Removable()) {
			for (int32 sessionIndex = 0; ; sessionIndex++) {
				Session *session = device->SessionAt(sessionIndex);
				if (!session)
					break;

				for (int32 partitionIndex = 0; ; partitionIndex++) {
					Partition *partition = session->PartitionAt(partitionIndex);
					if (!partition)
						break;

					if (partition->Mounted() == kMounted) {
						deviceHasMountedPartitions = true;
						break;
					}
				}
			}

			if (!deviceHasMountedPartitions
				&& TrackerSettings().EjectWhenUnmounting())
				params->result = partition->GetDevice()->Eject();
		}

		return partition;
	}

	return NULL;
}


static Partition *
NotifyFloppyNotMountable(Partition *partition, void *)
{
	if (partition->Mounted() != kMounted
		&& partition->GetDevice()->IsFloppy()
		&& !partition->Hidden()) {
		(new BAlert("", "The format of the floppy disk in the disk drive is "
			"not recognized or the disk has never been formatted.", "OK"))->Go(0);
		partition->GetDevice()->Eject();
	}
	return NULL;
}


static Device *
FindFloppyDevice(Device *device, void *)
{
	if (device->IsFloppy())
		return device;

	return 0;
}


#ifdef MOUNT_MENU_IN_DESKBAR

// just for testing

Partition *
AddMountableItemToMessage(Partition *partition, void *castToParams)
{
	BMessage *message = static_cast<BMessage *>(castToParams);

	message->AddString("DeviceName", partition->GetDevice()->Name());
	const char *name;
	if (*partition->VolumeName())
		name = partition->VolumeName();
	else if (*partition->Type())
		name = partition->Type();
	else
		name = partition->GetDevice()->DisplayName();

	message->AddString("DisplayName", name);
	BMessage invokeMsg;
	if (partition->GetDevice()->IsFloppy())
		invokeMsg.what = kTryMountingFloppy;
	else
		invokeMsg.what = kMountVolume;
	invokeMsg.AddInt32("id", partition->UniqueID());
	message->AddMessage("InvokeMessage", &invokeMsg);
	return NULL;	
}

#endif // #ifdef MOUNT_MENU_IN_DESKBAR


AutoMounter::AutoMounter(bool checkRemovableOnly, bool checkCDs,
		bool checkFloppies, bool checkOtherRemovable, bool autoMountRemovablesOnly,
		bool autoMountAll, bool autoMountAllBFS, bool autoMountAllHFS,
		bool initialMountAll, bool initialMountAllBFS, bool initialMountRestore,
		bool initialMountAllHFS)
	: BLooper("AutoMounter", B_LOW_PRIORITY),
	fInitialMountAll(initialMountAll),
	fInitialMountAllBFS(initialMountAllBFS),
	fInitialMountRestore(initialMountRestore),
	fInitialMountAllHFS(initialMountAllHFS),
	fSuspended(false),
	fQuitting(false)
{
	fScanParams.shortestRescanHartbeat = 5000000;
	fScanParams.checkFloppies = checkFloppies;
	fScanParams.checkCDROMs = checkCDs;
	fScanParams.checkOtherRemovable = checkOtherRemovable;
	fScanParams.removableOrUnknownOnly = checkRemovableOnly;

	fAutomountParams.mountAllFS = autoMountAll;
	fAutomountParams.mountBFS = autoMountAllBFS;
	fAutomountParams.mountHFS = autoMountAllHFS;
	fAutomountParams.mountRemovableDisksOnly = autoMountRemovablesOnly;

	gSilentAutoMounter = true;

	if (!BootedInSafeMode()) {
		ReadSettings();
		thread_id rescan = spawn_thread(AutoMounter::InitialRescanBinder, 
			"AutomountInitialScan", B_DISPLAY_PRIORITY, this);
		resume_thread(rescan);
	} else {
		// defeat automounter in safe mode, don't even care about the settings
		fAutomountParams.mountAllFS = false;
		fAutomountParams.mountBFS = false;
		fAutomountParams.mountHFS = false;
		fInitialMountAll = false;
		fInitialMountAllBFS = false;
		fInitialMountRestore = false;
		fInitialMountAllHFS = false;
	}

	//	Watch mount/unmount
	TTracker::WatchNode(0, B_WATCH_MOUNT, this);
}


AutoMounter::~AutoMounter()
{
}


Partition* AutoMounter::FindPartition(dev_t dev)
{
	FindPartitionByDeviceIDParams params;
	params.dev = dev;
	return fList.EachMountedPartition(FindPartitionByDeviceID, &params);
}


void 
AutoMounter::RescanDevices()
{
	stop_watching(this);
	fList.RescanDevices(true);
	fList.UpdateMountingInfo();
	fList.EachMountedPartition(TryWatchMountPoint, 0);
	TTracker::WatchNode(0, B_WATCH_MOUNT, this);
	fList.EachMountedPartition(TryWatchMountPoint, 0);
}


void
AutoMounter::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kAutomounterRescan:
			RescanDevices();
			break;

		case kStartPolling:	
			// PRINT(("starting the automounter\n"));

			fScanThread = spawn_thread(AutoMounter::WatchVolumeBinder, 
				"AutomountScan", B_LOW_PRIORITY, this);
			resume_thread(fScanThread);
			break;

		case kMountVolume:
			MountVolume(message);
			break;

		case kUnmountVolume:
			UnmountAndEjectVolume(message);
			break;

		case kSetAutomounterParams:
		{
			bool rescanNow = false;
			message->FindBool("rescanNow", &rescanNow);
			SetParams(message, rescanNow);
			WriteSettings();
			break;
		}

		case kMountAllNow:
			RescanDevices();
			MountAllNow();
			break;

		case kSuspendAutomounter:
			SuspendResume(true);
			break;

		case kResumeAutomounter:
			SuspendResume(false);
			break;

		case kTryMountingFloppy:
			TryMountingFloppy();
			break;

		case B_NODE_MONITOR:
		{
			int32 opcode;
			if (message->FindInt32("opcode", &opcode) != B_OK)
				break;

			switch (opcode) {
				case B_DEVICE_MOUNTED: {
					WRITELOG(("** Received Device Mounted Notification"));
					dev_t device;
					if (message->FindInt32("new device", &device) == B_OK) {
						Partition *partition =  FindPartition(device);
						if (partition == NULL || partition->Mounted() != kMounted) {
							WRITELOG(("Device %i not in device list.  Someone mounted it outside "
								"of Tracker", device));

							//
							// This is the worst case.  Someone has mounted
							// something from outside of tracker.  
							// Unfortunately, there's no easy way to tell which
							// partition was just mounted (or if we even know about the device),
							// so stop watching all nodes, rescan to see what is now mounted,
							// and start watching again.
							//
							RescanDevices();
						} else
							WRITELOG(("Found partition\n"));
					} else {
						WRITELOG(("ERROR: Could not find mounted device ID in message"));
						PRINT_OBJECT(*message);
					}

					break;
				}

				case B_DEVICE_UNMOUNTED: {
					WRITELOG(("*** Received Device Unmounted Notification"));
					dev_t device;
					if (message->FindInt32("device", &device) == B_OK) {
						Partition *partition = FindPartition(device);

						if (partition != 0) {
							WRITELOG(("Found device in device list. Updating state to unmounted."));
							partition->SetMountState(kNotMounted);
						} else
							WRITELOG(("Unmounted device %i was not in device list", device));
					} else {
						WRITELOG(("ERROR: Could not find unmounted device ID in message"));
						PRINT_OBJECT(*message);
					}

					break;
				}	

				//	The name of a mount point has changed
				case B_ENTRY_MOVED: {
					WRITELOG(("*** Received Mount Point Renamed Notification"));

					const char *newName;
					if (message->FindString("name", &newName) != B_OK) {
						WRITELOG(("ERROR: Couldn't find name field in update message"));
						PRINT_OBJECT(*message);
						break ;
					}

					//
					// When the node monitor reports a move, it gives the
					// parent device and inode that moved.  The problem is 
					// that  the inode is the inode of root *in* the filesystem,
					// which is generally always the same number for every 
					// filesystem of a type.
					//
					// What we'd really like is the device that the moved	
					// volume is mounted on.  Find this by using the 
					// *new* name and directory, and then stat()ing that to
					// find the device.
					//
					dev_t parentDevice;
					if (message->FindInt32("device", &parentDevice) != B_OK) {
						WRITELOG(("ERROR: Couldn't find 'device' field in update"
							" message"));
						PRINT_OBJECT(*message);
						break;
					}

					ino_t toDirectory;	
					if (message->FindInt64("to directory", &toDirectory)!=B_OK){
						WRITELOG(("ERROR: Couldn't find 'to directory' field in update"
						  "message"));
						PRINT_OBJECT(*message);
						break;
					}

					entry_ref root_entry(parentDevice, toDirectory, newName);

					BNode entryNode(&root_entry);
					if (entryNode.InitCheck() != B_OK) {
						WRITELOG(("ERROR: Couldn't create mount point entry node: %s/n", 
							strerror(entryNode.InitCheck())));
						break;
					}	

					node_ref mountPointNode;
					if (entryNode.GetNodeRef(&mountPointNode) != B_OK) {
						WRITELOG(("ERROR: Couldn't get node ref for new mount point"));
						break;
					}

					WRITELOG(("Attempt to rename device %li to %s", mountPointNode.device,
						newName));

					Partition *partition = FindPartition(mountPointNode.device);
					if (partition != NULL) {
						WRITELOG(("Found device, changing name."));

						BVolume mountVolume(partition->VolumeDeviceID());
						BDirectory mountDir;
						mountVolume.GetRootDirectory(&mountDir);
						BPath dirPath(&mountDir, 0);

						partition->SetMountedAt(dirPath.Path());
						partition->SetVolumeName(newName);					
						break;
					} else {
						WRITELOG(("ERROR: Device %li does not appear to be present",
							mountPointNode.device));
					}
				}
			}
			break;
		}

		default:
			BLooper::MessageReceived(message);
			break;
	}
}


status_t
AutoMounter::WatchVolumeBinder(void *castToThis)
{
	static_cast<AutoMounter *>(castToThis)->WatchVolumes();
	return B_OK;
}


void
AutoMounter::WatchVolumes()
{
	for(;;) {
		snooze(fScanParams.shortestRescanHartbeat);

		AutoLock<BLooper> lock(this);
		if (!lock)
			break;

		if (fQuitting) {
			lock.Unlock();
			break;
		}

		if (!fSuspended && fList.CheckDevicesChanged(&fScanParams)) {
			fList.UnmountDisappearedPartitions();
			fList.UpdateChangedDevices(&fScanParams);
			fList.EachMountablePartition(AutomountOne, &fAutomountParams);
		}
	}
}


void
AutoMounter::EachMountableItemAndFloppy(EachPartitionFunction func, void *passThru)
{
	AutoLock<BLooper> lock(this);

#if 0
	//
	//	Rescan now to see if anything has changed.  
	//
	if (fList.CheckDevicesChanged(&fScanParams)) {
		fList.UpdateChangedDevices(&fScanParams);
		fList.UnmountDisappearedPartitions();
	}
#endif

	//
	//	If the floppy has never been mounted, it won't have a partition
	// 	in the device list (but it will have a device entry).  If it has, the 
	//	partition will appear, but will be set not mounted.  Code here works 
	//	around this.
	//
	if (!IsFloppyMounted() && !FloppyInList()) {
		Device *floppyDevice = fList.EachDevice(FindFloppyDevice, 0);

		//
		//	See comments under 'EachPartition'
		//
		if (floppyDevice != 0) {
			Session session(floppyDevice, "floppy", 0, 0, 0);
			Partition partition(&session, "floppy", "unknown",
				"unknown", "unknown", "floppy", "", 0, 0, 0, false);

			(func)(&partition, passThru);
		}
	}

	fList.EachMountablePartition(func, passThru);
}


void
AutoMounter::EachMountedItem(EachPartitionFunction func, void *passThru)
{
	AutoLock<BLooper> lock(this);
	fList.EachMountedPartition(func, passThru);
}


Partition *
AutoMounter::EachPartition(EachPartitionFunction func, void *passThru)
{
	AutoLock<BLooper> lock(this);

	if (!IsFloppyMounted() && !FloppyInList()) {
		Device *floppyDevice = fList.EachDevice(FindFloppyDevice, 0);

		//
		//	Add a floppy to the list.  It normally doesn't appear
		//	there when a floppy hasn't been mounted because it
		//	doesn't have any partitions.  Note that this makes sure
		//	that a floppy device exists before adding it, because
		//	some systems don't have floppy drives (unbelievable, but
		//	true).
		//
		if (floppyDevice != 0) {
			Session session(floppyDevice, "floppy", 0, 0, 0);
			Partition partition(&session, "floppy", "unknown",
				"unknown", "unknown", "floppy", "", 0, 0, 0, false);
	
			Partition *result = func(&partition, passThru);
			if (result != NULL)
				return result;
		}
	}
	
	return fList.EachPartition(func, passThru);
}


void
AutoMounter::CheckVolumesNow()
{
	AutoLock<BLooper> lock(this);
	if (fList.CheckDevicesChanged(&fScanParams)) {
		fList.UnmountDisappearedPartitions();
		fList.UpdateChangedDevices(&fScanParams);
		if (!fSuspended)
			fList.EachMountablePartition(AutomountOne, &fAutomountParams);
	}
}


void 
AutoMounter::SuspendResume(bool suspend)
{
	AutoLock<BLooper> lock(this);
	fSuspended = suspend;
	if (fSuspended)
		suspend_thread(fScanThread);
	else
		resume_thread(fScanThread);
}


void
AutoMounter::MountAllNow()
{
	AutoLock<BLooper> lock(this);

	DeviceScanParams mountAllParams;
	mountAllParams.checkFloppies = true;
	mountAllParams.checkCDROMs = true;
	mountAllParams.checkOtherRemovable = true;
	mountAllParams.removableOrUnknownOnly = true;

	fList.UnmountDisappearedPartitions();
	fList.UpdateChangedDevices(&mountAllParams);
	fList.EachMountablePartition(TryMountingEveryOne, 0);
	fList.EachPartition(NotifyFloppyNotMountable, 0);
}


void
AutoMounter::TryMountingFloppy()
{
	AutoLock<BLooper> lock(this);

	DeviceScanParams mountAllParams;
	mountAllParams.checkFloppies = true;
	mountAllParams.checkCDROMs = false;
	mountAllParams.checkOtherRemovable = false;
	mountAllParams.removableOrUnknownOnly = false;

	fList.UnmountDisappearedPartitions();
	fList.UpdateChangedDevices(&mountAllParams);
	OneMountFloppyParams params;
	params.result = B_ERROR;
	fList.EachMountablePartition(OneTryMountingFloppy, &params);
	if (params.result != B_OK)
		(new BAlert("", "The format of the floppy disk in the disk drive is "
			"not recognized or the disk has never been formatted.", "OK"))
			->Go(0);
}


bool
AutoMounter::IsFloppyMounted()
{
	AutoLock<BLooper> lock(this);
	return fList.EachMountedPartition(OneMatchFloppy, 0) != NULL;
}


bool
AutoMounter::FloppyInList()
{
	AutoLock<BLooper> lock(this);
	return fList.EachPartition(OneMatchFloppy, 0) != NULL;
}


void
AutoMounter::MountVolume(BMessage *message)
{
	int32 uniqueID;
	if (message->FindInt32("id", &uniqueID) == B_OK) {
		if (uniqueID == kFloppyID) {
			TryMountingFloppy();
			return;
		}

		MountPartitionParams params;
		params.uniqueID = uniqueID;
		params.result = B_OK;

		if (EachPartition(TryMountVolumeByID, &params) == NULL) {
			(new BAlert("", "The format of this volume is unrecognized, or it has "
				"never been formatted", "OK"))->Go(0);
		} else if (params.result != B_OK)	{
			BString string;
			string << "Error mounting volume. (" << strerror(params.result) << ")";
			(new BAlert("", string.String(), "OK"))->Go(0);
		}
	}
}


status_t
AutoMounter::InitialRescanBinder(void *castToThis)
{
	// maybe this can help the strange Tracker lock-up at startup
	snooze(500000LL);	// wait half a second

	AutoMounter *self = static_cast<AutoMounter *>(castToThis);
	self->InitialRescan();

	// Start watching nodes that were mounted before tracker started
	(self->fList).EachMountedPartition(TryWatchMountPoint, 0);

	self->PostMessage(kStartPolling, 0);
	return B_OK;
}


void
AutoMounter::InitialRescan()
{
	AutoLock<BLooper> lock(this);

	fList.RescanDevices(false);
	fList.UpdateMountingInfo();

	// if called after spawn_thread, must lock fList
	if (fInitialMountAll) {
//+		PRINT(("mounting all volumes\n"));
		fList.EachMountablePartition(TryMountingEveryOne, NULL);
	}
	
	if (fInitialMountAllHFS) {
//+		PRINT(("mounting all hfs volumes\n"));
		fList.EachMountablePartition(TryMountingHFSOne, NULL);
	}
	
	if (fInitialMountAllBFS) {
//+		PRINT(("mounting all bfs volumes\n"));
		fList.EachMountablePartition(TryMountingBFSOne, NULL);
	}

	if (fInitialMountRestore) {
//+		PRINT(("restoring all volumes\n"));
		fList.EachMountablePartition(TryMountingRestoreOne, NULL);
	}
}


void
AutoMounter::UnmountAndEjectVolume(BMessage *message)
{
	dev_t device;
	if (message->FindInt32("device_id", &device) != B_OK)
		return;

	PRINT(("Unmount device %i\n", device));

	AutoLock<BLooper> lock(this);

	UnmountDeviceParams params;
	params.device = device;
	params.result = B_OK;
	Partition *partition = fList.EachMountedPartition(UnmountIfMatchingID, 
		&params);

	if (!partition) {
		PRINT(("Couldn't unmount partition.  Rescan and try again\n"));

		// could not find partition - must have been mounted by someone
		// else
		// sync up and try again
		// this should really be handled by watching for mount and unmount
		// events like the tracker does, not doing that because it is
		// a bigger change and we are close to freezing
		fList.UnmountDisappearedPartitions();
		
		DeviceScanParams syncRescanParams;
		syncRescanParams.checkFloppies = true;
		syncRescanParams.checkCDROMs = true;
		syncRescanParams.checkOtherRemovable = true;
		syncRescanParams.removableOrUnknownOnly = true;
		
		fList.UpdateChangedDevices(&syncRescanParams);
		partition = fList.EachMountedPartition(UnmountIfMatchingID, &params);	
	}
	
	if (!partition) {
		PRINT(("Device not in list, unmounting directly\n"));

		char path[B_FILE_NAME_LENGTH];

		BVolume vol(device);
		status_t err = vol.InitCheck();
		if (err == B_OK) {
			BDirectory mountPoint;		
			if (err == B_OK)
				err = vol.GetRootDirectory(&mountPoint);		
	
			BPath mountPointPath;
			if (err == B_OK)
				err = mountPointPath.SetTo(&mountPoint, ".");
	
			if (err == B_OK)
				strcpy(path, mountPointPath.Path());	
		}

		if (err == B_OK) {
			PRINT(("unmounting '%s'\n", path));
			err = unmount(path);
		}
		
		if (err == B_OK) {
			PRINT(("deleting '%s'\n", path));
			err = rmdir(path);
		}

		if (err != B_OK) {
		
			PRINT(("error %s\n", strerror(err)));
			BString text;
			text << "Could not unmount disk";
			(new BAlert("", text.String(), "OK", NULL, NULL, B_WIDTH_AS_USUAL, 
				B_WARNING_ALERT))->Go(0);
		}
		
	} else if (params.result != B_OK) {
		BString text;
		text << "Could not unmount disk  " << partition->VolumeName() <<
			". An item on the disk is busy.";
		(new BAlert("", text.String(), "OK", NULL, NULL, B_WIDTH_AS_USUAL, 
			B_WARNING_ALERT))->Go(0);
	}
}


bool
AutoMounter::QuitRequested()
{
	if (!BootedInSafeMode()) {
		// don't write out settings in safe mode - this would overwrite the
		// normal, non-safe mode settings
		WriteSettings();
	}

	return true;
}


void
AutoMounter::ReadSettings()
{
	BPath directoryPath;
	if (FSFindTrackerSettingsDir(&directoryPath) != B_OK)
		return;

	BPath path(directoryPath);
	path.Append(kAutoMounterSettings);
	fPrefsFile.SetTo(path.Path(), O_RDWR);

	if (fPrefsFile.InitCheck() != B_OK) {
		// no prefs file yet, create a new one

		BDirectory dir(directoryPath.Path());
		dir.CreateFile(kAutoMounterSettings, &fPrefsFile);
		return;
	}

	ssize_t settingsSize = (ssize_t)fPrefsFile.Seek(0, SEEK_END);
	if (settingsSize == 0)
		return;

	ASSERT(settingsSize != 0);
	char *buffer = new char[settingsSize];

	fPrefsFile.Seek(0, 0);
	if (fPrefsFile.Read(buffer, (size_t)settingsSize) != settingsSize) {
		PRINT(("error reading automounter settings\n"));
		delete [] buffer;
		return;
	}

	BMessage message('stng');
	status_t result = message.Unflatten(buffer);
	if (result != B_OK) {
		PRINT(("error %s unflattening settings, size %d\n", strerror(result), 
			settingsSize));
		delete [] buffer;
		return;
	}

	delete [] buffer;
//	PRINT(("done unflattening settings\n"));
	SetParams(&message, true);
}


void
AutoMounter::WriteSettings()
{
	if (fPrefsFile.InitCheck() != B_OK)
		return;

	BMessage message('stng');
	GetSettings(&message);

	ssize_t settingsSize = message.FlattenedSize();

	char *buffer = new char[settingsSize];
	status_t result = message.Flatten(buffer, settingsSize);

	fPrefsFile.Seek(0, 0);
	result = fPrefsFile.Write(buffer, (size_t)settingsSize);
	if (result != settingsSize)
		PRINT(("error writing settings, %s\n", strerror(result)));

	delete [] buffer;
}


void
AutoMounter::GetSettings(BMessage *message)
{
	message->AddBool("checkRemovableOnly", fScanParams.removableOrUnknownOnly);
	message->AddBool("checkCDs", fScanParams.checkCDROMs);
	message->AddBool("checkFloppies", fScanParams.checkFloppies);
	message->AddBool("checkOtherRemovables", fScanParams.checkOtherRemovable);
	message->AddBool("autoMountRemovableOnly", fAutomountParams.mountRemovableDisksOnly);
	message->AddBool("autoMountAll", fAutomountParams.mountAllFS);
	message->AddBool("autoMountAllBFS", fAutomountParams.mountBFS);
	message->AddBool("autoMountAllHFS", fAutomountParams.mountHFS);
	message->AddBool("initialMountAll", fInitialMountAll);
	message->AddBool("initialMountAllBFS", fInitialMountAllBFS);
	message->AddBool("initialMountRestore", fInitialMountRestore);
	message->AddBool("initialMountAllHFS", fInitialMountAllHFS);
	message->AddBool("suspended", fSuspended);

	// Save mounted volumes so we can optionally mount them on next
	// startup
	BVolumeRoster volumeRoster;
	BVolume volume;
	while (volumeRoster.GetNextVolume(&volume) == B_OK) {
        fs_info info;
        if (fs_stat_dev(volume.Device(), &info) == 0
			&& info.flags & (B_FS_IS_REMOVABLE | B_FS_IS_PERSISTENT))
			message->AddString(info.device_name, info.volume_name);
	}
}


void
AutoMounter::SetParams(BMessage *message, bool rescan)
{
	bool result;
	if (message->FindBool("checkRemovableOnly", &result) == B_OK)
		fScanParams.removableOrUnknownOnly = result;
	if (message->FindBool("checkCDs", &result) == B_OK)
		fScanParams.checkCDROMs = result;
	if (message->FindBool("checkFloppies", &result) == B_OK)
		fScanParams.checkFloppies = result;
	if (message->FindBool("checkOtherRemovables", &result) == B_OK)
		fScanParams.checkOtherRemovable = result;
	if (message->FindBool("autoMountRemovableOnly", &result) == B_OK)
		fAutomountParams.mountRemovableDisksOnly = result;
	if (message->FindBool("autoMountAll", &result) == B_OK)
		fAutomountParams.mountAllFS = result;
	if (message->FindBool("autoMountAllBFS", &result) == B_OK)
		fAutomountParams.mountBFS = result;
	if (message->FindBool("autoMountAllHFS", &result) == B_OK)
		fAutomountParams.mountHFS = result;
	if (message->FindBool("initialMountAll", &result) == B_OK)
		fInitialMountAll = result;
	if (message->FindBool("initialMountAllBFS", &result) == B_OK)
		fInitialMountAllBFS = result;
	if (message->FindBool("initialMountRestore", &result) == B_OK) {
		fInitialMountRestore = result;
		if (fInitialMountRestore)
			gSettingsMessage = *message;
	}
	if (message->FindBool("initialMountAllHFS", &result) == B_OK)
		fInitialMountAllHFS = result;

	if (message->FindBool("suspended", &result) == B_OK)
		SuspendResume(result);

	if (rescan)
		CheckVolumesNow();
}

#endif	// !__HAIKU__
