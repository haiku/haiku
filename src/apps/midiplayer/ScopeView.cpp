/*
 * Copyright (c) 2004 Matthijs Hollemans
 * Copyright (c) 2008-2014 Haiku, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#include "ScopeView.h"

#include <Catalog.h>
#include <Locale.h>
#include <Synth.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Scope View"


//	#pragma mark - ScopeView


ScopeView::ScopeView()
	:
	BView(BRect(0, 0, 180, 63), NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW),
	fIsPlaying(false),
	fIsEnabled(true),
	fHaveFile(false),
	fIsLoading(false),
	fIsLiveInput(false),
	fSampleCount(Bounds().IntegerWidth()),
	fLeftSamples(new int16[fSampleCount]),
	fRightSamples(new int16[fSampleCount]),
	fScopeThreadID(-1)
{
}


ScopeView::~ScopeView()
{
	delete[] fLeftSamples;
	delete[] fRightSamples;
}


void ScopeView::AttachedToWindow()
{
	SetViewColor(0, 0, 0);

	fIsFinished = false;
	fScopeThreadID = spawn_thread(_Thread, "ScopeThread",
		B_NORMAL_PRIORITY, this);
	if (fScopeThreadID > 0)
		resume_thread(fScopeThreadID);
}


void
ScopeView::DetachedFromWindow()
{
	if (fScopeThreadID > 0) {
		fIsFinished = true;
		status_t exitValue;
		wait_for_thread(fScopeThreadID, &exitValue);
	}
}


void
ScopeView::Draw(BRect updateRect)
{
	super::Draw(updateRect);

	if (fIsLoading)
		DrawLoading();
	else if (!fHaveFile && !fIsLiveInput)
		DrawNoFile();
	else if (!fIsEnabled)
		DrawDisabled();
	else if (fIsPlaying || fIsLiveInput)
		DrawPlaying();
	else
		DrawStopped();
}


int32
ScopeView::_Thread(void* data)
{
	return ((ScopeView*) data)->Thread();
}


int32
ScopeView::Thread()
{
	// Because Pulse() was too slow, I created a thread that tells the
	// ScopeView to repaint itself. Note that we need to call LockLooper
	// with a timeout, otherwise we'll deadlock in DetachedFromWindow().

	while (!fIsFinished) {
		if (fIsEnabled && (fIsPlaying || fIsLiveInput)) {
			if (LockLooperWithTimeout(50000) == B_OK) {
				Invalidate();
				UnlockLooper();
			}
		}
		snooze(50000);
	}

	return 0;
}


void
ScopeView::DrawLoading()
{
	DrawText(B_TRANSLATE("Loading instruments" B_UTF8_ELLIPSIS));
}


void
ScopeView::DrawNoFile()
{
	DrawText(B_TRANSLATE("Drop MIDI file here"));
}


void
ScopeView::DrawDisabled()
{
	SetHighColor(64, 64, 64);

	StrokeLine(BPoint(0, Bounds().Height() / 2),
		BPoint(Bounds().Width(), Bounds().Height() / 2));
}


void
ScopeView::DrawStopped()
{
	SetHighColor(0, 130, 0);

	StrokeLine(BPoint(0, Bounds().Height() / 2),
		BPoint(Bounds().Width(), Bounds().Height() / 2));
}


void
ScopeView::DrawPlaying()
{
	int32 width = (int32) Bounds().Width();
	int32 height = (int32) Bounds().Height();

	// Scope drawing magic based on code by Michael Pfeiffer.

	int32 size = be_synth->GetAudio(fLeftSamples, fRightSamples, fSampleCount);
	if (size > 0) {
		SetHighColor(255, 0, 130);
		SetLowColor(0, 130, 0);

		int32 x;
		int32 y;
		int32 sx = 0;
		int32 f = (height << 16) / 65535;
		int32 dy = height / 2 + 1;
		for (int32 i = 0; i < width; i++) {
			x = sx / width;
			y = ((fLeftSamples[x] * f) >> 16) + dy;
			FillRect(BRect(i, y, i, y));

			y = ((fRightSamples[x] * f) >> 16) + dy;
			FillRect(BRect(i, y, i, y), B_SOLID_LOW);

			sx += size;
		}
	}
}


void
ScopeView::DrawText(const char* text)
{
	font_height height;
	GetFontHeight(&height);

	float stringWidth = StringWidth(text);
	float stringHeight = height.ascent + height.descent;

	float x = (Bounds().Width() - stringWidth) / 2;
	float y = height.ascent + (Bounds().Height() - stringHeight) / 2;

	SetHighColor(255, 255, 255);
	SetLowColor(ViewColor());
	SetDrawingMode(B_OP_OVER);

	DrawString(text, BPoint(x, y));
}
