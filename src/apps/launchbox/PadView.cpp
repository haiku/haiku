/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "PadView.h"

#include <stdio.h>

#include <Application.h>
#include <GroupLayout.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <Screen.h>
#include <SpaceLayoutItem.h>

#include "LaunchButton.h"
#include "MainWindow.h"

bigtime_t kActivationDelay = 40000;

enum {
	MSG_TOGGLE_LAYOUT			= 'tgll'
};

// constructor
PadView::PadView(const char* name)
	: BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE, NULL),
	  fDragging(false),
	  fClickTime(0),
	  fButtonLayout(new BGroupLayout(B_VERTICAL, 4))
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	get_click_speed(&kActivationDelay); 

	fButtonLayout->SetInsets(2, 7, 2, 2);
	SetLayout(fButtonLayout);
}

// destructor
PadView::~PadView()
{
}

// Draw
void
PadView::Draw(BRect updateRect)
{
	rgb_color background = LowColor();
	rgb_color light = tint_color(background, B_LIGHTEN_MAX_TINT);
	rgb_color shadow = tint_color(background, B_DARKEN_2_TINT);
	BRect r(Bounds());
	BeginLineArray(4);
		AddLine(BPoint(r.left, r.bottom), BPoint(r.left, r.top), light);
		AddLine(BPoint(r.left + 1.0, r.top), BPoint(r.right, r.top), light);
		AddLine(BPoint(r.right, r.top + 1.0), BPoint(r.right, r.bottom), shadow);
		AddLine(BPoint(r.right - 1.0, r.bottom), BPoint(r.left + 1.0, r.bottom), shadow);
	EndLineArray();
	r.InsetBy(1.0, 1.0);
	StrokeRect(r, B_SOLID_LOW);
	r.InsetBy(1.0, 1.0);
	// dots along top
	BPoint dot = r.LeftTop();
	int32 current;
	int32 stop;
	BPoint offset;
	BPoint next;
	if (Orientation() == B_VERTICAL) {
		current = (int32)dot.x;
		stop = (int32)r.right;
		offset = BPoint(0, 1);
		next = BPoint(1, -4);
		r.top += 5.0;
	} else {
		current = (int32)dot.y;
		stop = (int32)r.bottom;
		offset = BPoint(1, 0);
		next = BPoint(-4, 1);
		r.left += 5.0;
	}
	int32 num = 1;
	while (current <= stop) {
		rgb_color col1;
		rgb_color col2;
		switch (num) {
			case 1:
				col1 = shadow;
				col2 = background;
				break;
			case 2:
				col1 = background;
				col2 = light;
				break;
			case 3:
				col1 = background;
				col2 = background;
				num = 0;
				break;
		}
		SetHighColor(col1);
		StrokeLine(dot, dot, B_SOLID_HIGH);
		SetHighColor(col2);
		dot += offset;
		StrokeLine(dot, dot, B_SOLID_HIGH);
		dot += offset;
		StrokeLine(dot, dot, B_SOLID_LOW);
		dot += offset;
		SetHighColor(col1);
		StrokeLine(dot, dot, B_SOLID_HIGH);
		dot += offset;
		SetHighColor(col2);
		StrokeLine(dot, dot, B_SOLID_HIGH);
		// next pixel
		num++;
		dot += next;
		current++;
	}
	FillRect(r, B_SOLID_LOW);
}

// MessageReceived
void
PadView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_TOGGLE_LAYOUT:
			if (fButtonLayout->Orientation() == B_HORIZONTAL) {
				fButtonLayout->SetInsets(2, 7, 2, 2);
				fButtonLayout->SetOrientation(B_VERTICAL);
			} else {
				fButtonLayout->SetInsets(7, 2, 2, 2);
				fButtonLayout->SetOrientation(B_HORIZONTAL);
			}
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}

