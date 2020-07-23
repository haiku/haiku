/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef INSTALLER_APP_H
#define INSTALLER_APP_H


#include <Application.h>
#include <Catalog.h>

#include "EULAWindow.h"
#include "InstallerWindow.h"


class InstallerApp : public BApplication {
public:
								InstallerApp();

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				ReadyToRun();
	virtual	void				Quit();

private:
			EULAWindow*			fEULAWindow;
};

#endif // INSTALLER_APP_H
