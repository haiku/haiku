/* PoorManApplication.cpp
 *
 *	Philip Harrison
 *	Started: 4/25/2004
 *	Version: 0.1
 */

#include <Application.h>
#include <Alert.h>
#include <Directory.h>

#include "constants.h"
#include "PoorManApplication.h"
#include "PoorManWindow.h"

PoorManApplication::PoorManApplication()
	: BApplication(STR_APP_SIG),
	  mainWindow(NULL)
{
	BRect mainRect(82.0f, 30.0f, 400.0f, 350.0f);
	mainWindow = new PoorManWindow(mainRect);
	mainWindow->Hide();
	mainWindow->Show();
	
	BDirectory webDir;
	if(mainWindow->ReadSettings() != B_OK){
		if(webDir.SetTo(STR_DEFAULT_WEB_DIRECTORY) != B_OK)
			mainWindow->DefaultSettings();
	} else {
		if(webDir.SetTo(mainWindow->WebDir()) != B_OK)
			mainWindow->DefaultSettings();
	}
	
	mainWindow->StartServer();
	mainWindow->SetDirLabel(mainWindow->WebDir());
	mainWindow->Show();
}

void 
PoorManApplication::AboutRequested()
{
	BAlert* aboutBox = new BAlert(STR_ABOUT_TITLE,
		STR_ABOUT_DESC, STR_ABOUT_BUTTON);
	aboutBox->Go(); 
}
