/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2005, Jérôme DUVAL
 *  All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef INSTALLER_WINDOW_H
#define INSTALLER_WINDOW_H

#include <String.h>
#include <Window.h>

namespace BPrivate {
	class PaneSwitch;
};
using namespace BPrivate;

class BButton;
class BLayoutItem;
class BLocker;
class BMenu;
class BMenuField;
class BStatusBar;
class BStringView;
class BTextView;
class CopyEngine;
class PackagesView;

enum InstallStatus {
	kReadyForInstall,
	kInstalling,
	kFinished,
	kCancelled
};

const uint32 STATUS_MESSAGE = 'iSTM';
const uint32 INSTALL_FINISHED = 'iIFN';
const uint32 RESET_INSTALL = 'iRSI';
const uint32 kWriteBootSector = 'iWBS';

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
			void				_UpdateControls();
			void				_PublishPackages();
			void				_SetStatusMessage(const char* text);

			void				_QuitCopyEngine(bool askUser);

	static	int					_ComparePackages(const void* firstArg,
									const void* secondArg);

			BTextView*			fStatusView;
			BMenu*				fSrcMenu;
			BMenu*				fDestMenu;
			BMenuField*			fSrcMenuField;
			BMenuField*			fDestMenuField;

			PaneSwitch*			fPackagesSwitch;
			PackagesView*		fPackagesView;
			BStringView*		fSizeView;

			BStatusBar*			fProgressBar;

			BLayoutItem*		fPkgSwitchLayoutItem;
			BLayoutItem*		fPackagesLayoutItem;
			BLayoutItem*		fSizeViewLayoutItem;
			BLayoutItem*		fProgressLayoutItem;

			BButton*			fBeginButton;
			BButton*			fSetupButton;
			BButton*			fMakeBootableButton;

			bool				fNeedsToCenterOnScreen;

			bool				fDriveSetupLaunched;
			InstallStatus		fInstallStatus;

			CopyEngine*			fCopyEngine;
			BString				fLastStatus;
			BLocker*			fCopyEngineLock;
};

#endif // INSTALLER_WINDOW_H
