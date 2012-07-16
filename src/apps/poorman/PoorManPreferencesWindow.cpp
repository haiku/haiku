/* PoorManPreferencesWindow.cpp
 *
 *	Philip Harrison
 *	Started: 4/27/2004
 *	Version: 0.1
 */

#include <Box.h>
#include <Catalog.h>
#include <Debug.h>
#include <Directory.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Window.h>

#include "constants.h"
#include "PoorManWindow.h"
#include "PoorManApplication.h"
#include "PoorManPreferencesWindow.h"
#include "PoorManServer.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PoorMan"


PoorManPreferencesWindow::PoorManPreferencesWindow(BRect frame, char * name)
	: BWindow(frame, name, B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE
		| B_CLOSE_ON_ESCAPE | B_AUTO_UPDATE_SIZE_LIMITS),
	fWebDirFilePanel(NULL),
	fLogFilePanel(NULL)
{
	fCancelButton = new BButton("Cancel Button", B_TRANSLATE("Cancel"),
		new BMessage(MSG_PREF_BTN_CANCEL));
	fDoneButton = new BButton("Done Button", B_TRANSLATE("Done"),
		new BMessage(MSG_PREF_BTN_DONE));

	fPrefTabView = new BTabView("Pref Tab View");

	// Site Tab
	fSiteTab = new BTab();
	fSiteView = new PoorManSiteView("Site View");
	fPrefTabView->AddTab(fSiteView, fSiteTab);
	fSiteTab->SetLabel(STR_TAB_SITE);

	// Logging Tab
	fLoggingTab = new BTab();
	fLoggingView = new PoorManLoggingView("Logging View");
	fPrefTabView->AddTab(fLoggingView, fLoggingTab);
	fLoggingTab->SetLabel(STR_TAB_LOGGING);

	// Advanced Tab
	fAdvancedTab = new BTab();
	fAdvancedView = new PoorManAdvancedView("Advanced View");
	fPrefTabView->AddTab(fAdvancedView, fAdvancedTab);
	fAdvancedTab->SetLabel(STR_TAB_ADVANCED);

	// FilePanels
	BWindow * change_title;

	BMessenger messenger(this);
	BMessage message(MSG_FILE_PANEL_SELECT_WEB_DIR);
	fWebDirFilePanel = new BFilePanel(B_OPEN_PANEL, &messenger, NULL,
		B_DIRECTORY_NODE, false, &message, NULL, true);

	fWebDirFilePanel->SetPanelDirectory(
		new BDirectory("/boot/home/public_html"));
	fWebDirFilePanel->SetButtonLabel(B_DEFAULT_BUTTON, B_TRANSLATE("Select"));
	change_title = fWebDirFilePanel->Window();
	change_title->SetTitle(STR_FILEPANEL_SELECT_WEB_DIR);

	message.what = MSG_FILE_PANEL_CREATE_LOG_FILE;
	fLogFilePanel = new BFilePanel(B_SAVE_PANEL, &messenger, NULL,
		B_FILE_NODE, false, &message);
	fLogFilePanel->SetButtonLabel(B_DEFAULT_BUTTON, B_TRANSLATE("Create"));
	change_title = fLogFilePanel->Window();
	change_title->SetTitle(STR_FILEPANEL_CREATE_LOG_FILE);
	

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(fPrefTabView)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fCancelButton)
			.Add(fDoneButton);
}


PoorManPreferencesWindow::~PoorManPreferencesWindow()
{
	delete fLogFilePanel;
	delete fWebDirFilePanel;
}


