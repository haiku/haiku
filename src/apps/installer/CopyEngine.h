/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _CopyEngine_h
#define _CopyEngine_h

#include <DiskDeviceVisitor.h>
#include <DiskDevice.h>
#include <Looper.h>
#include <Messenger.h>
#include <Partition.h>
#include <Volume.h>

class InstallerWindow;

class CopyEngine : public BLooper/*, BDiskDeviceVisitor*/ {
public:
	CopyEngine(InstallerWindow *window);
	void Start();
	void ScanDisksPartitions(BMenu *srcMenu, BMenu *dstMenu);

	virtual bool Visit(BDiskDevice *device);
	virtual bool Visit(BPartition *partition, int32 level);
private:
	void LaunchInitScript(BVolume *volume);
	void LaunchFinishScript(BVolume *volume);
	InstallerWindow *fWindow;
};

#endif /* _CopyEngine_h */
