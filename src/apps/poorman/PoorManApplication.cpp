/* PoorManApplication.cpp
 *
 *	Philip Harrison
 *	Started: 4/25/2004
 *	Version: 0.1
 */

#include <Alert.h>

#include "constants.h"
#include "PoorManView.h"
#include "PoorManApplication.h"


PoorManApplication::PoorManApplication()
	: BApplication(STR_APP_SIG),
	  status(false), hits(0)
{
	// -------------------------------------------
	// Main Window
	//PoorManView		* mainView;
	BRect			  mainRect;
	
	mainRect.Set(30.0f, 30.0f, 355.0f, 185.0f);
	mainWindow = new PoorManWindow(mainRect);
	
	//mainRect.OffsetTo(B_ORIGIN);
	//mainView = new PoorManView(mainRect, STR_APP_NAME);
	//mainView->SetViewColor(216,216,216,255);
	
	mainWindow->Show();	
}

void 
PoorManApplication::AboutRequested()
{
	BAlert* aboutBox = new BAlert(STR_ABOUT_TITLE,
		STR_ABOUT_DESC, STR_ABOUT_BUTTON);
	aboutBox->Go(); 
}
