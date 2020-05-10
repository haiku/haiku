/*
 * Copyright 2007-2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "AutoMounter.h"

#include <new>

#include <string.h>
#include <unistd.h>

#include <Alert.h>
#include <AutoLocker.h>
#include <Catalog.h>
#include <Debug.h>
#include <Directory.h>
#include <DiskDevice.h>
#include <DiskDeviceRoster.h>
#include <DiskDeviceList.h>
#include <DiskDeviceTypes.h>
#include <DiskSystem.h>
#include <FindDirectory.h>
#include <fs_info.h>
#include <fs_volume.h>
#include <LaunchRoster.h>
#include <Locale.h>
#include <Message.h>
#include <Node.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PropertyInfo.h>
#include <String.h>
#include <VolumeRoster.h>

#include "MountServer.h"

#include "Utilities.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AutoMounter"


static const char* kMountServerSettings = "mount_server";
static const char* kMountFlagsKeyExtension = " mount flags";

static const char* kInitialMountEvent = "initial_volumes_mounted";


class MountVisitor : public BDiskDeviceVisitor {
public:
								MountVisitor(mount_mode normalMode,
									mount_mode removableMode,
									bool initialRescan, BMessage& previous,
									partition_id deviceID);
	virtual						~MountVisitor()
									{}

	virtual	bool				Visit(BDiskDevice* device);
	virtual	bool				Visit(BPartition* partition, int32 level);

private:
			bool				_WasPreviouslyMounted(const BPath& path,
									const BPartition* partition);

private:
			mount_mode			fNormalMode;
			mount_mode			fRemovableMode;
			bool				fInitialRescan;
			BMessage&			fPrevious;
			partition_id		fOnlyOnDeviceID;
};


class MountArchivedVisitor : public BDiskDeviceVisitor {
public:
								MountArchivedVisitor(
									const BDiskDeviceList& devices,
									const BMessage& archived);
	virtual						~MountArchivedVisitor();

	virtual	bool				Visit(BDiskDevice* device);
	virtual	bool				Visit(BPartition* partition, int32 level);

private:
			int					_Score(BPartition* partition);

private:
			const BDiskDeviceList& fDevices;
			const BMessage&		fArchived;
			int					fBestScore;
			partition_id		fBestID;
};


static bool
BootedInSafeMode()
{
	const char* safeMode = getenv("SAFEMODE");
	return safeMode != NULL && strcmp(safeMode, "yes") == 0;
}


class ArchiveVisitor : public BDiskDeviceVisitor {
public:
								ArchiveVisitor(BMessage& message);
	virtual						~ArchiveVisitor();

	virtual	bool				Visit(BDiskDevice* device);
	virtual	bool				Visit(BPartition* partition, int32 level);

private:
			BMessage&			fMessage;
};


// #pragma mark - MountVisitor


MountVisitor::MountVisitor(mount_mode normalMode, mount_mode removableMode,
		bool initialRescan, BMessage& previous, partition_id deviceID)
	:
	fNormalMode(normalMode),
	fRemovableMode(removableMode),
	fInitialRescan(initialRescan),
	fPrevious(previous),
	fOnlyOnDeviceID(deviceID)
{
}


bool
MountVisitor::Visit(BDiskDevice* device)
{
	return Visit(device, 0);
}


bool
MountVisitor::Visit(BPartition* partition, int32 level)
{
	if (fOnlyOnDeviceID >= 0) {
		// only mount partitions on the given device id
		// or if the partition ID is already matched
		BPartition* device = partition;
		while (device->Parent() != NULL) {
			if (device->ID() == fOnlyOnDeviceID) {
				// we are happy
				break;
			}
			device = device->Parent();
		}
		if (device->ID() != fOnlyOnDeviceID)
			return false;
	}

	mount_mode mode = !fInitialRescan && partition->Device()->IsRemovableMedia()
		? fRemovableMode : fNormalMode;
	if (mode == kNoVolumes || partition->IsMounted()
		|| !partition->ContainsFileSystem()) {
		return false;
	}

	BPath path;
	if (partition->GetPath(&path) != B_OK)
		return false;

	if (mode == kRestorePreviousVolumes) {
		// mount all volumes that were stored in the settings file
		if (!_WasPreviouslyMounted(path, partition))
			return false;
	} else if (mode == kOnlyBFSVolumes) {
		if (partition->ContentType() == NULL
			|| strcmp(partition->ContentType(), kPartitionTypeBFS))
			return false;
	}

	uint32 mountFlags;
	if (!fInitialRescan) {
		// Ask the user about mount flags if this is not the
		// initial scan.
		if (!AutoMounter::_SuggestMountFlags(partition, &mountFlags))
			return false;
	} else {
		BString mountFlagsKey(path.Path());
		mountFlagsKey << kMountFlagsKeyExtension;
		if (fPrevious.FindInt32(mountFlagsKey.String(),
				(int32*)&mountFlags) < B_OK) {
			mountFlags = 0;
		}
	}

	if (partition->Mount(NULL, mountFlags) != B_OK) {
		// TODO: Error to syslog
	}
	return false;
}


bool
MountVisitor::_WasPreviouslyMounted(const BPath& path,
	const BPartition* partition)
{
	// We only check the legacy config data here; the current method
	// is implemented in ArchivedVolumeVisitor -- this can be removed
	// some day.
	const char* volumeName = NULL;
	if (partition->ContentName() == NULL
		|| fPrevious.FindString(path.Path(), &volumeName) != B_OK
		|| strcmp(volumeName, partition->ContentName()) != 0)
		return false;

	return true;
}


// #pragma mark - MountArchivedVisitor


MountArchivedVisitor::MountArchivedVisitor(const BDiskDeviceList& devices,
		const BMessage& archived)
	:
	fDevices(devices),
	fArchived(archived),
	fBestScore(-1),
	fBestID(-1)
{
}


MountArchivedVisitor::~MountArchivedVisitor()
{
	if (fBestScore >= 6) {
		uint32 mountFlags = fArchived.GetUInt32("mountFlags", 0);
		BPartition* partition = fDevices.PartitionWithID(fBestID);
		if (partition != NULL)
			partition->Mount(NULL, mountFlags);
	}
}


bool
MountArchivedVisitor::Visit(BDiskDevice* device)
{
	return Visit(device, 0);
}


bool
MountArchivedVisitor::Visit(BPartition* partition, int32 level)
{
	if (partition->IsMounted() || !partition->ContainsFileSystem())
		return false;

	int score = _Score(partition);
	if (score > fBestScore) {
		fBestScore = score;
		fBestID = partition->ID();
	}

	return false;
}


int
MountArchivedVisitor::_Score(BPartition* partition)
{
	BPath path;
	if (partition->GetPath(&path) != B_OK)
		return false;

	int score = 0;

	int64 capacity = fArchived.GetInt64("capacity", 0);
	if (capacity == partition->ContentSize())
		score += 4;

	BString deviceName = fArchived.GetString("deviceName");
	if (deviceName == path.Path())
		score += 3;

	BString volumeName = fArchived.GetString("volumeName");
	if (volumeName == partition->ContentName())
		score += 2;

	BString fsName = fArchived.FindString("fsName");
	if (fsName == partition->ContentType())
		score += 1;

	uint32 blockSize = fArchived.GetUInt32("blockSize", 0);
	if (blockSize == partition->BlockSize())
		score += 1;

	return score;
}


// #pragma mark - ArchiveVisitor


ArchiveVisitor::ArchiveVisitor(BMessage& message)
	:
	fMessage(message)
{
}


ArchiveVisitor::~ArchiveVisitor()
{
}


bool
ArchiveVisitor::Visit(BDiskDevice* device)
{
	return Visit(device, 0);
}


bool
ArchiveVisitor::Visit(BPartition* partition, int32 level)
{
	if (!partition->ContainsFileSystem())
		return false;

	BPath path;
	if (partition->GetPath(&path) != B_OK)
		return false;

	BMessage info;
	info.AddUInt32("blockSize", partition->BlockSize());
	info.AddInt64("capacity", partition->ContentSize());
	info.AddString("deviceName", path.Path());
	info.AddString("volumeName", partition->ContentName());
	info.AddString("fsName", partition->ContentType());

	fMessage.AddMessage("info", &info);
	return false;
}


// #pragma mark -


AutoMounter::AutoMounter()
	:
	BServer(kMountServerSignature, true, NULL),
	fNormalMode(kRestorePreviousVolumes),
	fRemovableMode(kAllVolumes),
	fEjectWhenUnmounting(true)
{
	set_thread_priority(Thread(), B_LOW_PRIORITY);

	if (!BootedInSafeMode()) {
		_ReadSettings();
	} else {
		// defeat automounter in safe mode, don't even care about the settings
		fNormalMode = kNoVolumes;
		fRemovableMode = kNoVolumes;
	}

	BDiskDeviceRoster().StartWatching(this,
		B_DEVICE_REQUEST_DEVICE | B_DEVICE_REQUEST_DEVICE_LIST);
	BLaunchRoster().RegisterEvent(this, kInitialMountEvent, B_STICKY_EVENT);
}


AutoMounter::~AutoMounter()
{
	BLaunchRoster().UnregisterEvent(this, kInitialMountEvent);
	BDiskDeviceRoster().StopWatching(this);
}


void
AutoMounter::ReadyToRun()
{
	// Do initial scan
	_MountVolumes(fNormalMode, fRemovableMode, true);
	BLaunchRoster().NotifyEvent(this, kInitialMountEvent);
}


void
AutoMounter::MessageReceived(BMessage* message)
{
	switch (message->what) {
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
			_GetSettings(&fSettings);
			_WriteSettings();

			if (rescanNow)
				_MountVolumes(fNormalMode, fRemovableMode);
			break;
		}

		case kGetAutomounterParams:
		{
			BMessage reply;
			_GetSettings(&reply);
			message->SendReply(&reply);
			break;
		}

		case kMountAllNow:
			_MountVolumes(kAllVolumes, kAllVolumes);
			break;

		case B_DEVICE_UPDATE:
			int32 event;
			if (message->FindInt32("event", &event) != B_OK
				|| (event != B_DEVICE_MEDIA_CHANGED
					&& event != B_DEVICE_ADDED))
				break;

			partition_id deviceID;
			if (message->FindInt32("id", &deviceID) != B_OK)
				break;

			_MountVolumes(kNoVolumes, fRemovableMode, false, deviceID);
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
						WRITELOG(("ERROR: Couldn't find name field in update "
							"message"));
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
						WRITELOG(("ERROR: Couldn't find 'device' field in "
							"update message"));
						PRINT_OBJECT(*message);
						break;
					}

					ino_t toDirectory;
					if (message->FindInt64("to directory", &toDirectory)
						!= B_OK) {
						WRITELOG(("ERROR: Couldn't find 'to directory' field "
							"in update message"));
						PRINT_OBJECT(*message);
						break;
					}

					entry_ref root_entry(parentDevice, toDirectory, newName);

					BNode entryNode(&root_entry);
					if (entryNode.InitCheck() != B_OK) {
						WRITELOG(("ERROR: Couldn't create mount point entry "
							"node: %s/n", strerror(entryNode.InitCheck())));
						break;
					}

					node_ref mountPointNode;
					if (entryNode.GetNodeRef(&mountPointNode) != B_OK) {
						WRITELOG(("ERROR: Couldn't get node ref for new mount "
							"point"));
						break;
					}

					WRITELOG(("Attempt to rename device %li to %s",
						mountPointNode.device, newName));

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
						WRITELOG(("ERROR: Device %li does not appear to be "
							"present", mountPointNode.device));
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
		// Don't write out settings in safe mode - this would overwrite the
		// normal, non-safe mode settings.
		_WriteSettings();
	}

	return true;
}


// #pragma mark - private methods


void
AutoMounter::_MountVolumes(mount_mode normal, mount_mode removable,
	bool initialRescan, partition_id deviceID)
{
	if (normal == kNoVolumes && removable == kNoVolumes)
		return;

	BDiskDeviceList devices;
	status_t status = devices.Fetch();
	if (status != B_OK)
		return;

	if (normal == kRestorePreviousVolumes) {
		BMessage archived;
		for (int32 index = 0;
				fSettings.FindMessage("info", index, &archived) == B_OK;
				index++) {
			MountArchivedVisitor visitor(devices, archived);
			devices.VisitEachPartition(&visitor);
		}
	}

	MountVisitor visitor(normal, removable, initialRescan, fSettings, deviceID);
	devices.VisitEachPartition(&visitor);
}


void
AutoMounter::_MountVolume(const BMessage* message)
{
	int32 id;
	if (message->FindInt32("id", &id) != B_OK)
		return;

	BDiskDeviceRoster roster;
	BPartition *partition;
	BDiskDevice device;
	if (roster.GetPartitionWithID(id, &device, &partition) != B_OK)
		return;

	uint32 mountFlags;
	if (!_SuggestMountFlags(partition, &mountFlags))
		return;

	status_t status = partition->Mount(NULL, mountFlags);
	if (status < B_OK) {
		char text[512];
		snprintf(text, sizeof(text),
			B_TRANSLATE("Error mounting volume:\n\n%s"), strerror(status));
		BAlert* alert = new BAlert(B_TRANSLATE("Mount error"), text,
			B_TRANSLATE("OK"));
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go(NULL);
	}
}


bool
AutoMounter::_SuggestForceUnmount(const char* name, status_t error)
{
	char text[1024];
	snprintf(text, sizeof(text),
		B_TRANSLATE("Could not unmount disk \"%s\":\n\t%s\n\n"
			"Should unmounting be forced?\n\n"
			"Note: If an application is currently writing to the volume, "
			"unmounting it now might result in loss of data.\n"),
		name, strerror(error));

	BAlert* alert = new BAlert(B_TRANSLATE("Force unmount"), text,
		B_TRANSLATE("Cancel"), B_TRANSLATE("Force unmount"), NULL,
		B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	alert->SetShortcut(0, B_ESCAPE);
	int32 choice = alert->Go();

	return choice == 1;
}


void
AutoMounter::_ReportUnmountError(const char* name, status_t error)
{
	char text[512];
	snprintf(text, sizeof(text), B_TRANSLATE("Could not unmount disk "
		"\"%s\":\n\t%s"), name, strerror(error));

	BAlert* alert = new BAlert(B_TRANSLATE("Unmount error"), text,
		B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go(NULL);
}


void
AutoMounter::_UnmountAndEjectVolume(BPartition* partition, BPath& mountPoint,
	const char* name)
{
	BDiskDevice deviceStorage;
	BDiskDevice* device;
	if (partition == NULL) {
		// Try to retrieve partition
		BDiskDeviceRoster().FindPartitionByMountPoint(mountPoint.Path(),
			&deviceStorage, &partition);
			device = &deviceStorage;
	} else {
		device = partition->Device();
	}

	status_t status;
	if (partition != NULL)
		status = partition->Unmount();
	else
		status = fs_unmount_volume(mountPoint.Path(), 0);

	if (status != B_OK) {
		if (!_SuggestForceUnmount(name, status))
			return;

		if (partition != NULL)
			status = partition->Unmount(B_FORCE_UNMOUNT);
		else
			status = fs_unmount_volume(mountPoint.Path(), B_FORCE_UNMOUNT);
	}

	if (status != B_OK) {
		_ReportUnmountError(name, status);
		return;
	}

	if (fEjectWhenUnmounting && partition != NULL) {
		// eject device if it doesn't have any mounted partitions left
		class IsMountedVisitor : public BDiskDeviceVisitor {
		public:
			IsMountedVisitor()
				:
				fHasMounted(false)
			{
			}

			virtual bool Visit(BDiskDevice* device)
			{
				return Visit(device, 0);
			}

			virtual bool Visit(BPartition* partition, int32 level)
			{
				if (partition->IsMounted()) {
					fHasMounted = true;
					return true;
				}

				return false;
			}

			bool HasMountedPartitions() const
			{
				return fHasMounted;
			}

		private:
			bool	fHasMounted;
		} visitor;

		device->VisitEachDescendant(&visitor);

		if (!visitor.HasMountedPartitions())
			device->Eject();
	}

	// remove the directory if it's a directory in rootfs
	if (dev_for_path(mountPoint.Path()) == dev_for_path("/"))
		rmdir(mountPoint.Path());
}


void
AutoMounter::_UnmountAndEjectVolume(BMessage* message)
{
	int32 id;
	if (message->FindInt32("id", &id) == B_OK) {
		BDiskDeviceRoster roster;
		BPartition *partition;
		BDiskDevice device;
		if (roster.GetPartitionWithID(id, &device, &partition) != B_OK)
			return;

		BPath path;
		if (partition->GetMountPoint(&path) == B_OK)
			_UnmountAndEjectVolume(partition, path, partition->ContentName());
	} else {
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
			snprintf(name, sizeof(name), "device:%" B_PRIdDEV, device);

		BPath path;
		if (status == B_OK) {
			BDirectory mountPoint;
			status = volume.GetRootDirectory(&mountPoint);
			if (status == B_OK)
				status = path.SetTo(&mountPoint, ".");
		}

		if (status == B_OK)
			_UnmountAndEjectVolume(NULL, path, name);
	}
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


mount_mode
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
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &directoryPath, true)
		!= B_OK) {
		return;
	}

	BPath path(directoryPath);
	path.Append(kMountServerSettings);
	fPrefsFile.SetTo(path.Path(), O_RDWR);

	if (fPrefsFile.InitCheck() != B_OK) {
		// no prefs file yet, create a new one

		BDirectory dir(directoryPath.Path());
		dir.CreateFile(kMountServerSettings, &fPrefsFile);
		return;
	}

	ssize_t settingsSize = (ssize_t)fPrefsFile.Seek(0, SEEK_END);
	if (settingsSize == 0)
		return;

	ASSERT(settingsSize != 0);
	char *buffer = new(std::nothrow) char[settingsSize];
	if (buffer == NULL) {
		PRINT(("error writing automounter settings, out of memory\n"));
		return;
	}

	fPrefsFile.Seek(0, 0);
	if (fPrefsFile.Read(buffer, (size_t)settingsSize) != settingsSize) {
		PRINT(("error reading automounter settings\n"));
		delete [] buffer;
		return;
	}

	BMessage message('stng');
	status_t result = message.Unflatten(buffer);
	if (result != B_OK) {
		PRINT(("error %s unflattening automounter settings, size %" B_PRIdSSIZE "\n",
			strerror(result), settingsSize));
		delete [] buffer;
		return;
	}

	delete [] buffer;

	// update flags and modes from the message
	_UpdateSettingsFromMessage(&message);
	// copy the previously mounted partitions
	fSettings = message;
}


void
AutoMounter::_WriteSettings()
{
	if (fPrefsFile.InitCheck() != B_OK)
		return;

	BMessage message('stng');
	_GetSettings(&message);

	ssize_t settingsSize = message.FlattenedSize();

	char* buffer = new(std::nothrow) char[settingsSize];
	if (buffer == NULL) {
		PRINT(("error writing automounter settings, out of memory\n"));
		return;
	}

	status_t result = message.Flatten(buffer, settingsSize);

	fPrefsFile.Seek(0, SEEK_SET);
	fPrefsFile.SetSize(0);

	result = fPrefsFile.Write(buffer, (size_t)settingsSize);
	if (result != settingsSize)
		PRINT(("error writing automounter settings, %s\n", strerror(result)));

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

	// eject settings
	bool eject;
	if (message->FindBool("ejectWhenUnmounting", &eject) == B_OK)
		fEjectWhenUnmounting = eject;
}


void
AutoMounter::_GetSettings(BMessage *message)
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

	message->AddBool("ejectWhenUnmounting", fEjectWhenUnmounting);

	// Save mounted volumes so we can optionally mount them on next
	// startup
	ArchiveVisitor visitor(*message);
	BDiskDeviceRoster().VisitEachMountedPartition(&visitor);
}


/*static*/ bool
AutoMounter::_SuggestMountFlags(const BPartition* partition, uint32* _flags)
{
	uint32 mountFlags = 0;

	bool askReadOnly = true;

	if (partition->ContentType() != NULL
		&& strcmp(partition->ContentType(), kPartitionTypeBFS) == 0) {
		askReadOnly = false;
	}

	BDiskSystem diskSystem;
	status_t status = partition->GetDiskSystem(&diskSystem);
	if (status == B_OK && !diskSystem.SupportsWriting())
		askReadOnly = false;

	if (partition->IsReadOnly())
		askReadOnly = false;

	if (askReadOnly) {
		// Suggest to the user to mount read-only until Haiku is more mature.
		BString string;
		if (partition->ContentName() != NULL) {
			char buffer[512];
			snprintf(buffer, sizeof(buffer),
				B_TRANSLATE("Mounting volume '%s'\n\n"),
				partition->ContentName());
			string << buffer;
		} else
			string << B_TRANSLATE("Mounting volume <unnamed volume>\n\n");

		// TODO: Use distro name instead of "Haiku"...
		string << B_TRANSLATE("The file system on this volume is not the "
			"Be file system. It is recommended to mount it in read-only "
			"mode, to prevent unintentional data loss because of bugs "
			"in Haiku.");

		BAlert* alert = new BAlert(B_TRANSLATE("Mount warning"),
			string.String(), B_TRANSLATE("Mount read/write"),
			B_TRANSLATE("Cancel"), B_TRANSLATE("Mount read-only"),
			B_WIDTH_FROM_WIDEST, B_WARNING_ALERT);
		alert->SetShortcut(1, B_ESCAPE);
		int32 choice = alert->Go();
		switch (choice) {
			case 0:
				break;
			case 1:
				return false;
			case 2:
				mountFlags |= B_MOUNT_READ_ONLY;
				break;
		}
	}

	*_flags = mountFlags;
	return true;
}


// #pragma mark -


int
main(int argc, char* argv[])
{
	AutoMounter app;

	app.Run();
	return 0;
}


