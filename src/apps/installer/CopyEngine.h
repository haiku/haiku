/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _CopyEngine_h
#define _CopyEngine_h

#include "InstallerCopyLoopControl.h"

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
	void Start(BMenu *srcMenu, BMenu *targetMenu);
	void ScanDisksPartitions(BMenu *srcMenu, BMenu *targetMenu);

private:
	void LaunchInitScript(BPath &path);
	void LaunchFinishScript(BPath &path);
	
	InstallerWindow *fWindow;
	BDiskDeviceRoster fDDRoster;
	InstallerCopyLoopControl *fControl;
};

#endif /* _CopyEngine_h */
