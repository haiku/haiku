// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        HEventItem.cpp
//  Author:      Jérôme Duval, Oliver Ruiz Dorantes, Atsushi Takamatsu
//  Description: Sounds Preferences
//  Created :    November 24, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include "HEventItem.h"
#include <View.h>
#include <NodeInfo.h>
#include <Path.h>
#include <MediaFiles.h>
#include <Alert.h>
#include <Beep.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HEventItem::HEventItem(const char* name, const char* path)
	: BListItem(),
		fName(name)
{
	SetPath(path);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HEventItem::~HEventItem()
{
}

/***********************************************************
 * Set path
 ***********************************************************/
void
HEventItem::SetPath(const char* in_path)
{
	fPath = in_path;
}


/***********************************************************
 * Remove
 ***********************************************************/
void
HEventItem::Remove(const char *type)
{
	BMediaFiles().RemoveItem(type, Name());
}


/***********************************************************
 * DrawItem
 ***********************************************************/
void 	
HEventItem::DrawItem(BView *owner, BRect itemRect, bool complete)
{
	rgb_color kBlack = { 0,0,0,0 };
	rgb_color kHighlight = { 206,207,206,0 };
		
	if (IsSelected() || complete) {
		rgb_color color;
		if (IsSelected())
			color = kHighlight;
		else
			color = owner->ViewColor();
		
		owner->SetHighColor(color);
		owner->SetLowColor(color);
		owner->FillRect(itemRect);
		owner->SetHighColor(kBlack);
		
	} else {
		owner->SetLowColor(owner->ViewColor());
	}
	
	BPoint point = itemRect.LeftTop() + BPoint(5, 10);
	
	owner->SetHighColor(kBlack);
	owner->SetFont(be_plain_font);
	owner->MovePenTo(point);
	owner->DrawString(Name());
	
	point += BPoint(100, 0);
	BPath path(Path());
	owner->MovePenTo(point);
	owner->DrawString(path.Leaf() ? path.Leaf() : "<none>");
}
