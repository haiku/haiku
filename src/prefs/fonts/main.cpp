/*! \file main.cpp
    \brief Code for the main class.
    
*/

#ifndef MAIN_WINDOW_H

	#include "MainWindow.h"
	
#endif
#ifndef MAIN_H

	#include "main.h"
	
#endif

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
			:BApplication("application/x-vnd.MSM-FontPrefPanel"){
	
	/*
	 * The main interface window.
	 */		
	MainWindow *Main;
	
	/*
	 * Sets the size for the main window.
	 */
	BRect MainWindowRect;
	
	MainWindowRect.Set(100, 80, 450, 350);
	Main = new MainWindow(MainWindowRect);
	
	Main->Show();

}