// MouseDown
void
PadView::MouseDown(BPoint where)
{
	BWindow* window = Window();
	if (window == NULL)
		return;

	BRegion region;
	GetClippingRegion(&region);
	if (!region.Contains(where))
		return;

	bool handle = true;
	for (int32 i = 0; BView* child = ChildAt(i); i++) {
		if (child->Frame().Contains(where)) {
			handle = false;
			break;
		}
	}
	if (!handle)
		return;

	BMessage* message = window->CurrentMessage();
	if (message == NULL)
		return;

	uint32 buttons;
	message->FindInt32("buttons", (int32*)&buttons);
	if (buttons & B_SECONDARY_MOUSE_BUTTON) {
		BRect r = Bounds();
		r.InsetBy(2.0, 2.0);
		r.top += 6.0;
		if (r.Contains(where)) {
			DisplayMenu(where);
		} else {
			// sends the window to the back
			window->Activate(false);
		}
	} else {
		if (system_time() - fClickTime < kActivationDelay) {
			window->Minimize(true);
			fClickTime = 0;
		} else {
			window->Activate();
			fDragOffset = ConvertToScreen(where) - window->Frame().LeftTop();
			fDragging = true;
			SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
			fClickTime = system_time();
		}
	}
}

// MouseUp
void
PadView::MouseUp(BPoint where)
{
	if (BWindow* window = Window()) {
		uint32 buttons;
		window->CurrentMessage()->FindInt32("buttons", (int32*)&buttons);
		if (buttons & B_PRIMARY_MOUSE_BUTTON
			&& system_time() - fClickTime < kActivationDelay
			&& window->IsActive())
			window->Activate();
	}
	fDragging = false;
}

// MouseMoved
void
PadView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	MainWindow* window = dynamic_cast<MainWindow*>(Window());
	if (window == NULL)
		return;

	if (fDragging) {
		window->MoveTo(ConvertToScreen(where) - fDragOffset);
	} else if (window->AutoRaise()) {
		where = ConvertToScreen(where);
		BScreen screen(window);
		BRect frame = screen.Frame();
		BRect windowFrame = window->Frame();
		if (where.x == frame.left || where.x == frame.right
			|| where.y == frame.top || where.y == frame.bottom) {
			BPoint position = window->ScreenPosition();
			bool raise = false;
			if (fabs(0.5 - position.x) > fabs(0.5 - position.y)) {
				// left or right border
				if (where.y >= windowFrame.top && where.y <= windowFrame.bottom) {
					if (position.x < 0.5 && where.x == frame.left)
						raise = true;
					else if (position.x > 0.5 && where.x == frame.right)
						raise = true;
				}
			} else {
				// top or bottom border
				if (where.x >= windowFrame.left && where.x <= windowFrame.right) {
					if (position.y < 0.5 && where.y == frame.top)
						raise = true;
					else if (position.y > 0.5 && where.y == frame.top)
						raise = true;
				}
			}
			if (raise)
				window->Activate();
		}
	}
}

// AddButton
void
PadView::AddButton(LaunchButton* button, LaunchButton* beforeButton)
{
	if (beforeButton)
		fButtonLayout->AddView(fButtonLayout->IndexOfView(beforeButton), button);
	else
		fButtonLayout->AddView(button);
}

// RemoveButton
bool
PadView::RemoveButton(LaunchButton* button)
{
	return fButtonLayout->RemoveView(button);
}

// ButtonAt
LaunchButton*
PadView::ButtonAt(int32 index) const
{
	return dynamic_cast<LaunchButton*>(ChildAt(index));
}

