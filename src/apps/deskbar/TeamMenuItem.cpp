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

#include <Bitmap.h>
#include <ControlLook.h>
#include <Debug.h>
#include <Font.h>
#include <Region.h>
#include <Roster.h>
#include <Resources.h>

#include "BarApp.h"
#include "BarMenuBar.h"
#include "BarView.h"
#include "ExpandoMenuBar.h"
#include "ResourceSet.h"
#include "ShowHideMenuItem.h"
#include "TeamMenu.h"
#include "WindowMenu.h"
#include "WindowMenuItem.h"


const float kHPad = 8.0f;
const float kVPad = 1.0f;
const float kLabelOffset = 8.0f;
const float kSwitchWidth = 12;


TTeamMenuItem::TTeamMenuItem(BList* team, BBitmap* icon, char* name, char* sig,
	float width, float height, bool drawLabel, bool vertical)
	:
	BMenuItem(new TWindowMenu(team, sig))
{
	_InitData(team, icon, name, sig, width, height, drawLabel, vertical);
}


TTeamMenuItem::TTeamMenuItem(float width, float height, bool vertical)
	:
	BMenuItem("", NULL)
{
	_InitData(NULL, NULL, strdup(""), strdup(""), width, height, false,
		vertical);
}


TTeamMenuItem::~TTeamMenuItem()
{
	delete fTeam;
	delete fIcon;
	free(fName);
	free(fSig);
}


status_t
TTeamMenuItem::Invoke(BMessage* message)
{
	if (fBarView->InvokeItem(Signature())) {
		// handles drop on application
		return B_OK;
	}

	// if the app could not handle the drag message
	// and we were dragging, then kill the drag
	// should never get here, disabled item will not invoke
	if (fBarView != NULL && fBarView->Dragging())
		fBarView->DragStop();

	// bring to front or minimize shortcuts
	uint32 mods = modifiers();
	if (mods & B_CONTROL_KEY) {
		TShowHideMenuItem::TeamShowHideCommon((mods & B_SHIFT_KEY)
			? B_MINIMIZE_WINDOW : B_BRING_TO_FRONT, Teams());
	}

	return BMenuItem::Invoke(message);
}


void
TTeamMenuItem::SetOverrideWidth(float width)
{
	fOverrideWidth = width;
}


void
TTeamMenuItem::SetOverrideHeight(float height)
{
	fOverrideHeight = height;
}


void
TTeamMenuItem::SetOverrideSelected(bool selected)
{
	fOverriddenSelected = selected;
	Highlight(selected);
}


void
TTeamMenuItem::SetArrowDirection(int32 direction)
{
	fArrowDirection = direction;
}


void
TTeamMenuItem::SetHasLabel(bool drawLabel)
{
	fDrawLabel = drawLabel;
}


void
TTeamMenuItem::GetContentSize(float* width, float* height)
{
	BRect iconBounds;

	if (fIcon != NULL)
		iconBounds = fIcon->Bounds();
	else
		iconBounds = BRect(0, 0, kMinimumIconSize - 1, kMinimumIconSize - 1);

	BMenuItem::GetContentSize(width, height);

	if (fOverrideWidth != -1.0f)
		*width = fOverrideWidth;
	else {
		*width = kHPad + iconBounds.Width() + kHPad;
		if (iconBounds.Width() <= 32 && fDrawLabel)
			*width += LabelWidth() + kHPad;
	}

	if (fOverrideHeight != -1.0f)
		*height = fOverrideHeight;
	else {
		if (fVertical) {
			*height = iconBounds.Height() + kVPad * 4;
			if (fDrawLabel && iconBounds.Width() > 32)
				*height += fLabelAscent + fLabelDescent;
		} else {
			*height = iconBounds.Height() + kVPad * 4;
		}
	}
	*height += 2;
}


