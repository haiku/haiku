// Screen V1.00 by Rafael Romo for the OpenBeOS Preferences team.
// web.tiscalinet.it/rockman
// rockman@tiscalinet.it
// Modified by Stefano Ceccherini ( burton666@freemail.it ) 

#include <Application.h>
#include <Alert.h>

#include <cstdio>
#include <cstring>

#include "ScreenApplication.h"
#include "ScreenWindow.h"
#include "ScreenSettings.h"
#include "Constants.h"

ScreenApplication::ScreenApplication()
	:
		BApplication(kAppSignature),
		fScreenWindow(new ScreenWindow(new ScreenSettings()))
{
}


void
ScreenApplication::AboutRequested()
{
	BAlert *AboutAlert = new BAlert("About", "Screen by Rafael Romo, Stefano Ceccherini\nThe OBOS place to configure your monitor",
		"Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_INFO_ALERT);
	AboutAlert->SetShortcut(0, B_OK);
	AboutAlert->Go();
}


void
ScreenApplication::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
		case SET_INITIAL_MODE_MSG:
		{
			fScreenWindow->PostMessage(new BMessage(SET_INITIAL_MODE_MSG));
			
			break;
		}
		
		case SET_CUSTOM_REFRESH_MSG:
		{
			BMessage message(SET_CUSTOM_REFRESH_MSG);
			
			float Value;
		
			message.FindFloat("refresh", &Value);
		
			message.AddFloat("refresh", Value);
		
			fScreenWindow->PostMessage(&message);
			
			break;
		}
		
		case MAKE_INITIAL_MSG:
		{
			fScreenWindow->PostMessage(new BMessage(MAKE_INITIAL_MSG));
		
			break;
		}
	
		default:
			BApplication::MessageReceived(message);			
			break;
	}
}


int
main()
{
	ScreenApplication Application;
	
	Application.Run();
	
	return(0);
}
