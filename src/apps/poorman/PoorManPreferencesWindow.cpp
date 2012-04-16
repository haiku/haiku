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
		| B_CLOSE_ON_ESCAPE),
	webDirFilePanel(NULL),
	logFilePanel(NULL)
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
	float buttonHeight = 26.0f;

	float widthCancel = prefView->StringWidth(B_TRANSLATE("Cancel")) + 24.0f;
	float widthDone = prefView->StringWidth(B_TRANSLATE("Done")) + 24.0f;

	float gap = 5.0f;

	BRect button1(prefView->Bounds().Width() - 2 * gap - widthCancel
		- widthDone, buttonTop, prefView->Bounds().Width() - 2 * gap - widthDone, 
		buttonTop + buttonHeight);
	cancelButton = new BButton(button1, "Cancel Button", B_TRANSLATE("Cancel"), 
		new BMessage(MSG_PREF_BTN_CANCEL));

	BRect button2(prefView->Bounds().Width() - gap - widthDone, buttonTop, 
		prefView->Bounds().Width() - gap, buttonTop + buttonHeight);
	doneButton = new BButton(button2, "Done Button", B_TRANSLATE("Done"), 
		new BMessage(MSG_PREF_BTN_DONE));

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

	BMessenger messenger(this);
	BMessage message(MSG_FILE_PANEL_SELECT_WEB_DIR);
	webDirFilePanel = new BFilePanel(B_OPEN_PANEL, &messenger, NULL,
		B_DIRECTORY_NODE, false, &message, NULL, true);

	webDirFilePanel->SetPanelDirectory(new BDirectory("/boot/home/public_html"));
	webDirFilePanel->SetButtonLabel(B_DEFAULT_BUTTON, B_TRANSLATE("Select"));
	change_title = webDirFilePanel->Window();
	change_title->SetTitle(STR_FILEPANEL_SELECT_WEB_DIR);

	message.what = MSG_FILE_PANEL_CREATE_LOG_FILE;
	logFilePanel = new BFilePanel(B_SAVE_PANEL, &messenger, NULL,
		B_FILE_NODE, false, &message);
	logFilePanel->SetButtonLabel(B_DEFAULT_BUTTON, B_TRANSLATE("Create"));
	change_title = logFilePanel->Window();
	change_title->SetTitle(STR_FILEPANEL_CREATE_LOG_FILE);
}


PoorManPreferencesWindow::~PoorManPreferencesWindow()
{
	delete logFilePanel;
	delete webDirFilePanel;
}


void
PoorManPreferencesWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PREF_BTN_DONE:
			PoorManWindow* win;
			PoorManServer* server;
			win = ((PoorManApplication *)be_app)->GetPoorManWindow();
			server = win->GetServer();
	
			PRINT(("Pref Window: sendDir CheckBox: %d\n",
				siteView->SendDirValue()));
			server->SetListDir(siteView->SendDirValue());
			win->SetDirListFlag(siteView->SendDirValue());
			PRINT(("Pref Window: indexFileName TextControl: %s\n",
				siteView->IndexFileName()));
			if (server->SetIndexName(siteView->IndexFileName()) == B_OK)
				win->SetIndexFileName(siteView->IndexFileName());
			PRINT(("Pref Window: webDir: %s\n", siteView->WebDir()));
			if (server->SetWebDir(siteView->WebDir()) == B_OK) {
				win->SetWebDir(siteView->WebDir());
				win->SetDirLabel(siteView->WebDir());
			}

			PRINT(("Pref Window: logConsole CheckBox: %d\n", 
				loggingView->LogConsoleValue()));
			win->SetLogConsoleFlag(loggingView->LogConsoleValue());
			PRINT(("Pref Window: logFile CheckBox: %d\n",
				loggingView->LogFileValue()));
			win->SetLogFileFlag(loggingView->LogFileValue());
			PRINT(("Pref Window: logFileName: %s\n",
				loggingView->LogFileName()));
			win->SetLogPath(loggingView->LogFileName());
	
			PRINT(("Pref Window: MaxConnections Slider: %ld\n", 
				advancedView->MaxSimultaneousConnections()));
			server->SetMaxConns(advancedView->MaxSimultaneousConnections());
			win->SetMaxConnections(
				(int16)advancedView->MaxSimultaneousConnections());
	
			if (Lock())
				Quit();
			break;
		case MSG_PREF_BTN_CANCEL:
			if (Lock())
				Quit();
			break;
		case MSG_PREF_SITE_BTN_SELECT:
			// Select the Web Directory, root directory to look in.
			webDirFilePanel->SetTarget(this);
			webDirFilePanel->SetMessage(new BMessage(MSG_FILE_PANEL_SELECT_WEB_DIR));
			if (!webDirFilePanel->IsShowing())
				webDirFilePanel->Show();
			break;
		case MSG_FILE_PANEL_SELECT_WEB_DIR:
			// handle the open BMessage from the Select Web Directory File Panel
			PRINT(("Select Web Directory:\n"));
			SelectWebDir(message);
			break;
		case MSG_PREF_LOG_BTN_CREATE_FILE:
			// Create the Log File
			logFilePanel->Show();
			break;
		case MSG_FILE_PANEL_CREATE_LOG_FILE:
			// handle the save BMessage from the Create Log File Panel
			PRINT(("Create Log File:\n"));
			CreateLogFile(message);
			break;
		case MSG_PREF_ADV_SLD_MAX_CONNECTION:
			max_connections = advancedView->MaxSimultaneousConnections();
			PRINT(("Max Connections: %ld\n", max_connections));
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

	if (message->FindRef("refs", &ref) != B_OK
		|| message->FindString("name", &name) != B_OK
		|| entry.SetTo(&ref) != B_OK) {
		return;
	}
	entry.GetPath(&path);

	PRINT(("DIR: %s\n", path.Path()));
	siteView->SetWebDir(path.Path());
	
	bool temp;
	if (message->FindBool("Default Dialog", &temp) == B_OK) {
		PoorManWindow* win = ((PoorManApplication *)be_app)->GetPoorManWindow();
		win->StartServer();
		if (win->GetServer()->SetWebDir(siteView->WebDir()) == B_OK) {
			win->SetWebDir(siteView->WebDir());
			win->SetDirLabel(siteView->WebDir());
			win->SaveSettings();
			win->Show();		
		}
		if (Lock())
			Quit();
	}
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
	PRINT(("Log File: %s\n", path.Path()));

	if (err == B_OK) {
		loggingView->SetLogFileName(path.Path());
		loggingView->SetLogFileValue(true);
	}

	// mark the checkbox
}


/*A special version for "the default dialog", don't use it in MessageReceived()*/
void
PoorManPreferencesWindow::ShowWebDirFilePanel()
{
	BMessage message(MSG_FILE_PANEL_SELECT_WEB_DIR);
	message.AddBool("Default Dialog", true);

	webDirFilePanel->SetTarget(be_app);
	webDirFilePanel->SetMessage(&message);
	if (!webDirFilePanel->IsShowing())
		webDirFilePanel->Show();
}
