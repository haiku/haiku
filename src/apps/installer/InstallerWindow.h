/*
 * Copyright 2009-2010, Stephan Aßmus <superstippi@gmx.de>
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
class BGroupView;
class BMenu;
class BMenuField;
class BMenuItem;
class BStatusBar;
class BStringView;
class BTextView;
class PackagesView;
class WorkerThread;

enum InstallStatus {
	kReadyForInstall,
	kInstalling,
	kFinished,
	kCancelled
};


class InstallerWindow : public BWindow {
public:
								InstallerWindow();
	virtual						~InstallerWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();
private:
			void				_ShowOptionalPackages();
			void				_LaunchDriveSetup();
			void				_LaunchBootManager();
			void				_DisableInterface(bool disable);
			void				_ScanPartitions();
			void				_UpdateControls();
			void				_PublishPackages();
			void				_SetStatusMessage(const char* text);

			void				_SetCopyEngineCancelSemaphore(sem_id id,
									bool alreadyLocked = false);
			void				_QuitCopyEngine(bool askUser);

	static	int					_ComparePackages(const void* firstArg,
									const void* secondArg);

			BGroupView*			fLogoGroup;
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
			BButton*			fLaunchDriveSetupButton;
			BMenuItem*			fLaunchBootManagerItem;
			BMenuItem*			fMakeBootableItem;

			bool				fEncouragedToSetupPartitions;

			bool				fDriveSetupLaunched;
			bool				fBootManagerLaunched;
			InstallStatus		fInstallStatus;

			WorkerThread*		fWorkerThread;
			BString				fLastStatus;
			sem_id				fCopyEngineCancelSemaphore;
};


#endif // INSTALLER_WINDOW_H
