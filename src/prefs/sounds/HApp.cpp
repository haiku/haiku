// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        HApp.cpp
//  Author:      Jérôme Duval, Oliver Ruiz Dorantes, Atsushi Takamatsu
//  Description: Sounds Preferences
//  Created :    November 24, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <Alert.h>
#include "HApp.h"
#include "HWindow.h"

#define APP_SIG "application/x-vnd.OBOS-Sounds"

/***********************************************************
 * Constructor
 ***********************************************************/
HApp::HApp() :BApplication(APP_SIG)
{
	BRect rect;
	rect.Set(200,150,500,450);
	
	HWindow *win = new HWindow(rect,"Sounds");
	win->Show();
}	

/***********************************************************
 * Destructor
 ***********************************************************/
HApp::~HApp()
{

}

/***********************************************************
 * AboutRequested
 ***********************************************************/
void
HApp::AboutRequested()
{
	(new BAlert("About Sounds", "Sounds\n"
			    "  Brought to you by :\n"
			    "	Oliver Ruiz Dorantes\n"
			    "	Jérôme DUVAL.\n"
			    "  Original work from Atsushi Takamatsu.\n"
			    "OpenBeOS, 2003","OK"))->Go();
}

int main()
{
	HApp app;
	app.Run();
	return 0;
}

