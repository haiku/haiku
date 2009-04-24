/*
 * Copyright 2005-2008, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "CopyEngine.h"
#include "InstallerWindow.h"
#include "PartitionMenuItem.h"
#include "FSUndoRedo.h"
#include "FSUtils.h"
#include <stdio.h>
#include <Alert.h>
#include <DiskDeviceVisitor.h>
#include <DiskDeviceTypes.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>
#include <VolumeRoster.h>

//#define COPY_TRACE
#ifdef COPY_TRACE
#define CALLED() 		printf("CALLED %s\n",__PRETTY_FUNCTION__)
#define ERR2(x, y...)	fprintf(stderr, "CopyEngine: "x" %s\n", y, strerror(err))
#define ERR(x)			fprintf(stderr, "CopyEngine: "x" %s\n", strerror(err))
#else
#define CALLED()
#define ERR(x)
#define ERR2(x, y...)
#endif

const char BOOT_PATH[] = "/boot";

extern void SizeAsString(off_t size, char *string);

class SourceVisitor : public BDiskDeviceVisitor
{
	public:
		SourceVisitor(BMenu *menu);
		virtual bool Visit(BDiskDevice *device);
		virtual bool Visit(BPartition *partition, int32 level);
	private:
		BMenu *fMenu;
};


class TargetVisitor : public BDiskDeviceVisitor
{
	public:
		TargetVisitor(BMenu *menu);
		virtual bool Visit(BDiskDevice *device);
		virtual bool Visit(BPartition *partition, int32 level);
	private:
		void _MakeLabel(BPartition *partition, char *label, char *menuLabel);
		BMenu *fMenu;
};


CopyEngine::CopyEngine(InstallerWindow *window)
	: BLooper("copy_engine"),
	fWindow(window),
	fPackages(NULL),
	fSpaceRequired(0)
{
	fControl = new InstallerCopyLoopControl(window);
	Run();
}


void
CopyEngine::MessageReceived(BMessage*msg)
{
	CALLED();
	switch (msg->what) {
		case ENGINE_START:
		{
			Start(fWindow->GetSourceMenu(), fWindow->GetTargetMenu());
			break;
		}
	}
}


void
CopyEngine::SetStatusMessage(char *status)
{
	BMessage msg(STATUS_MESSAGE);
	msg.AddString("status", status);
	BMessenger(fWindow).SendMessage(&msg);
}


void
CopyEngine::LaunchInitScript(BPath &path)
{
	BPath bootPath;
	find_directory(B_BEOS_BOOT_DIRECTORY, &bootPath);
	BString command("/bin/sh ");
	command += bootPath.Path();
	command += "/InstallerInitScript ";
	command += path.Path();
	SetStatusMessage("Starting Installation.");
	system(command.String());
}


void
CopyEngine::LaunchFinishScript(BPath &path)
{
	BPath bootPath;
	find_directory(B_BEOS_BOOT_DIRECTORY, &bootPath);
	BString command("/bin/sh ");
	command += bootPath.Path();
	command += "/InstallerFinishScript ";
	command += path.Path();
	SetStatusMessage("Finishing Installation.");
	system(command.String());
}


void
CopyEngine::Start(BMenu *srcMenu, BMenu *targetMenu)
{
	CALLED();

	BPath targetDirectory, srcDirectory;
	BDirectory targetDir, srcDir;
	BDiskDevice device;
	BPartition *partition;
	BVolume targetVolume;
	status_t err = B_OK;
	
	fControl->Reset();

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
				SetStatusMessage("The disk can't be mounted. Please choose a "
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
				SetStatusMessage("The disk can't be mounted. Please choose a "
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
		SetStatusMessage("You can't install the contents of a disk onto "
			"itself. Please choose a different disk.");
		goto error;
	}

	// check not installing on boot volume
	if ((strncmp(BOOT_PATH, targetDirectory.Path(), strlen(BOOT_PATH)) == 0)
		&& ((new BAlert("", "Are you sure you want to install onto the "
			"current boot disk? The installer will have to reboot your "
			"machine if you proceed.", "OK", "Cancel", 0,
			B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go() != 0)) {
		SetStatusMessage("Installation stopped.");
		goto error;
	}

	LaunchInitScript(targetDirectory);

	// copy source volume
	targetDir.SetTo(targetDirectory.Path());
	srcDir.SetTo(srcDirectory.Path());
	err = CopyFolder(srcDir, targetDir);
	
	if (err != B_OK || fControl->CheckUserCanceled())
		goto error;

	// copy selected packages
	if (fPackages) {
		srcDirectory.Append(PACKAGES_DIRECTORY);
		srcDir.SetTo(srcDirectory.Path());
		BDirectory packageDir;
		int32 count = fPackages->CountItems();
		for (int32 i = 0; i < count; i++) {
			Package *p = static_cast<Package*>(fPackages->ItemAt(i));
			packageDir.SetTo(&srcDir, p->Folder());
			err = CopyFolder(packageDir, targetDir);
			if (err != B_OK || fControl->CheckUserCanceled())
				goto error;
		}
	}

	LaunchFinishScript(targetDirectory);

	BMessenger(fWindow).SendMessage(INSTALL_FINISHED);

	return;
error:
	if (err == B_CANCELED || fControl->CheckUserCanceled())
		SetStatusMessage("Installation canceled.");
	ERR("Start failed");
	BMessenger(fWindow).SendMessage(RESET_INSTALL);
}


status_t
CopyEngine::CopyFolder(BDirectory &srcDir, BDirectory &targetDir)
{
	BEntry entry;
	status_t err;
	while (srcDir.GetNextEntry(&entry) == B_OK
		&& !fControl->CheckUserCanceled()) {
		StatStruct statbuf;
		entry.GetStat(&statbuf);
		
		Undo undo;
		if (S_ISDIR(statbuf.st_mode)) {
			char name[B_FILE_NAME_LENGTH];
			if (entry.GetName(name) == B_OK
				&& (strcmp(name, PACKAGES_DIRECTORY) == 0
				|| strcmp(name, VAR_DIRECTORY) == 0)) {
				continue;
			}
			err = FSCopyFolder(&entry, &targetDir, fControl, NULL, false, undo);
		} else {
			err = FSCopyFile(&entry, &statbuf, &targetDir, fControl, NULL,
				false, undo);
		}
		if (err != B_OK) {
			BPath path;
			entry.GetPath(&path);
			ERR2("error while copying %s", path.Path());
			return err;
		}
	}

	return B_OK;
}


void
CopyEngine::ScanDisksPartitions(BMenu *srcMenu, BMenu *targetMenu)
{
	BDiskDevice device;
	BPartition *partition = NULL;

	printf("ScanDisksPartitions partitions begin\n");
	SourceVisitor srcVisitor(srcMenu);
	fDDRoster.VisitEachMountedPartition(&srcVisitor, &device, &partition);

	printf("ScanDisksPartitions partitions begin\n");
	TargetVisitor targetVisitor(targetMenu);
	fDDRoster.VisitEachPartition(&targetVisitor, &device, &partition);
}


void
CopyEngine::SetPackagesList(BList *list)
{
	delete fPackages;
	fPackages = list;
}


bool
CopyEngine::Cancel()
{
	return fControl->Cancel();
}


// #pragma mark -


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

	if (!partition->ContentType())
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


// #pragma mark -


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
		// reject partitions which are too small anyways
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

	char label[255], menuLabel[255];
	_MakeLabel(partition, label, menuLabel);
	fMenu->AddItem(new PartitionMenuItem(partition->ContentName(), label,
		menuLabel, new BMessage(TARGET_PARTITION), partition->ID()));
	return false;
}


void
TargetVisitor::_MakeLabel(BPartition *partition, char *label, char *menuLabel)
{
	char size[15];
	SizeAsString(partition->ContentSize(), size);
	BPath path;
	partition->GetPath(&path);

	sprintf(label, "%s - %s [%s] [%s]", partition->ContentName(),
		size, partition->ContentType(), path.Path());
	sprintf(menuLabel, "%s - %s [%s]", partition->ContentName(), size,
		partition->ContentType());

}

