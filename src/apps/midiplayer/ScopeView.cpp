/*
 * Copyright (c) 2004 Matthijs Hollemans
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

#include <Catalog.h>
#include <Locale.h>
#include <Synth.h>
#include <Window.h>

#include "ScopeView.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Scope View"


//------------------------------------------------------------------------------

ScopeView::ScopeView()
	: BView(BRect(0, 0, 180, 63), NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP,
	        B_WILL_DRAW)
{
	SetViewColor(0, 0, 0);

	playing = false;
	enabled = true;
	haveFile = false;
	loading = false;
	liveInput = false;

	sampleCount = (int32) Bounds().Width();
	leftSamples = new int16[sampleCount];
	rightSamples = new int16[sampleCount];
}

//------------------------------------------------------------------------------

ScopeView::~ScopeView()
{
	delete[] leftSamples;
	delete[] rightSamples;
}

//------------------------------------------------------------------------------

void ScopeView::AttachedToWindow()
{
	finished = false;
	threadId = spawn_thread(_Thread, "ScopeThread", B_NORMAL_PRIORITY, this);
	if (threadId >= B_OK)
	{
		resume_thread(threadId);
	}
}

//------------------------------------------------------------------------------

void ScopeView::DetachedFromWindow()
{
	if (threadId >= B_OK)
	{
		finished = true;
		status_t exitValue;
		wait_for_thread(threadId, &exitValue);
	}
}

//------------------------------------------------------------------------------

void ScopeView::Draw(BRect updateRect)
{
	super::Draw(updateRect);

	if (loading)
	{
		DrawLoading();
	}
	else if (!haveFile && !liveInput)
	{
		DrawNoFile();
	}
	else if (!enabled)
	{
		DrawDisabled();
	}
	else if (playing || liveInput)
	{
		DrawPlaying();
	}
	else
	{
		DrawStopped();
	}
}

//------------------------------------------------------------------------------

void ScopeView::SetPlaying(bool flag)
{
	playing = flag;
}

//------------------------------------------------------------------------------

void ScopeView::SetEnabled(bool flag)
{
	enabled = flag;
}

//------------------------------------------------------------------------------

void ScopeView::SetHaveFile(bool flag)
{
	haveFile = flag;
}

//------------------------------------------------------------------------------

void ScopeView::SetLoading(bool flag)
{
	loading = flag;
}

//------------------------------------------------------------------------------

void ScopeView::SetLiveInput(bool flag)
{
	liveInput = flag;
}

//------------------------------------------------------------------------------

int32 ScopeView::_Thread(void* data)
{
	return ((ScopeView*) data)->Thread();
}

//------------------------------------------------------------------------------

int32 ScopeView::Thread()
{
	// Because Pulse() was too slow, I created a thread that tells the
	// ScopeView to repaint itself. Note that we need to call LockLooper
	// with a timeout, otherwise we'll deadlock in DetachedFromWindow().

	while (!finished)
	{
		if (enabled && (playing || liveInput))
		{
			if (LockLooperWithTimeout(50000) == B_OK)
			{
				Invalidate();
				UnlockLooper();
			}
		}
		snooze(50000);
	}
	return 0;
}

//------------------------------------------------------------------------------

void ScopeView::DrawLoading()
{
	DrawText("Loading instruments" B_UTF8_ELLIPSIS);
}

//------------------------------------------------------------------------------

void ScopeView::DrawNoFile()
{
	DrawText(B_TRANSLATE("Drop MIDI file here"));
}

//------------------------------------------------------------------------------

void ScopeView::DrawDisabled()
{
	SetHighColor(64, 64, 64);

	StrokeLine(
		BPoint(0, Bounds().Height() / 2),
		BPoint(Bounds().Width(), Bounds().Height() / 2));
}

//------------------------------------------------------------------------------

void ScopeView::DrawStopped()
{
	SetHighColor(0, 130, 0);

	StrokeLine(
		BPoint(0, Bounds().Height() / 2),
		BPoint(Bounds().Width(), Bounds().Height() / 2));
}

//------------------------------------------------------------------------------

void ScopeView::DrawPlaying()
{
	int32 width = (int32) Bounds().Width();
	int32 height = (int32) Bounds().Height();

	// Scope drawing magic based on code by Michael Pfeiffer.

	int32 size = be_synth->GetAudio(leftSamples, rightSamples, sampleCount);
	if (size > 0)
	{
		SetHighColor(255, 0, 130);
		SetLowColor(0, 130, 0);
		#define N 16
		int32 x, y, sx = 0, f = (height << N) / 65535, dy = height / 2 + 1;
		for (int32 i = 0; i < width; i++)
		{
			x = sx / width;
			y = ((leftSamples[x] * f) >> N) + dy;
			FillRect(BRect(i, y, i, y));
			y = ((rightSamples[x] * f) >> N) + dy;
			FillRect(BRect(i, y, i, y), B_SOLID_LOW);
			sx += size;
		}
	}
}

//------------------------------------------------------------------------------

void ScopeView::DrawText(const char* text)
{
	font_height height;
	GetFontHeight(&height);

	float strWidth = StringWidth(text);
	float strHeight = height.ascent + height.descent;

	float x = (Bounds().Width() - strWidth)/2;
	float y = height.ascent + (Bounds().Height() - strHeight)/2;

	SetHighColor(255, 255, 255);
	SetLowColor(ViewColor());
	SetDrawingMode(B_OP_OVER);

	DrawString(text, BPoint(x, y));
}

//------------------------------------------------------------------------------
