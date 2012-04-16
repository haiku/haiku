/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2005-2008, Jérôme DUVAL.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "WorkerThread.h"

#include <errno.h>
#include <stdio.h>

#include <Alert.h>
#include <Autolock.h>
#include <Catalog.h>
#include <Directory.h>
#include <DiskDeviceVisitor.h>
#include <DiskDeviceTypes.h>
#include <FindDirectory.h>
#include <fs_index.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <Path.h>
#include <String.h>
#include <VolumeRoster.h>

#include "AutoLocker.h"
#include "CopyEngine.h"
#include "InstallerWindow.h"
#include "PackageViews.h"
#include "PartitionMenuItem.h"
#include "ProgressReporter.h"
#include "StringForSize.h"
#include "UnzipEngine.h"


#define B_TRANSLATION_CONTEXT "InstallProgress"


//#define COPY_TRACE
#ifdef COPY_TRACE
#define CALLED() 		printf("CALLED %s\n",__PRETTY_FUNCTION__)
#define ERR2(x, y...)	fprintf(stderr, "WorkerThread: "x" %s\n", y, strerror(err))
#define ERR(x)			fprintf(stderr, "WorkerThread: "x" %s\n", strerror(err))
#else
#define CALLED()
#define ERR(x)
#define ERR2(x, y...)
#endif

const char BOOT_PATH[] = "/boot";

const uint32 MSG_START_INSTALLING = 'eSRT';


class SourceVisitor : public BDiskDeviceVisitor {
public:
	SourceVisitor(BMenu* menu);
	virtual bool Visit(BDiskDevice* device);
	virtual bool Visit(BPartition* partition, int32 level);

private:
	BMenu* fMenu;
};


class TargetVisitor : public BDiskDeviceVisitor {
public:
	TargetVisitor(BMenu* menu);
	virtual bool Visit(BDiskDevice* device);
	virtual bool Visit(BPartition* partition, int32 level);

private:
	BMenu* fMenu;
};


// #pragma mark - WorkerThread


WorkerThread::WorkerThread(InstallerWindow *window)
	:
	BLooper("copy_engine"),
	fWindow(window),
	fPackages(NULL),
	fSpaceRequired(0),
	fCancelSemaphore(-1)
{
	Run();
}


void
WorkerThread::MessageReceived(BMessage* message)
{
	CALLED();

	switch (message->what) {
		case MSG_START_INSTALLING:
			_PerformInstall(fWindow->GetSourceMenu(), fWindow->GetTargetMenu());
			break;

		case MSG_WRITE_BOOT_SECTOR:
		{
			int32 id;
			if (message->FindInt32("id", &id) != B_OK) {
				_SetStatusMessage(B_TRANSLATE("Boot sector not written "
					"because of an internal error."));
				break;
			}

			// TODO: Refactor with _PerformInstall()
			BPath targetDirectory;
			BDiskDevice device;
			BPartition* partition;

			if (fDDRoster.GetPartitionWithID(id, &device, &partition) == B_OK) {
				if (!partition->IsMounted()) {
					if (partition->Mount() < B_OK) {
						_SetStatusMessage(B_TRANSLATE("The partition can't be "
							"mounted. Please choose a different partition."));
						break;
					}
				}
				if (partition->GetMountPoint(&targetDirectory) != B_OK) {
					_SetStatusMessage(B_TRANSLATE("The mount point could not "
						"be retrieved."));
					break;
				}
			} else if (fDDRoster.GetDeviceWithID(id, &device) == B_OK) {
				if (!device.IsMounted()) {
					if (device.Mount() < B_OK) {
						_SetStatusMessage(B_TRANSLATE("The disk can't be "
							"mounted. Please choose a different disk."));
						break;
					}
				}
				if (device.GetMountPoint(&targetDirectory) != B_OK) {
					_SetStatusMessage(B_TRANSLATE("The mount point could not "
						"be retrieved."));
					break;
				}
			}

			_LaunchFinishScript(targetDirectory);
			// TODO: Get error from executing script!
			_SetStatusMessage(
				B_TRANSLATE("Boot sector successfully written."));
		}
		default:
			BLooper::MessageReceived(message);
	}
}




