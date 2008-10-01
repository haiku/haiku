/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ThemeItem.h"
#include <View.h>
#include <Font.h>
#include <stdio.h>


// #pragma mark - 


ThemeItem::ThemeItem(int32 id, const char *name, bool ro)
	: BStringItem(name)
{
	fCurrent = false;
	fId = id;
	fRo = ro;
}


ThemeItem::~ThemeItem()
{
}


void
ThemeItem::DrawItem(BView *owner, BRect frame, bool complete)
{
	rgb_color col;
	if (fCurrent || fRo)
		owner->PushState();
	if (fCurrent) {
		BFont f;
		owner->GetFont(&f);
		f.SetFace(B_BOLD_FACE);
		owner->SetFont(&f);
	}
	if (fRo) {
		col = owner->LowColor();
		if (col.red < 220)
			col.red += 15;
		else {
			if (col.green > 20)
				col.green -= 10;
			if (col.blue > 20)
				col.blue -= 10;
		}
		owner->SetLowColor(col);
		owner->FillRect(frame, B_SOLID_LOW);
	}
	BStringItem::DrawItem(owner, frame, complete);
	if (fCurrent || fRo)
		owner->PopState();
}


int32
ThemeItem::ThemeId()
{
	return fId;
}


bool
ThemeItem::IsCurrent()
{
	return fCurrent;
}


void
ThemeItem::SetCurrent(bool set)
{
	fCurrent = set;
}


bool
ThemeItem::IsReadOnly()
{
	return fRo;
}


void
ThemeItem::SetReadOnly(bool set)
{
	fRo = set;
}

