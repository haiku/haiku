/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _THEMESAPP_H_
#define _THEMESAPP_H_

#include <Application.h>

class ThemesApp : public BApplication {
public:
	ThemesApp();
	virtual ~ThemesApp();
	void ReadyToRun();
	void MessageReceived(BMessage *message);

private:
};

#endif	// _THEMESAPP_H_