void
WorkerThread::ScanDisksPartitions(BMenu *srcMenu, BMenu *targetMenu)
{
	// NOTE: This is actually executed in the window thread.
	BDiskDevice device;
	BPartition *partition = NULL;

	printf("\nScanDisksPartitions source partitions begin\n");
	SourceVisitor srcVisitor(srcMenu);
	fDDRoster.VisitEachMountedPartition(&srcVisitor, &device, &partition);

	printf("\nScanDisksPartitions target partitions begin\n");
	TargetVisitor targetVisitor(targetMenu);
	fDDRoster.VisitEachPartition(&targetVisitor, &device, &partition);
}


void
WorkerThread::SetPackagesList(BList *list)
{
	// Executed in window thread.
	BAutolock _(this);

	delete fPackages;
	fPackages = list;
}


void
WorkerThread::StartInstall()
{
	// Executed in window thread.
	PostMessage(MSG_START_INSTALLING, this);
}


void
WorkerThread::WriteBootSector(BMenu* targetMenu)
{
	// Executed in window thread.
	CALLED();

	PartitionMenuItem* item = (PartitionMenuItem*)targetMenu->FindMarked();
	if (item == NULL) {
		ERR("bad menu items\n");
		return;
	}

	BMessage message(MSG_WRITE_BOOT_SECTOR);
	message.AddInt32("id", item->ID());
	PostMessage(&message, this);
}


// #pragma mark -


void
WorkerThread::_LaunchInitScript(BPath &path)
{
	BPath bootPath;
	find_directory(B_BEOS_BOOT_DIRECTORY, &bootPath);
	BString command("/bin/sh ");
	command += bootPath.Path();
	command += "/InstallerInitScript ";
	command += "\"";
	command += path.Path();
	command += "\"";
	_SetStatusMessage(B_TRANSLATE("Starting Installation."));
	system(command.String());
}


void
WorkerThread::_LaunchFinishScript(BPath &path)
{
	BPath bootPath;
	find_directory(B_BEOS_BOOT_DIRECTORY, &bootPath);
	BString command("/bin/sh ");
	command += bootPath.Path();
	command += "/InstallerFinishScript ";
	command += "\"";
	command += path.Path();
	command += "\"";
	_SetStatusMessage(B_TRANSLATE("Finishing Installation."));
	system(command.String());
}


