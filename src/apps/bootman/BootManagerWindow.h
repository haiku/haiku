/*
 * Copyright 2008, Michael Pfeiffer, laplace@users.sourceforge.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef BOOT_MANAGER_WINDOW_H
#define BOOT_MANAGER_WINDOW_H


#include <Window.h>

#include "BootManagerController.h"

class WizardView;

class BootManagerWindow : public BWindow
{
public:
	BootManagerWindow();
	virtual ~BootManagerWindow();

	virtual void MessageReceived(BMessage* message);
	virtual bool QuitRequested();
	
	virtual void WindowActivated(bool active);

private:

	void _CenterWindow();
	
	bool fWindowCentered;

	BootManagerController fController;	
	WizardView* fWizardView;
};

#endif	// BOOT_MANAGER_WINDOW_H
