/*
 * Time.cpp
 * Time mccall@@digitalparadise.co.uk
 *
 */

#include <Alert.h>

#include "Time.h"
#include "TimeSettings.h"
#include "TimeMessages.h"


int main()
{
	new TimeApplication();

	be_app->Run();

	delete be_app;
	return(0);
}


TimeApplication::TimeApplication()
		:BApplication(HAIKU_APP_SIGNATURE)
{
	f_settings = new TimeSettings();
	f_window = new TTimeWindow();
}


TimeApplication::~TimeApplication()
{
	delete f_settings;
}


void
TimeApplication::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case ERROR_DETECTED:
			{
				(new BAlert("Error", "Something has gone wrong!","OK",NULL,NULL,B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT))->Go();
				be_app->PostMessage(B_QUIT_REQUESTED);
			}
			break;			
		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
TimeApplication::ReadyToRun(void)
{
	f_window->Show();
}


void
TimeApplication::AboutRequested(void)
{
	(new BAlert("about", "...by Andrew Edward McCall\n...Mike Berg too", "Big Deal"))->Go();
}


void
TimeApplication::SetWindowCorner(BPoint corner)
{
	f_settings->SetWindowCorner(corner);
}
