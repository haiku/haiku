/*
 * Copyright 2009-2010, Philippe Houdoin, phoudoin@haiku-os.org. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <new>

#include <stdio.h>
#include <string.h>

#include <AppMisc.h>
#include <Bitmap.h>
#include <ColumnTypes.h>
#include <FindDirectory.h>
#include <MimeType.h>
#include <MessageRunner.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>

#include "TeamsListView.h"


// #pragma mark - BitmapStringField


BBitmapStringField::BBitmapStringField(BBitmap* bitmap, const char* string)
	:
	Inherited(string),
	fBitmap(bitmap)
{
}


BBitmapStringField::~BBitmapStringField()
{
	delete fBitmap;
}


void
BBitmapStringField::SetBitmap(BBitmap* bitmap)
{
	delete fBitmap;
	fBitmap = bitmap;
	// TODO: cause a redraw?
}


// #pragma mark - TeamsColumn


float TeamsColumn::sTextMargin = 0.0;


TeamsColumn::TeamsColumn(const char* title, float width, float minWidth,
		float maxWidth, uint32 truncateMode, alignment align)
	:
	Inherited(title, width, minWidth, maxWidth, align),
	fTruncateMode(truncateMode)
{
	SetWantsEvents(true);
}


void
TeamsColumn::DrawField(BField* field, BRect rect, BView* parent)
{
	BBitmapStringField* bitmapField
		= dynamic_cast<BBitmapStringField*>(field);
	BStringField* stringField = dynamic_cast<BStringField*>(field);

	if (bitmapField) {
		const BBitmap* bitmap = bitmapField->Bitmap();

		// figure out the placement
		float x = 0.0;
		BRect r = bitmap ? bitmap->Bounds() : BRect(0, 0, 15, 15);
		float y = rect.top + ((rect.Height() - r.Height()) / 2);
		float width = 0.0;

		switch (Alignment()) {
			default:
			case B_ALIGN_LEFT:
			case B_ALIGN_CENTER:
				x = rect.left + sTextMargin;
				width = rect.right - (x + r.Width()) - (2 * sTextMargin);
				r.Set(x + r.Width(), rect.top, rect.right - width, rect.bottom);
				break;

			case B_ALIGN_RIGHT:
				x = rect.right - sTextMargin - r.Width();
				width = (x - rect.left - (2 * sTextMargin));
				r.Set(rect.left, rect.top, rect.left + width, rect.bottom);
				break;
		}

		if (width != bitmapField->Width()) {
			BString truncatedString(bitmapField->String());
			parent->TruncateString(&truncatedString, fTruncateMode, width + 2);
			bitmapField->SetClippedString(truncatedString.String());
			bitmapField->SetWidth(width);
		}

		// draw the bitmap
		if (bitmap) {
			parent->SetDrawingMode(B_OP_ALPHA);
			parent->DrawBitmap(bitmap, BPoint(x, y));
			parent->SetDrawingMode(B_OP_OVER);
		}

		// draw the string
		DrawString(bitmapField->ClippedString(), parent, r);

	} else if (stringField) {

		float width = rect.Width() - (2 * sTextMargin);

		if (width != stringField->Width()) {
			BString truncatedString(stringField->String());

			parent->TruncateString(&truncatedString, fTruncateMode, width + 2);
			stringField->SetClippedString(truncatedString.String());
			stringField->SetWidth(width);
		}

		DrawString(stringField->ClippedString(), parent, rect);
	}
}


float
TeamsColumn::GetPreferredWidth(BField *_field, BView* parent) const
{
	BBitmapStringField* bitmapField
		= dynamic_cast<BBitmapStringField*>(_field);
	BStringField* stringField = dynamic_cast<BStringField*>(_field);

	float parentWidth = Inherited::GetPreferredWidth(_field, parent);
	float width = 0.0;

	if (bitmapField) {
		const BBitmap* bitmap = bitmapField->Bitmap();
		BFont font;
		parent->GetFont(&font);
		width = font.StringWidth(bitmapField->String()) + 3 * sTextMargin;
		if (bitmap)
			width += bitmap->Bounds().Width();
		else
			width += 16;
	} else if (stringField) {
		BFont font;
		parent->GetFont(&font);
		width = font.StringWidth(stringField->String()) + 2 * sTextMargin;
	}
	return max_c(width, parentWidth);
}


bool
TeamsColumn::AcceptsField(const BField* field) const
{
	return dynamic_cast<const BStringField*>(field) != NULL;
}


void
TeamsColumn::InitTextMargin(BView* parent)
{
	BFont font;
	parent->GetFont(&font);
	sTextMargin = ceilf(font.Size() * 0.8);
}


// #pragma mark - TeamRow


enum {
	kNameColumn,
	kIDColumn,
	kThreadCountColumn,
};


TeamRow::TeamRow(team_info& info)
	: BRow(20.0)
{
	_SetTo(info);
}


TeamRow::TeamRow(team_id team)
	: BRow(20.0)
{
	team_info info;
	get_team_info(team, &info);
	_SetTo(info);
}


status_t
TeamRow::_SetTo(team_info& info)
{
	team_info teamInfo = fTeamInfo = info;

	// strip any trailing space(s)...
	for (int len = strlen(teamInfo.args) - 1;
			len >= 0 && teamInfo.args[len] == ' '; len--) {
		teamInfo.args[len] = 0;
	}
	
	app_info appInfo;
	status_t status = be_roster->GetRunningAppInfo(teamInfo.team, &appInfo);
	if (status != B_OK) {
		// Not an application known to be_roster
		
		if (teamInfo.team == B_SYSTEM_TEAM) {
			// Get icon and name from kernel image
			system_info	systemInfo;
			get_system_info(&systemInfo);

			BPath kernelPath;
			find_directory(B_BEOS_SYSTEM_DIRECTORY, &kernelPath);
			kernelPath.Append(systemInfo.kernel_name);

			get_ref_for_path(kernelPath.Path(), &appInfo.ref);
		
		} else
			BPrivate::get_app_ref(teamInfo.team, &appInfo.ref);
	}

	BBitmap* icon = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_RGBA32);

	status = BNodeInfo::GetTrackerIcon(&appInfo.ref, icon, B_MINI_ICON);
	if (status != B_OK) {
			BMimeType genericAppType(B_APP_MIME_TYPE);
			status = genericAppType.GetIcon(icon, B_MINI_ICON);
	}

	if (status != B_OK) {
		delete icon;
		icon = NULL;
	}

	BString tmp;
	tmp << teamInfo.team;

	SetField(new BBitmapStringField(icon, teamInfo.args), kNameColumn);
	SetField(new BStringField(tmp), kIDColumn);

	tmp = "";
	tmp << teamInfo.thread_count;

	SetField(new BStringField(tmp), kThreadCountColumn);

	return status;
}


//	#pragma mark - TeamsListView


TeamsListView::TeamsListView(BRect frame, const char* name)
	:
	Inherited(frame, name, B_FOLLOW_ALL, 0, B_NO_BORDER, true),
	fUpdateRunner(NULL)
{
	AddColumn(new TeamsColumn("Name", 400, 100, 600,
		B_TRUNCATE_BEGINNING), kNameColumn);
	AddColumn(new TeamsColumn("ID", 80, 40, 100,
		B_TRUNCATE_MIDDLE, B_ALIGN_RIGHT), kIDColumn);

/*
	AddColumn(new TeamsColumn("Thread count", 100, 50, 500,
		B_TRUNCATE_MIDDLE, B_ALIGN_RIGHT), kThreadCountColumn);
*/
	SetSortingEnabled(false);

	team_info tmi;
	get_team_info(B_CURRENT_TEAM, &tmi);
	fThisTeam = tmi.team;
