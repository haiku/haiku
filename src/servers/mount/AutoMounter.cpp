/*
 * Copyright 2007-2010, Haiku, Inc. All rights reserved.
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
#include <Locale.h>
#include <Message.h>
#include <Node.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PropertyInfo.h>
#include <String.h>
#include <VolumeRoster.h>

//#include "AutoMounterSettings.h"
#include "MountServer.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AutoMounter"


static const char* kMountServerSettings = "mount_server";
static const uint32 kMsgInitialScan = 'insc';
static const char* kMountFlagsKeyExtension = " mount flags";


bool
BootedInSafeMode()
{
	const char *safeMode = getenv("SAFEMODE");
	return (safeMode && strcmp(safeMode, "yes") == 0);
}


// #pragma mark -


AutoMounter::AutoMounter()
	:
	BApplication(kMountServerSignature),
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
}


AutoMounter::~AutoMounter()
{
	BDiskDeviceRoster().StopWatching(this);
}


void
AutoMounter::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_EXECUTE_PROPERTY:
		{
			int32 index;
			BMessage specifier;
			int32 what;
			const char* property = NULL;
			if (message->GetCurrentSpecifier(&index, &specifier, &what,
					&property) < B_OK
				|| !_ScriptReceived(message, index, &specifier, what,
					property)) {
				BApplication::MessageReceived(message);
			}
			break;
		}

		case kMsgInitialScan:
			_MountVolumes(fNormalMode, fRemovableMode, true);
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


// #pragma mark - scripting


const uint32 kApplication = 0;

static property_info sPropertyInfo[] = {
	{
		"InitialScan",
		{B_EXECUTE_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, kApplication,
		{B_STRING_TYPE},
		{},
		{}
	},
	{}
};


BHandler*
AutoMounter::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 what, const char* property)
{
	BPropertyInfo propInfo(sPropertyInfo);

	uint32 data;
	if (propInfo.FindMatch(message, 0, specifier, what, property, &data) >= 0) {
		if (data == kApplication)
			return this;

		BMessage reply(B_MESSAGE_NOT_UNDERSTOOD);
		reply.AddInt32("error", B_ERROR);
		reply.AddString("message", "Unkown specifier.");
		message->SendReply(&reply);
		return NULL;
	}

	return BApplication::ResolveSpecifier(message, index, specifier, what,
		property);
}


status_t
AutoMounter::GetSupportedSuites(BMessage* data)
{
	if (data == NULL)
		return B_BAD_VALUE;

	status_t status = data->AddString("suites",
		"suite/vnd.Haiku-mount_server");
	if (status != B_OK)
		return status;

	BPropertyInfo propertyInfo(sPropertyInfo);
	status = data->AddFlat("messages", &propertyInfo);
	if (status != B_OK)
		return status;

	return BApplication::GetSupportedSuites(data);
}


bool
AutoMounter::_ScriptReceived(BMessage *message, int32 index,
	BMessage *specifier, int32 what, const char *property)
{
	BMessage reply(B_REPLY);
	status_t err = B_BAD_SCRIPT_SYNTAX;

	switch (message->what) {
		case B_EXECUTE_PROPERTY:
			if (strcmp("InitialScan", property) == 0) {
				_MountVolumes(fNormalMode, fRemovableMode, true);
				err = reply.AddString("result",
					B_TRANSLATE("Previous volumes mounted."));
			}
			break;
	}

	if (err == B_BAD_SCRIPT_SYNTAX)
		return false;

	if (err != B_OK) {
		reply.what = B_MESSAGE_NOT_UNDERSTOOD;
		reply.AddString("message", strerror(err));
	}
	reply.AddInt32("error", err);
	message->SendReply(&reply);
	return true;
}


// #pragma mark -


static bool
suggest_mount_flags(const BPartition* partition, uint32* _flags)
{
	uint32 mountFlags = 0;

	bool askReadOnly = true;
	bool isBFS = false;

	if (partition->ContentType() != NULL
		&& strcmp(partition->ContentType(), kPartitionTypeBFS) == 0) {
#if 0
		askReadOnly = false;
#endif
		isBFS = true;
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
		if (!isBFS) {
			string << B_TRANSLATE("The file system on this volume is not the "
				"Haiku file system. It is strongly suggested to mount it in "
				"read-only mode. This will prevent unintentional data loss "
				"because of errors in Haiku.");
		} else {
			string << B_TRANSLATE("It is suggested to mount all additional "
				"Haiku volumes in read-only mode. This will prevent "
				"unintentional data loss because of errors in Haiku.");
		}

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


void
AutoMounter::_MountVolumes(mount_mode normal, mount_mode removable,
	bool initialRescan, partition_id deviceID)
{
	if (normal == kNoVolumes && removable == kNoVolumes)
		return;

	class InitialMountVisitor : public BDiskDeviceVisitor {
		public:
			InitialMountVisitor(mount_mode normalMode, mount_mode removableMode,
					bool initialRescan, BMessage& previous,
					partition_id deviceID)
				:
				fNormalMode(normalMode),
				fRemovableMode(removableMode),
				fInitialRescan(initialRescan),
				fPrevious(previous),
				fOnlyOnDeviceID(deviceID)
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

				mount_mode mode = !fInitialRescan
					&& partition->Device()->IsRemovableMedia()
					? fRemovableMode : fNormalMode;
				if (mode == kNoVolumes
					|| partition->IsMounted()
					|| !partition->ContainsFileSystem())
					return false;

				BPath path;
				if (partition->GetPath(&path) != B_OK)
					return false;

				if (mode == kRestorePreviousVolumes) {
					// mount all volumes that were stored in the settings file
					const char *volumeName = NULL;
					if (partition->ContentName() == NULL
						|| fPrevious.FindString(path.Path(), &volumeName)
							!= B_OK
						|| strcmp(volumeName, partition->ContentName()))
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
					if (!suggest_mount_flags(partition, &mountFlags))
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

		private:
			mount_mode		fNormalMode;
			mount_mode		fRemovableMode;
			bool			fInitialRescan;
			BMessage&		fPrevious;
			partition_id	fOnlyOnDeviceID;
	} visitor(normal, removable, initialRescan, fSettings, deviceID);

	BDiskDeviceList devices;
	status_t status = devices.Fetch();
	if (status == B_OK)
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
	if (!suggest_mount_flags(partition, &mountFlags))
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
	BDiskDevice device;
	if (partition == NULL) {
		// Try to retrieve partition
		BDiskDeviceRoster().FindPartitionByMountPoint(mountPoint.Path(),
			&device, &partition);
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

		partition->Device()->VisitEachDescendant(&visitor);

		if (!visitor.HasMountedPartitions())
			partition->Device()->Eject();
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

	char *buffer = new(std::nothrow) char[settingsSize];
	if (buffer == NULL) {
		PRINT(("error writing automounter settings, out of memory\n"));
		return;
	}

	status_t result = message.Flatten(buffer, settingsSize);

	fPrefsFile.Seek(0, SEEK_SET);
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
	BVolumeRoster volumeRoster;
	BVolume volume;
	while (volumeRoster.GetNextVolume(&volume) == B_OK) {
        fs_info info;
        if (fs_stat_dev(volume.Device(), &info) == 0
			&& (info.flags & (B_FS_IS_REMOVABLE | B_FS_IS_PERSISTENT)) != 0) {
			message->AddString(info.device_name, info.volume_name);

			BString mountFlagsKey(info.device_name);
			mountFlagsKey << kMountFlagsKeyExtension;
			uint32 mountFlags = 0;
			if (volume.IsReadOnly())
				mountFlags |= B_MOUNT_READ_ONLY;
			message->AddInt32(mountFlagsKey.String(), mountFlags);
		}
	}
}


// #pragma mark -


int
main(int argc, char* argv[])
{
	AutoMounter app;

	app.Run();
	return 0;
}


