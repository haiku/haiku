/*
** Copyright 2004, the Haiku project. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Author : mccall@digitalparadise.co.uk, Jérôme Duval
*/

#include <Alert.h>

#include "Keyboard.h"
#include "KeyboardWindow.h"
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

	new KeyboardWindow();
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
KeyboardApplication::AboutRequested(void)
{
	(new BAlert("about", "...by Andrew Edward McCall", "Dig Deal"))->Go();
}
