// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        KeymapApplication.cpp
//  Author:      Sandor Vroemisse, Jérôme Duval
//  Description: Keymap Preferences
//  Created :    July 12, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include "KeymapApplication.h"

KeymapApplication::KeymapApplication()
	:	BApplication( APP_SIGNATURE )
{
	// create the window
	fWindow = new KeymapWindow();
	fWindow->Show();
}

void KeymapApplication::MessageReceived( BMessage * message )
{
	BApplication::MessageReceived( message );
}


int 
main ()
{
	new KeymapApplication;
	be_app->Run();
	delete be_app;
	return B_OK;
}
