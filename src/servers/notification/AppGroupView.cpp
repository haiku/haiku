/*
 * Copyright 2010-2017, Haiku, Inc. All Rights Reserved.
 * Copyright 2008-2009, Pier Luigi Fiorini. All Rights Reserved.
 * Copyright 2004-2008, Michael Davidson. All Rights Reserved.
 * Copyright 2004-2007, Mikael Eiman. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Davidson, slaad@bong.com.au
 *		Mikael Eiman, mikael@eiman.tv
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 *		Brian Hill, supernova@tycho.email
 */

#include <algorithm>

#include <ControlLook.h>
#include <GroupLayout.h>
#include <GroupView.h>

#include "AppGroupView.h"

#include "NotificationWindow.h"
#include "NotificationView.h"

const float kCloseSize				= 6;
const float kEdgePadding			= 2;


AppGroupView::AppGroupView(const BMessenger& messenger, const char* label)
	:
	BGroupView("appGroup", B_VERTICAL, 0),
	fLabel(label),
	fMessenger(messenger),
	fCollapsed(false),
	fCloseClicked(false),
	fPreviewModeOn(false)
{
	SetFlags(Flags() | B_WILL_DRAW);

	fHeaderSize = be_bold_font->Size()
		+ be_control_look->ComposeSpacing(B_USE_ITEM_SPACING);
	static_cast<BGroupLayout*>(GetLayout())->SetInsets(0, fHeaderSize, 0, 0);
}


void
AppGroupView::Draw(BRect updateRect)
{
	rgb_color menuColor = ViewColor();
	BRect bounds = Bounds();
	bounds.bottom = bounds.top + fHeaderSize;

	// Draw the header background
	SetHighColor(tint_color(menuColor, 1.22));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	StrokeLine(bounds.LeftTop(), bounds.LeftBottom());
	uint32 borders = BControlLook::B_TOP_BORDER
		| BControlLook::B_BOTTOM_BORDER | BControlLook::B_RIGHT_BORDER;
	be_control_look->DrawButtonBackground(this, bounds, bounds, menuColor,
		0, borders);

	// Draw the buttons
	fCollapseRect.top = (fHeaderSize - kExpandSize) / 2;
	fCollapseRect.left = kEdgePadding * 3;
	fCollapseRect.right = fCollapseRect.left + 1.5 * kExpandSize;
	fCollapseRect.bottom = fCollapseRect.top + kExpandSize;

	fCloseRect = bounds;
	fCloseRect.top = (fHeaderSize - kCloseSize) / 2;
	// Take off the 1 to line this up with the close button on the
	// notification view
	fCloseRect.right -= kEdgePadding * 3 - 1;
	fCloseRect.left = fCloseRect.right - kCloseSize;
	fCloseRect.bottom = fCloseRect.top + kCloseSize;

	uint32 arrowDirection = fCollapsed
		? BControlLook::B_DOWN_ARROW : BControlLook::B_UP_ARROW;
	be_control_look->DrawArrowShape(this, fCollapseRect, fCollapseRect,
		LowColor(), arrowDirection, 0, B_DARKEN_3_TINT);

	SetPenSize(kPenSize);

	// Draw the dismiss widget
	_DrawCloseButton(updateRect);

	// Draw the label
	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
	BString label = fLabel;
	if (fCollapsed)
		label << " (" << fInfo.size() << ")";

	SetFont(be_bold_font);
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	float y = (bounds.top + bounds.bottom - ceilf(fontHeight.ascent)
		- ceilf(fontHeight.descent)) / 2.0 + ceilf(fontHeight.ascent);

	DrawString(label.String(),
		BPoint(fCollapseRect.right + 4 * kEdgePadding, y));
}


