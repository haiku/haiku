/*
 * Time.cpp
 * Time mccall@digitalparadise.co.uk
 *
 */

#include <Alert.h>
#include <Screen.h>

#include "Time.h"
#include "TimeWindow.h"
#include "TimeSettings.h"
#include "TimeMessages.h"

const char TimeApplication::kTimeApplicationSig[] = "application/x-vnd.OpenBeOS-TIME";

int main(int, char**)
{
	TimeApplication	myApplication;

	myApplication.Run();

	return(0);
}

TimeApplication::TimeApplication()
					:BApplication(kTimeApplicationSig)
{

	TimeWindow		*window;
	
	fSettings = new TimeSettings();
		
	window = new TimeWindow();

}

void
TimeApplication::MessageReceived(BMessage *message)
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
TimeApplication::SetWindowCorner(BPoint corner)
{
	fSettings->SetWindowCorner(corner);
}

void
TimeApplication::AboutRequested(void)
{
	(new BAlert("about", "...by Andrew Edward McCall", "Dig Deal"))->Go();
}

TimeApplication::~TimeApplication()
{
	delete fSettings;
}
