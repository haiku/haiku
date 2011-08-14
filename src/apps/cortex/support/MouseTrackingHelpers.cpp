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


// MouseTrackingHelpers.cpp
// e.moon 8mar99

#include "MouseTrackingHelpers.h"

#include "debug_tools.h"

__USE_CORTEX_NAMESPACE


MouseTrackingSourceView::MouseTrackingSourceView(BRect frame, const char* name,
		uint32 resizeMode, uint32 flags, uint32 trackingFlags)
	: BView(frame, name, resizeMode, flags),
	m_pDest(0),
	m_trackingFlags(trackingFlags),
	m_bTracking(false)
{
	//FrameResized(frame.Width(), frame.Height());
}


MouseTrackingSourceView::~MouseTrackingSourceView()
{
}


// get mouse-down point in screen coordinates; returns
// B_OK on success, or B_ERROR if no longer tracking
// the mouse.
status_t MouseTrackingSourceView::getTrackingOrigin(
	BPoint* poPoint) const {
	if(!m_bTracking)	
		return B_ERROR;
	*poPoint = m_initPoint;
	return B_OK;
}

// fetch/set the destination handler

status_t MouseTrackingSourceView::setTrackingDestination(
	IMouseTrackingDestination* pDest) {
	if(m_bTracking)
		return B_ERROR;
	
	m_pDest = pDest;
	return B_OK;
}

// ---------------------------------------------------------------- //
// BView impl.
// ---------------------------------------------------------------- //

// handle mouse events
void MouseTrackingSourceView::MouseDown(BPoint point) {
	if(!m_trackingFlags)
		return;

	// get mouse state & initial point
	uint32 buttons;
	GetMouse(&point, &buttons);
	m_prevPoint = ConvertToScreen(point);		
	m_initPoint = m_prevPoint;
		
	// start tracking the mouse
	SetMouseEventMask(B_POINTER_EVENTS,
		B_LOCK_WINDOW_FOCUS|B_NO_POINTER_HISTORY);
	m_bTracking = true;

	// notify destination
	if(m_pDest)
		m_pDest->mouseTrackingBegin(this, buttons, point);		
}	

void MouseTrackingSourceView::MouseMoved(BPoint point, uint32 transit,
	const BMessage* pMsg) {

	if(m_bTracking) {

		// mouse-tracking update: figure number of pixels moved
		// (along the axes I care about)
		uint32 buttons;
		GetMouse(&point, &buttons, false);
		ConvertToScreen(&point);
		
		if(point == m_prevPoint) // no motion?
			return;
				
		float xDelta = m_trackingFlags & TRACK_HORIZONTAL ?
			point.x - m_prevPoint.x : 0.0;
		float yDelta = m_trackingFlags & TRACK_VERTICAL ?
			point.y - m_prevPoint.y : 0.0;

		// pass info to destination view
		if(m_pDest)
			m_pDest->mouseTrackingUpdate(buttons, xDelta, yDelta, point);

		// store point for future delta calculations			
		m_prevPoint = point;
	}
}

void MouseTrackingSourceView::MouseUp(BPoint point) {
	if(m_bTracking) {
//		PRINT(( "MouseTrackingSourceView::MouseUp()\n"));
		
		// +++++ handle final update
		
		// clean up
		m_bTracking = false;
		if(m_pDest)
			m_pDest->mouseTrackingEnd();				
	}
}

// look for a default destination
void MouseTrackingSourceView::AttachedToWindow() {
	if(m_pDest) // already have a destination
		return;
		
	for(BView* pParent = Parent(); pParent; pParent = pParent->Parent()) {
		IMouseTrackingDestination* pFound = 
			dynamic_cast<IMouseTrackingDestination*>(pParent);
		if(pFound) // found a valid destination
			m_pDest = pFound;
	}
}

/*
// track current frame rectangle
void MouseTrackingSourceView::FrameResized(float width, float height) {
	_inherited::FrameResized(width, height);
	m_prevFrame = Frame();
	
	// +++++ adjust if currently tracking?
}
*/

// END -- MouseTrackingHelpers.cpp --

