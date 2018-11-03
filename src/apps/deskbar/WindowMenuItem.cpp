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


#include "WindowMenuItem.h"

#include <Bitmap.h>
#include <Debug.h>
#include <NaturalCompare.h>

#include "BarApp.h"
#include "BarMenuBar.h"
#include "BarView.h"
#include "ExpandoMenuBar.h"
#include "ResourceSet.h"
#include "TeamMenu.h"
#include "WindowMenu.h"

#include "icons.h"


const float kHPad = 10.0f;
const float kVPad = 2.0f;
const float kLabelOffset = 8.0f;

const BRect kIconRect(1.0f, 1.0f, 13.0f, 14.0f);


//	#pragma mark - TWindowMenuItem


TWindowMenuItem::TWindowMenuItem(const char* name, int32 id, bool mini,
	bool currentWorkspace, bool dragging)
	:
	TTruncatableMenuItem(name, NULL),
	fID(id),
	fMini(mini),
	fCurrentWorkSpace(currentWorkspace),
	fDragging(dragging),
	fExpanded(false),
	fRequireUpdate(false),
	fModified(false)
{
	_Init(name);
}


void
TWindowMenuItem::GetContentSize(float* width, float* height)
{
	if (width != NULL) {
		if (!fExpanded) {
			*width = kHPad + fLabelWidth + kHPad;
			if (fID >= 0)
				*width += fBitmap->Bounds().Width() + kLabelOffset;
		} else
			*width = Frame().Width()/* - kHPad*/;
	}

	// Note: when the item is in "expanded mode", ie embedded into
	// the Deskbar itself, then a truncated label is used in SetLabel()
	// The code here is ignorant of this fact, but it doesn't seem to
	// hurt anything.

	if (height != NULL) {
		*height = (fID >= 0) ? fBitmap->Bounds().Height() : 0.0f;
		float labelHeight = fLabelAscent + fLabelDescent;
		*height = (labelHeight > *height) ? labelHeight : *height;
		*height += kVPad * 2;
	}
}


void
TWindowMenuItem::Draw()
{
	if (!fExpanded) {
		BMenuItem::Draw();
		return;
	}

	// TODO: Tint this smartly based on the low color, this does
	// nothing to black.
	rgb_color menuColor = tint_color(ui_color(B_MENU_BACKGROUND_COLOR), 1.07);
	BRect frame(Frame());
	BMenu* menu = Menu();

	menu->PushState();

	// if not selected or being tracked on, fill with gray
	TBarView* barview = (static_cast<TBarApp*>(be_app))->BarView();
	if ((!IsSelected() && !menu->IsRedrawAfterSticky())
		|| barview->Dragging() || !IsEnabled()) {

		rgb_color shadow = tint_color(menuColor, 1.09);
		menu->SetHighColor(shadow);
		frame.right = frame.left + kHPad / 2;
		menu->FillRect(frame);

		menu->SetHighColor(menuColor);
		frame.left = frame.right + 1;
		frame.right = Frame().right;
		menu->FillRect(frame);
	}

	if (IsEnabled() && IsSelected() && !menu->IsRedrawAfterSticky()) {
		// fill
		rgb_color backColor = tint_color(menuColor,
			B_HIGHLIGHT_BACKGROUND_TINT);
		menu->SetLowColor(backColor);
		menu->SetHighColor(backColor);
		menu->FillRect(frame);
	} else {
		menu->SetLowColor(menuColor);
		menu->SetHighColor(menuColor);
	}

	DrawContent();

	menu->PopState();
}


void
TWindowMenuItem::DrawContent()
{
	BMenu* menu = Menu();
	BPoint contentLocation = ContentLocation() + BPoint(kHPad, 0);

	if (fID >= 0) {
		menu->SetDrawingMode(B_OP_OVER);

		float width = fBitmap->Bounds().Width();
		if (width > 16)
			contentLocation.x -= 8;

		float height;
		GetContentSize(NULL, &height);
		contentLocation.y += (height - fBitmap->Bounds().Height()) / 2;

		menu->MovePenTo(contentLocation);
		menu->DrawBitmapAsync(fBitmap);

		if (width > 16)
			contentLocation.x += 8;

		contentLocation.x += kIconRect.Width() + kLabelOffset;
	}
	contentLocation.y = ContentLocation().y + kVPad + fLabelAscent;

	menu->SetDrawingMode(B_OP_COPY);
	menu->MovePenTo(contentLocation);

	if (IsSelected())
		menu->SetHighColor(ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR));
	else
		menu->SetHighColor(ui_color(B_MENU_ITEM_TEXT_COLOR));

	float labelWidth = menu->StringWidth(Label());
	BPoint penLocation = menu->PenLocation();
	float offset = penLocation.x - Frame().left;

	menu->DrawString(Label(labelWidth + offset));
}


status_t
TWindowMenuItem::Invoke(BMessage* /*message*/)
{
	if (!fDragging) {
		if (fID >= 0) {
			int32 action = (modifiers() & B_CONTROL_KEY) != 0
				? B_MINIMIZE_WINDOW : B_BRING_TO_FRONT;

			bool doZoom = false;
			BRect zoomRect(0.0f, 0.0f, 0.0f, 0.0f);
			BMenuItem* item;
			if (!fExpanded)
				item = Menu()->Superitem();
			else
				item = this;

			if (item->Menu()->Window() != NULL) {
				zoomRect = item->Menu()->ConvertToScreen(item->Frame());
				doZoom = (fMini && action == B_BRING_TO_FRONT)
					|| (!fMini && action == B_MINIMIZE_WINDOW);
			}

			do_window_action(fID, action, zoomRect, doZoom);
		}
	}
	return B_OK;
}


void
TWindowMenuItem::SetTo(const char* name, int32 id, bool mini,
	bool currentWorkspace, bool dragging)
{
	fModified = fCurrentWorkSpace != currentWorkspace || fMini != mini;

	fID = id;
	fMini = mini;
	fCurrentWorkSpace = currentWorkspace;
	fDragging = dragging;
	fRequireUpdate = false;

	_Init(name);
}


/*static*/ int32
TWindowMenuItem::InsertIndexFor(BMenu* menu, int32 startIndex,
	TWindowMenuItem* newItem)
{
	for (int32 index = startIndex;; index++) {
		TWindowMenuItem* item
			= dynamic_cast<TWindowMenuItem*>(menu->ItemAt(index));
		if (item == NULL
			|| NaturalCompare(item->Label(), newItem->Label()) > 0) {
			return index;
		}
	}
}


//	#pragma mark - private methods


void
TWindowMenuItem::_Init(const char* name)
{
	if (fMini) {
		fBitmap = fCurrentWorkSpace
			? AppResSet()->FindBitmap(B_MESSAGE_TYPE, R_WindowHiddenIcon)
			: AppResSet()->FindBitmap(B_MESSAGE_TYPE, R_WindowHiddenSwitchIcon);
	} else {
		fBitmap = fCurrentWorkSpace
			? AppResSet()->FindBitmap(B_MESSAGE_TYPE, R_WindowShownIcon)
			: AppResSet()->FindBitmap(B_MESSAGE_TYPE, R_WindowShownSwitchIcon);
	}

	BFont font(be_plain_font);
	fLabelWidth = ceilf(font.StringWidth(name));
	font_height fontHeight;
	font.GetHeight(&fontHeight);
	fLabelAscent = ceilf(fontHeight.ascent);
	fLabelDescent = ceilf(fontHeight.descent + fontHeight.leading);

	SetLabel(name);
}
