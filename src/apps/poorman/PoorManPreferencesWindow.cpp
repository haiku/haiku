/* PoorManPreferencesWindow.cpp
 *
 *	Philip Harrison
 *	Started: 4/27/2004
 *	Version: 0.1
 */
 
#include <Window.h>
#include <Box.h>
#include <Directory.h>
#include <iostream>

#include "constants.h"
#include "PoorManWindow.h"
#include "PoorManApplication.h"
#include "PoorManPreferencesWindow.h"


PoorManPreferencesWindow::PoorManPreferencesWindow(BRect frame, char * name)
	: BWindow(frame, name, B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{

	frame = Bounds();
	
	prefView = new PoorManView(frame, STR_WIN_NAME_PREF);
	//prefView->SetViewColor(216,216,216,255);
	prefView->SetViewColor(BACKGROUND_COLOR);
	AddChild(prefView);
	
	
	
	// Button View
	BRect buttonRect;
	buttonRect = Bounds();
	buttonRect.top = buttonRect.bottom - 30;
	
	buttonView = new PoorManView(buttonRect, "Button View");
	buttonView->SetViewColor(BACKGROUND_COLOR);
	prefView->AddChild(buttonView);
	
	// Buttons 
	float buttonTop = 0.0f; 
	float buttonWidth = 52.0f;
	float buttonHeight = 26.0f;
	float buttonLeft = 265.0f;
   
	BRect button1;
	button1 = buttonView->Bounds();
	button1.Set(buttonLeft, buttonTop, buttonLeft + buttonWidth, buttonTop + buttonHeight); 
	cancelButton = new BButton(button1, "Cancel Button", "Cancel", new BMessage(MSG_PREF_BTN_CANCEL)); 
	
	buttonLeft = 325.0f;
	BRect button2;
	button2 = buttonView->Bounds();
	button2.Set(buttonLeft, buttonTop, buttonLeft + buttonWidth, buttonTop + buttonHeight); 
	doneButton = new BButton(button2, "Done Button", "Done", new BMessage(MSG_PREF_BTN_DONE)); 

	buttonView->AddChild(cancelButton); 
	buttonView->AddChild(doneButton); 
	
	
		
	// Create tabs
	BRect r;
	r = Bounds();
	//r.InsetBy(5, 5);
	r.top	 += 8.0;
	r.bottom -= 38.0;
	
	prefTabView = new BTabView(r, "Pref Tab View");
	prefTabView->SetViewColor(BACKGROUND_COLOR); 
	
	r = prefTabView->Bounds();
	r.InsetBy(5, 5);
	r.bottom -= prefTabView->TabHeight();

	// Site Tab
	siteTab = new BTab();
	siteView = new PoorManSiteView(r, "Site View");
	prefTabView->AddTab(siteView, siteTab);
	siteTab->SetLabel(STR_TAB_SITE);
	
	// Logging Tab
	loggingTab = new BTab();
	loggingView = new PoorManLoggingView(r, "Logging View");
	prefTabView->AddTab(loggingView, loggingTab);
	loggingTab->SetLabel(STR_TAB_LOGGING);
	
	// Advanced Tab
	advancedTab = new BTab();
	advancedView = new PoorManAdvancedView(r, "Advanced View");
	prefTabView->AddTab(advancedView, advancedTab);
	advancedTab->SetLabel(STR_TAB_ADVANCED);

	prefView->AddChild(prefTabView); 	
	
	// FilePanels
	BWindow * change_title;
	
	webDirFilePanel = new BFilePanel(
						B_OPEN_PANEL,
						new BMessenger(this),
						NULL,
						B_DIRECTORY_NODE,
						false,
						new BMessage(MSG_FILE_PANEL_SELECT_WEB_DIR)
						);
	webDirFilePanel->SetPanelDirectory(new BDirectory("/boot/home/public_html"));
	webDirFilePanel->SetButtonLabel(B_DEFAULT_BUTTON, "Select");
	change_title = webDirFilePanel->Window();
	change_title->SetTitle(STR_FILEPANEL_SELECT_WEB_DIR);


	logFilePanel = new BFilePanel(
						B_SAVE_PANEL,
						new BMessenger(this),
						NULL,
						B_FILE_NODE,
						false,
						new BMessage(MSG_FILE_PANEL_CREATE_LOG_FILE)
						);
	logFilePanel->SetButtonLabel(B_DEFAULT_BUTTON, "Create");
	change_title = logFilePanel->Window();
	change_title->SetTitle(STR_FILEPANEL_CREATE_LOG_FILE);
				
  
	Show();

}

void 
PoorManPreferencesWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case MSG_PREF_BTN_DONE:
			PoorManWindow		*	win;
			win = ((PoorManApplication *)be_app)->GetPoorManWindow();
			
			std::cout << "Pref Window: sendDir CheckBox: " << siteView->SendDirValue() << std::endl;
			win->SetDirListFlag(siteView->SendDirValue());
			std::cout << "Pref Window: indexFileName TextControl: " << siteView->IndexFileName() << std::endl;
			win->SetIndexFileName(siteView->IndexFileName());
			std::cout << "Pref Window: webDir: " << siteView->WebDir() << std::endl;
			win->SetWebDir(siteView->WebDir());
			win->SetDirLabel(siteView->WebDir());

			std::cout << "Pref Window: logConsole CheckBox: " << loggingView->LogConsoleValue() << std::endl;
			win->SetLogConsoleFlag(loggingView->LogConsoleValue());
			std::cout << "Pref Window: logFile CheckBox: " << loggingView->LogFileValue() << std::endl;
			win->SetLogFileFlag(loggingView->LogFileValue());
			std::cout << "Pref Window: logFileName: " << loggingView->LogFileName() << std::endl;
			win->SetLogPath(loggingView->LogFileName());
			
			std::cout << "Pref Window: MaxConnections Slider: " << advancedView->MaxSimultaneousConnections() << std::endl;
			win->SetMaxConnections(advancedView->MaxSimultaneousConnections());
			

			if (Lock())
				Quit();
		break;
	case MSG_PREF_BTN_CANCEL:
			if (Lock())
				Quit();
		break;
	case MSG_PREF_SITE_BTN_SELECT:
			// Select the Web Directory, root directory to look in.
			if (!webDirFilePanel->IsShowing())
				webDirFilePanel->Show();
		break;
	case MSG_FILE_PANEL_SELECT_WEB_DIR:
			// handle the open BMessage from the Select Web Directory File Panel
			std::cout << "Select Web Directory:\n" << std::endl;
			SelectWebDir(message);
		break;
	case MSG_PREF_LOG_BTN_CREATE_FILE:
			// Create the Log File
			logFilePanel->Show();
		break;
	case MSG_FILE_PANEL_CREATE_LOG_FILE:
			// handle the save BMessage from the Create Log File Panel
			std::cout << "Create Log File:\n" << std::endl;
			CreateLogFile(message);
		break;
	case MSG_PREF_ADV_SLD_MAX_CONNECTION:
			max_connections = advancedView->MaxSimultaneousConnections();
			std::cout << "Max Connections: " << max_connections << std::endl;	
		break;

	default:
		BWindow::MessageReceived(message);
		break;
	}
}

