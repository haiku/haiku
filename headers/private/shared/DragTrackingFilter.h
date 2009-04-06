/*
 * Copyright 2009, Alexandre Deckner, alex@zappotek.com
 * Distributed under the terms of the MIT License.
 */
#ifndef DRAG_TRACKING_FILTER_H
#define DRAG_TRACKING_FILTER_H

#include <MessageFilter.h>
#include <Point.h>

class BView;
class BHandler;

namespace BPrivate {

class DragTrackingFilter : public BMessageFilter {
public:
						DragTrackingFilter(BView* targetView, uint32 messageWhat);

	filter_result		Filter(BMessage* message, BHandler** _target);

private:
			BView*		fTargetView;
			uint32		fMessageWhat;
			bool		fIsTracking;
			BPoint		fClickPoint;
			uint32		fClickButtons;
};

}	// namespace BPrivate

using BPrivate::DragTrackingFilter;

#endif	// DRAG_TRACKING_FILTER_H
