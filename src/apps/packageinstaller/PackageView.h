/*
 * Copyright 2007-2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#ifndef PACKAGE_VIEW_H
#define PACKAGE_VIEW_H


#include "PackageInfo.h"
#include "PackageInstall.h"
#include "PackageStatus.h"

#include <Box.h>
#include <Button.h>
#include <FilePanel.h>
#include <MenuField.h>
#include <View.h>

class BPopUpMenu;
class BTextView;


enum {
	P_MSG_GROUP_CHANGED = 'gpch',
	P_MSG_PATH_CHANGED,
	P_MSG_OPEN_PANEL,
	P_MSG_INSTALL
};

class PackageView : public BView {
	public:
		PackageView(BRect frame, const entry_ref *ref);
		~PackageView();

		void AttachedToWindow();
		void MessageReceived(BMessage *msg);

		int32 ItemExists(PackageItem &item, BPath &path, int32 &policy);

		BPath *GetCurrentPath()          { return &fCurrentPath; }
		PackageInfo *GetPackageInfo()    { return &fInfo; }
		uint32 GetCurrentType()          { return fCurrentType; }
		PackageStatus *GetStatusWindow() { return fStatusWindow; }

	private:
		void _InitView();
		void _InitProfiles();

		status_t _GroupChanged(int32 index);

		BPopUpMenu *fInstallTypes;
		BTextView *fInstallDesc;
		BPopUpMenu *fDestination;
		BMenuField *fDestField;
		BButton *fInstall;

		BFilePanel *fOpenPanel;
		BPath fCurrentPath;
		uint32 fCurrentType;

		PackageInfo fInfo;
		PackageStatus *fStatusWindow;
		PackageInstall fInstallProcess;
};

#endif	// PACKAGE_VIEW_H

