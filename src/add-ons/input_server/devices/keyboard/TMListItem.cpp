// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004-2005, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        TMListItem.cpp
//  Author:      Jérôme Duval
//  Description: Keyboard input server addon
//  Created :    October 24, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <NodeInfo.h>
#include <View.h>

#include "TMListItem.h"

#include <string.h>


static const int32 kItemMargin = 2;


TMListItem::TMListItem(team_info &tinfo) 
	: BListItem(),
	fInfo(tinfo),
#ifdef __HAIKU__
	fIcon(BRect(0, 0, 15, 15), B_RGBA32),
	fLargeIcon(BRect(0, 0, 31, 31), B_RGBA32)
#else
	fIcon(BRect(0, 0, 15, 15), B_CMAP8),
	fLargeIcon(BRect(0, 0, 31, 31), B_CMAP8)
#endif
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
}


TMListItem::~TMListItem()
{
}


void
TMListItem::DrawItem(BView *owner, BRect frame, bool complete)
{
	rgb_color kHighlight = { 140,140,140,0 };
	rgb_color kBlack = { 0,0,0,0 };
	rgb_color kBlue = { 0,0,255,0 };

	BRect r(frame);

	if (IsSelected() || complete) {
		rgb_color color;
		if (IsSelected()) {
			color = kHighlight;
		} else {
			color = owner->ViewColor();
		}
		owner->SetHighColor(color);
		owner->SetLowColor(color);
		owner->FillRect(r);
		owner->SetHighColor(kBlack);
	} else {
		owner->SetLowColor(owner->ViewColor());
	}

	frame.left += 4;
	BRect iconFrame(frame);
	iconFrame.Set(iconFrame.left, iconFrame.top+1, iconFrame.left+15, iconFrame.top+16);
#ifdef __HAIKU__
	owner->SetDrawingMode(B_OP_ALPHA);
	owner->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
#else
	owner->SetDrawingMode(B_OP_OVER);
#endif
	owner->DrawBitmap(&fIcon, iconFrame);
	owner->SetDrawingMode(B_OP_COPY);

	frame.left += 16;
	owner->SetHighColor(IsSystemServer() ? kBlue : kBlack);

	BFont font = be_plain_font;
	font_height	finfo;
	font.GetHeight(&finfo);
	owner->SetFont(&font);
	owner->MovePenTo(frame.left+8, frame.top + ((frame.Height() - (finfo.ascent + finfo.descent + finfo.leading)) / 2) +
					(finfo.ascent + finfo.descent) - 1);
	owner->DrawString(fPath.Leaf());
}


/*static*/
int32
TMListItem::MinimalHeight()
{
	return 16 + kItemMargin;
}


void
TMListItem::Update(BView *owner, const BFont *font)
{
	// we need to override the update method so we can make sure
	// the list item size doesn't change
	BListItem::Update(owner, font);
	if (Height() < MinimalHeight())
		SetHeight(MinimalHeight());
}


const team_info *
TMListItem::GetInfo()
{
	return &fInfo;
}


bool
TMListItem::IsSystemServer()
{
	char *system = "/boot/beos/system/";
	if (strncmp(system, fInfo.args, strlen(system)) == 0)
		return true;

	system = "/system/servers/";
	if (strncmp(system, fInfo.args, strlen(system)) == 0)
		return true;

	return false;
}
