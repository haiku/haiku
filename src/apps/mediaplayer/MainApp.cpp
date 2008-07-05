/*
 * MainApp.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 * Copyright (C) 2008 Stephan Aßmus <superstippi@gmx.de>
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
#include <MediaRoster.h>
#include <Path.h>
#include <Roster.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "EventQueue.h"


MainApp *gMainApp;
const char* kAppSig = "application/x-vnd.Haiku-MediaPlayer";

static const char* kMediaServerSig = "application/x-vnd.Be.media-server";
static const char* kMediaServerAddOnSig = "application/x-vnd.Be.addon-host";


MainApp::MainApp()
	: BApplication(kAppSig),
	  fPlayerCount(0),
	  fFirstWindow(NewWindow()),

	  fMediaServerRunning(false),
	  fMediaAddOnServerRunning(false)
{
}


MainApp::~MainApp()
{
}


BWindow *
MainApp::NewWindow()
{
	BAutolock _(this);
	fPlayerCount++;
	return new MainWin();
}


void
MainApp::ReadyToRun()
{
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
MainApp::RefsReceived(BMessage *msg)
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
	
	entry_ref ref;
	for (int i = 0; B_OK == msg->FindRef("refs", i, &ref); i++) {
		BWindow *win;
		win = (i == 0 && IsLaunching()) ? fFirstWindow : NewWindow();
		BMessage m(B_REFS_RECEIVED);
		m.AddRef("refs", &ref);
		win->PostMessage(&m);
	}
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

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
MainApp::AboutRequested()
{
	fFirstWindow->PostMessage(B_ABOUT_REQUESTED);
}


void
MainApp::_BroadcastMessage(const BMessage& _message)
{
	for (int32 i = 0; BWindow* window = WindowAt(i); i++) {
		BMessage message(_message);
		window->PostMessage(&message);
	}
}


// #pragma mark -


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
