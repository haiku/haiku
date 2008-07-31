/*
 * Copyright (C) 1998-1999 Be Incorporated. All rights reseved.
 * Distributed under the terms of the Be Sample Code license.
 *
 * Copyright (C) 2001-2008 Stephan Aßmus. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#include "PeakView.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Bitmap.h>
#include <MenuItem.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <Window.h>

using std::nothrow;

enum {
	MSG_PULSE		= 'puls',
	MSG_LOCK_PEAKS	= 'lpks'
};

// constructor
PeakView::PeakView(const char* name, bool useGlobalPulse, bool displayLabels)
	: BView(BRect(0.0, 0.0, 155.0 + 4.0, 10.0 + 4.0),
		name, B_FOLLOW_LEFT | B_FOLLOW_TOP,
			useGlobalPulse ? B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS
			: B_WILL_DRAW | B_FRAME_EVENTS),
	  fUseGlobalPulse(useGlobalPulse),
	  fDisplayLabels(displayLabels),
	  fPeakLocked(false),

	  fRefreshDelay(20000),
	  fPulse(NULL),

	  fCurrentMaxL(0.0),
	  fLastMaxL(0.0),
	  fOvershotL(false),

	  fCurrentMaxR(0.0),
	  fLastMaxR(0.0),
	  fOvershotR(false),

	  fBackBitmap(new BBitmap(BRect(0.0, 0.0, 256.0, 18.0), B_CMAP8)),
	  fPeakNotificationWhat(0)
{
	GetFontHeight(&fFontHeight);

	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetViewColor(B_TRANSPARENT_COLOR);

	FrameResized(Bounds().Width(), Bounds().Height());

	if (fDisplayLabels)
		ResizeBy(0, ceilf(fFontHeight.ascent + fFontHeight.descent));
}

// destructor
PeakView::~PeakView()
{
	delete fPulse;
	delete fBackBitmap;
}

// MessageReceived
void
PeakView::MessageReceived(BMessage* message)
{
	if (message->what == fPeakNotificationWhat) {
		float maxL;
		if (message->FindFloat("max", 0, &maxL) < B_OK)
			maxL = 0.0;
		float maxR;
		if (message->FindFloat("max", 1, &maxR) < B_OK)
			maxR = 0.0;
		SetMax(maxL, maxR);
		return;	
	}

	switch (message->what) {
		case MSG_PULSE:
			Pulse();
			break;

		case MSG_LOCK_PEAKS:
			fPeakLocked = !fPeakLocked;
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}

// AttachedToWindow
void
PeakView::AttachedToWindow()
{
	if (!fUseGlobalPulse) {
		delete fPulse;
		BMessage message(MSG_PULSE);
		fPulse = new BMessageRunner(BMessenger(this), &message,
									fRefreshDelay);
	}
}

// DetachedFromWindow
void
PeakView::DetachedFromWindow()
{
	delete fPulse;
	fPulse = NULL;
}

// MouseDown
void
PeakView::MouseDown(BPoint where)
{
	int32 buttons;
	if (Window()->CurrentMessage()->FindInt32("buttons", &buttons) < B_OK)
		buttons = B_PRIMARY_MOUSE_BUTTON;

	if (buttons & B_PRIMARY_MOUSE_BUTTON) {
		fOvershotL = false;
		fOvershotR = false;
		fLastMaxL = fCurrentMaxL;
		fLastMaxR = fCurrentMaxR;
	} else if (buttons & B_TERTIARY_MOUSE_BUTTON) {
		fPeakLocked = !fPeakLocked;
	} else {
		BPopUpMenu* menu = new BPopUpMenu("peak context menu");
		BMenuItem* item = new BMenuItem("Lock Peaks",
			new BMessage(MSG_LOCK_PEAKS));
		item->SetMarked(fPeakLocked);
		menu->AddItem(item);
		menu->SetTargetForItems(this);

		menu->SetAsyncAutoDestruct(true);
		menu->SetFont(be_plain_font);
	
		where = ConvertToScreen(where);
		bool keepOpen = false; // ?
		if (keepOpen) {
			BRect mouseRect(where, where);
			mouseRect.InsetBy(-3.0, -3.0);
			where += BPoint(3.0, 3.0);
			menu->Go(where, true, false, mouseRect, true);
		} else {
			where += BPoint(3.0, 3.0);
			menu->Go(where, true, false, true);
		}
	}
}

// Draw
void
PeakView::Draw(BRect area)
{
	rgb_color background = LowColor();
	rgb_color lightShadow = tint_color(background, B_DARKEN_1_TINT);
	rgb_color darkShadow = tint_color(background, B_DARKEN_4_TINT);
	rgb_color light = tint_color(background, B_LIGHTEN_MAX_TINT);
	rgb_color black = tint_color(background, B_DARKEN_MAX_TINT);
	BRect r(_BackBitmapFrame());
	float width = r.Width();
	r.InsetBy(-2.0, -2.0);
	// frame
	BeginLineArray(9);
		AddLine(BPoint(r.left, r.bottom), BPoint(r.left, r.top), lightShadow);
		AddLine(BPoint(r.left + 1.0, r.top), BPoint(r.right, r.top),
			lightShadow);
		AddLine(BPoint(r.right, r.top + 1.0), BPoint(r.right, r.bottom),
			light);
		AddLine(BPoint(r.right - 1.0, r.bottom),
			BPoint(r.left + 1.0, r.bottom), light);
		r.InsetBy(1.0, 1.0);
		AddLine(BPoint(r.left, r.bottom), BPoint(r.left, r.top), darkShadow);
		AddLine(BPoint(r.left + 1.0, r.top), BPoint(r.right, r.top),
			darkShadow);
		AddLine(BPoint(r.right, r.top + 1.0), BPoint(r.right, r.bottom),
			lightShadow);
		AddLine(BPoint(r.right - 1.0, r.bottom),
			BPoint(r.left + 1.0, r.bottom), lightShadow);
		r.InsetBy(1.0, 1.0);
		AddLine(BPoint(r.left, (r.top + r.bottom) / 2.0),
			BPoint(r.right, (r.top + r.bottom) / 2.0), black);
	EndLineArray();

	// peak bitmap
	if (fBackBitmap)
		_DrawBitmap();

	// dB
	if (fDisplayLabels) {
		font_height fh;
		GetFontHeight(&fh);
		float y = Bounds().bottom;
		y -= fh.descent;
		DrawString("0", BPoint(4.0 + width - StringWidth("0"), y));
		DrawString("-6", BPoint(0.477 * width, y));
		DrawString("-12", BPoint(0.227 * width, y));
	}
}

// FrameResized
void
PeakView::FrameResized(float width, float height)
{
	BRect bitmapFrame = _BackBitmapFrame();
	_ResizeBackBitmap(bitmapFrame.IntegerWidth() + 1);
	_UpdateBackBitmap();
}

// Pulse
void
PeakView::Pulse()
{
	if (!fBackBitmap)
		return;

	if (!fPeakLocked) {
		fLastMaxL = 0.90 * fLastMaxL;
		if (fCurrentMaxL > fLastMaxL)
			fLastMaxL = fCurrentMaxL;
		fLastMaxR = 0.90 * fLastMaxR;
		if (fCurrentMaxR > fLastMaxR)
			fLastMaxR = fCurrentMaxR;
	}
	_UpdateBackBitmap();
	fCurrentMaxL = 0.0;
	fCurrentMaxR = 0.0;

	_DrawBitmap();
	Flush();
}

// GetPreferredSize
void
PeakView::GetPreferredSize(float* _width, float* _height)
{
	float minWidth = 0;
	float minHeight = 0;
	if (fBackBitmap) {
		minWidth = 20 + 4;
		minHeight = 3 + 4;
	}
	if (fDisplayLabels) {
		font_height fh;
		GetFontHeight(&fh);
		minWidth = max_c(60.0, minWidth);
		minHeight += ceilf(fh.ascent + fh.descent);
	}
	if (_width)
		*_width = minWidth;
	if (_height)
		*_height = minHeight;
}

// IsValid
bool
PeakView::IsValid() const
{
	return fBackBitmap && fBackBitmap->IsValid();
}

// SetPeakRefreshDelay
void
PeakView::SetPeakRefreshDelay(bigtime_t delay)
{
	if (fRefreshDelay == delay)
		return;
	fRefreshDelay = delay;
	if (fPulse)
		fPulse->SetInterval(fRefreshDelay);
}

// SetPeakNotificationWhat
void
PeakView::SetPeakNotificationWhat(uint32 what)
{
	fPeakNotificationWhat = what;
}

// SetMax
void
PeakView::SetMax(float maxL, float maxR)
{
	if (fCurrentMaxL < maxL)
		fCurrentMaxL = maxL;
	if (fCurrentMaxR < maxR)
		fCurrentMaxR = maxR;

	if (fCurrentMaxL > 1.0)
		fOvershotL = true;
	if (fCurrentMaxR > 1.0)
		fOvershotR = true;
}

// #pragma mark -

// _BackBitmapFrame
BRect
PeakView::_BackBitmapFrame() const
{
	BRect frame = Bounds();
	frame.InsetBy(2, 2);
	if (fDisplayLabels)
		frame.bottom -= ceilf(fFontHeight.ascent + fFontHeight.descent);
	return frame;
}

// _ResizeBackBitmap
void
PeakView::_ResizeBackBitmap(int32 width)
{
	if (fBackBitmap) {
		if (fBackBitmap->Bounds().IntegerWidth() + 1 == width)
			return;
	}
	delete fBackBitmap;
	fBackBitmap = new (nothrow) BBitmap(BRect(0, 0, width - 1, 1), 0, B_RGB32);
	if (!fBackBitmap || !fBackBitmap->IsValid()) {
		delete fBackBitmap;
		fBackBitmap = NULL;
		return;
	}
	memset(fBackBitmap->Bits(), 0, fBackBitmap->BitsLength());
}

// _UpdateBackBitmap
void
PeakView::_UpdateBackBitmap()
{
	if (!fBackBitmap)
		return;

	uint8* l = (uint8*)fBackBitmap->Bits();
	uint8* r = l + fBackBitmap->BytesPerRow();
	uint32 width = fBackBitmap->Bounds().IntegerWidth() + 1;
	_RenderSpan(l, width, fCurrentMaxL, fLastMaxL, fOvershotL);
	_RenderSpan(r, width, fCurrentMaxR, fLastMaxR ,fOvershotR);
}

// _RenderSpan
void
PeakView:: _RenderSpan(uint8* span, uint32 width, float current, float peak,
	bool overshot)
{
	uint8 emptyR = 15;
	uint8 emptyG = 36;
	uint8 emptyB = 16;

	uint8 fillR = 41;
	uint8 fillG = 120;
	uint8 fillB = 45;

	uint8 currentR = 45;
	uint8 currentG = 255;
	uint8 currentB = 45;

	uint8 lastR = 255;
	uint8 lastG = 229;
	uint8 lastB = 87;

	uint8 overR = 255;
	uint8 overG = 89;
	uint8 overB = 7;

	uint8 kFadeFactor = 180;

	uint32 split = (uint32)(current * (width - 2) + 0.5);
	split += split & 1;
	uint32 last = (uint32)(peak * (width - 2) + 0.5);
	last += last & 1;
	uint32 over = overshot ? width - 1 : width;
	over += over & 1;

	for (uint32 x = 0; x < width - 1; x += 2) {
		uint8 fadedB = (uint8)(((int)span[0] * kFadeFactor) >> 8);
		uint8 fadedG = (uint8)(((int)span[1] * kFadeFactor) >> 8);
		uint8 fadedR = (uint8)(((int)span[2] * kFadeFactor) >> 8);
		if (x < split) {
			span[0] = max_c(fillB, fadedB);
			span[1] = max_c(fillG, fadedG);
			span[2] = max_c(fillR, fadedR);
		} else if (x == split) {
			span[0] = currentB;
			span[1] = currentG;
			span[2] = currentR;
		} else if (x > split) {
			span[0] = max_c(emptyB, fadedB);
			span[1] = max_c(emptyG, fadedG);
			span[2] = max_c(emptyR, fadedR);
		}
		if (x == last) {
			span[0] = lastB;
			span[1] = lastG;
			span[2] = lastR;
		}
		if (x == over) {
			span[0] = overB;
			span[1] = overG;
			span[2] = overR;
		}
		span += 8;
	}
}

// _DrawBitmap
void
PeakView::_DrawBitmap()
{
	BRect r = _BackBitmapFrame();
	BRect topHalf = r;
	topHalf.bottom = (r.top + r.bottom) / 2.0 - 1;
	BRect bottomHalf = r;
	bottomHalf.top = topHalf.bottom + 2;
	BRect bitmapRect = fBackBitmap->Bounds();
	bitmapRect.bottom = bitmapRect.top;
	DrawBitmapAsync(fBackBitmap, bitmapRect, topHalf);
	bitmapRect.OffsetBy(0, 1);
	DrawBitmapAsync(fBackBitmap, bitmapRect, bottomHalf);
}


