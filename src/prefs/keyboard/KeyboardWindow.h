/*
** Copyright 2004, the Haiku project. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Author : mccall@digitalparadise.co.uk, Jérôme Duval
*/

#ifndef KEYBOARD_WINDOW_H
#define KEYBOARD_WINDOW_H

#include <Window.h>

#include "KeyboardSettings.h"
#include "KeyboardView.h"

class KeyboardWindow : public BWindow 
{
public:
	KeyboardWindow();
	
	bool QuitRequested();
	void MessageReceived(BMessage *message);
	
private:
	KeyboardView	*fView;
	
	KeyboardSettings	fSettings;
};

#endif
