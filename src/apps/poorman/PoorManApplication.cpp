/* PoorManApplication.cpp
 *
 *	Philip Harrison
 *	Started: 4/25/2004
 *	Version: 0.1
 */


#include "PoorManApplication.h"

#include <Application.h>
#include <Alert.h>
#include <Catalog.h>
#include <Directory.h>

#include "constants.h"
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
	if (mainWindow->ReadSettings() != B_OK) {
		if (webDir.SetTo(STR_DEFAULT_WEB_DIRECTORY) != B_OK)
			mainWindow->DefaultSettings();
		else
			PostMessage(kStartServer);
	} else {
		if (webDir.SetTo(mainWindow->WebDir()) != B_OK)
			mainWindow->DefaultSettings();
		else
			PostMessage(kStartServer);
	}
}


void
PoorManApplication::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case MSG_FILE_PANEL_SELECT_WEB_DIR:
			mainWindow->MessageReceived(message);
			break;

		case kStartServer:
			mainWindow->StartServer();
			mainWindow->SetDirLabel(mainWindow->WebDir());
			mainWindow->Show();
			break;

		case B_CANCEL: {
			BDirectory webDir;
			if (mainWindow->ReadSettings() != B_OK) {
				if (webDir.SetTo(STR_DEFAULT_WEB_DIRECTORY) != B_OK)
					mainWindow->DefaultSettings();
				else
					mainWindow->StartServer();
			} else {
				if (webDir.SetTo(mainWindow->WebDir()) != B_OK)
					mainWindow->DefaultSettings();
				else
					mainWindow->StartServer();
			}
		}
			break;
		default:
			BApplication::MessageReceived(message);
	}
}

