/*
 * Copyright 2009-2010, Philippe Houdoin, phoudoin@haiku-os.org. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <new>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <Application.h>
#include <ListView.h>
#include <ScrollView.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>

#include "MessageCodes.h"
#include "SettingsManager.h"
#include "TeamsWindow.h"
#include "TeamsListView.h"


TeamsWindow::TeamsWindow(SettingsManager* settingsManager)
	:
	BWindow(BRect(100, 100, 500, 250), "Teams", B_DOCUMENT_WINDOW,
		B_ASYNCHRONOUS_CONTROLS),
	fTeamsListView(NULL),
	fSettingsManager(settingsManager)
{
}


TeamsWindow::~TeamsWindow()
{
}


/*static*/ TeamsWindow*
TeamsWindow::Create(SettingsManager* settingsManager)
{
	TeamsWindow* self = new TeamsWindow(settingsManager);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
TeamsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgDebugThisTeam:
		{
			TeamRow* row = dynamic_cast<TeamRow*>(fTeamsListView->CurrentSelection());
			if (row != NULL) {
				BMessage message(MSG_DEBUG_THIS_TEAM);
				message.AddInt32("team", row->TeamID());
				be_app_messenger.SendMessage(&message);
			}
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
TeamsWindow::QuitRequested()
{
	_SaveSettings();

	be_app_messenger.SendMessage(MSG_TEAMS_WINDOW_CLOSED);
	return true;
}


// #pragma mark --


void
TeamsWindow::_Init()
{
	BMessage settings;
	_LoadSettings(settings);

	BRect frame;
	if (settings.FindRect("teams window frame", &frame) == B_OK) {
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	}

	// TODO: add button to start a team debugger
	// TODO: add UI to setup arguments and environ, launch a program
	//       and start his team debugger

	// Add a teams list view
	frame = Bounds();
	frame.InsetBy(-1, -1);
	fTeamsListView = new TeamsListView(frame, "TeamsList");
	fTeamsListView->SetInvocationMessage(new BMessage(kMsgDebugThisTeam));

	AddChild(fTeamsListView);
}


status_t
TeamsWindow::_OpenSettings(BFile& file, uint32 mode)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append("Debugger settings");

	return file.SetTo(path.Path(), mode);
}


status_t
TeamsWindow::_LoadSettings(BMessage& settings)
{
	BFile file;
	status_t status = _OpenSettings(file, B_READ_ONLY);
	if (status < B_OK)
		return status;

	return settings.Unflatten(&file);
}


status_t
TeamsWindow::_SaveSettings()
{
	BFile file;
	status_t status = _OpenSettings(file,
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);

	if (status < B_OK)
		return status;

	BMessage settings('hdbg');
	status = settings.AddRect("teams window frame", Frame());
	if (status != B_OK)
		return status;

	if (status == B_OK)
		status = settings.Flatten(&file);

	return status;
}
