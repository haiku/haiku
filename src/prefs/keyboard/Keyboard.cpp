/*
 * Keyboard.cpp
 * Keyboard mccall@digitalparadise.co.uk
 *
 */

#include <Alert.h>
#include <Screen.h>

#include "Keyboard.h"
#include "KeyboardWindow.h"
#include "KeyboardSettings.h"
#include "KeyboardMessages.h"

const char KeyboardApplication::kKeyboardApplicationSig[] = "application/x-vnd.OpenBeOS-KYBD";

int main(int, char**)
{
	KeyboardApplication	myApplication;

	myApplication.Run();

	return(0);
}

KeyboardApplication::KeyboardApplication()
					:BApplication(kKeyboardApplicationSig)
{

	KeyboardWindow		*window;
	
	fSettings = new KeyboardSettings();
		
	window = new KeyboardWindow();

}

void
KeyboardApplication::MessageReceived(BMessage *message)
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
KeyboardApplication::SetWindowCorner(BPoint corner)
{
	fSettings->SetWindowCorner(corner);
}

void
KeyboardApplication::SetKeyboardRepeatRate(int32 rate)
{
	fSettings->SetKeyboardRepeatRate(rate);
}

void
KeyboardApplication::SetKeyboardRepeatDelay(int32 rate)
{
	fSettings->SetKeyboardRepeatDelay(rate);
}

void
KeyboardApplication::AboutRequested(void)
{
	(new BAlert("about", "...by Andrew Edward McCall", "Dig Deal"))->Go();
}

KeyboardApplication::~KeyboardApplication()
{
	delete fSettings;
}