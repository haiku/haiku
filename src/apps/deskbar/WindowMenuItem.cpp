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

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

#include <Debug.h>
#include <stdio.h>
#include <Bitmap.h>

#include "BarApp.h"
#include "BarMenuBar.h"
#include "ExpandoMenuBar.h"
#include "ResourceSet.h"
#include "TeamMenu.h"
#include "WindowMenu.h"
#include "WindowMenuItem.h"
#include "icons.h"


const float	kHPad = 10.0f;
const float	kVPad = 2.0f;
const float	kLabelOffset = 8.0f;
const BRect	kIconRect(1.0f, 1.0f, 13.0f, 14.0f);


TWindowMenuItem::TWindowMenuItem(const char *title, int32 id,
	bool mini, bool currentWorkspace, bool dragging)
	:	BMenuItem(title, NULL),
	fID(id),
	fMini(mini),
	fCurrentWorkSpace(currentWorkspace),
	fDragging(dragging),
	fExpanded(false),
	fRequireUpdate(false),
	fModified(false),
	fFullTitle("")
{
	Initialize(title);
}


void
TWindowMenuItem::Initialize(const char *title)
{
	if (fMini)
 		fBitmap = fCurrentWorkSpace
			? AppResSet()->FindBitmap(B_MESSAGE_TYPE, R_WindowHiddenIcon)
			: AppResSet()->FindBitmap(B_MESSAGE_TYPE, R_WindowHiddenSwitchIcon);
	else
 		fBitmap = fCurrentWorkSpace
			? AppResSet()->FindBitmap(B_MESSAGE_TYPE, R_WindowShownIcon)
			: AppResSet()->FindBitmap(B_MESSAGE_TYPE, R_WindowShownSwitchIcon);

	BFont font(be_plain_font);
	fTitleWidth = ceilf(font.StringWidth(title));
	font_height fontHeight;
	font.GetHeight(&fontHeight);
	fTitleAscent = ceilf(fontHeight.ascent);
	fTitleDescent = ceilf(fontHeight.descent + fontHeight.leading);
}


void
TWindowMenuItem::SetTo(const char *title, int32 id,
	bool mini, bool currentWorkspace, bool dragging)
{
	fModified = fCurrentWorkSpace != currentWorkspace || fMini != mini;

	fID = id;
	fMini = mini;
	fCurrentWorkSpace = currentWorkspace;
	fDragging = dragging;
	fRequireUpdate = false;

	Initialize(title);
}


bool
TWindowMenuItem::ChangedState()
{
	return fModified;
}


void
TWindowMenuItem::SetLabel(const char* string)
{
	fFullTitle = string;
	BString truncatedTitle = fFullTitle;

	if (fExpanded && Menu()) {
		BPoint contLoc = ContentLocation() + BPoint(kHPad, kVPad);
		contLoc.x += kIconRect.Width() + kLabelOffset;
	
		be_plain_font->TruncateString(&truncatedTitle, B_TRUNCATE_MIDDLE,
									  Frame().Width() - contLoc.x - 3.0f);
	}

	if (strcmp(Label(), truncatedTitle.String()) != 0)
		BMenuItem::SetLabel(truncatedTitle.String());
}


int32
TWindowMenuItem::ID()
{
	return fID;
}


void
TWindowMenuItem::GetContentSize(float *width, float *height)
{
	if (!fExpanded) {
		*width = kHPad + fTitleWidth + kHPad;
		if (fID >= 0)
			*width += fBitmap->Bounds().Width() + kLabelOffset;
	} else
		*width = Frame().Width()/* - kHPad*/;

	// Note: when the item is in "expanded mode", ie embedded into
	// the Deskbar itself, then a truncated label is used in SetLabel()
	// The code here is ignorant of this fact, but it doesn't seem to
	// hurt anything.

	*height = (fID >= 0) ? fBitmap->Bounds().Height() : 0.0f;
	float labelHeight = fTitleAscent + fTitleDescent;
	*height = (labelHeight > *height) ? labelHeight : *height;
	*height += kVPad * 2;
}


