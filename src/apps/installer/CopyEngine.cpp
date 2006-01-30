/*
 * Copyright 2005-2006, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "CopyEngine.h"
#include "InstallerWindow.h"
#include "PartitionMenuItem.h"
#include "FSUndoRedo.h"
#include "FSUtils.h"
#include <Alert.h>
#include <DiskDeviceVisitor.h>
#include <DiskDeviceTypes.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>
#include <VolumeRoster.h>

//#define COPY_TRACE
#ifdef COPY_TRACE
#define CALLED() 			printf("CALLED %s\n",__PRETTY_FUNCTION__)
#else
#define CALLED()
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
	void MakeLabel(BPartition *partition, char *label, char *menuLabel);
	BMenu *fMenu;
};


CopyEngine::CopyEngine(InstallerWindow *window)
	: BLooper("copy_engine"),
	fWindow(window),
	fPackages(NULL)
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
			Start(fWindow->GetSourceMenu(), fWindow->GetTargetMenu());
			break;
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
	PartitionMenuItem *targetItem = (PartitionMenuItem *)targetMenu->FindMarked();
	PartitionMenuItem *srcItem = (PartitionMenuItem *)srcMenu->FindMarked();
	if (!srcItem || !targetItem) {
		fprintf(stderr, "bad menu items\n");
		return;
	}

	BPath targetDirectory;
	BDiskDevice device;
	BPartition *partition;
	if (fDDRoster.GetPartitionWithID(targetItem->ID(), &device, &partition) == B_OK) {
		if (partition->GetMountPoint(&targetDirectory)!=B_OK)
			return;
	} else if (fDDRoster.GetDeviceWithID(targetItem->ID(), &device) == B_OK) {
		if (device.GetMountPoint(&targetDirectory)!=B_OK)
			return;
	} else 
		return; // shouldn't happen

	// check if target is initialized
	// ask if init or mount as is
	// check if target has enough space

	BPath srcDirectory;
        if (fDDRoster.GetPartitionWithID(srcItem->ID(), &device, &partition) == B_OK) {
                if (partition->GetMountPoint(&srcDirectory)!=B_OK)
                        return;
        } else if (fDDRoster.GetDeviceWithID(srcItem->ID(), &device) == B_OK) {
                if (device.GetMountPoint(&srcDirectory)!=B_OK)
                        return;
        } else
                return; // shouldn't happen
	
	// check not installing on itself
	if (strcmp(srcDirectory.Path(), targetDirectory.Path()) == 0) {
		SetStatusMessage("You can't install the contents of a disk onto itself. Please choose a different disk.");
		return;
	}
	
	// check not installing on boot volume
	if ((strncmp(BOOT_PATH, targetDirectory.Path(), strlen(BOOT_PATH)) == 0) 
		&& ((new BAlert("", "Are you sure you want to install onto the current boot disk? \
The installer will have to reboot your machine if you proceed.", "OK", "Cancel", 0,
                        B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go() != 0)) {
		SetStatusMessage("Installation stopped.");
		return;
	}

	LaunchInitScript(targetDirectory);

	// copy source volume
	BDirectory targetDir(targetDirectory.Path());
	BDirectory srcDir(srcDirectory.Path());
	CopyFolder(srcDir, targetDir);

	// copy selected packages
	if (fPackages) {
		srcDirectory.Append(PACKAGES_DIRECTORY);
		srcDir.SetTo(srcDirectory.Path());
		BDirectory packageDir;
		int32 count = fPackages->CountItems();
		for (int32 i=0; i<count; i++) {
			Package *p = static_cast<Package*>(fPackages->ItemAt(i));
			packageDir.SetTo(&srcDir, p->Folder());
			CopyFolder(packageDir, targetDir);
		}
	}

	LaunchFinishScript(targetDirectory);

	BMessage msg(INSTALL_FINISHED);
	BMessenger(fWindow).SendMessage(&msg);
}


void
CopyEngine::CopyFolder(BDirectory &srcDir, BDirectory &targetDir)
{
	BEntry entry;
	status_t status;
	while (srcDir.GetNextEntry(&entry) == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		entry.GetName(name);
		if (strcmp(name, PACKAGES_DIRECTORY) == 0)
			continue;

		Undo undo;
		status = FSCopyFolder(&entry, &targetDir, fControl, NULL, false, undo);
		if (status != B_OK) {
			BPath path;
			entry.GetPath(&path);
			fprintf(stderr, "error while copying %s : %s\n", path.Path(), strerror(status));
		}
	}
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
	if (fPackages)
		delete fPackages;
	fPackages = list;
}


SourceVisitor::SourceVisitor(BMenu *menu)
	: fMenu(menu)
{
}

bool
SourceVisitor::Visit(BDiskDevice *device)
{
	if (!device->ContentType() || strcmp(device->ContentType(), kPartitionTypeBFS)!=0)
		return false;
	BPath path;
	if (device->GetPath(&path)==B_OK)
		printf("SourceVisitor::Visit(BDiskDevice *) : %s type:%s, contentType:%s\n", 
			path.Path(), device->Type(), device->ContentType());
	PartitionMenuItem *item = new PartitionMenuItem(NULL, device->ContentName(), NULL, new BMessage(SRC_PARTITION), device->ID());
	if (device->IsMounted()) {
		BPath mountPoint;
		device->GetMountPoint(&mountPoint);
		if (strcmp(BOOT_PATH, mountPoint.Path())==0)
			item->SetMarked(true);
	}
	fMenu->AddItem(item);
	return false;
}


bool
SourceVisitor::Visit(BPartition *partition, int32 level)
{
	if (!partition->ContentType() || strcmp(partition->ContentType(), kPartitionTypeBFS)!=0)
		return false;
	BPath path;
	if (partition->GetPath(&path)==B_OK)
		printf("SourceVisitor::Visit(BPartition *) : %s\n", path.Path());
	printf("SourceVisitor::Visit(BPartition *) : %s\n", partition->Name());
	PartitionMenuItem *item = new PartitionMenuItem(NULL, partition->ContentName(), NULL, new BMessage(SRC_PARTITION), partition->ID());
	if (partition->IsMounted()) {
		BPath mountPoint;
		partition->GetMountPoint(&mountPoint);
		if (strcmp(BOOT_PATH, mountPoint.Path())==0)
			item->SetMarked(true);
	}
	fMenu->AddItem(item);
	return false;
}


TargetVisitor::TargetVisitor(BMenu *menu)
	: fMenu(menu)
{
}


bool
TargetVisitor::Visit(BDiskDevice *device)
{
	if (device->IsReadOnly() || device->IsReadOnlyMedia())
		return false;
	BPath path;
	if (device->GetPath(&path)==B_OK)
		printf("TargetVisitor::Visit(BDiskDevice *) : %s\n", path.Path());
	char label[255], menuLabel[255];
	MakeLabel(device, label, menuLabel);
	fMenu->AddItem(new PartitionMenuItem(device->ContentName(), label, menuLabel, new BMessage(TARGET_PARTITION), device->ID()));
	return false;
}


bool
TargetVisitor::Visit(BPartition *partition, int32 level)
{
	if (partition->IsReadOnly())
		return false;
	BPath path;
	if (partition->GetPath(&path)==B_OK) 
		printf("TargetVisitor::Visit(BPartition *) : %s\n", path.Path());
	printf("TargetVisitor::Visit(BPartition *) : %s\n", partition->Name());
	char label[255], menuLabel[255];
	MakeLabel(partition, label, menuLabel);
	fMenu->AddItem(new PartitionMenuItem(partition->ContentName(), label, menuLabel, new BMessage(TARGET_PARTITION), partition->ID()));
	return false;
}


void
TargetVisitor::MakeLabel(BPartition *partition, char *label, char *menuLabel)
{
	char size[15];
	SizeAsString(partition->ContentSize(), size);
	BPath path;
	if (partition->Parent())
		partition->Parent()->GetPath(&path);
	
	sprintf(label, "%s - %s [%s] [%s partition:%li]", partition->ContentName(), size, partition->ContentType(),
			path.Path(), partition->ID());
	sprintf(menuLabel, "%s - %s [%s]", partition->ContentName(), size, partition->ContentType());

}

