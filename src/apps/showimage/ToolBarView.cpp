/*
 * Copyright 2011 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "ToolBarView.h"

#include <ControlLook.h>
#include <IconButton.h>
#include <Message.h>
#include <SeparatorView.h>
#include <SpaceLayoutItem.h>


ToolBarView::ToolBarView(BRect frame)
	:
	BGroupView(B_HORIZONTAL)
{
	float inset = ceilf(be_control_look->DefaultItemSpacing() / 2);
	GroupLayout()->SetInsets(inset, inset, inset, inset);

	GroupLayout()->AddItem(BSpaceLayoutItem::CreateGlue());

	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(), frame.Height());
	SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
}


ToolBarView::~ToolBarView()
{
}


void
ToolBarView::AddAction(uint32 command, BHandler* target, const BBitmap* icon,
	const char* toolTipText)
{
	AddAction(new BMessage(command), target, icon, toolTipText);
}


void
ToolBarView::AddAction(BMessage* message, BHandler* target, const BBitmap* icon,
	const char* toolTipText)
{
	BIconButton* button = new BIconButton(NULL, 0, NULL, message, target);
	button->SetIcon(icon);
	if (toolTipText != NULL)
		button->SetToolTip(toolTipText);
	_AddView(button);
}


void
ToolBarView::AddSeparator()
{
	_AddView(new BSeparatorView(B_VERTICAL, B_PLAIN_BORDER));
}


void
ToolBarView::SetActionEnabled(uint32 command, bool enabled)
{
	if (BIconButton* button = _FindIconButton(command))
		button->SetEnabled(enabled);
}


void
ToolBarView::SetActionPressed(uint32 command, bool pressed)
{
	if (BIconButton* button = _FindIconButton(command))
		button->SetPressed(pressed);
}


void
ToolBarView::_AddView(BView* view)
{
	// Add before the space layout item at the end
	GroupLayout()->AddView(GroupLayout()->CountItems() - 1, view);
}


BIconButton*
ToolBarView::_FindIconButton(uint32 command) const
{
	for (int32 i = 0; BView* view = ChildAt(i); i++) {
		BIconButton* button = dynamic_cast<BIconButton*>(view);
		if (button == NULL)
			continue;
		BMessage* message = button->Message();
		if (message == NULL)
			continue;
		if (message->what == command) {
			return button;
			// Assumes there is only one button with this message...
			break;
		}
	}
	return NULL;
}


