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

#include "CountView.h"

#include <Application.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Locale.h>

#include "AutoLock.h"
#include "Bitmaps.h"
#include "ContainerWindow.h"
#include "DirMenu.h"
#include "PoseView.h"
#include "Utilities.h"


const bigtime_t kBarberPoleDelay = 500000;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CountView"

BCountView::BCountView(BRect bounds, BPoseView* view)
	: BView(bounds, "CountVw", B_FOLLOW_LEFT + B_FOLLOW_BOTTOM,
			B_PULSE_NEEDED | B_WILL_DRAW),
	fLastCount(-1),
	fPoseView(view),
	fShowingBarberPole(false),
	fBorderHighlighted(false),
	fBarberPoleMap(NULL),
	fLastBarberPoleOffset(5),
	fStartSpinningAfter(0),
	fTypeAheadString(""),
	fFilterString("")
{
 	GetTrackerResources()->GetBitmapResource(B_MESSAGE_TYPE,
 		R_BarberPoleBitmap, &fBarberPoleMap);
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

	// When the barber pole just starts spinning we need to invalidate
	// the whole rectangle of text and barber pole.
	// After this the text needs no updating since only the pole changes.
	if (fStartSpinningAfter) {
		fStartSpinningAfter = 0;
		Invalidate(TextAndBarberPoleRect());
	} else
		Invalidate(BarberPoleInnerRect());
}


void
BCountView::Pulse()
{
	TrySpinningBarberPole();
}


void
BCountView::WindowActivated(bool active)
{
	if (fBorderHighlighted) {
		BRect dirty(Bounds());
		dirty.bottom = dirty.top;
		Invalidate(dirty);
	}
}


void
BCountView::EndBarberPole()
{
	if (!fShowingBarberPole)
		return;

	fShowingBarberPole = false;
	Invalidate();
}


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

	// if the barber pole is not present, use its space for text
	if (fShowingBarberPole)
		result.right -= 10;

	return result;
}


BRect
BCountView::TextAndBarberPoleRect() const
{
	BRect result = Bounds();
	result.InsetBy(4, 2);

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
BCountView::Draw(BRect updateRect)
{
	BRect bounds(Bounds());

	if (be_control_look != NULL) {
		rgb_color base = ViewColor();
		if (fBorderHighlighted && Window()->IsActive())
			SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		else
			SetHighColor(tint_color(base, B_DARKEN_2_TINT));
		StrokeLine(bounds.LeftTop(), bounds.RightTop());
		bounds.top++;
		be_control_look->DrawMenuBarBackground(this, bounds, updateRect,
			ViewColor());
	}

	BString itemString;
	if (IsTypingAhead())
		itemString << TypeAhead();
	else if (IsFiltering()) {
		itemString << fLastCount << " " << Filter();
	} else {
		if (fLastCount == 0)
			itemString << B_TRANSLATE("no items");
		else if (fLastCount == 1)
			itemString << B_TRANSLATE("1 item");
		else {
			itemString.SetTo(B_TRANSLATE("%num items"));
			char numString[256];
			snprintf(numString, sizeof(numString), "%ld", fLastCount);
			itemString.ReplaceFirst("%num", numString);
		}
	}

	BRect textRect(TextInvalRect());

	TruncateString(&itemString, IsTypingAhead() ? B_TRUNCATE_BEGINNING
			: IsFiltering() ? B_TRUNCATE_MIDDLE : B_TRUNCATE_END,
		textRect.Width());

	if (IsTypingAhead()) {
		// use a muted gray for the typeahead
		SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_4_TINT));
	} else
		SetHighColor(0, 0, 0);

	MovePenTo(textRect.LeftBottom());
	DrawString(itemString.String());

	bounds.top++;

	rgb_color light = tint_color(ViewColor(), B_LIGHTEN_MAX_TINT);
	rgb_color shadow = tint_color(ViewColor(), B_DARKEN_2_TINT);
	rgb_color lightShadow = tint_color(ViewColor(), B_DARKEN_1_TINT);

	BeginLineArray(fShowingBarberPole && !fStartSpinningAfter ? 9 : 5);

	if (be_control_look == NULL) {
		AddLine(bounds.LeftTop(), bounds.RightTop(), light);
		AddLine(bounds.LeftTop(), bounds.LeftBottom(), light);
		bounds.top--;

		AddLine(bounds.LeftTop(), bounds.RightTop(), shadow);
		AddLine(BPoint(bounds.right, bounds.top + 2), bounds.RightBottom(), lightShadow);
		AddLine(bounds.LeftBottom(), bounds.RightBottom(), lightShadow);
	}

	if (!fShowingBarberPole || fStartSpinningAfter) {
		EndLineArray();
		return;
	}

	BRect barberPoleRect(BarberPoleOuterRect());

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
	BContainerWindow* window = dynamic_cast<BContainerWindow*>(Window());
	window->Activate();
	window->UpdateIfNeeded();

	if (fPoseView->IsFilePanel() || !fPoseView->TargetModel())
		return;

	if (!window->TargetModel()->IsRoot()) {
		BDirMenu* menu = new BDirMenu(NULL, be_app, B_REFS_RECEIVED);
		BEntry entry;
		if (entry.SetTo(window->TargetModel()->EntryRef()) == B_OK)
			menu->Populate(&entry, Window(), false, false, true, false, true);
		else
			menu->Populate(NULL, Window(), false, false, true, false, true);

		BPoint point = Bounds().LeftBottom();
		point.y += 3;
		ConvertToScreen(&point);
		BRect clickToOpenRect(Bounds());
		ConvertToScreen(&clickToOpenRect);
		menu->Go(point, true, true, clickToOpenRect);
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
BCountView::SetTypeAhead(const char* string)
{
	fTypeAheadString = string;
	Invalidate();
}


const char*
BCountView::TypeAhead() const
{
	return fTypeAheadString.String();
}


bool
BCountView::IsTypingAhead() const
{
	return fTypeAheadString.Length() != 0;
}


void
BCountView::AddFilterCharacter(const char* character)
{
	fFilterString.AppendChars(character, 1);
	Invalidate();
}


void
BCountView::RemoveFilterCharacter()
{
	fFilterString.TruncateChars(fFilterString.CountChars() - 1);
	Invalidate();
}


void
BCountView::CancelFilter()
{
	fFilterString.Truncate(0);
	Invalidate();
}


const char*
BCountView::Filter() const
{
	return fFilterString.String();
}


bool
BCountView::IsFiltering() const
{
	return fFilterString.Length() > 0;
}


void
BCountView::SetBorderHighlighted(bool highlighted)
{
	if (fBorderHighlighted == highlighted)
		return;

	fBorderHighlighted = highlighted;
	BRect dirty(Bounds());
	dirty.bottom = dirty.top;
	Invalidate(dirty);
}
