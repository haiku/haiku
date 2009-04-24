/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef INSTALLER_WINDOW_H
#define INSTALLER_WINDOW_H

#include <Box.h>
#include <Button.h>
#include <Menu.h>
#include <MenuField.h>
#include <ScrollView.h>
#include <String.h>
#include <TextView.h>
#include <Window.h>

#include "CopyEngine.h"
#include "PackageViews.h"

namespace BPrivate {
	class PaneSwitch;
};

class BLayoutItem;

enum InstallStatus {
	kReadyForInstall,
	kInstalling,
	kFinished,
	kCancelled
};

const uint32 STATUS_MESSAGE = 'iSTM';
const uint32 INSTALL_FINISHED = 'iIFN';
const uint32 RESET_INSTALL = 'iRSI';
const char PACKAGES_DIRECTORY[] = "_packages_";
const char VAR_DIRECTORY[] = "var";

class InstallerWindow : public BWindow {
public:
								InstallerWindow();
	virtual						~InstallerWindow();

	virtual	void				FrameResized(float width, float height);
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

			BMenu*				GetSourceMenu() { return fSrcMenu; };
			BMenu*				GetTargetMenu() { return fDestMenu; };
private:
			void				_ShowOptionalPackages();
			void				_LaunchDriveSetup();
			void				_DisableInterface(bool disable);
			void				_ScanPartitions();
			void				_UpdateMenus();
			void				_PublishPackages();
			void				_SetStatusMessage(const char* text);

	static	int					_ComparePackages(const void* firstArg,
									const void* secondArg);

			BButton*			fBeginButton;
			BButton*			fSetupButton;
			PaneSwitch*			fDrawButton;

			bool				fNeedsToCenterOnScreen;

			bool				fDriveSetupLaunched;
			InstallStatus		fInstallStatus;

			BTextView*			fStatusView;
			BMenu*				fSrcMenu;
			BMenu*				fDestMenu;
			BMenuField*			fSrcMenuField;
			BMenuField*			fDestMenuField;
			PackagesView*		fPackagesView;
			BStringView*		fSizeView;

			BLayoutItem*		fPackagesLayoutItem;
			BLayoutItem*		fSizeViewLayoutItem;

			CopyEngine*			fCopyEngine;
			BString				fLastStatus;
};

#endif // INSTALLER_WINDOW_H
