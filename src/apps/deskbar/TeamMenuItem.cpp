/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered
trademarks of Be Incorporated in the United States and other countries. Other
brand product names are registered trademarks or trademarks of their respective
holders.
All rights reserved.
*/


#include "TeamMenuItem.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>

#include <Bitmap.h>
#include <ControlLook.h>
#include <Debug.h>
#include <Font.h>
#include <Mime.h>
#include <Region.h>
#include <Roster.h>
#include <Resources.h>
#include <Window.h>

#include "BarApp.h"
#include "BarMenuBar.h"
#include "BarView.h"
#include "ExpandoMenuBar.h"
#include "ResourceSet.h"
#include "ShowHideMenuItem.h"
#include "StatusView.h"
#include "TeamMenu.h"
#include "WindowMenu.h"
#include "WindowMenuItem.h"


static float sHPad, sVPad, sLabelOffset = 0.0f;


//	#pragma mark - TTeamMenuItem


TTeamMenuItem::TTeamMenuItem(BList* team, BBitmap* icon, char* name,
	char* signature, float width, float height)
	:
	TTruncatableMenuItem(new TWindowMenu(team, signature))
{
	_Init(team, icon, name, signature, width, height);
}


TTeamMenuItem::TTeamMenuItem(float width, float height)
	:
	TTruncatableMenuItem("", NULL)
{
	_Init(NULL, NULL, strdup(""), strdup(""), width, height);
}


TTeamMenuItem::~TTeamMenuItem()
{
	delete fTeam;
	delete fIcon;
	free(fSignature);
}


/*!	Vulcan Death Grip and other team mouse button handling

	\returns true if handled, false otherwise
*/
bool
TTeamMenuItem::HandleMouseDown(BPoint where)
{
	BMenu* menu = Menu();
	if (menu == NULL)
		return false;

	BWindow* window = menu->Window();
	if (window == NULL)
		return false;

	BMessage* message = window->CurrentMessage();
	if (message == NULL)
		return false;

	int32 modifiers = 0;
	int32 buttons = 0;
	message->FindInt32("modifiers", &modifiers);
	message->FindInt32("buttons", &buttons);

	// check for three finger salute, a.k.a. Vulcan Death Grip
	if ((modifiers & B_COMMAND_KEY) != 0
		&& (modifiers & B_CONTROL_KEY) != 0
		&& (modifiers & B_SHIFT_KEY) != 0) {
		BMessage appMessage(B_SOME_APP_QUIT);
		int32 teamCount = fTeam->CountItems();
		BMessage quitMessage(teamCount == 1 ? B_SOME_APP_QUIT : kRemoveTeam);
		quitMessage.AddInt32("itemIndex", menu->IndexOf(this));
		for (int32 index = 0; index < teamCount; index++) {
			team_id team = (addr_t)fTeam->ItemAt(index);
			appMessage.AddInt32("be:team", team);
			quitMessage.AddInt32("team", team);

			kill_team(team);
		}
		be_app->PostMessage(&appMessage);
		TExpandoMenuBar* expando = dynamic_cast<TExpandoMenuBar*>(menu);
		window->PostMessage(&quitMessage, expando != NULL ? expando : menu);
		return true;
	} else if ((modifiers & B_SHIFT_KEY) == 0
		&& (buttons & B_TERTIARY_MOUSE_BUTTON) != 0) {
		// launch new team
		be_roster->Launch(Signature());
		return true;
	} else if ((modifiers & B_CONTROL_KEY) != 0) {
		// control click - show all/hide all shortcut
		BMessage showMessage((modifiers & B_SHIFT_KEY) != 0
			? kMinimizeTeam : kBringTeamToFront);
		showMessage.AddInt32("itemIndex", menu->IndexOf(this));
		window->PostMessage(&showMessage, menu);
		return true;
	}

	return false;
}


