/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "CopyEngine.h"
#include "InstallerWindow.h"
#include "PartitionMenuItem.h"
#include "FSUndoRedo.h"
#include "FSUtils.h"
#include <FindDirectory.h>
#include <DiskDeviceVisitor.h>
#include <DiskDeviceTypes.h>
#include <Path.h>
#include <String.h>
#include <VolumeRoster.h>

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
	fWindow(window)
{
	fControl = new InstallerCopyLoopControl(window);
}


void
CopyEngine::LaunchInitScript(BPath &path)
{
	BPath bootPath;
	find_directory(B_BEOS_BOOT_DIRECTORY, &bootPath);
	BString command(bootPath.Path());
	command += "/InstallerInitScript ";
	command += path.Path();
	fWindow->SetStatusMessage("Starting Installation.");	
	system(command.String());
}


void
CopyEngine::LaunchFinishScript(BPath &path)
{
	BPath bootPath;
	find_directory(B_BEOS_BOOT_DIRECTORY, &bootPath);
	BString command(bootPath.Path());
	command += "/InstallerFinishScript ";
	command += path.Path();
	fWindow->SetStatusMessage("Finishing Installation.");	
	system(command.String());
}


void
CopyEngine::Start(BMenu *srcMenu, BMenu *targetMenu)
{
	PartitionMenuItem *targetItem = (PartitionMenuItem *)targetMenu->FindMarked();
	PartitionMenuItem *srcItem = (PartitionMenuItem *)srcMenu->FindMarked();
	if (!srcItem || !targetItem)
		return;

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

	BPath srcDirectory;
        if (fDDRoster.GetPartitionWithID(srcItem->ID(), &device, &partition) == B_OK) {
                if (partition->GetMountPoint(&srcDirectory)!=B_OK)
                        return;
        } else if (fDDRoster.GetDeviceWithID(srcItem->ID(), &device) == B_OK) {
                if (device.GetMountPoint(&srcDirectory)!=B_OK)
                        return;
        } else
                return; // shouldn't happen
	
	// check not installing on boot volume
	BVolume bootVolume;
	BDirectory bootDir;
	BEntry bootEntry;
	BPath bootPath;
	BVolumeRoster().GetBootVolume(&bootVolume);
	bootVolume.GetRootDirectory(&bootDir);
	bootDir.GetEntry(&bootEntry);
	bootEntry.GetPath(&bootPath);
	if (strncmp(bootPath.Path(), targetDirectory.Path(), strlen(bootPath.Path())) == 0) {
		
	}

	// check if target is initialized

	// ask if init ou mount as is

	LaunchInitScript(targetDirectory);

	// copy source volume
	BDirectory targetDir(targetDirectory.Path());
	srcDirectory.Append("beos");
	BEntry srcEntry(srcDirectory.Path());
	Undo undo;
	printf("Copying folder '%s' to '%s'.\n", srcDirectory.Path(), targetDirectory.Path());
	FSCopyFolder(&srcEntry, &targetDir, fControl, NULL, false, undo);

	// copy selected packages

	LaunchFinishScript(targetDirectory);
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
	fMenu->AddItem(new PartitionMenuItem(NULL, device->ContentName(), NULL, new BMessage(SRC_PARTITION), device->ID()));
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
	fMenu->AddItem(new PartitionMenuItem(NULL, partition->ContentName(), NULL, new BMessage(SRC_PARTITION), partition->ID()));
	return false;
}


TargetVisitor::TargetVisitor(BMenu *menu)
	: fMenu(menu)
{
}


bool
TargetVisitor::Visit(BDiskDevice *device)
{
	if (device->IsReadOnly())
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

