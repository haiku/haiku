/*
 * Copyright 2004-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */

#include "TeamListItem.h"

#include <string.h>

#include <FindDirectory.h>
#include <LocaleRoster.h>
#include <NodeInfo.h>
#include <Path.h>
#include <View.h>


static const int32 kItemMargin = 2;


bool gLocalizedNamePreferred;


TeamListItem::TeamListItem(team_info &teamInfo)
	:
	fTeamInfo(teamInfo),
	fAppInfo(),
	fMiniIcon(BRect(0, 0, 15, 15), B_RGBA32),
	fLargeIcon(BRect(0, 0, 31, 31), B_RGBA32),
	fFound(false),
	fRefusingToQuit(false)
{
	int32 cookie = 0;
	image_info info;
	if (get_next_image_info(teamInfo.team, &cookie, &info) == B_OK) {
		fPath = BPath(info.name);
		BNode node(info.name);
		BNodeInfo nodeInfo(&node);
		nodeInfo.GetTrackerIcon(&fMiniIcon, B_MINI_ICON);
		nodeInfo.GetTrackerIcon(&fLargeIcon, B_LARGE_ICON);
	}

	if (be_roster->GetRunningAppInfo(fTeamInfo.team, &fAppInfo) != B_OK)
		fAppInfo.signature[0] = '\0';

	CacheLocalizedName();
}


TeamListItem::~TeamListItem()
{
}


void
TeamListItem::CacheLocalizedName()
{
	if (BLocaleRoster::Default()->GetLocalizedFileName(fLocalizedName,
			fAppInfo.ref, true) != B_OK)
		fLocalizedName = fPath.Leaf();
}


void
TeamListItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	rgb_color kHighlight = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
	rgb_color kHighlightText = ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR);
	rgb_color kText = ui_color(B_LIST_ITEM_TEXT_COLOR);

	rgb_color kIdealRed = { 255, 0, 0, 0 };
	rgb_color kIdealBlue = { 0, 0, 255, 0 };
	rgb_color kRed = mix_color(kIdealRed, kText, 191);
	rgb_color kBlue = mix_color(kIdealBlue, kText, 191);
	rgb_color kHighlightRed = mix_color(kIdealRed, kHighlightText, 191);
	rgb_color kHighlightBlue = mix_color(kIdealBlue, kHighlightText, 191);

	BRect r(frame);

	if (IsSelected() || complete) {
		owner->SetHighColor(kHighlight);
		owner->SetLowColor(kHighlight);
		owner->FillRect(r);
	}

	frame.left += 4;
	BRect iconFrame(frame);
	iconFrame.Set(iconFrame.left, iconFrame.top + 1, iconFrame.left + 15,
		iconFrame.top + 16);
	owner->SetDrawingMode(B_OP_ALPHA);
	owner->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	owner->DrawBitmap(&fMiniIcon, iconFrame);
	owner->SetDrawingMode(B_OP_COPY);

	frame.left += 16;
	if (fRefusingToQuit)
		owner->SetHighColor(IsSelected() ? kHighlightRed : kRed);
	else {
		if (IsSystemServer())
			owner->SetHighColor(IsSelected() ? kHighlightBlue : kBlue);
		else
			owner->SetHighColor(IsSelected() ? kHighlightText : kText);
	}
	BFont font = be_plain_font;
	font_height	finfo;
	font.GetHeight(&finfo);
	owner->SetFont(&font);
	owner->MovePenTo(frame.left + 8, frame.top + ((frame.Height()
			- (finfo.ascent + finfo.descent + finfo.leading)) / 2)
		+ finfo.ascent);

	if (gLocalizedNamePreferred)
		owner->DrawString(fLocalizedName.String());
	else
		owner->DrawString(fPath.Leaf());
}


/*static*/ int32
TeamListItem::MinimalHeight()
{
	return 16 + kItemMargin;
}


void
TeamListItem::Update(BView* owner, const BFont* font)
{
	// we need to override the update method so we can make sure
	// the list item size doesn't change
	BListItem::Update(owner, font);
	if (Height() < MinimalHeight())
		SetHeight(MinimalHeight());
}


const team_info*
TeamListItem::GetInfo()
{
	return &fTeamInfo;
}


bool
TeamListItem::IsSystemServer()
{
	static bool firstCall = true;
	static BPath systemServersPath;
	static BPath trackerPath;
	static BPath deskbarPath;

	if (firstCall) {
		find_directory(B_SYSTEM_SERVERS_DIRECTORY, &systemServersPath);

		find_directory(B_SYSTEM_DIRECTORY, &trackerPath);
		trackerPath.Append("Tracker");

		find_directory(B_SYSTEM_DIRECTORY, &deskbarPath);
		deskbarPath.Append("Deskbar");

		firstCall = false;
	}

	if (strncmp(systemServersPath.Path(), fTeamInfo.args,
			strlen(systemServersPath.Path())) == 0)
		return true;

	if (strncmp(trackerPath.Path(), fTeamInfo.args,
			strlen(trackerPath.Path())) == 0)
		return true;

	if (strncmp(deskbarPath.Path(), fTeamInfo.args,
			strlen(deskbarPath.Path())) == 0)
		return true;

	return false;
}


bool
TeamListItem::IsApplication() const
{
	return fAppInfo.signature[0] != '\0';
}


void
TeamListItem::SetRefusingToQuit(bool refusing)
{
	fRefusingToQuit = refusing;
}


bool
TeamListItem::IsRefusingToQuit()
{
	return fRefusingToQuit;
}
