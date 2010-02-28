/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2001, Be Incorporated. All rights reserved.

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

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/


#include "BmapButton.h"

#include <Application.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <ColorTools.h>
#include <Resources.h>

#include <stdlib.h>


BList BmapButton::fBitmapCache;
BLocker BmapButton::fBmCacheLock;

		
struct BitmapItem {
	BBitmap* bm;
	int32 id;
	int32 openCount;
};


BmapButton::BmapButton(BRect frame, 
	const char* name, 
	const char* label,
	int32 enabledID,
	int32 disabledID,
	int32 rollID,
	int32 pressedID,
	bool showLabel,
	BMessage* message,
	uint32 resizeMask,
	uint32 flags) :
	BControl(frame, name, label, message, resizeMask, flags),
	fPressing(false),
	fIsInBounds(false),
	fShowLabel(showLabel),
	fActive(true),
	fIButtons(0)
{
	fEnabledBM = RetrieveBitmap(enabledID);
	fDisabledBM = RetrieveBitmap(disabledID);
	fRollBM = RetrieveBitmap(rollID);
	fPressedBM = RetrieveBitmap(pressedID);
}


BmapButton::~BmapButton(void)
{
	ReleaseBitmap(fEnabledBM);
	ReleaseBitmap(fDisabledBM);
	ReleaseBitmap(fRollBM);
	ReleaseBitmap(fPressedBM);
}


const BBitmap*
BmapButton::RetrieveBitmap(int32 id)
{
	// Lock access to the list
	BAutolock lock(fBmCacheLock);
	if (!lock.IsLocked())
		return NULL;
	
	// Check for the bitmap in the cache first
	BitmapItem* item;
	for (int32 i=0; (item=(BitmapItem*)fBitmapCache.ItemAt(i)) != NULL; i++) {
		if (item->id == id) {
			item->openCount++;
			return item->bm;
		}
	}
	
	// If it's not in the cache, try to load it 
	BResources* res = BApplication::AppResources();
	if (!res) return NULL;
	
	size_t size = 0;
	const void*  data = res->LoadResource('BMAP', id, &size);
	if (!data) return NULL;
	BMemoryIO mio(data, size);
	BMessage arch;
	if (arch.Unflatten(&mio) != B_OK) return NULL;
	
	BArchivable* obj = instantiate_object(&arch);
	BBitmap* bm = dynamic_cast<BBitmap*>(obj);
	if (!bm) {
		delete obj;
		return NULL;
	}
	
	item = (BitmapItem*)malloc(sizeof(BitmapItem));
	item->bm = bm;
	item->id = id;
	item->openCount = 1;
	fBitmapCache.AddItem(item);
	return bm;
}


status_t
BmapButton::ReleaseBitmap(const BBitmap* bm)
{
	BAutolock lock(fBmCacheLock);
	if (!lock.IsLocked())
		return B_ERROR;
		
	BitmapItem* item;
	for (int32 i = 0;; i++)
	{
		item = static_cast<BitmapItem*>(fBitmapCache.ItemAt(i));
		if (item == NULL)
			break;

		if (item->bm == bm) {
			if (--item->openCount <= 0) {
				fBitmapCache.RemoveItem(i);
				delete item->bm;
				free(item);
			}
			return B_OK;
		}
	}
	return B_ERROR;
}

#define F_SHOW_GEOMETRY 0

