// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        Mouse.cpp
//  Author:      Jérôme Duval, Andrew McCall (mccall@digitalparadise.co.uk)
//  Description: Media Preferences
//  Created :    
//	Modified : December 10, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <Alert.h>
#include <Screen.h>

#include "Mouse.h"
#include "MouseWindow.h"

const char kMouseApplicationSig[] = "application/x-vnd.OpenBeOS-MOUS";

int main(int, char**)
{
	MouseApplication	myApplication;

	myApplication.Run();

	return(0);
}

MouseApplication::MouseApplication()
					:BApplication(kMouseApplicationSig)					
{
	BRect rect(0, 0, 397, 293);
	MouseWindow *window = new MouseWindow(rect);
	window->MoveTo(window->fSettings.WindowCorner());

	window->Show();
}


void
MouseApplication::AboutRequested(void)
{
	(new BAlert("about", "...by Andrew Edward McCall", "Dig Deal"))->Go();
}
