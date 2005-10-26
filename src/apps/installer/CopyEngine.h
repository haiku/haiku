/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _CopyEngine_h
#define _CopyEngine_h

#include <DiskDevice.h>
#include <DiskDeviceRoster.h>
#include <Looper.h>
#include <Messenger.h>
#include <Partition.h>
#include <Volume.h>

class InstallerWindow;

class CopyEngine : public BLooper {
public:
	CopyEngine(InstallerWindow *window);
	void Start();
	void ScanDisksPartitions(BMenu *srcMenu, BMenu *targetMenu);

private:
	void LaunchInitScript(BVolume *volume);
	void LaunchFinishScript(BVolume *volume);
	
	InstallerWindow *fWindow;
	BDiskDeviceRoster fDDRoster;
};

#endif /* _CopyEngine_h */
