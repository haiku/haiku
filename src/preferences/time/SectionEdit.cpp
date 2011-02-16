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

#include <Bitmap.h>
#include <ControlLook.h>
#include <List.h>
#include <Window.h>

#include "TimeMessages.h"


TSection::TSection(BRect frame)
	:
	fFrame(frame)
{
}


BRect
TSection::Bounds() const
{
	BRect frame(fFrame);
	return frame.OffsetByCopy(B_ORIGIN);
}


void
TSection::SetFrame(BRect frame)
{
	fFrame = frame;
}


BRect
TSection::Frame() const
{
	return fFrame;
}


const uint32 kArrowAreaWidth = 16;


TSectionEdit::TSectionEdit(BRect frame, const char* name, uint32 sections)
	:
	BControl(frame, name, NULL, NULL, B_FOLLOW_NONE, B_NAVIGABLE | B_WILL_DRAW),
	fSectionList(NULL),
	fFocus(-1),
	fSectionCount(sections),
	fHoldValue(0)
{
	InitView();
}


TSectionEdit::~TSectionEdit()
{
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
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}


void
TSectionEdit::Draw(BRect updateRect)
{
	DrawBorder(updateRect);

	for (uint32 idx = 0; idx < fSectionCount; idx++) {
		DrawSection(idx, ((uint32)fFocus == idx) && IsFocus());
		if (idx < fSectionCount - 1)
			DrawSeparator(idx);
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
	else if (fSectionList->CountItems() > 0) {
		TSection* section;
		for (uint32 idx = 0; idx < fSectionCount; idx++) {
			section = (TSection*)fSectionList->ItemAt(idx);
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
TSectionEdit::KeyDown(const char* bytes, int32 numbytes)
{
	if (fFocus == -1)
		SectionFocus(0);

	switch (bytes[0]) {
		case B_LEFT_ARROW:
			fFocus -= 1;
			if (fFocus < 0)
				fFocus = fSectionCount - 1;
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
	// setup sections
	fSectionList = new BList(fSectionCount);
	fSectionArea = Bounds().InsetByCopy(2, 2);
	fSectionArea.right -= kArrowAreaWidth;
}


void
TSectionEdit::DrawBorder(const BRect& updateRect)
{
	BRect bounds(Bounds());
	bool showFocus = (IsFocus() && Window() && Window()->IsActive());

	be_control_look->DrawBorder(this, bounds, updateRect, ViewColor(),
		B_FANCY_BORDER, showFocus ? BControlLook::B_FOCUSED : 0);

	// draw up/down control

	bounds.left = bounds.right - kArrowAreaWidth;
	bounds.right = Bounds().right - 2;
	fUpRect.Set(bounds.left + 3, bounds.top + 2, bounds.right,
		bounds.bottom / 2.0);
	fDownRect = fUpRect.OffsetByCopy(0, fUpRect.Height() + 2);

	BPoint middle(floorf(fUpRect.left + fUpRect.Width() / 2), fUpRect.top + 1);
	BPoint left(fUpRect.left + 3, fUpRect.bottom - 1);
	BPoint right(left.x + 2 * (middle.x - left.x), fUpRect.bottom - 1);

	SetPenSize(2);

	if (updateRect.Intersects(fUpRect)) {
		FillRect(fUpRect, B_SOLID_LOW);
		BeginLineArray(2);
			AddLine(left, middle, HighColor());
			AddLine(middle, right, HighColor());
		EndLineArray();
	}
	if (updateRect.Intersects(fDownRect)) {
		middle.y = fDownRect.bottom - 1;
		left.y = right.y = fDownRect.top + 1;

		FillRect(fDownRect, B_SOLID_LOW);
		BeginLineArray(2);
			AddLine(left, middle, HighColor());
			AddLine(middle, right, HighColor());
		EndLineArray();
	}

	SetPenSize(1);
}


float
TSectionEdit::SeparatorWidth() const
{
	return 0.0f;
}
