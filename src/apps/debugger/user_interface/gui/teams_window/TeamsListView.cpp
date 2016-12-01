/*
 * Copyright 2009-2010, Philippe Houdoin, phoudoin@haiku-os.org. All rights reserved.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "TeamsListView.h"

#include <algorithm>
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

#include <AutoLocker.h>

#include "TargetHostInterface.h"


enum {
	MSG_SELECTED_INTERFACE_CHANGED = 'seic',
	MSG_TEAM_ADDED = 'tead',
	MSG_TEAM_REMOVED = 'tere',
	MSG_TEAM_RENAMED = 'tern'
};


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
	kIDColumn
};


TeamRow::TeamRow(TeamInfo* info)
	: BRow(std::max(20.0f, ceilf(be_plain_font->Size() * 1.4)))
{
	_SetTo(info);
}


bool
TeamRow::NeedsUpdate(TeamInfo* info)
{
	// Check if we need to rebuilt the row's fields because the team critical
	// info (basically, app image running under that team ID) has changed

	if (info->Arguments() != fTeamInfo.Arguments()) {
		_SetTo(info);
		return true;
	}

	return false;
}


status_t
TeamRow::_SetTo(TeamInfo* info)
{
	fTeamInfo = *info;

	app_info appInfo;
	status_t status = be_roster->GetRunningAppInfo(fTeamInfo.TeamID(),
		&appInfo);
	if (status != B_OK) {
		// Not an application known to be_roster

		if (fTeamInfo.TeamID() == B_SYSTEM_TEAM) {
			// Get icon and name from kernel image
			system_info	systemInfo;
			get_system_info(&systemInfo);

			BPath kernelPath;
			find_directory(B_BEOS_SYSTEM_DIRECTORY, &kernelPath);
			kernelPath.Append(systemInfo.kernel_name);

			get_ref_for_path(kernelPath.Path(), &appInfo.ref);

		} else
			BPrivate::get_app_ref(fTeamInfo.TeamID(), &appInfo.ref);
	}

	BBitmap* icon = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1),
		B_RGBA32);

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
	tmp << fTeamInfo.TeamID();

	SetField(new BBitmapStringField(icon, fTeamInfo.Arguments()), kNameColumn);
	SetField(new BStringField(tmp), kIDColumn);

	return status;
}


//	#pragma mark - TeamsListView


TeamsListView::TeamsListView(const char* name)
	:
	Inherited(name, B_NAVIGABLE, B_PLAIN_BORDER),
	TargetHost::Listener(),
	TeamsWindow::Listener(),
	fInterface(NULL)
{
	AddColumn(new TeamsColumn("Name", 400, 100, 600,
		B_TRUNCATE_BEGINNING), kNameColumn);
	AddColumn(new TeamsColumn("ID", 80, 40, 100,
		B_TRUNCATE_MIDDLE, B_ALIGN_RIGHT), kIDColumn);
	SetSortingEnabled(false);
}


TeamsListView::~TeamsListView()
{
}


void
TeamsListView::AttachedToWindow()
{
	Inherited::AttachedToWindow();
	TeamsColumn::InitTextMargin(ScrollView());
}


void
TeamsListView::DetachedFromWindow()
{
	Inherited::DetachedFromWindow();
	_SetInterface(NULL);
}


void
TeamsListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SELECTED_INTERFACE_CHANGED:
		{
			TargetHostInterface* interface;
			if (message->FindPointer("interface", reinterpret_cast<void**>(
					&interface)) == B_OK) {
				_SetInterface(interface);
			}
			break;
		}

		case MSG_TEAM_ADDED:
		{
			TeamInfo* info;
			team_id team;
			if (message->FindInt32("team", &team) != B_OK)
				break;

			TargetHost* host = fInterface->GetTargetHost();
			AutoLocker<TargetHost> hostLocker(host);
			info = host->TeamInfoByID(team);
			if (info == NULL)
				break;

			TeamRow* row = new TeamRow(info);
			AddRow(row);
			break;
		}

		case MSG_TEAM_REMOVED:
		{
			team_id team;
			if (message->FindInt32("team", &team) != B_OK)
				break;

			TeamRow* row = FindTeamRow(team);
			if (row != NULL) {
				RemoveRow(row);
				delete row;
			}
			break;
		}

		case MSG_TEAM_RENAMED:
		{
			TeamInfo* info;
			team_id team;
			if (message->FindInt32("team", &team) != B_OK)
				break;

			TargetHost* host = fInterface->GetTargetHost();
			AutoLocker<TargetHost> hostLocker(host);
			info = host->TeamInfoByID(team);
			if (info == NULL)
				break;

			TeamRow* row = FindTeamRow(info->TeamID());
			if (row != NULL && row->NeedsUpdate(info))
				UpdateRow(row);

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
TeamsListView::TeamAdded(TeamInfo* info)
{
	BMessage message(MSG_TEAM_ADDED);
	message.AddInt32("team", info->TeamID());
	BMessenger(this).SendMessage(&message);
}


void
TeamsListView::TeamRemoved(team_id team)
{
	BMessage message(MSG_TEAM_REMOVED);
	message.AddInt32("team", team);
	BMessenger(this).SendMessage(&message);
}


void
TeamsListView::TeamRenamed(TeamInfo* info)
{
	BMessage message(MSG_TEAM_RENAMED);
	message.AddInt32("team", info->TeamID());
	BMessenger(this).SendMessage(&message);
}


void
TeamsListView::SelectedInterfaceChanged(TargetHostInterface* interface)
{
	BMessage message(MSG_SELECTED_INTERFACE_CHANGED);
	message.AddPointer("interface", interface);
	BMessenger(this).SendMessage(&message);
}


void
TeamsListView::_InitList()
{
	TargetHost* host = fInterface->GetTargetHost();
	AutoLocker<TargetHost> hostLocker(host);
	for (int32 i = 0; i < host->CountTeams(); i++) {
		TeamInfo* info = host->TeamInfoAt(i);
		BRow* row = new TeamRow(info);
		AddRow(row);
	}
}


void
TeamsListView::_SetInterface(TargetHostInterface* interface)
{
	if (interface == fInterface)
		return;

	if (fInterface != NULL) {
		Clear();
		fInterface->GetTargetHost()->RemoveListener(this);
	}

	fInterface = interface;
	if (fInterface == NULL)
		return;

	fInterface->GetTargetHost()->AddListener(this);
	_InitList();
}
