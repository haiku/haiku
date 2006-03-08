// MouseTrackingHelpers.h
// e.moon 8mar99
//
// Some helper classes for mouse tracking, designed to make
// it easier to delegate drag operations to a parent class
// (especially useful for pane-separator bars & other operations
//  in which a view is moved or resized as the user drags
//  within it.)
//
// HISTORY
//   e.moon 11may99: fixed m_bTracking bug (not init'd in ctor)
//                   fixed mouseTrackingBegin() documentation
//                   (point is in view, not screen, coords)

#ifndef __MouseTrackingHelpers_H__
#define __MouseTrackingHelpers_H__

#include <View.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class MouseTrackingSourceView;

// interface for a mouse-tracking destination
class IMouseTrackingDestination {
public:							// operations
	// a MouseTrackingSourceView has started tracking the mouse
	// (point is in pSource view coordinates)
	virtual void mouseTrackingBegin(
		MouseTrackingSourceView* pSource,
		uint32 buttons,
		BPoint point)=0;
	
	// mouse-tracking update from child view
	// (point is in screen coordinates)
	virtual void mouseTrackingUpdate(
		uint32 buttons,
		float xDelta,
		float yDelta,
		BPoint point)=0;
	
	// mouse-tracking done
	virtual void mouseTrackingEnd()=0;
};

// mouse-tracking 'source' view
// hands off to a destination view (by default its immediate
// parent)

class MouseTrackingSourceView : public BView {
	typedef BView _inherited;

public:							// types & constants
	// these bits determine which mouse axes will be tracked
	// (ie. if TRACK_HORIZONTAL isn't set, mouseTrackUpdate()
	//  will always be called with xDelta == 0.0)
	enum mouse_tracking_flag_t {
		TRACK_HORIZONTAL = 1,
		TRACK_VERTICAL = 2
	};

public:								// ctor/dtor
	MouseTrackingSourceView(
		BRect frame,
		const char* pName,
		uint32 resizeMode=B_FOLLOW_LEFT|B_FOLLOW_TOP,
		uint32 flags=B_WILL_DRAW|B_FRAME_EVENTS,
		uint32 trackingFlags=TRACK_HORIZONTAL|TRACK_VERTICAL);

	~MouseTrackingSourceView();

	bool isTracking() const { return m_bTracking; }

	// get mouse-down point in screen coordinates; returns
	// B_OK on success, or B_ERROR if no longer tracking
	// the mouse.
	status_t getTrackingOrigin(BPoint* poPoint) const;

	// fetch/set the destination handler
	// (if not yet attached to a window, or if no parent
	// implements IMouseTrackDestination, trackingDestination()
	// returns 0)
	//
	// setting the destination handler isn't allowed if a tracking
	// operation is currently in progress

	IMouseTrackingDestination* trackingDestination() const {
		return m_pDest;
	}
	status_t setTrackingDestination(IMouseTrackingDestination* pDest);

public:								// BView impl.

	// handle mouse events
	virtual void MouseDown(BPoint point);
	virtual void MouseMoved(BPoint point, uint32 transit,
		const BMessage* pMsg);
	virtual void MouseUp(BPoint point);

	// look for a default destination
	virtual void AttachedToWindow();

/*
	// track current frame rectangle
	virtual void FrameResized(float width, float height);
*/
protected:							// members

	IMouseTrackingDestination*		m_pDest;
	uint32				m_trackingFlags;

	// mouse-tracking state
	bool						m_bTracking;
	BPoint				m_prevPoint; // in screen coords
	BPoint				m_initPoint; // in screen coords
	
//	BRect					m_prevFrame;
};

__END_CORTEX_NAMESPACE
#endif /* __MouseTrackingHelpers_H__ */