void
TTeamMenuItem::Draw()
{
	BRect frame(Frame());
	BMenu* menu = Menu();

	menu->PushState();

	rgb_color menuColor = menu->LowColor();
	bool canHandle = !fBarView->Dragging()
		|| fBarView->AppCanHandleTypes(Signature());
	uint32 flags = 0;
	if (_IsSelected() && canHandle)
		flags |= BControlLook::B_ACTIVATED;

	uint32 borders = BControlLook::B_TOP_BORDER;
	if (fVertical) {
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
	if (fIcon != NULL) {
		if (fIcon->ColorSpace() == B_RGBA32) {
			menu->SetDrawingMode(B_OP_ALPHA);
			menu->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		} else
			menu->SetDrawingMode(B_OP_OVER);

		BRect frame(Frame());
		BRect iconBounds(fIcon->Bounds());
		BRect dstRect(iconBounds);
		float extra = fVertical ? 0.0f : -1.0f;
		BPoint contLoc = ContentLocation();
		BPoint drawLoc = contLoc + BPoint(kHPad, kVPad);

		if (!fDrawLabel || (fVertical && iconBounds.Width() > 32)) {
			float offsetx = contLoc.x
				+ ((frame.Width() - iconBounds.Width()) / 2) + extra;
			float offsety = contLoc.y + 3.0f + extra;

			dstRect.OffsetTo(BPoint(offsetx, offsety));
			menu->DrawBitmapAsync(fIcon, dstRect);

			drawLoc.x = ((frame.Width() - LabelWidth()) / 2);
			drawLoc.y = frame.top + iconBounds.Height() + 4.0f;
		} else {
			float offsetx = contLoc.x + kHPad;
			float offsety = contLoc.y + 
				((frame.Height() - iconBounds.Height()) / 2) + extra;

			dstRect.OffsetTo(BPoint(offsetx, offsety));
			menu->DrawBitmapAsync(fIcon, dstRect);

			float labelHeight = fLabelAscent + fLabelDescent;
			drawLoc.x += iconBounds.Width() + kLabelOffset;
			drawLoc.y = frame.top + ((frame.Height() - labelHeight) / 2) + extra;
		}

		menu->MovePenTo(drawLoc);
	}

	if (fDrawLabel) {
		menu->SetDrawingMode(B_OP_OVER);
		menu->SetHighColor(ui_color(B_MENU_ITEM_TEXT_COLOR));

		// override the drawing of the content when the item is disabled
		// the wrong lowcolor is used when the item is disabled since the
		// text color does not change
		DrawContentLabel();
	}

	if (fVertical && static_cast<TBarApp*>(be_app)->Settings()->superExpando
		&& fBarView->ExpandoState()) {
		DrawExpanderArrow();
	}
}


void
TTeamMenuItem::DrawContentLabel()
{
	BMenu* menu = Menu();
	menu->MovePenBy(0, fLabelAscent);

	float cachedWidth = menu->StringWidth(Label());
	if (Submenu() && fVertical)
		cachedWidth += 18;

	const char* label = Label();
	char* truncLabel = NULL;
	float max = 0;

	if (fVertical && static_cast<TBarApp*>(be_app)->Settings()->superExpando)
		max = menu->MaxContentWidth() - kSwitchWidth;
	else
		max = menu->MaxContentWidth() - 4.0f;

	if (max > 0) {
		BPoint penloc = menu->PenLocation();
		BRect frame = Frame();
		float offset = penloc.x - frame.left;
		if (cachedWidth + offset > max) {
			truncLabel = (char*)malloc(strlen(label) + 4);
			if (!truncLabel)
				return;
			TruncateLabel(max-offset, truncLabel);
			label = truncLabel;
		}
	}

	if (!label)
		label = Label();

	bool canHandle = !fBarView->Dragging()
		|| fBarView->AppCanHandleTypes(Signature());
	if (_IsSelected() && IsEnabled() && canHandle)
		menu->SetLowColor(tint_color(menu->LowColor(),
			B_HIGHLIGHT_BACKGROUND_TINT));
	else
		menu->SetLowColor(menu->LowColor());

	if (IsSelected())
		menu->SetHighColor(ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR));
	else
		menu->SetHighColor(ui_color(B_MENU_ITEM_TEXT_COLOR));

	menu->DrawString(label);

	free(truncLabel);
}


void
TTeamMenuItem::DrawExpanderArrow()
{
	BMenu* menu = Menu();
	BRect frame(Frame());
	BRect rect(0, 0, kSwitchWidth, 10);

	rect.OffsetTo(BPoint(frame.right - rect.Width(),
		ContentLocation().y + ((frame.Height() - rect.Height()) / 2)));
	be_control_look->DrawArrowShape(menu, rect, rect, menu->LowColor(),
		fArrowDirection, 0, B_DARKEN_3_TINT);
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
				int childIndex = 0;
				int totalChildren = sub->CountItems() - 4;
					// hide, show, close, separator.
				for (; childIndex < totalChildren; childIndex++) {
					windowItem = static_cast<TWindowMenuItem*>
						(sub->RemoveItem((int32)0));
					parent->AddItem(windowItem, myindex + childIndex);
					windowItem->ExpandedItem(true);
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
			int childIndex = parent->IndexOf(this) + 1;
			while (!parent->SubmenuAt(childIndex) && childIndex
				< parent->CountItems()) {
				windowItem = static_cast<TWindowMenuItem*>
					(parent->RemoveItem(childIndex));
				sub->AddItem(windowItem, 0);
				windowItem->ExpandedItem(false);
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
TTeamMenuItem::_InitData(BList* team, BBitmap* icon, char* name, char* sig,
	float width, float height, bool drawLabel, bool vertical)
{
	fTeam = team;
	fIcon = icon;
	fName = name;
	fSig = sig;
	if (fName == NULL) {
		char temp[32];
		snprintf(temp, sizeof(temp), "team %ld", (addr_t)team->ItemAt(0));
		fName = strdup(temp);
	}
	SetLabel(fName);
	fOverrideWidth = width;
	fOverrideHeight = height;
	fDrawLabel = drawLabel;
	fVertical = vertical;

	fBarView = static_cast<TBarApp*>(be_app)->BarView();
	BFont font(be_plain_font);
	fLabelWidth = ceilf(font.StringWidth(fName));
	font_height fontHeight;
	font.GetHeight(&fontHeight);
	fLabelAscent = ceilf(fontHeight.ascent);
	fLabelDescent = ceilf(fontHeight.descent + fontHeight.leading);

	fOverriddenSelected = false;

	fExpanded = false;
	fArrowDirection = BControlLook::B_RIGHT_ARROW;
}


bool
TTeamMenuItem::_IsSelected() const
{
	return IsSelected() || fOverriddenSelected;
}
