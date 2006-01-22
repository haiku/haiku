/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _InstallerWindow_h
#define _InstallerWindow_h

#include <Box.h>
#include <Button.h>
#include <Menu.h>
#include <MenuField.h>
#include <ScrollView.h>
#include <TextView.h>
#include <Window.h>
#include "CopyEngine.h"
#include "DrawButton.h"
#include "PackageViews.h"

#define INSTALLER_RIGHT 402

const uint32 STATUS_MESSAGE = 'iSTM';

class InstallerWindow : public BWindow {
public:
	InstallerWindow(BRect frameRect);
	virtual ~InstallerWindow();

	virtual void MessageReceived(BMessage *msg);
	virtual bool QuitRequested();
	void SetStatusMessage(const char *text);
	BMenu *GetSourceMenu() { return fSrcMenu; };
	BMenu *GetTargetMenu() { return fDestMenu; };
private:
	void DisableInterface(bool disable);
	void LaunchDriveSetup();
	void PublishPackages();
	void ShowBottom();
	void StartScan();
	void AdjustMenus();
	static int ComparePackages(const void *firstArg, const void *secondArg);
	BBox *fBackBox;
	BButton *fBeginButton, *fSetupButton;
	DrawButton *fDrawButton;
	bool fDriveSetupLaunched;
	BTextView *fStatusView;
	BMenu* fSrcMenu, *fDestMenu;
	BMenuField* fSrcMenuField, *fDestMenuField;
	PackagesView *fPackagesView;
	BScrollView *fPackagesScrollView;
	BStringView *fSizeView;

	BBitmap *fLogo;
	BPoint fDrawPoint;
	CopyEngine *fCopyEngine;
};

#endif /* _InstallerWindow_h */
