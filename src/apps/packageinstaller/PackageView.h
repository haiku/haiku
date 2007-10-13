/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#ifndef PACKAGEVIEW_H
#define PACKAGEVIEW_H

#include "PackageInfo.h"
#include "PackageStatus.h"
#include <View.h>
#include <Box.h>
#include <Button.h>
#include <MenuField.h>
#include <FilePanel.h>

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
		
		status_t Install();

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
};


#endif

