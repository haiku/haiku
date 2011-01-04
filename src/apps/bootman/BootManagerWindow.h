/*
 * Copyright 2008-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */
#ifndef BOOT_MANAGER_WINDOW_H
#define BOOT_MANAGER_WINDOW_H


#include <Window.h>

#include "BootManagerController.h"


class WizardView;


class BootManagerWindow : public BWindow {
public:
								BootManagerWindow();
	virtual						~BootManagerWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

private:
			BootManagerController fController;
			WizardView*			fWizardView;
};


#endif	// BOOT_MANAGER_WINDOW_H
