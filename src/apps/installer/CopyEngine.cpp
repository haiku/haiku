/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "CopyEngine.h"
#include "InstallerWindow.h"
#include <DiskDeviceRoster.h>
#include <Path.h>

CopyEngine::CopyEngine(InstallerWindow *window)
	: BLooper("copy_engine"),
	fWindow(window)
{

}


void
CopyEngine::LaunchInitScript(BVolume *volume)
{
	fWindow->SetStatusMessage("Starting Installation.");
}


void
CopyEngine::LaunchFinishScript(BVolume *volume)
{
	fWindow->SetStatusMessage("Finishing Installation.");
}


void
CopyEngine::Start()
{
	BVolume *volume;
	// check not installing on boot volume


	// check if target is initialized

	// ask if init ou mount as is

	LaunchInitScript(volume);

	// copy source volume

	// copy selected packages

	LaunchFinishScript(volume);
}


void
CopyEngine::ScanDisksPartitions(BMenu *srcMenu, BMenu *dstMenu)
{
	/*BDiskDeviceRoster roster;
	BDiskDevice device;
	roster.VisitEachDevice(this, &device);

	BPartition *partition;
	roster.VisitEachPartition(this, &device, &partition);*/
}


bool
CopyEngine::Visit(BDiskDevice *device)
{
	BPath path;
	if (device->GetPath(&path)==B_OK)
		printf("CopyEngine::Visit(BDiskDevice *) : %s\n", path.Path());
	return false;
}


bool
CopyEngine::Visit(BPartition *partition, int32 level)
{
	BPath path;
	if (partition->GetPath(&path)==B_OK) 
		printf("CopyEngine::Visit(BPartition *) : %s\n", path.Path());
	printf("CopyEngine::Visit(BPartition *) : %s\n", partition->Name());
	return false;
}

