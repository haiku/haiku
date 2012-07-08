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
	GroupLayout()->SetInsets(inset, 2, inset, 3);
	GroupLayout()->SetSpacing(inset);

	SetFlags(Flags() | B_FRAME_EVENTS | B_PULSE_NEEDED);

	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(), frame.Height());
	SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
}


ToolBarView::~ToolBarView()
{
}


void
ToolBarView::Hide()
{
	BView::Hide();
	// TODO: This could be fixed in BView instead. Looking from the 
	// BIconButtons, they are not hidden though, only their parent is...
	_HideToolTips();
}


void
ToolBarView::AddAction(uint32 command, BHandler* target, const BBitmap* icon,
	const char* toolTipText)
{
	AddAction(new BMessage(command), target, icon, toolTipText);
}


void
ToolBarView::AddAction(BMessage* message, BHandler* target,
	const BBitmap* icon, const char* toolTipText)
{
	BIconButton* button = new BIconButton(NULL, NULL, message, target);
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
ToolBarView::AddGlue()
{
	GroupLayout()->AddItem(BSpaceLayoutItem::CreateGlue());
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
ToolBarView::SetActionVisible(uint32 command, bool visible)
{
	BIconButton* button = _FindIconButton(command);
	if (button == NULL)
		return;
	for (int32 i = 0; BLayoutItem* item = GroupLayout()->ItemAt(i); i++) {
		if (item->View() != button)
			continue;
		item->SetVisible(visible);
		break;
	}
}


void
ToolBarView::Pulse()
{
	// TODO: Perhaps this could/should be addressed in BView instead.
	if (IsHidden())
		_HideToolTips();
}


void
ToolBarView::FrameResized(float width, float height)
{
	// TODO: There seems to be a bug in app_server which does not
	// correctly trigger invalidation of views which are shown, when
	// the resulting dirty area is somehow already part of an update region.
	Invalidate();
}


void
ToolBarView::_AddView(BView* view)
{
	GroupLayout()->AddView(view);
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


void
ToolBarView::_HideToolTips() const
{
	for (int32 i = 0; BView* view = ChildAt(i); i++)
		view->HideToolTip();
}

