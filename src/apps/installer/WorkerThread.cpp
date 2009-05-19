/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2005-2008, Jérôme DUVAL.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "WorkerThread.h"

#include <stdio.h>

#include <Alert.h>
#include <Autolock.h>
#include <Directory.h>
#include <DiskDeviceVisitor.h>
#include <DiskDeviceTypes.h>
#include <FindDirectory.h>
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

extern void SizeAsString(off_t size, char* string);


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
	void _MakeLabel(BPartition* partition, char* label, char* menuLabel,
		bool showContentType);

	BMenu* fMenu;
};


// #pragma mark - WorkerThread


WorkerThread::WorkerThread(InstallerWindow *window)
	: BLooper("copy_engine"),
	fWindow(window),
	fPackages(NULL),
	fSpaceRequired(0)
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
				_SetStatusMessage("Boot sector not written because of an "
					" internal error.");
				break;
			}

			// TODO: Refactor with _PerformInstall()
			BPath targetDirectory;
			BDiskDevice device;
			BPartition* partition;

			if (fDDRoster.GetPartitionWithID(id, &device, &partition) == B_OK) {
				if (!partition->IsMounted()) {
					if (partition->Mount() < B_OK) {
						_SetStatusMessage("The partition can't be mounted. "
							"Please choose a different partition.");
						break;
					}
				}
				if (partition->GetMountPoint(&targetDirectory) != B_OK) {
					_SetStatusMessage("The mount point could not be retrieve.");
					break;
				}
			} else if (fDDRoster.GetDeviceWithID(id, &device) == B_OK) {
				if (!device.IsMounted()) {
					if (device.Mount() < B_OK) {
						_SetStatusMessage("The disk can't be mounted. Please "
							"choose a different disk.");
						break;
					}
				}
				if (device.GetMountPoint(&targetDirectory) != B_OK) {
					_SetStatusMessage("The mount point could not be retrieve.");
					break;
				}
			}

			_LaunchFinishScript(targetDirectory);
			// TODO: Get error from executing script!
			_SetStatusMessage("Boot sector successfully written.");
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
	command += path.Path();
	_SetStatusMessage("Starting Installation.");
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
	command += path.Path();
	_SetStatusMessage("Finishing Installation.");
	system(command.String());
}