void 
PoorManPreferencesWindow::SelectWebDir(BMessage * message)
{
	entry_ref	ref;		
	const char	* name;
	BPath		path;
	BEntry		entry;
	status_t	err = B_OK;
	
	err = message->FindRef("refs", &ref) != B_OK;
	//if (err = message->FindRef("directory", &ref) != B_OK)
		//return err;
	err = message->FindString("name", &name) != B_OK;
	//if (err = message->FindString("name", &name) != B_OK)
	//	;//return err;
	err = entry.SetTo(&ref) != B_OK;
	//if (err = entry.SetTo(&ref) != B_OK)
	//	;//return err;
	entry.GetPath(&path);
	
	std::cout << "DIR: " << path.Path() << std::endl;
	siteView->SetWebDir(path.Path());
}

void 
PoorManPreferencesWindow::CreateLogFile(BMessage * message)
{
	entry_ref	ref;		
	const char	* name;
	BPath		path;
	BEntry		entry;
	status_t	err = B_OK;
	
	err = message->FindRef("directory", &ref) != B_OK;
	//if (err = message->FindRef("directory", &ref) != B_OK)
		//return err;
	err = message->FindString("name", &name) != B_OK;
	//if (err = message->FindString("name", &name) != B_OK)
	//	;//return err;
	err = entry.SetTo(&ref) != B_OK;
	//if (err = entry.SetTo(&ref) != B_OK)
	//	;//return err;
	entry.GetPath(&path);
	path.Append(name);
	std::cout << "Log File: " << path.Path() << std::endl;
	
	if (err == B_OK)
	{
		loggingView->SetLogFileName(path.Path());
		loggingView->SetLogFileValue(true);
	}
	
	// mark the checkbox
}