status_t
TTeamMenuItem::Invoke(BMessage* message)
{
	if (fBarView != NULL) {
		if (fBarView->InvokeItem(Signature())) {
			// handles drop on application
			return B_OK;
		}

		// if the app could not handle the drag message
		// and we were dragging, then kill the drag
		// should never get here, disabled item will not invoke
		if (fBarView->Dragging())
			fBarView->DragStop();
	}

	// bring to front or minimize shortcuts
	uint32 mods = modifiers();
	if (mods & B_CONTROL_KEY) {
		TShowHideMenuItem::TeamShowHideCommon((mods & B_SHIFT_KEY)
			? B_MINIMIZE_WINDOW : B_BRING_TO_FRONT, Teams());
	}

	return BMenuItem::Invoke(message);
}


void
TTeamMenuItem::SetOverrideSelected(bool selected)
{
	fOverriddenSelected = selected;
	Highlight(selected);
}


void
TTeamMenuItem::SetIcon(BBitmap* icon) {
	delete fIcon;
	fIcon = icon;
}


void
TTeamMenuItem::GetContentSize(float* width, float* height)
{
	BMenuItem::GetContentSize(width, height);

	if (fOverrideWidth != -1.0f)
		*width = fOverrideWidth;
	else {
		bool hideLabels = static_cast<TBarApp*>(be_app)->Settings()->hideLabels;
		float iconSize = static_cast<TBarApp*>(be_app)->TeamIconSize();
		float iconOnlyWidth = (be_control_look->ComposeSpacing(kIconPadding) * 2) + iconSize;

		if (fBarView->MiniState()) {
			if (hideLabels)
				*width = iconOnlyWidth;
			else
				*width = gMinimumWindowWidth - (gDragRegionWidth + kGutter) * 2;
		} else if (!fBarView->Vertical()) {
			TExpandoMenuBar* menu = static_cast<TExpandoMenuBar*>(Menu());
			*width = menu->MaxHorizontalItemWidth();
		} else
			*width = static_cast<TBarApp*>(be_app)->Settings()->width;
	}

	if (fOverrideHeight != -1.0f)
		*height = fOverrideHeight;
	else
		*height = fBarView->TeamMenuItemHeight();
}


void
TTeamMenuItem::Draw()
{
	BRect frame = Frame();
	BMenu* menu = Menu();

	menu->PushState();

	rgb_color menuColor = ui_color(B_MENU_BACKGROUND_COLOR);
	bool canHandle = !fBarView->Dragging()
		|| fBarView->AppCanHandleTypes(Signature());
	uint32 flags = 0;
	if (_IsSelected() && canHandle)
		flags |= BControlLook::B_ACTIVATED;

	uint32 borders = BControlLook::B_TOP_BORDER;
	if (fBarView->Vertical()) {
		menu->SetHighColor(tint_color(menuColor, B_DARKEN_1_TINT));
		borders |= BControlLook::B_LEFT_BORDER
			| BControlLook::B_RIGHT_BORDER;
		menu->StrokeLine(frame.LeftBottom(), frame.RightBottom());
		frame.bottom--;

		be_control_look->DrawMenuBarBackground(menu, frame, frame,
			menuColor, flags, borders);
	} else {
		if (flags & BControlLook::B_ACTIVATED)
			menu->SetHighColor(tint_color(menuColor, B_DARKEN_3_TINT));
		else
			menu->SetHighColor(tint_color(menuColor, 1.22));
		borders |= BControlLook::B_BOTTOM_BORDER;
		menu->StrokeLine(frame.LeftTop(), frame.LeftBottom());
		frame.left++;

		be_control_look->DrawButtonBackground(menu, frame, frame,
			menuColor, flags, borders);
	}

	menu->MovePenTo(ContentLocation());
	DrawContent();

	menu->PopState();
}


