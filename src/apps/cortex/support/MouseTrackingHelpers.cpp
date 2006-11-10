// MouseTrackingHelpers.cpp
// e.moon 8mar99

#include "MouseTrackingHelpers.h"

#include "debug_tools.h"

__USE_CORTEX_NAMESPACE

// ---------------------------------------------------------------- //
// ctor/dtor
// ---------------------------------------------------------------- //

MouseTrackingSourceView::MouseTrackingSourceView(
	BRect frame,
	const char* pName,
	uint32 resizeMode,
	uint32 flags,
	uint32 trackingFlags) :
	
	BView(frame, pName, resizeMode, flags),
	m_trackingFlags(trackingFlags),
	m_bTracking(false),
	m_pDest(0) {
	
//	FrameResized(frame.Width(), frame.Height());
}

MouseTrackingSourceView::~MouseTrackingSourceView() {}

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

