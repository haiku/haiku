/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "CheckBox.h"

#include <View.h>

#include "StringView.h"


// #pragma mark - CheckBox


CheckBox::CheckBox(BMessage* message, BMessenger target)
	: AbstractButton(BUTTON_POLICY_TOGGLE_ON_RELEASE, message, target)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


BSize
CheckBox::MinSize()
{
	return BSize(12, 12);
}


BSize
CheckBox::MaxSize()
{
	return MinSize();
}


void
CheckBox::Draw(BView* container, BRect updateRect)
{
	BRect rect(Bounds());

	if (IsPressed())
		container->SetHighColor((rgb_color){ 120, 0, 0, 255 });
	else
		container->SetHighColor((rgb_color){ 0, 0, 0, 255 });

	container->StrokeRect(rect);

	if (IsSelected()) {
		rect.InsetBy(2, 2);
		container->StrokeLine(rect.LeftTop(), rect.RightBottom());
		container->StrokeLine(rect.RightTop(), rect.LeftBottom());
	}
}


// #pragma mark - LabeledCheckBox


LabeledCheckBox::LabeledCheckBox(const char* label, BMessage* message,
	BMessenger target)
	: GroupView(B_HORIZONTAL),
	  fCheckBox(new CheckBox(message, target))
{
	SetSpacing(8, 0);

	AddChild(fCheckBox);
	if (label)
		AddChild(new StringView(label));
}


void
LabeledCheckBox::SetTarget(BMessenger messenger)
{
	fCheckBox->SetTarget(messenger);
}


void
LabeledCheckBox::SetSelected(bool selected)
{
	fCheckBox->SetSelected(selected);
}


bool
LabeledCheckBox::IsSelected() const
{
	return fCheckBox->IsSelected();
}