void
TTeamMenuItem::DrawContent()
{
	BMenu* menu = Menu();
	BRect frame = Frame();

	if (fIcon != NULL) {
		if (fIcon->ColorSpace() == B_RGBA32) {
			menu->SetDrawingMode(B_OP_ALPHA);
			menu->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		} else
			menu->SetDrawingMode(B_OP_OVER);

		BRect iconBounds = fIcon != NULL ? fIcon->Bounds()
			: BRect(0, 0, kMinimumIconSize - 1, kMinimumIconSize - 1);
		BRect updateRect = iconBounds;
		BPoint contentLocation = ContentLocation();
		BPoint drawLocation = contentLocation + BPoint(sHPad, sVPad);

		if (static_cast<TBarApp*>(be_app)->Settings()->hideLabels
			|| (fBarView->Vertical() && iconBounds.Width() > 32)) {
			// determine icon location (centered horizontally)
			float offsetx = contentLocation.x
				+ floorf((frame.Width() - iconBounds.Width()) / 2);
			float offsety = contentLocation.y + sVPad + kGutter;

			// draw icon
			updateRect.OffsetTo(BPoint(offsetx, offsety));
			menu->DrawBitmapAsync(fIcon, updateRect);

			// determine label position (below icon)
			drawLocation.x = floorf((frame.Width() - fLabelWidth) / 2);
			drawLocation.y = frame.top + sVPad + iconBounds.Height() + sVPad;
		} else {
			// determine icon location (centered vertically)
			float offsetx = contentLocation.x + sHPad;
			float offsety = contentLocation.y +
				floorf((frame.Height() - iconBounds.Height()) / 2);

			// draw icon
			updateRect.OffsetTo(BPoint(offsetx, offsety));
			menu->DrawBitmapAsync(fIcon, updateRect);

			// determine label position (centered vertically)
			drawLocation.x += iconBounds.Width() + sLabelOffset;
			drawLocation.y = frame.top
				+ ceilf((frame.Height() - fLabelHeight) / 2);
		}

		menu->MovePenTo(drawLocation);
	}

	// override the drawing of the content when the item is disabled
	// the wrong lowcolor is used when the item is disabled since the
	// text color does not change
	menu->SetDrawingMode(B_OP_OVER);
	menu->SetHighColor(ui_color(B_MENU_ITEM_TEXT_COLOR));

	bool canHandle = !fBarView->Dragging()
		|| fBarView->AppCanHandleTypes(Signature());
	if (_IsSelected() && IsEnabled() && canHandle)
		menu->SetLowColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR),
			B_HIGHLIGHT_BACKGROUND_TINT));
	else
		menu->SetLowColor(ui_color(B_MENU_BACKGROUND_COLOR));

	if (IsSelected())
		menu->SetHighColor(ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR));
	else
		menu->SetHighColor(ui_color(B_MENU_ITEM_TEXT_COLOR));

	menu->MovePenBy(0, fLabelAscent);

	// draw label
	if (!static_cast<TBarApp*>(be_app)->Settings()->hideLabels) {
		float labelWidth = menu->StringWidth(Label());
		BPoint penLocation = menu->PenLocation();
		// truncate to max width
		float offset = penLocation.x - frame.left;
		menu->DrawString(Label(labelWidth + offset));
	}

	// draw expander arrow
	if (fBarView->Vertical()
		&& static_cast<TBarApp*>(be_app)->Settings()->superExpando
		&& fBarView->ExpandoState()) {
		DrawExpanderArrow();
	}
}


void
TTeamMenuItem::DrawExpanderArrow()
{
	BRect frame = Frame();
	BRect rect(0.0f, 0.0f, kSwitchWidth, sHPad + 2.0f);
	rect.OffsetTo(BPoint(frame.right - rect.Width(),
		ContentLocation().y + ((frame.Height() - rect.Height()) / 2)));

	float colorTint = B_DARKEN_3_TINT;
	rgb_color bgColor = ui_color(B_MENU_BACKGROUND_COLOR);
	if (bgColor.red + bgColor.green + bgColor.blue <= 128 * 3)
		colorTint = B_LIGHTEN_2_TINT;

	be_control_look->DrawArrowShape(Menu(), rect, Menu()->Frame(),
		bgColor, fArrowDirection, 0, colorTint);
}