void
BmapButton::Draw(BRect updateRect)
{
	BRect bounds(Bounds());
	float labelHeight, labelWidth;

#if F_SHOW_GEOMETRY
	StrokeRect(bounds);
#endif

	// Draw Label
	if (fShowLabel) {
		font_height	fheight;
		
		BFont renderFont;
		renderFont = *be_plain_font;
		renderFont.GetHeight(&fheight);
		SetFont(&renderFont);

		labelHeight = fheight.leading + fheight.ascent + fheight.descent + 1;
		labelWidth = renderFont.StringWidth(Label());

		BRect textRect;
		textRect.left = (bounds.right - bounds.left - labelWidth + 1) / 2;
		textRect.right = textRect.left + labelWidth;
		textRect.bottom = bounds.bottom;
		textRect.top = textRect.bottom - fheight.descent - fheight.ascent - 1;

		// Only draw if it's within the update rect
		if (updateRect.Intersects(textRect)) {
			float baseLine = textRect.bottom - fheight.descent;

			if (IsFocus() && fActive)
				SetHighColor(0, 0, 255);
			else
				SetHighColor(ViewColor());
			StrokeLine(BPoint(textRect.left, baseLine),
				BPoint(textRect.right, baseLine));

			if (IsEnabled())
				SetHighColor(0, 0, 0);
			else {
				const rgb_color black = {0, 0, 0, 255};
				SetHighColor(disable_color(black, ViewColor()));
			}
			MovePenTo(textRect.left, baseLine);
			DrawString(Label());

#if F_SHOW_GEOMETRY
			FrameRect(textRect);
#endif
		}
	} else {
		labelHeight = 0;
		labelWidth = 0;
	}

	// Draw Bitmap

	// Select the bitmap to use
	const BBitmap* bm;

	if (!IsEnabled())
		bm = fDisabledBM;
	else if (fPressing) {
		if (fIsInBounds)
			bm = fPressedBM;
		else
			bm = fRollBM;
	} else {
		if (fIsInBounds)
			bm = fRollBM;
		else
			bm = fEnabledBM;
	}

	// Draw the bitmap
	if (bm) {
		fBitmapRect = bm->Bounds();
		fBitmapRect.OffsetTo(0, 0);
		fBitmapRect.OffsetBy((bounds.right - bounds.left - fBitmapRect.right
				- fBitmapRect.left) / 2,
			(bounds.bottom - bounds.top - labelHeight - fBitmapRect.bottom
				- fBitmapRect.top) / 2);
		// Update if within update rect

		SetDrawingMode(B_OP_OVER);

		if (updateRect.Intersects(fBitmapRect)) {
			DrawBitmap(bm, fBitmapRect);
			#if F_SHOW_GEOMETRY
			StrokeRect(fBitmapRect);
			#endif
		}

		SetDrawingMode(B_OP_COPY);
	}
}


void
BmapButton::GetPreferredSize(float* width, float* height)
{
	BRect prefBounds;
	
	if (fEnabledBM) {
		if (fShowLabel) {
			float labelHeight, labelWidth;
			font_height	fheight;
			BRect bmBounds(fEnabledBM->Bounds());
			BFont renderFont;
			renderFont = *be_plain_font;
			renderFont.GetHeight(&fheight);
			SetFont(&renderFont);
			
			labelHeight = fheight.leading + fheight.ascent + fheight.descent + 1;
			labelWidth = renderFont.StringWidth(Label());
			prefBounds.left = 0;
			prefBounds.top = 0;
			prefBounds.right = labelWidth > (bmBounds.right - bmBounds.left)
				? labelWidth : (bmBounds.right - bmBounds.left);
			prefBounds.bottom = labelHeight + (bmBounds.bottom - bmBounds.top);
		} else
			prefBounds = fEnabledBM->Bounds();
	} else
		prefBounds = Bounds();
	
	*width = prefBounds.IntegerWidth();
	*height = prefBounds.IntegerHeight();
}


void
BmapButton::MouseMoved(BPoint where, uint32 code, const BMessage* msg)
{
	// eliminate unused parameter warnings
	(void)where;
	(void)msg;

	if (IsEnabled() && fActive) {
		switch(code) {
			case B_ENTERED_VIEW:
				fIsInBounds = true;
				Invalidate(fBitmapRect);
				break;
				
			case B_EXITED_VIEW:
				fIsInBounds = false;
				Invalidate(fBitmapRect);
				break;
		}
	}
}


void
BmapButton::MouseDown(BPoint point)
{
	if (!IsEnabled())
		return;
	// Save Mouse State
	GetMouse(&point, &fButtons);
	fWhere = point;
	
	if (fButtons & fIButtons) {
		BMessage copy(*Message());
		copy.AddPoint("where", ConvertToScreen(fWhere));
		copy.AddInt32("buttons", fButtons);
		Invoke(&copy);
		return;
	}
	
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS | B_SUSPEND_VIEW_FOCUS | B_NO_POINTER_HISTORY);
	fPressing = true;
	Invalidate(fBitmapRect);
}


void
BmapButton::MouseUp(BPoint where)
{
	if (atomic_and(&fPressing, 0)) {
		SetMouseEventMask(0, 0);
		if (Bounds().Contains(where) && IsEnabled()) {
			BMessage copy(*Message());
			copy.AddPoint("where", ConvertToScreen(fWhere));
			copy.AddInt32("buttons", fButtons);
			Invoke(&copy);
		}
		Invalidate(fBitmapRect);
	}
}


void
BmapButton::ShowLabel(bool show)
{
	fShowLabel = show;
}


void
BmapButton::WindowActivated(bool active)
{
	fActive = active;
	if (IsFocus() || fIsInBounds) {
		fIsInBounds = false;
		Invalidate();
	}
	BControl::WindowActivated(active);
}


void
BmapButton::InvokeOnButton(uint32 button)
{
	fIButtons = button;
}

