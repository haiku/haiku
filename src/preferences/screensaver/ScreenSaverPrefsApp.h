/*
 * Copyright 2003, Michael Phipps. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _ScreenSaverPrefsApp_H
#define _ScreenSaverPrefsApp_H
#include "ScreenSaverWindow.h"

class ScreenSaverPrefsApp : public BApplication 
{
public:
	ScreenSaverPrefsApp();
	virtual void RefsReceived(BMessage *);
private:
	ScreenSaverWin *fScreenSaverWin;
};

#endif // _ScreenSaverPrefsApp_H
