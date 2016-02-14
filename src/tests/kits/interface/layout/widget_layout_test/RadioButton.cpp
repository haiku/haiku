/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "RadioButton.h"

#include <View.h>

#include "StringView.h"


// #pragma mark - RadioButton


RadioButton::RadioButton(BMessage* message, BMessenger target)
	: AbstractButton(BUTTON_POLICY_SELECT_ON_RELEASE, message, target)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


BSize
RadioButton::MinSize()
{
	return BSize(12, 12);
}


BSize
RadioButton::MaxSize()
{
	return MinSize();
}


void
RadioButton::Draw(BView* container, BRect updateRect)
{
	BRect rect(Bounds());

	if (IsPressed())
		container->SetHighColor((rgb_color){ 120, 0, 0, 255 });
	else
		container->SetHighColor((rgb_color){ 0, 0, 0, 255 });

	container->StrokeRect(rect);

	if (IsSelected()) {
		rect.InsetBy(4, 4);
		container->FillRect(rect);
	}
}


// #pragma mark - LabeledRadioButton


LabeledRadioButton::LabeledRadioButton(const char* label, BMessage* message,
	BMessenger target)
	: GroupView(B_HORIZONTAL),
	  fRadioButton(new RadioButton(message, target))
{
	SetSpacing(8, 0);

	AddChild(fRadioButton);
	if (label)
		AddChild(new StringView(label));
}


void
LabeledRadioButton::SetTarget(BMessenger messenger)
{
	fRadioButton->SetTarget(messenger);
}


void
LabeledRadioButton::SetSelected(bool selected)
{
	fRadioButton->SetSelected(selected);
}


bool
LabeledRadioButton::IsSelected() const
{
	return fRadioButton->IsSelected();
}


// #pragma mark - RadioButtonGroup


RadioButtonGroup::RadioButtonGroup(BMessage* message, BMessenger target)
	: BInvoker(message, target),
	  fButtons(10),
	  fSelected(NULL)
{
}


RadioButtonGroup::~RadioButtonGroup()
{
	// remove as listener from buttons
	for (int32 i = 0; AbstractButton* button = ButtonAt(i); i++)
		button->RemoveListener(this);
}


void
RadioButtonGroup::AddButton(AbstractButton* button)
{
	if (!button || fButtons.HasItem(button))
		return;

	// force radio button policy
	button->SetPolicy(BUTTON_POLICY_SELECT_ON_RELEASE);

	// deselect the button, if we do already have a selected one
	if (fSelected)
		button->SetSelected(false);

	// add ourselves as listener
	button->AddListener(this);

	// add the button to our list
	fButtons.AddItem(button);
}


bool
RadioButtonGroup::RemoveButton(AbstractButton* button)
{
	return RemoveButton(IndexOfButton(button));
}


AbstractButton*
RadioButtonGroup::RemoveButton(int32 index)
{
	// remove the button from our list
	AbstractButton* button = (AbstractButton*)fButtons.RemoveItem(index);
	if (!button)
		return NULL;

	// remove ourselves as listener
	button->RemoveListener(this);

	// if it was the selected one, we don't have a selection anymore
	if (button == fSelected) {
		fSelected = NULL;
		_SelectionChanged();
	}

	return button;
}


int32
RadioButtonGroup::CountButtons() const
{
	return fButtons.CountItems();
}


AbstractButton*
RadioButtonGroup::ButtonAt(int32 index) const
{
	return (AbstractButton*)fButtons.ItemAt(index);
}


int32
RadioButtonGroup::IndexOfButton(AbstractButton* button) const
{
	return fButtons.IndexOf(button);
}


void
RadioButtonGroup::SelectButton(AbstractButton* button)
{
	if (button && fButtons.HasItem(button))
		button->SetSelected(true);
}


void
RadioButtonGroup::SelectButton(int32 index)
{
	if (AbstractButton* button = ButtonAt(index))
		button->SetSelected(true);
}


AbstractButton*
RadioButtonGroup::SelectedButton() const
{
	return fSelected;
}


int32
RadioButtonGroup::SelectedIndex() const
{
	return (fSelected ? IndexOfButton(fSelected) : -1);
}


void
RadioButtonGroup::SelectionChanged(AbstractButton* button)
{
	// We're only interested in a notification when one of our buttons that
	// has not been selected one before has become selected.
	if (!button || !fButtons.HasItem(button) || !button->IsSelected()
		|| button == fSelected) {
		return;
	}

	// set the new selection
	AbstractButton* oldSelected = fSelected;
	fSelected = button;

	// deselect the old selected button
	if (oldSelected)
		oldSelected->SetSelected(false);

	// send out notifications
	_SelectionChanged();
}


void
RadioButtonGroup::_SelectionChanged()
{
	// send the message
	if (Message()) {
		BMessage message(*Message());
		message.AddPointer("button group", this);
		message.AddPointer("selected button", fSelected);
		message.AddInt32("selected index", SelectedIndex());
		InvokeNotify(&message);
	}
}