/*
#ifdef __HAIKU__
	SetFlags(Flags() | B_SUBPIXEL_PRECISE);
#endif
*/
}


TeamsListView::~TeamsListView()
{
	delete fUpdateRunner;
}


void
TeamsListView::AttachedToWindow()
{
	Inherited::AttachedToWindow();
	TeamsColumn::InitTextMargin(ScrollView());

	_InitList();

	be_roster->StartWatching(this, B_REQUEST_LAUNCHED | B_REQUEST_QUIT);

	BMessage msg(kMsgUpdateTeamsList);
	fUpdateRunner = new BMessageRunner(this, &msg, 100000L);	// 10Hz
}


void
TeamsListView::DetachedFromWindow()
{
	Inherited::DetachedFromWindow();

	be_roster->StopWatching(this);

	delete fUpdateRunner;
	fUpdateRunner = NULL;

	Clear(); // MakeEmpty();
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

			TeamRow* row = new(std::nothrow) TeamRow(team);
			if (row != NULL) {
				AddRow(row);
				/*else
					SortItems(&TeamListItem::Compare);
				*/
			}
			break;
		}

		case B_SOME_APP_QUIT:
		{
			team_id	team;
			if (message->FindInt32("be:team", &team) != B_OK)
				break;

			TeamRow* row = FindTeamRow(team);
			if (row != NULL) {
				RemoveRow(row);
				delete row;
			}
			break;
		}

		default:
			Inherited::MessageReceived(message);
	}
}


TeamRow*
TeamsListView::FindTeamRow(team_id teamId)
{
	for (int32 i = CountRows(); i-- > 0;) {
		TeamRow* row = dynamic_cast<TeamRow*>(RowAt(i));
		if (row == NULL)
			continue;

		if (row->TeamID() == teamId)
			return row;
	}

	return NULL;
}


void
TeamsListView::_InitList()
{
	int32 tmi_cookie = 0;
	team_info tmi;

	while (get_next_team_info(&tmi_cookie, &tmi) == B_OK) {
		TeamRow* row = new(std::nothrow)  TeamRow(tmi);
		if (row == NULL) {
			// Memory issue. Bail out.
			break;
		}

		if (tmi.team == B_SYSTEM_TEAM ||
			tmi.team == fThisTeam) {
			// We don't support debugging kernel and... ourself!
			row->SetEnabled(false);
		}

		AddRow(row);
	}
}


void
TeamsListView::_UpdateList()
{
	int32 tmi_cookie = 0;
	team_info tmi;
	TeamRow* row;
	int32 index = 0;

	// NOTA: assuming get_next_team_info() returns teams ordered by team ID...
	while (get_next_team_info(&tmi_cookie, &tmi) == B_OK) {

		row = dynamic_cast<TeamRow*>(RowAt(index));
		while (row && tmi.team > row->TeamID()) {
				RemoveRow(row);
				delete row;
				row = dynamic_cast<TeamRow*>(RowAt(index));
		}

		if (row == NULL || tmi.team != row->TeamID()) {
			// Team not found in previously known teams list: insert a new row
			TeamRow* newRow = new(std::nothrow) TeamRow(tmi);
			if (newRow != NULL) {
				if (row == NULL) {
					// No row found with bigger team id: append at list end
					AddRow(newRow);
				} else
					AddRow(newRow, index);
			}
		}
		index++;	// Move list sync head.
	}

	// Remove tail list rows, if we don't walk list thru the end
	while ((row = dynamic_cast<TeamRow*>(RowAt(index))) != NULL) {
		RemoveRow(row);
		delete row;
		row = dynamic_cast<TeamRow*>(RowAt(++index));
	}
}