void
WorkerThread::_PerformInstall(BMenu *srcMenu, BMenu *targetMenu)
{
	CALLED();

	BPath targetDirectory, srcDirectory;
	BDirectory targetDir;
	BDiskDevice device;
	BPartition *partition;
	BVolume targetVolume;
	status_t err = B_OK;
	int32 entries = 0;
	entry_ref testRef;

	BMessenger messenger(fWindow);
	CopyEngine engine(messenger, new BMessage(MSG_STATUS_MESSAGE));

	PartitionMenuItem *targetItem = (PartitionMenuItem *)targetMenu->FindMarked();
	PartitionMenuItem *srcItem = (PartitionMenuItem *)srcMenu->FindMarked();
	if (!srcItem || !targetItem) {
		ERR("bad menu items\n");
		goto error;
	}

	// check if target is initialized
	// ask if init or mount as is

	if (fDDRoster.GetPartitionWithID(targetItem->ID(), &device, &partition) == B_OK) {
		if (!partition->IsMounted()) {
			if ((err = partition->Mount()) < B_OK) {
				_SetStatusMessage("The disk can't be mounted. Please choose a "
					"different disk.");
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
				_SetStatusMessage("The disk can't be mounted. Please choose a "
					"different disk.");
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
		&& ((new BAlert("", "The destination disk may not have enough space. "
			"Try choosing a different disk or choose to not install optional "
			"items.", "Try installing anyway", "Cancel", 0,
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
		_SetStatusMessage("You can't install the contents of a disk onto "
			"itself. Please choose a different disk.");
		goto error;
	}

	// check not installing on boot volume
	if ((strncmp(BOOT_PATH, targetDirectory.Path(), strlen(BOOT_PATH)) == 0)
		&& ((new BAlert("", "Are you sure you want to install onto the "
			"current boot disk? The Installer will have to reboot your "
			"machine if you proceed.", "OK", "Cancel", 0,
			B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go() != 0)) {
		_SetStatusMessage("Installation stopped.");
		goto error;
	}

	targetDir.SetTo(targetDirectory.Path());

	// check target volume not empty
	// NOTE: It's ok if exactly this path exists: home/Desktop/Trash
	// and nothing else.
	while (targetDir.GetNextRef(&testRef) == B_OK) {
		if (strcmp(testRef.name, "home") == 0) {
			BDirectory homeDir(&testRef);
			while (homeDir.GetNextRef(&testRef) == B_OK) {
				if (strcmp(testRef.name, "Desktop") == 0) {
					BDirectory desktopDir(&testRef);
					while (desktopDir.GetNextRef(&testRef) == B_OK) {
						if (strcmp(testRef.name, "Trash") == 0) {
							BDirectory trashDir(&testRef);
							while (trashDir.GetNextRef(&testRef) == B_OK) {
								// Something in the Trash
								entries++;
								break;
							}
						} else {
							// Something besides Trash
							entries++;
						}

						if (entries > 0)
							break;
					}
				} else {
					// Something besides Desktop
					entries++;
				}

				if (entries > 0)
					break;
			}
		} else {
			// Something besides home
			entries++;
		}

		if (entries > 0)
			break;
	}
	if (entries != 0
		&& ((new BAlert("", "The target volume is not empty. Are you sure you "
			"want to install anyway?", "Install Anyway", "Cancel", 0,
			B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go() != 0)) {
		err = B_CANCELED;
		goto error;
	}


	_LaunchInitScript(targetDirectory);

	// let the engine collect information for the progress bar later on
	engine.ResetTargets();
	err = engine.CollectTargets(srcDirectory.Path());
	if (err != B_OK)
		goto error;

	// collect selected packages also
	if (fPackages) {
		BPath pkgRootDir(srcDirectory.Path(), PACKAGES_DIRECTORY);
		int32 count = fPackages->CountItems();
		for (int32 i = 0; i < count; i++) {
			Package *p = static_cast<Package*>(fPackages->ItemAt(i));
			BPath packageDir(pkgRootDir.Path(), p->Folder());
			err = engine.CollectTargets(packageDir.Path());
			if (err != B_OK)
				goto error;
		}
	}

	// copy source volume
	err = engine.CopyFolder(srcDirectory.Path(), targetDirectory.Path(),
		fCancelLock);
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
				fCancelLock);
			if (err != B_OK)
				goto error;
		}
	}

	_LaunchFinishScript(targetDirectory);

	BMessenger(fWindow).SendMessage(MSG_INSTALL_FINISHED);

	return;
error:
	BMessage statusMessage(MSG_RESET);
	if (err == B_CANCELED)
		_SetStatusMessage("Installation canceled.");
	else
		statusMessage.AddInt32("error", err);
	ERR("_PerformInstall failed");
	BMessenger(fWindow).SendMessage(&statusMessage);
}


void
WorkerThread::_SetStatusMessage(const char *status)
{
	BMessage msg(MSG_STATUS_MESSAGE);
	msg.AddString("status", status);
	BMessenger(fWindow).SendMessage(&msg);
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
	printf("SourceVisitor::Visit(BPartition *) : %s\n", partition->ContentName());

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

	PartitionMenuItem *item = new PartitionMenuItem(NULL,
		partition->ContentName(), NULL, new BMessage(SRC_PARTITION),
		partition->ID());
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
	printf("TargetVisitor::Visit(BPartition *) : %s\n", partition->ContentName());

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

	// Only BFS partitions are valid targets, but we want to display the
	// other partitions as well, in order not to irritate the user.
	bool isValidTarget = partition->ContentType() != NULL
		&& strcmp(partition->ContentType(), kPartitionTypeBFS) == 0;

	char label[255], menuLabel[255];
	_MakeLabel(partition, label, menuLabel, !isValidTarget);
	PartitionMenuItem* item = new PartitionMenuItem(partition->ContentName(),
		label, menuLabel, new BMessage(TARGET_PARTITION), partition->ID());

	item->SetIsValidTarget(isValidTarget);


	fMenu->AddItem(item);
	return false;
}


void
TargetVisitor::_MakeLabel(BPartition* partition, char* label, char* menuLabel,
	bool showContentType)
{
	char size[15];
	SizeAsString(partition->ContentSize(), size);

	BPath path;
	partition->GetPath(&path);

	if (showContentType) {
		const char* type = partition->ContentType();
		if (type == NULL)
			type = "Unknown Type";

		sprintf(label, "%s - %s [%s] (%s)", partition->ContentName(), size,
			path.Path(), type);
	} else {
		sprintf(label, "%s - %s [%s]", partition->ContentName(), size,
			path.Path());
	}

	sprintf(menuLabel, "%s - %s", partition->ContentName(), size);
}