void
AppGroupView::_DrawCloseButton(const BRect& updateRect)
{
	PushState();
	BRect closeRect = fCloseRect;

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	float tint = B_DARKEN_2_TINT;

	if (fCloseClicked) {
		BRect buttonRect(closeRect.InsetByCopy(-4, -4));
		be_control_look->DrawButtonFrame(this, buttonRect, updateRect,
			base, base,
			BControlLook::B_ACTIVATED | BControlLook::B_BLEND_FRAME);
		be_control_look->DrawButtonBackground(this, buttonRect, updateRect,
			base, BControlLook::B_ACTIVATED);
		tint *= 1.2;
		closeRect.OffsetBy(1, 1);
	}

	base = tint_color(base, tint);
	SetHighColor(base);
	SetPenSize(2);
	StrokeLine(closeRect.LeftTop(), closeRect.RightBottom());
	StrokeLine(closeRect.LeftBottom(), closeRect.RightTop());
	PopState();
}


void
AppGroupView::MouseDown(BPoint point)
{
	// Preview Mode ignores any mouse clicks
	if (fPreviewModeOn)
		return;

	if (BRect(fCloseRect).InsetBySelf(-5, -5).Contains(point)) {
		int32 children = fInfo.size();
		for (int32 i = 0; i < children; i++) {
			fInfo[i]->RemoveSelf();
			delete fInfo[i];
		}

		fInfo.clear();

		// Remove ourselves from the parent view
		BMessage message(kRemoveGroupView);
		message.AddPointer("view", this);
		fMessenger.SendMessage(&message);
	} else if (BRect(fCollapseRect).InsetBySelf(-5, -5).Contains(point)) {
		fCollapsed = !fCollapsed;
		int32 children = fInfo.size();
		if (fCollapsed) {
			for (int32 i = 0; i < children; i++) {
				if (!fInfo[i]->IsHidden())
					fInfo[i]->Hide();
			}
			GetLayout()->SetExplicitMaxSize(GetLayout()->MinSize());
		} else {
			for (int32 i = 0; i < children; i++) {
				if (fInfo[i]->IsHidden())
					fInfo[i]->Show();
			}
			GetLayout()->SetExplicitMaxSize(BSize(B_SIZE_UNSET, B_SIZE_UNSET));
		}

		InvalidateLayout();
		Invalidate(); // Need to redraw the collapse indicator and title
	}
}


void
AppGroupView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kRemoveView:
		{
			NotificationView* view = NULL;
			if (msg->FindPointer("view", (void**)&view) != B_OK)
				return;

			infoview_t::iterator vIt = find(fInfo.begin(), fInfo.end(), view);
			if (vIt == fInfo.end())
				break;

			fInfo.erase(vIt);
			view->RemoveSelf();
			delete view;

			fMessenger.SendMessage(msg);

			if (!this->HasChildren()) {
				Hide();
				BMessage removeSelfMessage(kRemoveGroupView);
				removeSelfMessage.AddPointer("view", this);
				fMessenger.SendMessage(&removeSelfMessage);
			}

			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}


void
AppGroupView::AddInfo(NotificationView* view)
{
	BString id = view->MessageID();
	bool found = false;

	if (id.Length() > 0) {
		int32 children = fInfo.size();
		for (int32 i = 0; i < children; i++) {
			if (id == fInfo[i]->MessageID()) {
				NotificationView* oldView = fInfo[i];
				oldView->RemoveSelf();
				delete oldView;
				fInfo[i] = view;
				found = true;
				break;
			}
		}
	}

	// Invalidate all children to show or hide the close buttons in the
	// notification view
	int32 children = fInfo.size();
	for (int32 i = 0; i < children; i++) {
		fInfo[i]->Invalidate();
	}

	if (!found) {
		fInfo.push_back(view);
	}
	GetLayout()->AddView(view);

	if (IsHidden())
		Show();
	if (view->IsHidden(view) && !fCollapsed)
		view->Show();
}


void
AppGroupView::SetPreviewModeOn(bool enabled)
{
	fPreviewModeOn = enabled;
}


const BString&
AppGroupView::Group() const
{
	return fLabel;
}


void
AppGroupView::SetGroup(const char* group)
{
	fLabel.SetTo(group);
	Invalidate();
}


bool
AppGroupView::HasChildren()
{
	return !fInfo.empty();
}


int32
AppGroupView::ChildrenCount()
{
	return fInfo.size();
}
