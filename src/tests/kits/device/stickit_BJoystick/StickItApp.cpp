//
// StickIt
// File: StickItApp.c
// Joystick application.
// Sampel code used in "Getting a Grip on BJoystick" by Eric Shepherd
//

#include "StickItWindow.h"
#include "StickItApp.h"

StickItApp::StickItApp() 
	:BApplication("application/x-vnd.Be-JoyDemoOne")
{
	fStickItWindow = new StickItWindow(BRect(50, 50, 500, 500));
	fStickItWindow->Show();	
}

int main(void) {
	StickItApp app;
	app.Run();
	return 0;
}
