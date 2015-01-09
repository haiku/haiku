/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Josef Gajdusek
 */

#ifndef EDITWINDOW_H
#define EDITWINDOW_H

#include <Window.h>

class BTextControl;

class EditWindow : public BWindow {
public:
						EditWindow(const char* placeholder, uint32 flags);

		void			MessageReceived(BMessage* message);
		BString			Go();

private:
	sem_id				fSem;
	BTextControl*		fTextControl;
};

#endif	// EDITWINDOW_H
