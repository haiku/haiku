//--------------------------------------------------------------------
//	
//	MenuApp.cpp
//
//	Written by: Owen Smith
//	
//--------------------------------------------------------------------

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Alert.h>

#include "constants.h"
#include "MenuApp.h"
#include "MenuWindow.h"

//====================================================================
//	MenuApp Implementation



//--------------------------------------------------------------------
//	MenuApp constructors, destructors, operators

MenuApp::MenuApp()
	: BApplication(STR_APP_SIG)
{
	// empty
}



//--------------------------------------------------------------------
//	MenuApp virtual function overrides

void MenuApp::AboutRequested(void)
{
	BAlert* aboutBox = new BAlert(STR_ABOUT_TITLE,
		STR_ABOUT_DESC, STR_ABOUT_BUTTON);
	aboutBox->Go(); 
}

void MenuApp::ReadyToRun(void)
{
	MenuWindow* pWin = new MenuWindow(STR_APP_NAME);
	pWin->Show();
}
