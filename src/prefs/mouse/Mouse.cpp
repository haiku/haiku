/*
 * Mouse.cpp
 * Mouse mccall@digitalparadise.co.uk
 *
 */

#include <Alert.h>
#include <Screen.h>

#include "Mouse.h"
#include "MouseWindow.h"
#include "MouseSettings.h"
#include "MouseMessages.h"

const char MouseApplication::kMouseApplicationSig[] = "application/x-vnd.OpenBeOS-MOUS";

int main(int, char**)
{
	MouseApplication	myApplication;

	myApplication.Run();

	return(0);
}

MouseApplication::MouseApplication()
					:BApplication(kMouseApplicationSig)
{

	MouseWindow		*window;
	
	fSettings = new MouseSettings();
		
	window = new MouseWindow();

}

void
MouseApplication::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case ERROR_DETECTED:
			{
				BAlert *errorAlert = new BAlert("Error", "Something has gone wrong!","OK",NULL,NULL,B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
				errorAlert->Go();
				be_app->PostMessage(B_QUIT_REQUESTED);
			}
			break;			
		default:
			BApplication::MessageReceived(message);
			break;
	}
}

void
MouseApplication::SetWindowCorner(BPoint corner)
{
	fSettings->SetWindowCorner(corner);
}

void
MouseApplication::SetMouseType(mouse_type type)
{
	fSettings->SetMouseType(type);
}

void
MouseApplication::SetClickSpeed(bigtime_t click_speed)
{
	fSettings->SetClickSpeed(click_speed);
}

void
MouseApplication::SetMouseSpeed(int32 speed)
{
	fSettings->SetMouseSpeed(speed);
}

void
MouseApplication::AboutRequested(void)
{
	(new BAlert("about", "...by Andrew Edward McCall", "Dig Deal"))->Go();
}

MouseApplication::~MouseApplication()
{
	delete fSettings;
}