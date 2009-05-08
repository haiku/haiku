/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ColorCheckBox.h"

#include <SpaceLayoutItem.h>


ColorCheckBox::ColorCheckBox(const char* label, const rgb_color& color,
	BMessage* message)
	:
	BGroupView(B_HORIZONTAL),
	fColor(color)
{
	SetFlags(Flags() | B_WILL_DRAW);

	fCheckBox = new BCheckBox(label, message);
	GroupLayout()->AddView(fCheckBox, 0);
	AddChild(BSpaceLayoutItem::CreateHorizontalStrut(15));
	AddChild(BSpaceLayoutItem::CreateGlue());
}


BCheckBox*
ColorCheckBox::CheckBox() const
{
	return fCheckBox;
}


void
ColorCheckBox::SetTarget(const BMessenger& target)
{
	fCheckBox->SetTarget(target);
}


void
ColorCheckBox::Draw(BRect updateRect)
{
	BGroupView::Draw(updateRect);

	BRect rect(Bounds());
	rect.left += fCheckBox->Frame().right + 5;
	rect.right = rect.left + 9;
	rect.top = floorf((rect.top + rect.bottom) / 2);
	rect.bottom = rect.top + 1;

	SetHighColor(fColor);
	FillRect(rect);
}
