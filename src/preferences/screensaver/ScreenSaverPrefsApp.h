/*
 * Copyright 2003-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *              Michael Phipps
 *              Jérôme Duval, jerome.duval@free.fr
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
