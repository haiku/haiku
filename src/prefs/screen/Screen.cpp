// Screen V0.80 by Rafael Romo for the OpenBeOS Preferences team.
// web.tiscalinet.it/rockman
// rockman@tiscalinet.it
// Additional code by Stefano Ceccherini ( burton666@freemail.it ) 

#include <Application.h>
#include <Alert.h>

#include <cstdio>
#include <cstring>

#include "ScreenApplication.h"
#include "ScreenWindow.h"
#include "ScreenSettings.h"
#include "Constants.h"

ScreenApplication::ScreenApplication()
	:	BApplication(kAppSignature),
		fScreenWindow(new ScreenWindow(new ScreenSettings()))
{
	fScreenWindow->Show();
}


void
ScreenApplication::AboutRequested()
{
	BAlert *aboutAlert = new BAlert("About", "Screen by Rafael Romo, Stefano Ceccherini\nThe OBOS place to configure your monitor",
		"Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_INFO_ALERT);
	aboutAlert->SetShortcut(0, B_OK);
	aboutAlert->Go();
}


void
ScreenApplication::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
		case SET_INITIAL_MODE_MSG:
		case SET_CUSTOM_REFRESH_MSG:
		case MAKE_INITIAL_MSG:
		{
			fScreenWindow->PostMessage(message);	
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
	ScreenApplication app;
	
	app.Run();
	
	return 0;
}
