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
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <View.h>


static const int32 kItemMargin = 2;


TeamListItem::TeamListItem(team_info &tinfo)
	:
	fInfo(tinfo),
	fIcon(BRect(0, 0, 15, 15), B_RGBA32),
	fLargeIcon(BRect(0, 0, 31, 31), B_RGBA32)
{
	int32 cookie = 0;
	image_info info;
	if (get_next_image_info(tinfo.team, &cookie, &info) == B_OK) {
		fPath = BPath(info.name);
		BNode node(info.name);
		BNodeInfo nodeInfo(&node);
		nodeInfo.GetTrackerIcon(&fIcon, B_MINI_ICON);
		nodeInfo.GetTrackerIcon(&fLargeIcon, B_LARGE_ICON);
	}

	app_info appInfo;
	if (be_roster->GetRunningAppInfo(fInfo.team, &appInfo) == B_OK)
		fAppSignature = appInfo.signature;
}


TeamListItem::~TeamListItem()
{
}


void
TeamListItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	rgb_color kHighlight = { 140, 140, 140, 0 };
	rgb_color kBlack = { 0, 0, 0, 0 };
	rgb_color kBlue = { 0, 0, 255, 0 };

	BRect r(frame);

	if (IsSelected() || complete) {
		rgb_color color;
		if (IsSelected())
			color = kHighlight;
		else
			color = owner->ViewColor();

		owner->SetHighColor(color);
		owner->SetLowColor(color);
		owner->FillRect(r);
		owner->SetHighColor(kBlack);
	} else {
		owner->SetLowColor(owner->ViewColor());
	}

	frame.left += 4;
	BRect iconFrame(frame);
	iconFrame.Set(iconFrame.left, iconFrame.top + 1, iconFrame.left + 15,
		iconFrame.top + 16);
	owner->SetDrawingMode(B_OP_ALPHA);
	owner->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	owner->DrawBitmap(&fIcon, iconFrame);
	owner->SetDrawingMode(B_OP_COPY);

	frame.left += 16;
	owner->SetHighColor(IsSystemServer() ? kBlue : kBlack);

	BFont font = be_plain_font;
	font_height	finfo;
	font.GetHeight(&finfo);
	owner->SetFont(&font);
	owner->MovePenTo(frame.left + 8, frame.top + ((frame.Height()
			- (finfo.ascent + finfo.descent + finfo.leading)) / 2)
		+ finfo.ascent);
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
	return &fInfo;
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
	
	if (strncmp(systemServersPath.Path(), fInfo.args,
			strlen(systemServersPath.Path())) == 0)
		return true;

	if (strncmp(trackerPath.Path(), fInfo.args, strlen(trackerPath.Path())) == 0)
		return true;

	if (strncmp(deskbarPath.Path(), fInfo.args, strlen(deskbarPath.Path())) == 0)
		return true;
	
	return false;		
}


bool
TeamListItem::IsApplication()
{
	if (fAppSignature.Length() > 0)
		return true;
	else
		return false;
}
