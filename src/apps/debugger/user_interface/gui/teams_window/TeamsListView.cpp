/*
 * Copyright 2009-2010, Philippe Houdoin, phoudoin@haiku-os.org. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <new>

#include <stdio.h>
#include <string.h>

#include <ListView.h>
#include <Bitmap.h>
#include <FindDirectory.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <MimeType.h>
#include <MessageRunner.h>
#include <String.h>

#include "TeamsListView.h"


TeamListItem::TeamListItem(team_info& info)
	:
	BStringItem("", false),
	fIcon(NULL)
{
	_SetTo(info);
}


TeamListItem::TeamListItem(team_id team)
	:
	BStringItem("", false),
	fIcon(NULL)
{
	team_info info;
	get_team_info(team, &info);
	_SetTo(info);
}


TeamListItem::~TeamListItem()
{
	delete fIcon;
}


void
TeamListItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	BRect rect = frame;

	if (fIcon) {
		rgb_color highColor = owner->HighColor();
		rgb_color lowColor = owner->LowColor();

		if (IsSelected() || complete) {
			// Draw the background...
			if (IsSelected())
				owner->SetLowColor(tint_color(lowColor, B_DARKEN_2_TINT));

			owner->FillRect(rect, B_SOLID_LOW);
		}

		BPoint point(rect.left + 2.0f,
		rect.top + (rect.Height() - B_MINI_ICON) / 2.0f);

		// Draw icon
		owner->SetDrawingMode(B_OP_ALPHA);
		owner->DrawBitmap(fIcon, point);
		owner->SetDrawingMode(B_OP_COPY);

		owner->MovePenTo(rect.left + B_MINI_ICON + 8.0f, frame.top + fBaselineOffset);

		if (!IsEnabled())
			owner->SetHighColor(tint_color(owner->HighColor(), B_LIGHTEN_2_TINT));
		else
			owner->SetHighColor(0, 0, 0);

		owner->DrawString(Text());

		owner->SetHighColor(highColor);
		owner->SetLowColor(lowColor);
	} else
		// No icon, fallback on plain StringItem...
		BStringItem::DrawItem(owner, rect, complete);
}


void
TeamListItem::Update(BView* owner, const BFont* font)
{
		BStringItem::Update(owner, font);

		if (Height() < B_MINI_ICON + 4.0f)
				SetHeight(B_MINI_ICON + 4.0f);

		font_height fontHeight;
		font->GetHeight(&fontHeight);

		fBaselineOffset = fontHeight.ascent +
			+ (Height() - ceilf(fontHeight.ascent + fontHeight.descent)) / 2.0f;
}


/* static */
int
TeamListItem::Compare(const void* a, const void* b)
{
	const BListItem*itemA = *static_cast<const BListItem* const *>(a);
	const BListItem*itemB = *static_cast<const BListItem* const *>(b);
	const TeamListItem*teamItemA = dynamic_cast<const TeamListItem*>(itemA);
	const TeamListItem*teamItemB = dynamic_cast<const TeamListItem*>(itemB);

	if (teamItemA != NULL && teamItemB != NULL) {
		return teamItemA->fTeamInfo.team - teamItemB->fTeamInfo.team;
	}

	return 0;
}


status_t
TeamListItem::_SetTo(team_info& info)
{
	BPath systemPath;
	team_info teamInfo = fTeamInfo = info;

	find_directory(B_BEOS_SYSTEM_DIRECTORY, &systemPath);

	// strip any trailing space(s)...
	for (int len = strlen(teamInfo.args) - 1;
			len >= 0 && teamInfo.args[len] == ' '; len--) {
		teamInfo.args[len] = 0;
	}

	app_info appInfo;
	status_t status = be_roster->GetRunningAppInfo(teamInfo.team, &appInfo);

	if (status == B_OK || teamInfo.team == B_SYSTEM_TEAM) {
		if (teamInfo.team == B_SYSTEM_TEAM) {
			// Get icon and name from kernel
			system_info	systemInfo;
			get_system_info(&systemInfo);

			BPath kernelPath(systemPath);
			kernelPath.Append(systemInfo.kernel_name);

			get_ref_for_path(kernelPath.Path(), &appInfo.ref);
		}
	} else {
		BEntry entry(teamInfo.args, true);
		entry.GetRef(&appInfo.ref);
	}

	SetText(teamInfo.args);

	fIcon = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_RGBA32);

	status = BNodeInfo::GetTrackerIcon(&appInfo.ref, fIcon, B_MINI_ICON);
	if (status != B_OK) {
			BMimeType genericAppType(B_APP_MIME_TYPE);
			status = genericAppType.GetIcon(fIcon, B_MINI_ICON);
	}

	if (status != B_OK) {
		delete fIcon;
		fIcon = NULL;
	}

	return status;
}

