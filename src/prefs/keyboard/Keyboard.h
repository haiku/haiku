/*
** Copyright 2004, the Haiku project. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Author : mccall@digitalparadise.co.uk, Jérôme Duval
*/

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <Application.h>

class KeyboardApplication : public BApplication 
{
public:
	KeyboardApplication();
	
	void MessageReceived(BMessage *message);
		
	void AboutRequested(void);
private:
	static const char kKeyboardApplicationSig[];
};

#endif
