/*
 * Copyright 2009, Philippe Houdoin, phoudoin@haiku-os.org. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "hdb.h"

#include "RunningTeamsWindow.h"
#include "TeamsListView.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <Application.h>
#include <ListView.h>
#include <ScrollView.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>

RunningTeamsWindow::RunningTeamsWindow()
    : BWindow(BRect(100, 100, 500, 250), "Running Teams", B_DOCUMENT_WINDOW,
        B_ASYNCHRONOUS_CONTROLS)
{
    BMessage settings;
    _LoadSettings(settings);

    BRect frame;
    if (settings.FindRect("running teams window frame", &frame) == B_OK) {
        MoveTo(frame.LeftTop());
        ResizeTo(frame.Width(), frame.Height());
    }

    // Add a teams list view
	frame = Bounds();
    frame.right -= B_V_SCROLL_BAR_WIDTH;

    fTeamsListView = new TeamsListView(frame, "RunningTeamsList");
    fTeamsListView->SetInvocationMessage(new BMessage(kMsgDebugThisTeam));

    BScrollView * teamsScroller = new BScrollView("RunningTeamsListScroller",
        fTeamsListView, B_FOLLOW_ALL_SIDES, 0, false, true, B_NO_BORDER);

	AddChild(teamsScroller);

	// small visual tweak
	if (BScrollBar* scrollBar = teamsScroller->ScrollBar(B_VERTICAL)) {
		scrollBar->MoveBy(0, -1);
		scrollBar->ResizeBy(0, -(B_H_SCROLL_BAR_HEIGHT - 2));
	}
}


RunningTeamsWindow::~RunningTeamsWindow()
{
}


void
RunningTeamsWindow::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case kMsgDebugThisTeam:
        {
            TeamListItem* item = dynamic_cast<TeamListItem*>(fTeamsListView->ItemAt(
                fTeamsListView->CurrentSelection()));

            if (item != NULL) {
                BMessage message(kMsgOpenTeamWindow);
                message.AddInt32("team_id", item->TeamID());
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
RunningTeamsWindow::QuitRequested()
{
    _SaveSettings();

    be_app_messenger.SendMessage(kMsgRunningTeamsWindowClosed);
    return true;
}


// #pragma mark --


status_t
RunningTeamsWindow::_OpenSettings(BFile& file, uint32 mode)
{
    BPath path;
    if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
        return B_ERROR;

    path.Append("Debugger settings");

    return file.SetTo(path.Path(), mode);
}


status_t
RunningTeamsWindow::_LoadSettings(BMessage& settings)
{
    BFile file;
    status_t status = _OpenSettings(file, B_READ_ONLY);
    if (status < B_OK)
        return status;

    return settings.Unflatten(&file);
}


status_t
RunningTeamsWindow::_SaveSettings()
{
    BFile file;
    status_t status = _OpenSettings(file,
        B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);

    if (status < B_OK)
        return status;

    BMessage settings('hdbg');
    status = settings.AddRect("running teams window frame", Frame());
    if (status != B_OK)
        return status;

    if (status == B_OK)
        status = settings.Flatten(&file);

    return status;
}
