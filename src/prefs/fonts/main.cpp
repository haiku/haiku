/*! \file main.cpp
    \brief Code for the main class.
    
*/

#ifndef MAIN_WINDOW_H

	#include "MainWindow.h"
	
#endif
#ifndef MAIN_H

	#include "main.h"
	
#endif

#include "FontsSettings.h"

/**
 * Main method.
 * 
 * Starts the whole thing.
 */
int main(int, char**){

	/**
	 * An instance of the application.
	 */
	Font_pref fontApp;
	
	fontApp.Run();
	
	return(0);
	
}

/*
 * Constructor.
 * 
 * Provides a contstructor for the application.
 */
Font_pref::Font_pref()
	:BApplication("application/x-vnd.Haiku-FNPL") {
	
	f_settings = new FontsSettings();
	BRect MainWindowRect;
	MainWindowRect.Set(100, 80, 445, 343);
	window = new MainWindow(MainWindowRect);
	window->MoveTo(f_settings->WindowCorner());
}

Font_pref::~Font_pref()
{
	delete f_settings;
}


bool
Font_pref::QuitRequested()
{
	BMessage *message = CurrentMessage();
	BPoint point;
	if (message->FindPoint("corner", &point) == B_OK) {
		f_settings->SetWindowCorner(point);
	}
	
	return BApplication::QuitRequested();
}


void
Font_pref::ReadyToRun()
{
	window->Show();
}
