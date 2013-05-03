/*
 * Copyright 2009-2010, Philippe Houdoin, phoudoin@haiku-os.org. All rights reserved.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include <new>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <Application.h>
#include <Button.h>
#include <File.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <Path.h>
#include <ScrollView.h>

#include "MessageCodes.h"
#include "SettingsManager.h"
#include "TeamsWindow.h"
#include "TeamsListView.h"


enum {
	MSG_CREATE_NEW_TEAM = 'crnt',
	MSG_TEAM_SELECTION_CHANGED = 'tesc'
};


TeamsWindow::TeamsWindow(SettingsManager* settingsManager)
	:
	BWindow(BRect(100, 100, 500, 250), "Teams", B_DOCUMENT_WINDOW,
		B_ASYNCHRONOUS_CONTROLS),
	fTeamsListView(NULL),
	fAttachTeamButton(NULL),
	fCreateTeamButton(NULL),
	fSettingsManager(settingsManager)
{
	team_info info;
	get_team_info(B_CURRENT_TEAM, &info);
	fCurrentTeam = info.team;
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
		case MSG_CREATE_NEW_TEAM:
		{
			// TODO: implement
			break;
		}

		case MSG_DEBUG_THIS_TEAM:
		{
			TeamRow* row = dynamic_cast<TeamRow*>(
				fTeamsListView->CurrentSelection());
			if (row != NULL) {
				BMessage message(MSG_DEBUG_THIS_TEAM);
				message.AddInt32("team", row->TeamID());
				be_app_messenger.SendMessage(&message);
			}
			break;
		}

		case MSG_TEAM_SELECTION_CHANGED:
		{
			TeamRow* row = dynamic_cast<TeamRow*>(
				fTeamsListView->CurrentSelection());
			bool enabled = false;
			if (row != NULL) {
				team_id id = row->TeamID();
				if (id != fCurrentTeam && id != B_SYSTEM_TEAM)
					enabled = true;
			}

			fAttachTeamButton->SetEnabled(enabled);
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

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fTeamsListView = new TeamsListView("TeamsList", fCurrentTeam))
		.SetInsets(1.0f, 1.0f, 1.0f, 1.0f)
		.AddGroup(B_HORIZONTAL, 4.0f)
			.Add(fAttachTeamButton = new BButton("Attach"))
			.Add(fCreateTeamButton = new BButton("Create new team"
					B_UTF8_ELLIPSIS))
			.End()
		.End();

	fTeamsListView->SetInvocationMessage(new BMessage(MSG_DEBUG_THIS_TEAM));
	fTeamsListView->SetSelectionMessage(new BMessage(
			MSG_TEAM_SELECTION_CHANGED));

	fAttachTeamButton->SetMessage(new BMessage(MSG_DEBUG_THIS_TEAM));
	fAttachTeamButton->SetEnabled(false);
	fCreateTeamButton->SetMessage(new BMessage(MSG_CREATE_NEW_TEAM));
	// TODO: re-enable once action is implemented
	fAttachTeamButton->SetEnabled(false);
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
