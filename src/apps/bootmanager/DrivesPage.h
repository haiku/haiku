/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DRIVES_PAGE_H
#define DRIVES_PAGE_H


#include "WizardPageView.h"


class BListView;
class BTextView;
class BScrollView;
class DriveItem;
class WizardView;


class DrivesPage : public WizardPageView {
public:
								DrivesPage(WizardView* wizardView,
									BMessage* settings, const char* name);
	virtual						~DrivesPage();

	virtual	void				PageCompleted();

protected:
			void				AttachedToWindow();
			void				MessageReceived(BMessage* message);

private:
			bool				_FillDrivesView();
			DriveItem*			_SelectedDriveItem();
			void				_UpdateWizardButtons(DriveItem* item);

private:
			WizardView*			fWizardView;
			BListView*			fDrivesView;
};


#endif	// DRIVES_PAGE_H