void
TTeamMenuItem::ToggleExpandState(bool resizeWindow)
{
	fExpanded = !fExpanded;
	fArrowDirection = fExpanded ? BControlLook::B_DOWN_ARROW
		: BControlLook::B_RIGHT_ARROW;

	if (fExpanded) {
		// Populate Menu() with the stuff from SubMenu().
		TWindowMenu* sub = (static_cast<TWindowMenu*>(Submenu()));
		if (sub != NULL) {
			// force the menu to update it's contents.
			bool locked = sub->LockLooper();
				// if locking the looper failed, the menu is just not visible
			sub->AttachedToWindow();
			if (locked)
				sub->UnlockLooper();

			if (sub->CountItems() > 1) {
				TExpandoMenuBar* parent = static_cast<TExpandoMenuBar*>(Menu());
				int myindex = parent->IndexOf(this) + 1;

				TWindowMenuItem* windowItem = NULL;
				int32 childIndex = 0;
				int32 totalChildren = sub->CountItems() - 4;
					// hide, show, close, separator.
				for (; childIndex < totalChildren; childIndex++) {
					windowItem = static_cast<TWindowMenuItem*>
						(sub->RemoveItem((int32)0));
					parent->AddItem(windowItem, myindex + childIndex);
					windowItem->SetExpanded(true);
				}
				sub->SetExpanded(true, myindex + childIndex);

				if (resizeWindow)
					parent->SizeWindow(-1);
			}
		}
	} else {
		// Remove the goodies from the Menu() that should be in the SubMenu();
		TWindowMenu* sub = static_cast<TWindowMenu*>(Submenu());
		if (sub != NULL) {
			TExpandoMenuBar* parent = static_cast<TExpandoMenuBar*>(Menu());

			TWindowMenuItem* windowItem = NULL;
			int32 childIndex = parent->IndexOf(this) + 1;
			while (parent->SubmenuAt(childIndex) == NULL
				&& childIndex < parent->CountItems()) {
				windowItem = static_cast<TWindowMenuItem*>(
					parent->RemoveItem(childIndex));
				sub->AddItem(windowItem, 0);
				windowItem->SetExpanded(false);
			}
			sub->SetExpanded(false, 0);

			if (resizeWindow)
				parent->SizeWindow(1);
		}
	}
}


TWindowMenuItem*
TTeamMenuItem::ExpandedWindowItem(int32 id)
{
	if (!fExpanded) {
		// Paranoia
		return NULL;
	}

	TExpandoMenuBar* parent = static_cast<TExpandoMenuBar*>(Menu());
	int childIndex = parent->IndexOf(this) + 1;

	while (!parent->SubmenuAt(childIndex)
		&& childIndex < parent->CountItems()) {
		TWindowMenuItem* item
			= static_cast<TWindowMenuItem*>(parent->ItemAt(childIndex));
		if (item->ID() == id)
			return item;

		childIndex++;
	}
	return NULL;
}


BRect
TTeamMenuItem::ExpanderBounds() const
{
	BRect bounds(Frame());
	bounds.left = bounds.right - kSwitchWidth;
	return bounds;
}


//	#pragma mark - Private methods


void
TTeamMenuItem::_Init(BList* team, BBitmap* icon, char* name, char* signature,
	float width, float height)
{
	if (sHPad == 0.0f) {
		// Initialize the padding values.
		sHPad = be_control_look->ComposeSpacing(B_USE_SMALL_SPACING);
		sVPad = ceilf(be_control_look->ComposeSpacing(B_USE_SMALL_SPACING) / 4.0f);
		sLabelOffset = ceilf((be_control_look->DefaultLabelSpacing() / 3.0f) * 4.0f);
	}

	fTeam = team;
	fIcon = icon;
	fSignature = signature;

	if (name == NULL) {
		char temp[32];
		snprintf(temp, sizeof(temp), "team %ld", (addr_t)team->ItemAt(0));
		name = strdup(temp);
	}

	SetLabel(name);

	fOverrideWidth = width;
	fOverrideHeight = height;

	fBarView = static_cast<TBarApp*>(be_app)->BarView();

	BFont font(be_plain_font);
	fLabelWidth = ceilf(font.StringWidth(name));
	font_height fontHeight;
	font.GetHeight(&fontHeight);
	fLabelAscent = ceilf(fontHeight.ascent);
	fLabelDescent = ceilf(fontHeight.descent + fontHeight.leading);
	fLabelHeight = fLabelAscent + fLabelDescent;

	fOverriddenSelected = false;

	fExpanded = false;
	fArrowDirection = BControlLook::B_RIGHT_ARROW;
}


bool
TTeamMenuItem::_IsSelected() const
{
	return IsSelected() || fOverriddenSelected;
}