void
PoorManPreferencesWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PREF_BTN_DONE:
			PoorManWindow* win;
			PoorManServer* server;
			win = ((PoorManApplication*)be_app)->GetPoorManWindow();
			server = win->GetServer();
	
			PRINT(("Pref Window: sendDir CheckBox: %d\n",
				fSiteView->SendDirValue()));
			server->SetListDir(fSiteView->SendDirValue());
			win->SetDirListFlag(fSiteView->SendDirValue());
			PRINT(("Pref Window: indexFileName TextControl: %s\n",
				fSiteView->IndexFileName()));
			if (server->SetIndexName(fSiteView->IndexFileName()) == B_OK)
				win->SetIndexFileName(fSiteView->IndexFileName());
			PRINT(("Pref Window: webDir: %s\n", fSiteView->WebDir()));
			if (server->SetWebDir(fSiteView->WebDir()) == B_OK) {
				win->SetWebDir(fSiteView->WebDir());
				win->SetDirLabel(fSiteView->WebDir());
			}

			PRINT(("Pref Window: logConsole CheckBox: %d\n", 
				fLoggingView->LogConsoleValue()));
			win->SetLogConsoleFlag(fLoggingView->LogConsoleValue());
			PRINT(("Pref Window: logFile CheckBox: %d\n",
				fLoggingView->LogFileValue()));
			win->SetLogFileFlag(fLoggingView->LogFileValue());
			PRINT(("Pref Window: logFileName: %s\n",
				fLoggingView->LogFileName()));
			win->SetLogPath(fLoggingView->LogFileName());
	
			PRINT(("Pref Window: MaxConnections Slider: %ld\n", 
				fAdvancedView->MaxSimultaneousConnections()));
			server->SetMaxConns(fAdvancedView->MaxSimultaneousConnections());
			win->SetMaxConnections(
				(int16)fAdvancedView->MaxSimultaneousConnections());
	
			if (Lock())
				Quit();
			break;
		case MSG_PREF_BTN_CANCEL:
			if (Lock())
				Quit();
			break;
		case MSG_PREF_SITE_BTN_SELECT:
		{
			// Select the Web Directory, root directory to look in.
			fWebDirFilePanel->SetTarget(this);
			BMessage webDirSelectedMsg(MSG_FILE_PANEL_SELECT_WEB_DIR);
			fWebDirFilePanel->SetMessage(&webDirSelectedMsg);
			if (!fWebDirFilePanel->IsShowing())
				fWebDirFilePanel->Show();
			break;
		}
		case MSG_FILE_PANEL_SELECT_WEB_DIR:
			// handle the open BMessage from the Select Web Directory File Panel
			PRINT(("Select Web Directory:\n"));
			SelectWebDir(message);
			break;
		case MSG_PREF_LOG_BTN_CREATE_FILE:
			// Create the Log File
			fLogFilePanel->Show();
			break;
		case MSG_FILE_PANEL_CREATE_LOG_FILE:
			// handle the save BMessage from the Create Log File Panel
			PRINT(("Create Log File:\n"));
			CreateLogFile(message);
			break;
		case MSG_PREF_ADV_SLD_MAX_CONNECTION:
			fMaxConnections = fAdvancedView->MaxSimultaneousConnections();
			PRINT(("Max Connections: %ld\n", fMaxConnections));
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
PoorManPreferencesWindow::SelectWebDir(BMessage* message)
{
	entry_ref	ref;
	BPath		path;
	BEntry		entry;

	if (message->FindRef("refs", &ref) != B_OK || entry.SetTo(&ref) != B_OK) {
		return;
	}
	entry.GetPath(&path);

	PRINT(("DIR: %s\n", path.Path()));
	fSiteView->SetWebDir(path.Path());
	
	bool temp;
	if (message->FindBool("Default Dialog", &temp) == B_OK) {
		PoorManWindow* win = ((PoorManApplication *)be_app)->GetPoorManWindow();
		win->StartServer();
		if (win->GetServer()->SetWebDir(fSiteView->WebDir()) == B_OK) {
			win->SetWebDir(fSiteView->WebDir());
			win->SetDirLabel(fSiteView->WebDir());
			win->SaveSettings();
			win->Show();		
		}
		if (Lock())
			Quit();
	}
}


void
PoorManPreferencesWindow::CreateLogFile(BMessage* message)
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
	PRINT(("Log File: %s\n", path.Path()));

	if (err == B_OK) {
		fLoggingView->SetLogFileName(path.Path());
		fLoggingView->SetLogFileValue(true);
	}

	// mark the checkbox
}


/*A special version for "the default dialog", don't use it in MessageReceived()*/
void
PoorManPreferencesWindow::ShowWebDirFilePanel()
{
	BMessage message(MSG_FILE_PANEL_SELECT_WEB_DIR);
	message.AddBool("Default Dialog", true);

	fWebDirFilePanel->SetTarget(be_app);
	fWebDirFilePanel->SetMessage(&message);
	if (!fWebDirFilePanel->IsShowing())
		fWebDirFilePanel->Show();
}
