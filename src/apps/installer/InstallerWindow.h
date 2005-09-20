/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _InstallerWindow_h
#define _InstallerWindow_h

#include <Window.h>

class InstallerWindow : public BWindow {
public:
	InstallerWindow(BRect frameRect);
	virtual ~InstallerWindow();

	virtual void MessageReceived(BMessage *msg);
	virtual bool QuitRequested();
	
private:
};

#endif /* _InstallerWindow_h */
