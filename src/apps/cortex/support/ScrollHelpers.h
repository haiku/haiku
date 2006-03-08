// ScrollHelpers.h
// e.moon 9mar99
//
// An interface/driver view combination to simplify the task
// of redirecting a scroll bar: an invisible proxy view
// forwards scroll requests back to a scroll-handler.

#ifndef __EMOON_ScrollHelpers_H__
#define __EMOON_ScrollHelpers_H__

#include <View.h>
#include "debug_tools.h"

class IScrollTarget {
public:			// interface
	virtual void handleScrollBy(float xDelta, float yDelta)=0;
};

class ScrollProxyView : public BView {
	typedef BView _inherited;
public:
	ScrollProxyView(IScrollTarget* pTarget, BRect frame) :
		m_pTarget(pTarget),
		BView(frame, "scrollProxy", B_FOLLOW_LEFT|B_FOLLOW_TOP, 0) {}
	virtual ~ScrollProxyView() {}
	
	virtual void ScrollTo(BPoint where) {
		float xDelta = where.x - Bounds().left;
		float yDelta = where.y - Bounds().top;
		_inherited::ScrollTo(where);
		ASSERT(m_pTarget);
		m_pTarget->handleScrollBy(xDelta, yDelta);
	}
	
protected:
	IScrollTarget*		m_pTarget;
};

#endif /* __EMOON_ScrollHelpers_H__ */