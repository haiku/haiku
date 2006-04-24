/*
 * MainApp.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
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
#include <Path.h>
#include <Entry.h>
#include <Alert.h>
#include <Autolock.h>
#include <unistd.h>
#include <stdio.h>

#include "MainApp.h"

MainApp *gMainApp;

MainApp::MainApp()
 :	BApplication("application/x-vnd.Haiku.MediaPlayer")
 ,	fFirstWindow(NewWindow())
 ,	fPlayerCount(1)
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
MainApp::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_PLAYER_QUIT:
			fPlayerCount--;
			if (fPlayerCount == 0)
				PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			BApplication::MessageReceived(msg);
			break;
	}
}


int 
main()
{
	gMainApp = new MainApp;
	gMainApp->Run();
	return 0;
}
