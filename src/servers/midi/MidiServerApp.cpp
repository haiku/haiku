/**
 * @file MidiServerApp.cpp
 *
 * @author Matthijs Hollemans
 */

#include <Alert.h>

#include "MidiServerApp.h"

//------------------------------------------------------------------------------

MidiServerApp::MidiServerApp()
	: BApplication("application/x-vnd.be-midi-roster")
{
	// Do nothing.
}

//------------------------------------------------------------------------------

void MidiServerApp::AboutRequested()
{
	(new BAlert(0,
		"-write something funny here-",
		"Okay", 0, 0, B_WIDTH_AS_USUAL, 
		B_INFO_ALERT))->Go();
}

//------------------------------------------------------------------------------

int main()
{
	MidiServerApp app;
	app.Run();
	return 0;
}

//------------------------------------------------------------------------------

