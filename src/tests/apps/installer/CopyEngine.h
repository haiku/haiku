/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _CopyEngine_h
#define _CopyEngine_h

#include <Directory.h>
#include <Looper.h>
#include <Menu.h>
#include <Messenger.h>
#include <Path.h>

class InstallerWindow;

const uint32 ENGINE_START = 'eSRT';

class CopyEngine : public BLooper {
public:
	CopyEngine(InstallerWindow *window);
	void MessageReceived(BMessage *msg);
	void SetStatusMessage(char *status);
	void Start(BMenu *srcMenu, BMenu *targetMenu);
	void ScanDisksPartitions(BMenu *srcMenu, BMenu *targetMenu);
	void SetPackagesList(BList *list);
	void SetSpaceRequired(off_t bytes) { fSpaceRequired = bytes; };
private:
	void CopyFolder(BDirectory &srcDir, BDirectory &targetDir);
	
	InstallerWindow *fWindow;
	BList *fPackages;
	off_t fSpaceRequired;
};

#endif /* _CopyEngine_h */
