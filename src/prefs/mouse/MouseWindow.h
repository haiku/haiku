// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        MouseWindow.h
//  Author:      Jérôme Duval, Andrew McCall (mccall@digitalparadise.co.uk)
//  Description: Media Preferences
//  Created :   December 10, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
#ifndef MOUSE_WINDOW_H
#define MOUSE_WINDOW_H

#include <Window.h>

#include "MouseSettings.h"
#include "MouseView.h"

class MouseWindow : public BWindow 
{
public:
	MouseWindow(BRect rect);
	
	bool QuitRequested();
	void MessageReceived(BMessage *message);
		
	MouseSettings		fSettings;
private:
	MouseView	*fView;
};

#endif
