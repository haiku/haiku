/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _InstallerApp_h
#define _InstallerApp_h

#include <Application.h>
#include "InstallerWindow.h"

class InstallerApp : public BApplication {
public:
	InstallerApp();

public:
	virtual void AboutRequested();
	virtual void ReadyToRun();
	
private:
	InstallerWindow *fWindow;
};

#endif /* _InstallerApp_h */
