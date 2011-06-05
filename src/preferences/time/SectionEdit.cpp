/*
 * Copyright 2004-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *		Hamish Morrison <hamish@lavabit.com>
 */


#include "SectionEdit.h"

#include <Bitmap.h>
#include <ControlLook.h>
#include <LayoutUtils.h>
#include <List.h>
#include <Window.h>

#include "TimeMessages.h"


const uint32 kArrowAreaWidth = 16;


TSectionEdit::TSectionEdit(const char* name, uint32 sections)
	:
	BControl(name, NULL, NULL, B_WILL_DRAW | B_NAVIGABLE),
	fFocus(-1),
	fSectionCount(sections),
	fHoldValue(0)
{
}


TSectionEdit::~TSectionEdit()
{
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
		DrawSection(idx, FrameForSection(idx),
			((uint32)fFocus == idx) && IsFocus());
		if (idx < fSectionCount - 1)
			DrawSeparator(idx, FrameForSeparator(idx));
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
	else if (fSectionCount > 0) {
		for (uint32 idx = 0; idx < fSectionCount; idx++) {
			if (FrameForSection(idx).Contains(where)) {
				SectionFocus(idx);
				return;
			}
		}
	}
}


BSize
TSectionEdit::MaxSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(),
		BSize(B_SIZE_UNLIMITED, PreferredHeight()));
}


BSize
TSectionEdit::MinSize()
{
	BSize minSize;
	minSize.height = PreferredHeight();
	minSize.width = (SeparatorWidth() + MinSectionWidth())
		* fSectionCount;
	return BLayoutUtils::ComposeSize(ExplicitMinSize(),
		minSize);
}


BSize
TSectionEdit::PreferredSize()
{
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(),
		MinSize());
}


BRect
TSectionEdit::FrameForSection(uint32 index)
{
	BRect area = SectionArea();
	float sepWidth = SeparatorWidth();
	
	float width = (area.Width() -
		sepWidth * (fSectionCount - 1))
		/ fSectionCount;
	area.left += index * (width + sepWidth);
	area.right = area.left + width;

	return area;
}


BRect
TSectionEdit::FrameForSeparator(uint32 index)
{
	BRect area = SectionArea();
	float sepWidth = SeparatorWidth();

	float width = (area.Width() -
		sepWidth * (fSectionCount - 1))
		/ fSectionCount;
	area.left += (index + 1) * width + index * sepWidth;
	area.right = area.left + sepWidth;

	return area;
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
	return fSectionCount;
}


int32
TSectionEdit::FocusIndex() const
{
	return fFocus;
}


BRect
TSectionEdit::SectionArea() const
{
	BRect sectionArea = Bounds().InsetByCopy(2, 2);
	sectionArea.right -= kArrowAreaWidth;
	return sectionArea;
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

	BPoint middle(floorf(fUpRect.left + fUpRect.Width() / 2),
		fUpRect.top + 1);
	BPoint left(fUpRect.left + 3, fUpRect.bottom - 1);
	BPoint right(left.x + 2 * (middle.x - left.x), fUpRect.bottom - 1);

	SetPenSize(2);
	SetLowColor(ViewColor());

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
