/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *
 */

#include "SectionEdit.h"
#include "Bitmaps.h"
#include "TimeMessages.h"


#include <Bitmap.h>
#include <List.h>
#include <Window.h>


const uint32 kArrowAreaWidth = 16;


TSectionEdit::TSectionEdit(BRect frame, const char *name, uint32 sections)
	: BControl(frame, name, NULL, NULL, B_FOLLOW_NONE, B_NAVIGABLE | B_WILL_DRAW),
	  fUpArrow(NULL),
	  fDownArrow(NULL),
	  fSectionList(NULL),
	  fFocus(-1),
	  fSectionCount(sections)
{
	InitView();
}


TSectionEdit::~TSectionEdit()
{
	delete fUpArrow;
	delete fDownArrow;

	int32 count = fSectionList->CountItems();
	if (count > 0) {
		for (int32 index = 0; index < count; index++)
			delete (TSection*)fSectionList->ItemAt(index);
	}
	delete fSectionList;
}


void
TSectionEdit::AttachedToWindow()
{
	if (Parent()) {
		SetViewColor(Parent()->ViewColor());
		ReplaceTransparentColor(fUpArrow, ViewColor());
		ReplaceTransparentColor(fDownArrow, ViewColor());
	}
}


void
TSectionEdit::Draw(BRect updateRect)
{
	DrawBorder();
	for (uint32 idx = 0; idx < fSectionCount; idx++) {
		DrawSection(idx, ((uint32)fFocus == idx) && IsFocus());
		if (idx < fSectionCount -1)
			DrawSeperator(idx);
	}
}


void
TSectionEdit::MouseDown(BPoint where)
{
	MakeFocus(true);
	
	if (fUpRect.Contains(where))
		DoUpPress();
	else if (fDownRect.Contains(where))
		DoDownPress();
	else if (fSectionList->CountItems()> 0) {
		TSection *section;
		for (uint32 idx = 0; idx < fSectionCount; idx++) {
			section = (TSection *)fSectionList->ItemAt(idx);
			if (section->Frame().Contains(where)) {
				SectionFocus(idx);
				return;
			}
		}	
	}
}


void
TSectionEdit::MakeFocus(bool focused)
{
	if (focused == IsFocus())
		return;

	BControl::MakeFocus(focused);
	
	if (fFocus == -1)
		SectionFocus(0);
	else
		SectionFocus(fFocus);
}


void
TSectionEdit::KeyDown(const char *bytes, int32 numbytes)
{
	if (fFocus == -1)
		SectionFocus(0);
		
	switch (bytes[0]) {
		case B_LEFT_ARROW:
			fFocus -= 1;
			if (fFocus < 0)
				fFocus = fSectionCount -1;
			SectionFocus(fFocus);
		break;
		
		case B_RIGHT_ARROW:
			fFocus += 1;
			if ((uint32)fFocus >= fSectionCount)
				fFocus = 0;
			SectionFocus(fFocus);
		break;
		
		case B_UP_ARROW:
			DoUpPress();
		break;
		
		case B_DOWN_ARROW:
			DoDownPress();
		break;
		
		default:
			BControl::KeyDown(bytes, numbytes);
		break;
	}
	Draw(Bounds());
}


void
TSectionEdit::DispatchMessage()
{
	BMessage message(H_USER_CHANGE);
	BuildDispatch(&message);
	Window()->PostMessage(&message);
}


uint32
TSectionEdit::CountSections() const
{
	return fSectionList->CountItems();
}


int32
TSectionEdit::FocusIndex() const
{
	return fFocus;
}


void
TSectionEdit::InitView()
{
	// create arrow bitmaps
	BRect rect(0, 0, kUpArrowWidth -1, kUpArrowHeight -1);
	fUpArrow = new BBitmap(rect, kUpArrowColorSpace);
	fUpArrow->SetBits(kUpArrowBits, (kUpArrowWidth) *(kUpArrowHeight+1), 0
		, kUpArrowColorSpace);
	
	rect = BRect(0, 0, kDownArrowWidth -1, kDownArrowHeight -2);
	fDownArrow = new BBitmap(rect, kDownArrowColorSpace);
	fDownArrow->SetBits(kDownArrowBits, (kDownArrowWidth) *(kDownArrowHeight)
		, 0, kDownArrowColorSpace);

	// setup sections
	fSectionList = new BList(fSectionCount);
	fSectionArea = Bounds().InsetByCopy(2, 2);
	fSectionArea.right -= kArrowAreaWidth; 
}


void
TSectionEdit::Draw3DFrame(BRect frame, bool inset)
{
	rgb_color color1 = LowColor();
	rgb_color color2 = HighColor();
	
	if (inset) {
		color1 = HighColor();
		color2 = LowColor();
	}

	BeginLineArray(4);
		// left side
		AddLine(frame.LeftBottom(), frame.LeftTop(), color2);
		// right side
		AddLine(frame.RightTop(), frame.RightBottom(), color1);
		// bottom side
		AddLine(frame.RightBottom(), frame.LeftBottom(), color1);
		// top side
		AddLine(frame.LeftTop(), frame.RightTop(), color2);
	EndLineArray();
}


void
TSectionEdit::DrawBorder()
{
	rgb_color bgcolor = ViewColor();
	rgb_color light = tint_color(bgcolor, B_LIGHTEN_MAX_TINT);
	rgb_color dark = tint_color(bgcolor, B_DARKEN_1_TINT);
	rgb_color darker = tint_color(bgcolor, B_DARKEN_3_TINT);

	SetHighColor(light);
	SetLowColor(dark);
	
	BRect bounds(Bounds());
	Draw3DFrame(bounds, true);
	StrokeLine(bounds.LeftBottom(), bounds.LeftBottom(), B_SOLID_LOW);
	
	bounds.InsetBy(1, 1);
	bounds.right -= kArrowAreaWidth;
	
	fShowFocus = (IsFocus() && Window() && Window()->IsActive());
	if (fShowFocus) {
		rgb_color navcolor = keyboard_navigation_color();
	
		SetHighColor(navcolor);
		StrokeRect(bounds);
	} else {
		// draw border thickening (erase focus)
		SetHighColor(darker);
		SetLowColor(bgcolor);	
		Draw3DFrame(bounds, false);
	}

	// draw up/down control
	SetHighColor(light);
	bounds.left = bounds.right +1;
	bounds.right = Bounds().right -1;
	fUpRect.Set(bounds.left +3, bounds.top +2, bounds.right, bounds.bottom /2.0);
	fDownRect = fUpRect.OffsetByCopy(0, fUpRect.Height()+2);
	
	if (fUpArrow)
		DrawBitmap(fUpArrow, fUpRect.LeftTop());
	
	if (fDownArrow)
		DrawBitmap(fDownArrow, fDownRect.LeftTop());
	
	Draw3DFrame(bounds, false);
	SetHighColor(dark);
	StrokeLine(bounds.LeftBottom(), bounds.RightBottom());
	StrokeLine(bounds.RightBottom(), bounds.RightTop());
	SetHighColor(light);
	StrokeLine(bounds.RightTop(), bounds.RightTop()); 
}


float
TSectionEdit::SeperatorWidth() const
{
	return 0.0f;
}

