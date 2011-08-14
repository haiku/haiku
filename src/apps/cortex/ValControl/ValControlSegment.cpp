/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// ValControlSegment.cpp
// e.moon 20jan99


#include "ValControlSegment.h"
#include "ValControl.h"

#include <Debug.h>

#include <cstdio>

__USE_CORTEX_NAMESPACE


ValControlSegment::ValControlSegment(underline_style underlineStyle)
	:BView(BRect(), "ValControlSegment", B_FOLLOW_LEFT|B_FOLLOW_TOP,
		B_WILL_DRAW|B_FRAME_EVENTS),
	fDragScaleFactor(fDefDragScaleFactor),
	fLastClickTime(0LL),
	fUnderlineStyle(underlineStyle),
	fXUnderlineLeft(0.0),
	fXUnderlineRight(0.0)
{
	_Init();
}


ValControlSegment::~ValControlSegment()
{
}


void
ValControlSegment::_Init()
{
	//m_pLeft = 0;
}


const double ValControlSegment::fDefDragScaleFactor = 4;


// get parent
ValControl*
ValControlSegment::parent() const
{
	return dynamic_cast<ValControl*>(Parent());
}


//
//// send message -> segment to my left
//void ValControlSegment::sendMessageLeft(BMessage* pMsg) {
//	if(!m_pLeft)
//		return;
//	BMessenger m(m_pLeft);
//	if(!m.IsValid())
//		return;
//	m.SendMessage(pMsg);
//	delete pMsg;
//}

	
// init mouse tracking
void
ValControlSegment::trackMouse(BPoint point, track_flags flags)
{
	fTrackFlags = flags;
	fPrevPoint = point;
	setTracking(true);
	SetMouseEventMask(B_POINTER_EVENTS,
		B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
}


void
ValControlSegment::setTracking(bool tracking)
{
	fTracking = tracking;
}


bool
ValControlSegment::isTracking() const
{
	return fTracking;
}


double
ValControlSegment::dragScaleFactor() const
{
	return fDragScaleFactor;
}


// -------------------------------------------------------- //
// default hook implementations
// -------------------------------------------------------- //

void
ValControlSegment::sizeUnderline(float* outLeft, float* outRight)
{
	*outLeft = 0.0;
	*outRight = Bounds().right;
}


void
ValControlSegment::AttachedToWindow()
{
//	if(parent()->backBuffer())
//		SetViewColor(B_TRANSPARENT_COLOR);

	// adopt parent's view color
	SetViewColor(parent()->ViewColor());
}


void
ValControlSegment::Draw(BRect updateRect)
{
	if (fUnderlineStyle == NO_UNDERLINE)	
		return;
	
	// +++++ move to layout function
	float fY = parent()->baselineOffset() + 1;
	
	rgb_color white = {255,255,255,255};
	rgb_color blue = {0,0,255,255};
	rgb_color gray = {140,140,140,255};
	rgb_color old = HighColor();

	SetLowColor(white);
	if(parent()->IsEnabled() && parent()->IsFocus())
		SetHighColor(blue);
	else
		SetHighColor(gray);

	if (fXUnderlineRight <= fXUnderlineLeft)
		sizeUnderline(&fXUnderlineLeft, &fXUnderlineRight);

//	PRINT((
//		"### ValControlSegment::Draw(): underline spans %.1f, %.1f\n",
//		fXUnderlineLeft, fXUnderlineRight));
//	
	StrokeLine(BPoint(fXUnderlineLeft, fY),
		BPoint(fXUnderlineRight, fY),
		(fUnderlineStyle == DOTTED_UNDERLINE) ? B_MIXED_COLORS :
			B_SOLID_HIGH);
	SetHighColor(old);
}


void
ValControlSegment::FrameResized(float width, float height)
{
	_Inherited::FrameResized(width, height);

//	PRINT((
//		"### ValControlSegment::FrameResized(%.1f,%.1f)\n",
//		fWidth, fHeight));
	sizeUnderline(&fXUnderlineLeft, &fXUnderlineRight);
}


void
ValControlSegment::MouseDown(BPoint point)
{
	if (!parent()->IsEnabled())
		return;

	parent()->MakeFocus();

	// not left button?
	uint32 nButton;
	GetMouse(&point, &nButton);
	if (!(nButton & B_PRIMARY_MOUSE_BUTTON))
		return;

	// double click?
	bigtime_t doubleClickInterval;
	if (get_click_speed(&doubleClickInterval) < B_OK) {
		PRINT(("* ValControlSegment::MouseDown():\n"
			"  get_click_speed() failed."));
		return;
	}

	bigtime_t now = system_time();
	if (now - fLastClickTime < doubleClickInterval) {
		if(isTracking()) {
			setTracking(false);
			mouseReleased();
		}

		// hand off to parent control
		parent()->showEditField();
		fLastClickTime = 0LL;
		return;
	}
	else
		fLastClickTime = now;

		
	// engage tracking
	trackMouse(point, TRACK_VERTICAL);
}


void
ValControlSegment::MouseMoved(BPoint point, uint32 transit,
	const BMessage* dragMessage)
{
	if (!parent()->IsEnabled())
		return;

	if (!isTracking() || fPrevPoint == point)
		return;

	// float fXDelta = fTrackFlags & TRACK_HORIZONTAL ?
	// point.x - fPrevPoint.x : 0.0;
	float fYDelta = fTrackFlags & TRACK_VERTICAL ?
		point.y - fPrevPoint.y : 0.0;

	// printf("MouseMoved(): %2f,%2f\n",
	// fXDelta, fYDelta);

	
	// hand off
	// +++++ config'able x/y response would be nice...
	float fRemaining = handleDragUpdate(fYDelta);
	
	fPrevPoint = point;
	fPrevPoint.y -= fRemaining;
}


void
ValControlSegment::MouseUp(BPoint point)
{
	if (isTracking()) {
		setTracking(false);
		mouseReleased();
	}
}	

	
void
ValControlSegment::MessageReceived(BMessage* message)
{
	switch(message->what) {
		default:
			_Inherited::MessageReceived(message);
	}
}


// -------------------------------------------------------- //
// archiving/instantiation
// -------------------------------------------------------- //

ValControlSegment::ValControlSegment(BMessage* archive)
	: BView(archive)
{
	_Init();
	archive->FindInt32("ulStyle", (int32*)&fUnderlineStyle);	
}


status_t
ValControlSegment::Archive(BMessage* archive, bool deep) const
{
	_Inherited::Archive(archive, deep);
	
	archive->AddInt32("ulStyle", fUnderlineStyle);
	return B_OK;
}
