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

// defines the status area drawn in the bottom left corner of a Tracker window

#include <Application.h>

#include "AutoLock.h"
#include "Bitmaps.h"
#include "CountView.h"
#include "ContainerWindow.h"
#include "DirMenu.h"
#include "PoseView.h"

BCountView::BCountView(BRect bounds, BPoseView* view)
	:	BView(bounds, "CountVw", B_FOLLOW_LEFT + B_FOLLOW_BOTTOM,
			B_PULSE_NEEDED | B_WILL_DRAW),
		fLastCount(-1),
		fPoseView(view),
		fShowingBarberPole(false),
		fBarberPoleMap(NULL),
		fLastBarberPoleOffset(5),
		fStartSpinningAfter(0),
		fTypeAheadString("")
		
{
 	GetTrackerResources()->GetBitmapResource(B_MESSAGE_TYPE, kResBarberPoleBitmap,
		&fBarberPoleMap);
}

BCountView::~BCountView()
{
	delete fBarberPoleMap;
}

void
BCountView::TrySpinningBarberPole()
{
	if (!fShowingBarberPole)
		return;

	if (fStartSpinningAfter && system_time() < fStartSpinningAfter)
		return;

	if (fStartSpinningAfter) {
		fStartSpinningAfter = 0;
		Invalidate(BarberPoleOuterRect());
	} else
		Invalidate(BarberPoleInnerRect());
}

void 
BCountView::Pulse()
{
	TrySpinningBarberPole();
}

void 
BCountView::EndBarberPole()
{
	if (!fShowingBarberPole)
		return;
	
	fShowingBarberPole = false;
	Invalidate();
}

const bigtime_t kBarberPoleDelay = 500000;

void 
BCountView::StartBarberPole()
{
	AutoLock<BWindow> lock(Window());
	if (fShowingBarberPole)
		return;

	fShowingBarberPole = true;
	fStartSpinningAfter = system_time() + kBarberPoleDelay;
		// wait a bit before showing the barber pole
}

BRect 
BCountView::BarberPoleInnerRect() const
{
	BRect result = Bounds();
	result.InsetBy(3, 4);
	result.left = result.right - 7;
	result.bottom = result.top + 7;
	return result;
}

BRect 
BCountView::BarberPoleOuterRect() const
{
	BRect result(BarberPoleInnerRect());
	result.InsetBy(-1, -1);
	return result;
}

BRect
BCountView::TextInvalRect() const
{
	BRect result = Bounds();
	result.InsetBy(4, 2);
	result.right -= 10;

	return result;
}

void
BCountView::CheckCount()
{
	// invalidate the count text area if necessary
	if (fLastCount != fPoseView->CountItems()) {
		fLastCount = fPoseView->CountItems();
		Invalidate(TextInvalRect());
	}
	// invalidate barber pole area if necessary
	TrySpinningBarberPole();
}

void
BCountView::Draw(BRect)
{
	BRect bounds(Bounds());
	BRect barberPoleRect;
	BString itemString;
	if (!IsTypingAhead()) {
		if (fLastCount == 0) 
			itemString << "no items";
		else if (fLastCount == 1) 
			itemString << "1 item";
		else 
			itemString << fLastCount << " items";
	} else
		itemString << TypeAhead();
		
	
	BString string(itemString);
	BRect textRect(TextInvalRect());

	if (fShowingBarberPole && !fStartSpinningAfter) {
		barberPoleRect = BarberPoleOuterRect();
		TruncateString(&string, B_TRUNCATE_END, textRect.Width());
	}

	if (IsTypingAhead())
		// use a muted gray for the typeahead
		SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_4_TINT));
	else
		SetHighColor(0, 0, 0);
	MovePenTo(textRect.LeftBottom());
	DrawString(string.String());

	bounds.top++;

	rgb_color light = tint_color(ViewColor(), B_LIGHTEN_MAX_TINT);
	rgb_color shadow = tint_color(ViewColor(), B_DARKEN_2_TINT);
	rgb_color lightShadow = tint_color(ViewColor(), B_DARKEN_1_TINT);

	BeginLineArray(fShowingBarberPole && !fStartSpinningAfter ? 9 : 5);
	AddLine(bounds.LeftTop(), bounds.RightTop(), light);
	AddLine(bounds.LeftTop(), bounds.LeftBottom(), light);
	bounds.top--;

	AddLine(bounds.LeftTop(), bounds.RightTop(), shadow);
	AddLine(BPoint(bounds.right, bounds.top + 2), bounds.RightBottom(), lightShadow);
	AddLine(bounds.LeftBottom(), bounds.RightBottom(), lightShadow);

	if (!fShowingBarberPole || fStartSpinningAfter) {
		EndLineArray();
		return;
	}

	AddLine(barberPoleRect.LeftTop(), barberPoleRect.RightTop(), shadow);
	AddLine(barberPoleRect.LeftTop(), barberPoleRect.LeftBottom(), shadow);
	AddLine(barberPoleRect.LeftBottom(), barberPoleRect.RightBottom(), light);
	AddLine(barberPoleRect.RightBottom(), barberPoleRect.RightTop(), light);
	EndLineArray();

	barberPoleRect.InsetBy(1, 1);

	BRect destRect(fBarberPoleMap ? fBarberPoleMap->Bounds() : BRect(0, 0, 0, 0));
	destRect.OffsetTo(barberPoleRect.LeftTop() - BPoint(0, fLastBarberPoleOffset));
	fLastBarberPoleOffset -= 1;
	if (fLastBarberPoleOffset < 0)
		fLastBarberPoleOffset = 5;

	BRegion region;
	region.Set(BarberPoleInnerRect());
	ConstrainClippingRegion(&region);	

	if (fBarberPoleMap)
		DrawBitmap(fBarberPoleMap, destRect);
}

void
BCountView::MouseDown(BPoint)
{
	BContainerWindow *window = dynamic_cast<BContainerWindow *>(Window());
	window->Activate();
	window->UpdateIfNeeded();

	if (fPoseView->IsFilePanel() || !fPoseView->TargetModel())
		return;

	if (!window->TargetModel()->IsRoot()) {
		BDirMenu *menu = new BDirMenu(NULL, B_REFS_RECEIVED);
		BEntry entry;
		if (entry.SetTo(window->TargetModel()->EntryRef()) == B_OK)
			menu->Populate(&entry, Window(), false, false, true, false, true);
		else
			menu->Populate(NULL, Window(), false, false, true, false, true);

		menu->SetTargetForItems(be_app);
		BPoint pop_pt = Bounds().LeftBottom();
		pop_pt.y += 3;
		ConvertToScreen(&pop_pt);
		BRect mouse_rect(Bounds());
		ConvertToScreen(&mouse_rect);
		menu->Go(pop_pt, true, true, mouse_rect);
		delete menu;
	}
}

void
BCountView::AttachedToWindow()
{
	SetFont(be_plain_font);
	SetFontSize(9);

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ViewColor());

	CheckCount();
}

void 
BCountView::SetTypeAhead(const char *string)
{
	fTypeAheadString = string;
	Invalidate();
}

const char * 
BCountView::TypeAhead() const
{
	return fTypeAheadString.String();
}

bool 
BCountView::IsTypingAhead() const
{
	return fTypeAheadString.Length() != 0;
}
