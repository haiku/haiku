/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DRIVES_PAGE_H
#define DRIVES_PAGE_H


#include "BootMenu.h"
#include "WizardPageView.h"


class BListView;
class BTextView;
class BScrollView;
class DriveItem;
class WizardView;


class DrivesPage : public WizardPageView {
public:
								DrivesPage(WizardView* wizardView,
									const BootMenuList& menus,
									BMessage* settings, const char* name);
	virtual						~DrivesPage();

	virtual	void				PageCompleted();

protected:
			void				AttachedToWindow();
			void				MessageReceived(BMessage* message);

private:
			void				_FillDrivesView(const BootMenuList& menus);
			DriveItem*			_SelectedDriveItem();
			void				_UpdateWizardButtons(DriveItem* item);

private:
			WizardView*			fWizardView;
			BListView*			fDrivesView;
			bool				fHasInstallableItems;
};


#endif	// DRIVES_PAGE_H
