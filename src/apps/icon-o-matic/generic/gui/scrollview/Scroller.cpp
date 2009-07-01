/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#include "Scroller.h"

#include <stdio.h>

#include <Point.h>
#include <Rect.h>

#include "Scrollable.h"

// constructor
Scroller::Scroller()
	: fScrollTarget(NULL),
	  fScrollingEnabled(true)
{
}

// destructor
Scroller::~Scroller()
{
}

// SetScrollTarget
//
// Sets a new scroll target. Notifies the old and the new target
// of the change if necessary .
void
Scroller::SetScrollTarget(Scrollable* target)
{
	Scrollable* oldTarget = fScrollTarget;
	if (oldTarget != target) {
		fScrollTarget = NULL;
		// Notify the old target, if it doesn't know about the change.
		if (oldTarget && oldTarget->ScrollSource() == this)
			oldTarget->SetScrollSource(NULL);
		fScrollTarget = target;
		// Notify the new target, if it doesn't know about the change.
		if (target && target->ScrollSource() != this)
			target->SetScrollSource(this);
		// Notify ourselves.
		ScrollTargetChanged(oldTarget, target);
	}
}

// ScrollTarget
//
// Returns the current scroll target. May be NULL, if we don't have any.
Scrollable*
Scroller::ScrollTarget() const
{
	return fScrollTarget;
}

// SetDataRect
//
// Sets the data rect of the scroll target, if we have one.
void
Scroller::SetDataRect(BRect dataRect)
{
	if (fScrollTarget)
		fScrollTarget->SetDataRect(dataRect);
}

// DataRect
//
// Returns the data rect of the scroll target or a undefined value, if
// we have none.
BRect
Scroller::DataRect() const
{
	if (fScrollTarget)
		return fScrollTarget->DataRect();
	return BRect();
}

// SetScrollOffset
//
// Sets the scroll offset of the scroll target, if we have one.
void
Scroller::SetScrollOffset(BPoint offset)
{
	if (fScrollTarget)
		fScrollTarget->SetScrollOffset(offset);
}

// ScrollOffset
//
// Returns the scroll offset of the scroll target or a undefined value, if
// we have none.
BPoint
Scroller::ScrollOffset() const
{
	if (fScrollTarget)
		return fScrollTarget->ScrollOffset();
	return BPoint(0.0, 0.0);
}

// SetVisibleSize
//
// Sets the visible size of the scroll target, if we have one.
void
Scroller::SetVisibleSize(float width, float height)
{
	if (fScrollTarget)
		fScrollTarget->SetVisibleSize(width, height);
}

// VisibleBounds
//
// Returns the visible bounds of the scroll target or a undefined value, if
// we have none.
BRect
Scroller::VisibleBounds() const
{
	if (fScrollTarget)
		return 	fScrollTarget->VisibleBounds();
	return BRect();
}

// VisibleRect
//
// Returns the visible rect of the scroll target or a undefined value, if
// we have none.
BRect
Scroller::VisibleRect() const
{
	if (fScrollTarget)
		return 	fScrollTarget->VisibleRect();
	return BRect();
}

// SetScrollingEnabled
void
Scroller::SetScrollingEnabled(bool enabled)
{
	fScrollingEnabled = enabled;
}

// IsScrolling
bool
Scroller::IsScrolling() const
{
	return false;
}

// hooks

// DataRectChanged
//
// Hook function. Implemented by derived classes to get notified when
// the data rect of the sroll target has changed.
void
Scroller::DataRectChanged(BRect /*oldDataRect*/, BRect /*newDataRect*/)
{
}

// ScrollOffsetChanged
//
// Hook function. Implemented by derived classes to get notified when
// the scroll offset of the sroll target has changed.
void
Scroller::ScrollOffsetChanged(BPoint /*oldOffset*/, BPoint /*newOffset*/)
{
}

// VisiblSizeChanged
//
// Hook function. Implemented by derived classes to get notified when
// the visible size of the sroll target has changed.
void
Scroller::VisibleSizeChanged(float /*oldWidth*/, float /*oldHeight*/,
							 float /*newWidth*/, float /*newHeight*/)
{
}

// ScrollTargetChanged
//
// Hook function. Implemented by derived classes to get notified when
// the sroll target has changed. /target/ may be NULL.
void
Scroller::ScrollTargetChanged(Scrollable* /*oldTarget*/,
							  Scrollable* /*newTarget*/)
{
}

