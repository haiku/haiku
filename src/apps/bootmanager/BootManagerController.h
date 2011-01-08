/*
 * Copyright 2008-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */
#ifndef BOOT_MANAGER_CONTROLLER_H
#define BOOT_MANAGER_CONTROLLER_H


#include "WizardController.h"

#include <Message.h>

#include "BootMenu.h"


class BootDrive;
class WizardView;
class WizardPageView;


/*	Remainder of Settings Message Format:
	(See also BootDrive.h)
   	"install" bool (whether install or uninstall should be performed)
   	"file" String (the file where the backup of the MBR is saved)
   	"defaultPartition" int32 (index of default partition)
   	"timeout" int32 (timeout in seconds, -1 for no timeout)
*/

class BootManagerController : public WizardController {
public:
								BootManagerController();
	virtual						~BootManagerController();

	virtual	void				Previous(WizardView* wizard);

protected:
	virtual	int32				InitialState();
	virtual	int32				NextState(int32 state);
	virtual	WizardPageView*		CreatePage(int32 state, WizardView* wizard);

private:
	enum State {
		kStateEntry,
		kStateErrorEntry,

		// Install states
		kStateSaveMBR,
		kStateMBRSaved,
		kStatePartitions,
		kStateDefaultPartitions,
		kStateInstallSummary,
		kStateInstalled,

		// Uninstall states
		kStateUninstall,
		kStateUninstalled
	};

			bool				_HasSelectedPartitions();
			bool				_WriteBootMenu();
			bool				_SaveMBR();
			bool				_RestoreMBR();

			WizardPageView*		_CreateErrorEntryPage();
			WizardPageView*		_CreateSaveMBRPage(BRect frame);
			WizardPageView*		_CreateMBRSavedPage();
			WizardPageView*		_CreateInstallSummaryPage();
			WizardPageView*		_CreateInstalledPage();
			WizardPageView*		_CreateUninstallPage(BRect frame);
			WizardPageView*		_CreateUninstalledPage();

			BMessage			fSettings;
			BootMenuList		fBootMenus;
			BootDrive*			fBootDrive;
			BootMenu*			fBootMenu;

			status_t			fCollectPartitionsStatus;
			status_t			fWriteBootMenuStatus;
			status_t			fSaveMBRStatus;
			status_t			fRestoreMBRStatus;
};


#endif	// BOOT_MANAGER_CONTROLLER_H