//	#pragma mark -


TeamsListView::TeamsListView(BRect frame, const char* name)
	:
	BListView(frame, name, B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL),
	fUpdateRunner(NULL)
{
	team_info tmi;
	get_team_info(B_CURRENT_TEAM, &tmi);
	fThisTeam = tmi.team;

#ifdef __HAIKU__
	SetFlags(Flags() | B_SUBPIXEL_PRECISE);
#endif
}


TeamsListView::~TeamsListView()
{
	delete fUpdateRunner;
}


void
TeamsListView::AttachedToWindow()
{
	BListView::AttachedToWindow();

	_InitList();

	be_roster->StartWatching(this, B_REQUEST_LAUNCHED | B_REQUEST_QUIT);

	BMessage msg(kMsgUpdateTeamsList);
	fUpdateRunner = new BMessageRunner(this, &msg, 100000L);	// 10Hz
}


void
TeamsListView::DetachedFromWindow()
{
	BListView::DetachedFromWindow();

	be_roster->StopWatching(this);

	delete fUpdateRunner;
	fUpdateRunner = NULL;

	// free all items, they will be retrieved again in AttachedToWindow()
	for (int32 i = CountItems(); i-- > 0;) {
		delete ItemAt(i);
	}
	MakeEmpty();
}


void
TeamsListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgUpdateTeamsList:
			_UpdateList();
			break;

		case B_SOME_APP_LAUNCHED:
		{
			team_id	team;
			if (message->FindInt32("be:team", &team) != B_OK)
				break;

			TeamListItem* item = new(std::nothrow) TeamListItem(team);
			if (item != NULL) {
				AddItem(item);
				SortItems(&TeamListItem::Compare);
			}
			break;
		}

		case B_SOME_APP_QUIT:
		{
			team_id	team;
			if (message->FindInt32("be:team", &team) != B_OK)
				break;

			TeamListItem* item = FindItem(team);
			if (item != NULL) {
				RemoveItem(item);
				delete item;
			}
			break;
		}

		default:
			BListView::MessageReceived(message);
	}
}


TeamListItem*
TeamsListView::FindItem(team_id teamId)
{
	for (int32 i = CountItems(); i-- > 0;) {
		TeamListItem* item = dynamic_cast<TeamListItem*>(ItemAt(i));
		if (item == NULL)
			continue;

		if (item->TeamID() == teamId)
			return item;
	}

	return NULL;
}


void
TeamsListView::_InitList()
{
	int32 tmi_cookie = 0;
	team_info tmi;

	while (get_next_team_info(&tmi_cookie, &tmi) == B_OK) {
		TeamListItem* item = new TeamListItem(tmi);

		if (tmi.team == B_SYSTEM_TEAM ||
			tmi.team == fThisTeam) {
			// We don't support debugging kernel and... ourself!
			item->SetEnabled(false);
		}

		AddItem(item);
	}

	// SortItems(&TeamListItem::Compare);
}


void
TeamsListView::_UpdateList()
{
	int32 tmi_cookie = 0;
	team_info tmi;
	TeamListItem* item;
	int32 index = 0;

	// NOTA: assuming get_next_team_info() returns teams ordered by team ID...
	while (get_next_team_info(&tmi_cookie, &tmi) == B_OK) {

		item = (TeamListItem*) ItemAt(index);
		while (item && tmi.team > item->TeamID()) {
				RemoveItem(item);
				delete item;
				item = (TeamListItem*) ItemAt(index);
		}

		if (!item || tmi.team != item->TeamID()) {
			// Team not found in previously known teams list: insert a new item
			TeamListItem* newItem = new(std::nothrow) TeamListItem(tmi);
			if (newItem != NULL) {
				if (!item) // No item found with bigger team id: append at end
					AddItem(newItem);
				else
					AddItem(newItem, index);
			}
		}
		index++;	// Move list sync head.
	}

	// Remove tail list items, if we don't walk list thru the end
	while ((item = (TeamListItem*) ItemAt(index)) != NULL) {
		RemoveItem(item);
		delete item;
		item = (TeamListItem*) ItemAt(++index);
	}
}