// DisplayMenu
void
PadView::DisplayMenu(BPoint where, LaunchButton* button) const
{
	MainWindow* window = dynamic_cast<MainWindow*>(Window());
	if (window == NULL)
		return;

	LaunchButton* nearestButton = button;
	if (!nearestButton) {
		// find the nearest button
		for (int32 i = 0; (nearestButton = ButtonAt(i)); i++) {
			if (nearestButton->Frame().top > where.y)
				break;
		}
	}
	BPopUpMenu* menu = new BPopUpMenu("launch popup", false, false);
	// add button
	BMessage* message = new BMessage(MSG_ADD_SLOT);
	message->AddPointer("be:source", (void*)nearestButton);
	BMenuItem* item = new BMenuItem("Add Button Here", message);
	item->SetTarget(window);
	menu->AddItem(item);
	// button options
	if (button) {
		// remove button
		message = new BMessage(MSG_CLEAR_SLOT);
		message->AddPointer("be:source", (void*)button);
		item = new BMenuItem("Clear Button", message);
		item->SetTarget(window);
		menu->AddItem(item);
		// remove button
		message = new BMessage(MSG_REMOVE_SLOT);
		message->AddPointer("be:source", (void*)button);
		item = new BMenuItem("Remove Button", message);
		item->SetTarget(window);
		menu->AddItem(item);
// TODO: disabled because Haiku does not yet support tool tips
//		if (button->Ref()) {
//			message = new BMessage(MSG_SET_DESCRIPTION);
//			message->AddPointer("be:source", (void*)button);
//			item = new BMenuItem("Set Description"B_UTF8_ELLIPSIS, message);
//			item->SetTarget(window);
//			menu->AddItem(item);
//		}
	}
	menu->AddSeparatorItem();
	// window settings
	BMenu* settingsM = new BMenu("Settings");
	settingsM->SetFont(be_plain_font);

	const char* toggleLayoutLabel;
	if (fButtonLayout->Orientation() == B_HORIZONTAL)
		toggleLayoutLabel = "Vertical Layout";
	else
		toggleLayoutLabel = "Horizontal Layout";
	item = new BMenuItem(toggleLayoutLabel, new BMessage(MSG_TOGGLE_LAYOUT));
	item->SetTarget(this);
	settingsM->AddItem(item);

	uint32 what = window->Look() == B_BORDERED_WINDOW_LOOK ? MSG_SHOW_BORDER : MSG_HIDE_BORDER;
	item = new BMenuItem("Show Window Border", new BMessage(what));
	item->SetTarget(window);
	item->SetMarked(what == MSG_HIDE_BORDER);
	settingsM->AddItem(item);

	item = new BMenuItem("Auto Raise", new BMessage(MSG_TOGGLE_AUTORAISE));
	item->SetTarget(window);
	item->SetMarked(window->AutoRaise());
	settingsM->AddItem(item);

	item = new BMenuItem("Show On All Workspaces", new BMessage(MSG_SHOW_ON_ALL_WORKSPACES));
	item->SetTarget(window);
	item->SetMarked(window->ShowOnAllWorkspaces());
	settingsM->AddItem(item);

	menu->AddItem(settingsM);

	menu->AddSeparatorItem();

	// pad commands
	BMenu* padM = new BMenu("Pad");
	padM->SetFont(be_plain_font);
	// new pad
	item = new BMenuItem("New", new BMessage(MSG_ADD_WINDOW));
	item->SetTarget(be_app);
	padM->AddItem(item);
	// new pad
	item = new BMenuItem("Clone", new BMessage(MSG_ADD_WINDOW));
	item->SetTarget(window);
	padM->AddItem(item);
	padM->AddSeparatorItem();
	// close
	item = new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED));
	item->SetTarget(window);
	padM->AddItem(item);
	menu->AddItem(padM);
	// app commands
	BMenu* appM = new BMenu("LaunchBox");
	appM->SetFont(be_plain_font);
	// about
	item = new BMenuItem("About", new BMessage(B_ABOUT_REQUESTED));
	item->SetTarget(be_app);
	appM->AddItem(item);
	// quit
	item = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED));
	item->SetTarget(be_app);
	appM->AddItem(item);
	menu->AddItem(appM);
	// finish popup
	menu->SetAsyncAutoDestruct(true);
	menu->SetFont(be_plain_font);
	where = ConvertToScreen(where);
	BRect mouseRect(where, where);
	mouseRect.InsetBy(-4.0, -4.0);
	menu->Go(where, true, false, mouseRect, true);
}

// SetOrientation
void
PadView::SetOrientation(enum orientation orientation)
{
	if (orientation == B_VERTICAL) {
		fButtonLayout->SetInsets(2, 7, 2, 2);
		fButtonLayout->SetOrientation(B_VERTICAL);
	} else {
		fButtonLayout->SetInsets(7, 2, 2, 2);
		fButtonLayout->SetOrientation(B_HORIZONTAL);
	}
}

// Orientation
enum orientation
PadView::Orientation() const
{
	return fButtonLayout->Orientation();
}


