/*
 * Copyright 2011 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "ToolBar.h"

#include <Button.h>
#include <ControlLook.h>
#include <Message.h>
#include <SeparatorView.h>
#include <SpaceLayoutItem.h>


namespace BPrivate {



// Button to adopt backgrond color of toolbar
class ToolBarButton : public BButton {
public:
			ToolBarButton(const char* name, const char* label,
				BMessage* message);

	void	AttachedToWindow();
};


ToolBarButton::ToolBarButton(const char* name, const char* label,
				BMessage* message)
	:
	BButton(name, label, message)
	{}


void
ToolBarButton::AttachedToWindow()
{
	BButton::AttachedToWindow();
	SetLowUIColor(B_PANEL_BACKGROUND_COLOR);
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
		// have to remove the darkening caused by BButton's drawing
}


//# pragma mark -


class LockableButton: public ToolBarButton {
public:
			LockableButton(const char* name, const char* label,
				BMessage* message);

	void	MouseDown(BPoint point);
};


LockableButton::LockableButton(const char* name, const char* label,
	BMessage* message)
	:
	ToolBarButton(name, label, message)
{
}


void
LockableButton::MouseDown(BPoint point)
{
	if ((modifiers() & B_SHIFT_KEY) != 0 || Value() == B_CONTROL_ON)
		SetBehavior(B_TOGGLE_BEHAVIOR);
	else
		SetBehavior(B_BUTTON_BEHAVIOR);

	Message()->SetInt32("behavior", Behavior());
	ToolBarButton::MouseDown(point);
}


//#pragma mark  -


BToolBar::BToolBar(BRect frame, orientation ont)
	:
	BGroupView(ont),
	fOrientation(ont)
{
	_Init();

	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(), frame.Height());
	SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
}


BToolBar::BToolBar(orientation ont)
	:
	BGroupView(ont),
	fOrientation(ont)
{
	_Init();
}


BToolBar::~BToolBar()
{
}


void
BToolBar::Hide()
{
	BView::Hide();
	// TODO: This could be fixed in BView instead. Looking from the
	// BButtons, they are not hidden though, only their parent is...
	_HideToolTips();
}


void
BToolBar::AddAction(uint32 command, BHandler* target, const BBitmap* icon,
	const char* toolTipText, const char* text, bool lockable)
{
	AddAction(new BMessage(command), target, icon, toolTipText, text, lockable);
}


void
BToolBar::AddAction(BMessage* message, BHandler* target,
	const BBitmap* icon, const char* toolTipText, const char* text,
	bool lockable)
{
	ToolBarButton* button;
	if (lockable)
		button = new LockableButton(NULL, NULL, message);
	else
		button = new ToolBarButton(NULL, NULL, message);
	button->SetIcon(icon);
	button->SetFlat(true);
	if (toolTipText != NULL)
		button->SetToolTip(toolTipText);
	if (text != NULL)
		button->SetLabel(text);
	AddView(button);
	button->SetTarget(target);
}


void
BToolBar::AddSeparator()
{
	orientation ont = (fOrientation == B_HORIZONTAL) ?
		B_VERTICAL : B_HORIZONTAL;
	AddView(new BSeparatorView(ont, B_PLAIN_BORDER));
}


void
BToolBar::AddGlue()
{
	GroupLayout()->AddItem(BSpaceLayoutItem::CreateGlue());
}


void
BToolBar::AddView(BView* view)
{
	GroupLayout()->AddView(view);
}


void
BToolBar::SetActionEnabled(uint32 command, bool enabled)
{
	if (BButton* button = FindButton(command))
		button->SetEnabled(enabled);
}


void
BToolBar::SetActionPressed(uint32 command, bool pressed)
{
	if (BButton* button = FindButton(command))
		button->SetValue(pressed);
}


void
BToolBar::SetActionVisible(uint32 command, bool visible)
{
	BButton* button = FindButton(command);
	if (button == NULL)
		return;
	for (int32 i = 0; BLayoutItem* item = GroupLayout()->ItemAt(i); i++) {
		if (item->View() != button)
			continue;
		item->SetVisible(visible);
		break;
	}
}


BButton*
BToolBar::FindButton(uint32 command) const
{
	for (int32 i = 0; BView* view = ChildAt(i); i++) {
		BButton* button = dynamic_cast<BButton*>(view);
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


// #pragma mark - Private methods

void
BToolBar::Pulse()
{
	// TODO: Perhaps this could/should be addressed in BView instead.
	if (IsHidden())
		_HideToolTips();
}


void
BToolBar::FrameResized(float width, float height)
{
	// TODO: There seems to be a bug in app_server which does not
	// correctly trigger invalidation of views which are shown, when
	// the resulting dirty area is somehow already part of an update region.
	Invalidate();
}


void
BToolBar::_Init()
{
	float inset = ceilf(be_control_look->DefaultItemSpacing() / 2);
	GroupLayout()->SetInsets(inset, 0, inset, 0);
	GroupLayout()->SetSpacing(1);

	SetFlags(Flags() | B_FRAME_EVENTS | B_PULSE_NEEDED);
}


void
BToolBar::_HideToolTips() const
{
	for (int32 i = 0; BView* view = ChildAt(i); i++)
		view->HideToolTip();
}


} // namespace BPrivate
