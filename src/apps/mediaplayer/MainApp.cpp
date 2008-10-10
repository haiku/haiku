/*
 * MainApp.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 * Copyright (C) 2008 Stephan Aßmus <superstippi@gmx.de> (MIT Ok)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#include "MainApp.h"

#include <Alert.h>
#include <Autolock.h>
#include <Entry.h>
#include <FilePanel.h>
#include <MediaRoster.h>
#include <Path.h>
#include <Roster.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "EventQueue.h"
#include "SettingsWindow.h"


MainApp* gMainApp;
const char* kAppSig = "application/x-vnd.Haiku-MediaPlayer";

static const char* kMediaServerSig = "application/x-vnd.Be.media-server";
static const char* kMediaServerAddOnSig = "application/x-vnd.Be.addon-host";


MainApp::MainApp()
	: BApplication(kAppSig),
	  fPlayerCount(0),
	  fFirstWindow(NULL),
	  fSettingsWindow(NULL),

	  fOpenFilePanel(NULL),
	  fSaveFilePanel(NULL),
	  fLastFilePanelFolder(),

	  fMediaServerRunning(false),
	  fMediaAddOnServerRunning(false)
{
}


MainApp::~MainApp()
{
	delete fOpenFilePanel;
	delete fSaveFilePanel;
}


bool
MainApp::QuitRequested()
{
	// Note: This needs to be done here, SettingsWindow::QuitRequested()
	// returns "false" always. (Standard BApplication quit procedure will
	// hang otherwise.)
	if (fSettingsWindow && fSettingsWindow->Lock())
		fSettingsWindow->Quit();
	fSettingsWindow = NULL;

	return BApplication::QuitRequested();
}


BWindow*
MainApp::FirstWindow()
{
	BAutolock _(this);
	if (fFirstWindow != NULL)
		return fFirstWindow;
	return NewWindow();
}


BWindow*
MainApp::NewWindow()
{
	BAutolock _(this);
	fPlayerCount++;
	BWindow* window = new MainWin();
	if (fFirstWindow == NULL)
		fFirstWindow = window;
	return window;
}


int32
MainApp::PlayerCount() const
{
	BAutolock _(const_cast<MainApp*>(this));
	return fPlayerCount;
}


// #pragma mark -


void
MainApp::ReadyToRun()
{
	// make sure we have at least one window open
	FirstWindow();

	// setup the settings window now, we need to have it
	fSettingsWindow = new SettingsWindow(BRect(150, 150, 450, 520));
	fSettingsWindow->Hide();
	fSettingsWindow->Show();

	// Now tell the application roster, that we're interested
	// in getting notifications of apps being launched or quit.
	// In this way we are going to detect a media_server restart.
	be_roster->StartWatching(BMessenger(this, this),
		B_REQUEST_LAUNCHED | B_REQUEST_QUIT);
	// we will keep track of the status of media_server
	// and media_addon_server
	fMediaServerRunning =  be_roster->IsRunning(kMediaServerSig);
	fMediaAddOnServerRunning = be_roster->IsRunning(kMediaServerAddOnSig);
}


void
MainApp::RefsReceived(BMessage* message)
{
	// The user dropped a file (or files) on this app's icon,
	// or double clicked a file that's handled by this app.
	// Command line arguments are also redirected to here by
	// ArgvReceived() but without MIME type check.
	// For each file we create a new window and send it a
	// B_REFS_RECEIVED message with a single file.
	// If IsLaunching() is true, we use fFirstWindow as first
	// window.
	printf("MainApp::RefsReceived\n");

	BWindow* window = NewWindow();
	if (window)
		window->PostMessage(message);
}


void
MainApp::ArgvReceived(int32 argc, char **argv)
{
	char cwd[B_PATH_NAME_LENGTH];
	getcwd(cwd, sizeof(cwd));

	BMessage m(B_REFS_RECEIVED);

	for (int i = 1; i < argc; i++) {
		printf("MainApp::ArgvReceived %s\n", argv[i]);
		BPath path;
		if (argv[i][0] != '/')
			path.SetTo(cwd, argv[i]);
		else
			path.SetTo(argv[i]);
		BEntry entry(path.Path(), true);
		if (!entry.Exists() || !entry.IsFile())
			continue;

		entry_ref ref;
		if (B_OK == entry.GetRef(&ref))
			m.AddRef("refs", &ref);
	}

	if (m.HasRef("refs")) {
		printf("MainApp::ArgvReceived calling RefsReceived\n");
		RefsReceived(&m);
	}
}


void
MainApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case M_PLAYER_QUIT:
			fPlayerCount--;
			if (fPlayerCount == 0)
				PostMessage(B_QUIT_REQUESTED);
			break;

		case B_SOME_APP_LAUNCHED:
		case B_SOME_APP_QUIT:
		{
			const char* mimeSig;
			if (message->FindString("be:signature", &mimeSig) < B_OK)
				break;

			bool isMediaServer = strcmp(mimeSig, kMediaServerSig) == 0;
			bool isAddonServer = strcmp(mimeSig, kMediaServerAddOnSig) == 0;
			if (!isMediaServer && !isAddonServer)
				break;

			bool running = (message->what == B_SOME_APP_LAUNCHED);
			if (isMediaServer)
				fMediaServerRunning = running;
			if (isAddonServer)
				fMediaAddOnServerRunning = running;

			if (!fMediaServerRunning && !fMediaAddOnServerRunning) {
				fprintf(stderr, "media server has quit.\n");
				// trigger closing of media nodes
				BMessage broadcast(M_MEDIA_SERVER_QUIT);
				_BroadcastMessage(broadcast);
			} else if (fMediaServerRunning && fMediaAddOnServerRunning) {
				fprintf(stderr, "media server has launched.\n");
				// HACK!
				// quit our now invalid instance of the media roster
				// so that before new nodes are created,
				// we get a new roster (it is a normal looper)
				// TODO: This functionality could become part of
				// BMediaRoster. It could detect the start/quit of
				// the servers like it is done here, and either quit
				// itself, or re-establish the connection, and send some
				// notification to the app... something along those lines.
				BMediaRoster* roster = BMediaRoster::CurrentRoster();
				if (roster) {
					roster->Lock();
					roster->Quit();
				}
				// give the servers some time to init...
				snooze(3000000);
				// trigger re-init of media nodes
				BMessage broadcast(M_MEDIA_SERVER_STARTED);
				_BroadcastMessage(broadcast);
			}
			break;
		}
		case M_SETTINGS:
			_ShowSettingsWindow();
			break;

		case M_SHOW_OPEN_PANEL:
			_ShowOpenFilePanel(message);
			break;
		case M_SHOW_SAVE_PANEL:
			_ShowSaveFilePanel(message);
			break;

		case M_OPEN_PANEL_RESULT:
			_HandleOpenPanelResult(message);
			break;
		case M_SAVE_PANEL_RESULT:
			_HandleSavePanelResult(message);
			break;

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
MainApp::AboutRequested()
{
	FirstWindow()->PostMessage(B_ABOUT_REQUESTED);
}


// #pragma mark -


void
MainApp::_BroadcastMessage(const BMessage& _message)
{
	for (int32 i = 0; BWindow* window = WindowAt(i); i++) {
		BMessage message(_message);
		window->PostMessage(&message);
	}
}


void
MainApp::_ShowSettingsWindow()
{
	BAutolock lock(fSettingsWindow);
	if (!lock.IsLocked())
		return;

	// If the window is already showing, don't jerk the workspaces around,
	// just pull it to the current one.
	uint32 workspace = 1UL << (uint32)current_workspace();
	uint32 windowWorkspaces = fSettingsWindow->Workspaces();
	if ((windowWorkspaces & workspace) == 0) {
		// window in a different workspace, reopen in current
		fSettingsWindow->SetWorkspaces(workspace);
	}

	if (fSettingsWindow->IsHidden())
		fSettingsWindow->Show();
	else
		fSettingsWindow->Activate();
}


// #pragma mark - file panels


void
MainApp::_ShowOpenFilePanel(const BMessage* message)
{
	if (fOpenFilePanel == NULL) {
		BMessenger target(this);
		fOpenFilePanel = new BFilePanel(B_OPEN_PANEL, &target);
	}

	_ShowFilePanel(fOpenFilePanel, M_OPEN_PANEL_RESULT, message,
		"Open", "Open");
}


void
MainApp::_ShowSaveFilePanel(const BMessage* message)
{
	if (fSaveFilePanel == NULL) {
		BMessenger target(this);
		fSaveFilePanel = new BFilePanel(B_SAVE_PANEL, &target);
	}

	_ShowFilePanel(fSaveFilePanel, M_SAVE_PANEL_RESULT, message,
		"Save", "Save");
}


void
MainApp::_ShowFilePanel(BFilePanel* panel, uint32 command,
	const BMessage* message, const char* defaultTitle,
	const char* defaultLabel)
{
//	printf("_ShowFilePanel()\n");
//	message->PrintToStream();

	BMessage panelMessage(command);

	if (message != NULL) {
		BMessage targetMessage;
		if (message->FindMessage("message", &targetMessage) == B_OK)
			panelMessage.AddMessage("message", &targetMessage);
	
		BMessenger target;
		if (message->FindMessenger("target", &target) == B_OK)
			panelMessage.AddMessenger("target", target);

		const char* panelTitle;
		if (message->FindString("title", &panelTitle) != B_OK)
			panelTitle = defaultTitle;
		{
			BString finalPanelTitle = "MediaPlayer: ";
			finalPanelTitle << panelTitle;
			BAutolock lock(panel->Window());
			panel->Window()->SetTitle(finalPanelTitle.String());
		}
		const char* buttonLabel;
		if (message->FindString("label", &buttonLabel) != B_OK)
			buttonLabel = defaultLabel;
		panel->SetButtonLabel(B_DEFAULT_BUTTON, buttonLabel);
	}

//	panelMessage.PrintToStream();
	panel->SetMessage(&panelMessage);

	if (fLastFilePanelFolder != entry_ref()) {
		panel->SetPanelDirectory(&fLastFilePanelFolder);
	}

	panel->Show();
}


void
MainApp::_HandleOpenPanelResult(const BMessage* message)
{
	_HandleFilePanelResult(fOpenFilePanel, message);
}


void
MainApp::_HandleSavePanelResult(const BMessage* message)
{
	_HandleFilePanelResult(fSaveFilePanel, message);
}


void
MainApp::_HandleFilePanelResult(BFilePanel* panel, const BMessage* message)
{
//	printf("_HandleFilePanelResult()\n");
//	message->PrintToStream();

	panel->GetPanelDirectory(&fLastFilePanelFolder);

	BMessage targetMessage;
	if (message->FindMessage("message", &targetMessage) != B_OK)
		targetMessage.what = message->what;

	BMessenger target;
	if (message->FindMessenger("target", &target) != B_OK) {
		if (targetMessage.what == M_OPEN_PANEL_RESULT
			|| targetMessage.what == M_SAVE_PANEL_RESULT) {
			// prevent endless message cycle
			return;
		}
		// send result message to ourselves
		target = BMessenger(this);
	}

	// copy the important contents of the message
	// save panel
	entry_ref directory;
	if (message->FindRef("directory", &directory) == B_OK)
		targetMessage.AddRef("directory", &directory);
	const char* name;
	if (message->FindString("name", &name) == B_OK)
		targetMessage.AddString("name", name);
	// open panel
	entry_ref ref;
	for (int32 i = 0; message->FindRef("refs", i, &ref) == B_OK; i++)
		targetMessage.AddRef("refs", &ref);

	target.SendMessage(&targetMessage);
}


// #pragma mark - main


int
main()
{
	EventQueue::CreateDefault();

	srand(system_time());

	gMainApp = new MainApp;
	gMainApp->Run();
	delete gMainApp;

	EventQueue::DeleteDefault();

	return 0;
}