void
TWindowMenuItem::Draw()
{
	if (fExpanded) {
		rgb_color menuColor = ui_color(B_MENU_BACKGROUND_COLOR);
		BRect frame(Frame());
		BMenu *menu = Menu();

		menu->PushState();

		//	if not selected or being tracked on, fill with gray
		TBarView *barview = (static_cast<TBarApp *>(be_app))->BarView();
		if (!IsSelected() && !menu->IsRedrawAfterSticky() || barview->Dragging() || !IsEnabled()) {
			menu->SetHighColor(menuColor);
			menu->FillRect(frame);

			if (fExpanded) {
				rgb_color shadow = tint_color(menuColor, (B_NO_TINT + B_DARKEN_1_TINT) / 2.0f);
				menu->SetHighColor(shadow);
				frame.right = frame.left + kHPad / 2;
				menu->FillRect(frame);
			}	
		}

		if (IsEnabled() && IsSelected() && !menu->IsRedrawAfterSticky()) {
			// fill
			menu->SetHighColor(tint_color(menuColor, B_HIGHLIGHT_BACKGROUND_TINT));
			menu->FillRect(frame);
		} else 
			menu->SetLowColor(menuColor);

		DrawContent();
		menu->PopState();
	}
	BMenuItem::Draw();
}


void
TWindowMenuItem::DrawContent()
{
	BMenu *menu = Menu();
	menu->PushState();

	BRect frame(Frame());
	BPoint contLoc = ContentLocation() + BPoint(kHPad, kVPad);
//	if (fExpanded)
//		contLoc.x += kHPad;
	menu->SetDrawingMode(B_OP_COPY);

	if (fID >= 0) {
		menu->SetDrawingMode(B_OP_OVER);
		
		float width = fBitmap->Bounds().Width();

		if (width > 16)
			contLoc.x -= 8;

		menu->MovePenTo(contLoc);
		menu->DrawBitmapAsync(fBitmap);

		if (width > 16)
			contLoc.x += 8;

		contLoc.x += kIconRect.Width() + kLabelOffset;

		menu->SetDrawingMode(B_OP_COPY);
	}
	contLoc.y = frame.top + ((frame.Height() - fTitleAscent - fTitleDescent) / 2) + 1.0f;
	menu->PopState();

	menu->MovePenTo(contLoc);
	// Set the pen color so that the label is always visible.
	menu->SetHighColor(0, 0, 0);

	BMenuItem::DrawContent();
}


status_t
TWindowMenuItem::Invoke(BMessage *)
{
	if (!fDragging) {
		if (fID >= 0) {
			int32 action = (modifiers() & B_CONTROL_KEY)
				? B_MINIMIZE_WINDOW :B_BRING_TO_FRONT;

			bool doZoom = false;
			BRect zoomRect(0.0f, 0.0f, 0.0f, 0.0f);
			BMenuItem *item;
			if (!fExpanded)
				item = Menu()->Superitem();
			else
				item = this;

			if (item->Menu()->Window() != NULL) {
				zoomRect = item->Menu()->ConvertToScreen(item->Frame());
				doZoom = fMini && (action == B_BRING_TO_FRONT) ||
						  !fMini && (action == B_MINIMIZE_WINDOW);
			}

			do_window_action(fID, action, zoomRect, doZoom);
		}
	}
	return B_OK;
}


void
TWindowMenuItem::ExpandedItem(bool status)
{
	if (fExpanded != status) {
		fExpanded = status;
		SetLabel(fFullTitle.String());
	}
}


void
TWindowMenuItem::SetRequireUpdate()
{
	fRequireUpdate = true;
}


bool
TWindowMenuItem::RequiresUpdate()
{
	return fRequireUpdate;
}