void
WorkerThread::_PerformInstall(BMenu* srcMenu, BMenu* targetMenu)
{
	CALLED();

	BPath targetDirectory;
	BPath srcDirectory;
	BPath trashPath;
	BPath testPath;
	BDirectory targetDir;
	BDiskDevice device;
	BPartition* partition;
	BVolume targetVolume;
	status_t err = B_OK;
	int32 entries = 0;
	entry_ref testRef;
	const char* mountError = B_TRANSLATE("The disk can't be mounted. Please "
		"choose a different disk.");

	BMessenger messenger(fWindow);
	ProgressReporter reporter(messenger, new BMessage(MSG_STATUS_MESSAGE));
	CopyEngine engine(&reporter);
	BList unzipEngines;

	PartitionMenuItem* targetItem = (PartitionMenuItem*)targetMenu->FindMarked();
	PartitionMenuItem* srcItem = (PartitionMenuItem*)srcMenu->FindMarked();
	if (!srcItem || !targetItem) {
		ERR("bad menu items\n");
		goto error;
	}

	// check if target is initialized
	// ask if init or mount as is
	if (fDDRoster.GetPartitionWithID(targetItem->ID(), &device,
			&partition) == B_OK) {
		if (!partition->IsMounted()) {
			if ((err = partition->Mount()) < B_OK) {
				_SetStatusMessage(mountError);
				ERR("BPartition::Mount");
				goto error;
			}
		}
		if ((err = partition->GetVolume(&targetVolume)) != B_OK) {
			ERR("BPartition::GetVolume");
			goto error;
		}
		if ((err = partition->GetMountPoint(&targetDirectory)) != B_OK) {
			ERR("BPartition::GetMountPoint");
			goto error;
		}
	} else if (fDDRoster.GetDeviceWithID(targetItem->ID(), &device) == B_OK) {
		if (!device.IsMounted()) {
			if ((err = device.Mount()) < B_OK) {
				_SetStatusMessage(mountError);
				ERR("BDiskDevice::Mount");
				goto error;
			}
		}
		if ((err = device.GetVolume(&targetVolume)) != B_OK) {
			ERR("BDiskDevice::GetVolume");
			goto error;
		}
		if ((err = device.GetMountPoint(&targetDirectory)) != B_OK) {
			ERR("BDiskDevice::GetMountPoint");
			goto error;
		}
	} else
		goto error; // shouldn't happen

	// check if target has enough space
	if ((fSpaceRequired > 0 && targetVolume.FreeBytes() < fSpaceRequired)
		&& ((new BAlert("", B_TRANSLATE("The destination disk may not have "
			"enough space. Try choosing a different disk or choose to not "
			"install optional items."), B_TRANSLATE("Try installing anyway"),
			B_TRANSLATE("Cancel"), 0,
			B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go() != 0)) {
		goto error;
	}

	if (fDDRoster.GetPartitionWithID(srcItem->ID(), &device, &partition) == B_OK) {
		if ((err = partition->GetMountPoint(&srcDirectory)) != B_OK) {
			ERR("BPartition::GetMountPoint");
			goto error;
		}
	} else if (fDDRoster.GetDeviceWithID(srcItem->ID(), &device) == B_OK) {
		if ((err = device.GetMountPoint(&srcDirectory)) != B_OK) {
			ERR("BDiskDevice::GetMountPoint");
			goto error;
		}
	} else
		goto error; // shouldn't happen

	// check not installing on itself
	if (strcmp(srcDirectory.Path(), targetDirectory.Path()) == 0) {
		_SetStatusMessage(B_TRANSLATE("You can't install the contents of a "
			"disk onto itself. Please choose a different disk."));
		goto error;
	}

	// check not installing on boot volume
	if ((strncmp(BOOT_PATH, targetDirectory.Path(), strlen(BOOT_PATH)) == 0)
		&& ((new BAlert("", B_TRANSLATE("Are you sure you want to install "
			"onto the current boot disk? The Installer will have to reboot "
			"your machine if you proceed."), B_TRANSLATE("OK"),
			B_TRANSLATE("Cancel"), 0,
			B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go() != 0)) {
		_SetStatusMessage("Installation stopped.");
		goto error;
	}

	// check if target volume's trash dir has anything in it
	// (target volume w/ only an empty trash dir is considered
	// an empty volume)
	if (find_directory(B_TRASH_DIRECTORY, &trashPath, false,
		&targetVolume) == B_OK && targetDir.SetTo(trashPath.Path()) == B_OK) {
			while (targetDir.GetNextRef(&testRef) == B_OK) {
				// Something in the Trash
				entries++;
				break;
			}
	}

	targetDir.SetTo(targetDirectory.Path());

	// check if target volume otherwise has any entries
	while (entries == 0 && targetDir.GetNextRef(&testRef) == B_OK) {
		if (testPath.SetTo(&testRef) == B_OK && testPath != trashPath)
			entries++;
	}

	if (entries != 0
		&& ((new BAlert("", B_TRANSLATE("The target volume is not empty. Are "
			"you sure you want to install anyway?\n\nNote: The 'system' folder "
			"will be a clean copy from the source volume, all other folders "
			"will be merged, whereas files and links that exist on both the "
			"source and target volume will be overwritten with the source "
			"volume version."),
			B_TRANSLATE("Install anyway"), B_TRANSLATE("Cancel"), 0,
			B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go() != 0)) {
		// TODO: Would be cool to offer the option here to clean additional
		// folders at the user's choice (like /boot/common and /boot/develop).
		err = B_CANCELED;
		goto error;
	}

	// Begin actual installation

	_LaunchInitScript(targetDirectory);

	// Create the default indices which should always be present on a proper
	// boot volume. We don't care if the source volume does not have them.
	// After all, the user might be re-installing to another drive and may
	// want problems fixed along the way...
	err = _CreateDefaultIndices(targetDirectory);
	if (err != B_OK)
		goto error;
	// Mirror all the indices which are present on the source volume onto
	// the target volume.
	err = _MirrorIndices(srcDirectory, targetDirectory);
	if (err != B_OK)
		goto error;

	// Let the engine collect information for the progress bar later on
	engine.ResetTargets(srcDirectory.Path());
	err = engine.CollectTargets(srcDirectory.Path(), fCancelSemaphore);
	if (err != B_OK)
		goto error;

	// Collect selected packages also
	if (fPackages) {
		BPath pkgRootDir(srcDirectory.Path(), PACKAGES_DIRECTORY);
		int32 count = fPackages->CountItems();
		for (int32 i = 0; i < count; i++) {
			Package *p = static_cast<Package*>(fPackages->ItemAt(i));
			BPath packageDir(pkgRootDir.Path(), p->Folder());
			err = engine.CollectTargets(packageDir.Path(), fCancelSemaphore);
			if (err != B_OK)
				goto error;
		}
	}

	// collect information about all zip packages
	err = _ProcessZipPackages(srcDirectory.Path(), targetDirectory.Path(),
		&reporter, unzipEngines);
	if (err != B_OK)
		goto error;

	reporter.StartTimer();

	// copy source volume
	err = engine.CopyFolder(srcDirectory.Path(), targetDirectory.Path(),
		fCancelSemaphore);
	if (err != B_OK)
		goto error;

	// copy selected packages
	if (fPackages) {
		BPath pkgRootDir(srcDirectory.Path(), PACKAGES_DIRECTORY);
		int32 count = fPackages->CountItems();
		for (int32 i = 0; i < count; i++) {
			Package *p = static_cast<Package*>(fPackages->ItemAt(i));
			BPath packageDir(pkgRootDir.Path(), p->Folder());
			err = engine.CopyFolder(packageDir.Path(), targetDirectory.Path(),
				fCancelSemaphore);
			if (err != B_OK)
				goto error;
		}
	}

	// Extract all zip packages. If an error occured, delete the rest of
	// the engines, but stop extracting.
	for (int32 i = 0; i < unzipEngines.CountItems(); i++) {
		UnzipEngine* engine = reinterpret_cast<UnzipEngine*>(
			unzipEngines.ItemAtFast(i));
		if (err == B_OK)
			err = engine->UnzipPackage();
		delete engine;
	}
	if (err != B_OK)
		goto error;

	_LaunchFinishScript(targetDirectory);

	BMessenger(fWindow).SendMessage(MSG_INSTALL_FINISHED);

	return;
error:
	BMessage statusMessage(MSG_RESET);
	if (err == B_CANCELED)
		_SetStatusMessage(B_TRANSLATE("Installation canceled."));
	else
		statusMessage.AddInt32("error", err);
	ERR("_PerformInstall failed");
	BMessenger(fWindow).SendMessage(&statusMessage);
}


status_t
WorkerThread::_MirrorIndices(const BPath& sourceDirectory,
	const BPath& targetDirectory) const
{
	dev_t sourceDevice = dev_for_path(sourceDirectory.Path());
	if (sourceDevice < 0)
		return (status_t)sourceDevice;
	dev_t targetDevice = dev_for_path(targetDirectory.Path());
	if (targetDevice < 0)
		return (status_t)targetDevice;
	DIR* indices = fs_open_index_dir(sourceDevice);
	if (indices == NULL) {
		printf("%s: fs_open_index_dir(): (%d) %s\n", sourceDirectory.Path(),
			errno, strerror(errno));
		// Opening the index directory will fail for example on ISO-Live
		// CDs. The default indices have already been created earlier, so
		// we simply bail.
		return B_OK;
	}
	while (dirent* index = fs_read_index_dir(indices)) {
		if (strcmp(index->d_name, "name") == 0
			|| strcmp(index->d_name, "size") == 0
			|| strcmp(index->d_name, "last_modified") == 0) {
			continue;
		}

		index_info info;
		if (fs_stat_index(sourceDevice, index->d_name, &info) != B_OK) {
			printf("Failed to mirror index %s: fs_stat_index(): (%d) %s\n",
				index->d_name, errno, strerror(errno));
			continue;
		}

		uint32 flags = 0;
			// Flags are always 0 for the moment.
		if (fs_create_index(targetDevice, index->d_name, info.type, flags)
			!= B_OK) {
			if (errno == B_FILE_EXISTS)
				continue;
			printf("Failed to mirror index %s: fs_create_index(): (%d) %s\n",
				index->d_name, errno, strerror(errno));
			continue;
		}
	}
	fs_close_index_dir(indices);
	return B_OK;
}


status_t
WorkerThread::_CreateDefaultIndices(const BPath& targetDirectory) const
{
	dev_t targetDevice = dev_for_path(targetDirectory.Path());
	if (targetDevice < 0)
		return (status_t)targetDevice;

	struct IndexInfo {
		const char* name;
		uint32_t	type;
	};

	const IndexInfo defaultIndices[] = {
		{ "BEOS:APP_SIG", B_STRING_TYPE },
		{ "BEOS:LOCALE_LANGUAGE", B_STRING_TYPE },
		{ "BEOS:LOCALE_SIGNATURE", B_STRING_TYPE },
		{ "_trk/qrylastchange", B_INT32_TYPE },
		{ "_trk/recentQuery", B_INT32_TYPE },
		{ "be:deskbar_item_status", B_STRING_TYPE }
	};

	uint32 flags = 0;
		// Flags are always 0 for the moment.

	for (uint32 i = 0; i < sizeof(defaultIndices) / sizeof(IndexInfo); i++) {
		const IndexInfo& info = defaultIndices[i];
		if (fs_create_index(targetDevice, info.name, info.type, flags)
			!= B_OK) {
			if (errno == B_FILE_EXISTS)
				continue;
			printf("Failed to create index %s: fs_create_index(): (%d) %s\n",
				info.name, errno, strerror(errno));
			return errno;
		}
	}

	return B_OK;
}


status_t
WorkerThread::_ProcessZipPackages(const char* sourcePath,
	const char* targetPath, ProgressReporter* reporter, BList& unzipEngines)
{
	// TODO: Put those in the optional packages list view
	// TODO: Implement mechanism to handle dependencies between these
	// packages. (Selecting one will auto-select others.)
	BPath pkgRootDir(sourcePath, PACKAGES_DIRECTORY);
	BDirectory directory(pkgRootDir.Path());
	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		if (entry.GetName(name) != B_OK)
			continue;
		int nameLength = strlen(name);
		if (nameLength <= 0)
			continue;
		char* nameExtension = name + nameLength - 4;
		if (strcasecmp(nameExtension, ".zip") != 0)
			continue;
		printf("found .zip package: %s\n", name);

		UnzipEngine* unzipEngine = new(std::nothrow) UnzipEngine(reporter,
			fCancelSemaphore);
		if (unzipEngine == NULL || !unzipEngines.AddItem(unzipEngine)) {
			delete unzipEngine;
			return B_NO_MEMORY;
		}
		BPath path;
		entry.GetPath(&path);
		status_t ret = unzipEngine->SetTo(path.Path(), targetPath);
		if (ret != B_OK)
			return ret;

		reporter->AddItems(unzipEngine->ItemsToUncompress(),
			unzipEngine->BytesToUncompress());
	}

	return B_OK;
}


void
WorkerThread::_SetStatusMessage(const char *status)
{
	BMessage msg(MSG_STATUS_MESSAGE);
	msg.AddString("status", status);
	BMessenger(fWindow).SendMessage(&msg);
}


static void
make_partition_label(BPartition* partition, char* label, char* menuLabel,
	bool showContentType)
{
	char size[20];
	string_for_size(partition->Size(), size, sizeof(size));

	BPath path;
	partition->GetPath(&path);

	if (showContentType) {
		const char* type = partition->ContentType();
		if (type == NULL)
			type = B_TRANSLATE_COMMENT("Unknown Type", "Partition content type");

		sprintf(label, "%s - %s [%s] (%s)", partition->ContentName(), size,
			path.Path(), type);
	} else {
		sprintf(label, "%s - %s [%s]", partition->ContentName(), size,
			path.Path());
	}

	sprintf(menuLabel, "%s - %s", partition->ContentName(), size);
}


// #pragma mark - SourceVisitor


SourceVisitor::SourceVisitor(BMenu *menu)
	: fMenu(menu)
{
}

bool
SourceVisitor::Visit(BDiskDevice *device)
{
	return Visit(device, 0);
}


bool
SourceVisitor::Visit(BPartition *partition, int32 level)
{
	BPath path;
	if (partition->GetPath(&path) == B_OK)
		printf("SourceVisitor::Visit(BPartition *) : %s\n", path.Path());
	printf("SourceVisitor::Visit(BPartition *) : %s\n",
		partition->ContentName());

	if (partition->ContentType() == NULL)
		return false;

	bool isBootPartition = false;
	if (partition->IsMounted()) {
		BPath mountPoint;
		partition->GetMountPoint(&mountPoint);
		isBootPartition = strcmp(BOOT_PATH, mountPoint.Path()) == 0;
	}

	if (!isBootPartition
		&& strcmp(partition->ContentType(), kPartitionTypeBFS) != 0) {
		// Except only BFS partitions, except this is the boot partition
		// (ISO9660 with write overlay for example).
		return false;
	}

	// TODO: We could probably check if this volume contains
	// the Haiku kernel or something. Does it make sense to "install"
	// from your BFS volume containing the music collection?
	// TODO: Then the check for BFS could also be removed above.

	char label[255];
	char menuLabel[255];
	make_partition_label(partition, label, menuLabel, false);
	PartitionMenuItem* item = new PartitionMenuItem(partition->ContentName(),
		label, menuLabel, new BMessage(SOURCE_PARTITION), partition->ID());
	item->SetMarked(isBootPartition);
	fMenu->AddItem(item);
	return false;
}


// #pragma mark - TargetVisitor


TargetVisitor::TargetVisitor(BMenu *menu)
	: fMenu(menu)
{
}


bool
TargetVisitor::Visit(BDiskDevice *device)
{
	if (device->IsReadOnlyMedia())
		return false;
	return Visit(device, 0);
}


bool
TargetVisitor::Visit(BPartition *partition, int32 level)
{
	BPath path;
	if (partition->GetPath(&path) == B_OK)
		printf("TargetVisitor::Visit(BPartition *) : %s\n", path.Path());
	printf("TargetVisitor::Visit(BPartition *) : %s\n",
		partition->ContentName());

	if (partition->ContentSize() < 20 * 1024 * 1024) {
		// reject partitions which are too small anyway
		// TODO: Could depend on the source size
		printf("  too small\n");
		return false;
	}

	if (partition->CountChildren() > 0) {
		// Looks like an extended partition, or the device itself.
		// Do not accept this as target...
		printf("  no leaf partition\n");
		return false;
	}

	// TODO: After running DriveSetup and doing another scan, it would
	// be great to pick the partition which just appeared!

	bool isBootPartition = false;
	if (partition->IsMounted()) {
		BPath mountPoint;
		partition->GetMountPoint(&mountPoint);
		isBootPartition = strcmp(BOOT_PATH, mountPoint.Path()) == 0;
	}

	// Only non-boot BFS partitions are valid targets, but we want to display the
	// other partitions as well, in order not to irritate the user.
	bool isValidTarget = isBootPartition == false
		&& partition->ContentType() != NULL
		&& strcmp(partition->ContentType(), kPartitionTypeBFS) == 0;

	char label[255];
	char menuLabel[255];
	make_partition_label(partition, label, menuLabel, !isValidTarget);
	PartitionMenuItem* item = new PartitionMenuItem(partition->ContentName(),
		label, menuLabel, new BMessage(TARGET_PARTITION), partition->ID());

	item->SetIsValidTarget(isValidTarget);


	fMenu->AddItem(item);
	return false;
}

