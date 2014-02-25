/*
 * Copyright 2007-2014, Haiku, Inc.
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

#include <View.h>

class BBox;
class BButton;
class BFilePanel;
class BMenu;
class BMenuField;
class BPopUpMenu;
class BTextView;


enum {
	P_MSG_INSTALL_TYPE_CHANGED = 'gpch',
	P_MSG_PATH_CHANGED,
	P_MSG_OPEN_PANEL,
	P_MSG_INSTALL
};

class PackageView : public BView {
public:
								PackageView(const entry_ref* ref);
	virtual						~PackageView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

			int32				ItemExists(PackageItem& item,
									BPath& path, int32& policy);

			BPath*				CurrentPath()
									{ return &fCurrentPath; }
			PackageInfo*		GetPackageInfo()
									{ return &fInfo; }
			uint32				CurrentType() const
									{ return fCurrentType; }
			PackageStatus*		StatusWindow()
									{ return fStatusWindow; }

private:
			void				_InitView();
			void				_InitProfiles();

			status_t			_InstallTypeChanged(int32 index);

			BString				_NamePlusSizeString(BString name,
									size_t size, const char* format) const;
			BMenuItem*			_AddInstallTypeMenuItem(BString baseName,
									size_t size, int32 index) const;
			BMenuItem*			_AddDestinationMenuItem(BString baseName,
									size_t size, const char* path) const;
			BMenuItem*			_AddMenuItem(const char* name,
									BMessage* message, BMenu* menu) const;

			bool				_ValidateFilePanelMessage(BMessage* message);

private:
			BPopUpMenu*			fInstallTypes;
			BTextView*			fInstallTypeDescriptionView;
			BPopUpMenu*			fDestination;
			BMenuField*			fDestField;
			BButton*			fBeginButton;

			BFilePanel*			fOpenPanel;
			bool				fExpectingOpenPanelResult;

			BPath				fCurrentPath;
			uint32				fCurrentType;

			PackageInfo			fInfo;
			PackageStatus*		fStatusWindow;
			PackageInstall		fInstallProcess;
};

#endif	// PACKAGE_VIEW_H

