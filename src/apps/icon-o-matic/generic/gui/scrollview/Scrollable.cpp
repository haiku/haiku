/*
 * Copyright 2001-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "Scrollable.h"

#include <algorithm>
#include <stdio.h>

#include "Scroller.h"

// constructor
Scrollable::Scrollable()
	: fDataRect(0.0, 0.0, 0.0, 0.0),
	  fScrollOffset(0.0, 0.0),
	  fVisibleWidth(0),
	  fVisibleHeight(0),
	  fScrollSource(NULL)
{
}

// destructor
Scrollable::~Scrollable()
{
	if (fScrollSource)
		fScrollSource->SetScrollTarget(NULL);
}

// SetScrollSource
//
// Sets a new scroll source. Notifies the old and the new source
// of the change if necessary .
void
Scrollable::SetScrollSource(Scroller* source)
{
	Scroller* oldSource = fScrollSource;
	if (oldSource != source) {
		fScrollSource = NULL;
		// Notify the old source, if it doesn't know about the change.
		if (oldSource && oldSource->ScrollTarget() == this)
			oldSource->SetScrollTarget(NULL);
		fScrollSource = source;
		// Notify the new source, if it doesn't know about the change.
		if (source && source->ScrollTarget() != this)
			source->SetScrollTarget(this);
		// Notify ourselves.
		ScrollSourceChanged(oldSource, fScrollSource);
	}
}

// ScrollSource
//
// Returns the current scroll source. May be NULL, if we don't have any.
Scroller*
Scrollable::ScrollSource() const
{
	return fScrollSource;
}

// SetDataRect
//
// Sets the data rect.
void
Scrollable::SetDataRect(BRect dataRect, bool validateScrollOffset)
{
	if (fDataRect != dataRect && dataRect.IsValid()) {
		BRect oldDataRect = fDataRect;
		fDataRect = dataRect;
		// notify ourselves
		DataRectChanged(oldDataRect, fDataRect);
		// notify scroller
		if (fScrollSource)
			fScrollSource->DataRectChanged(oldDataRect, fDataRect);
		// adjust the scroll offset, if necessary
		if (validateScrollOffset) {
			BPoint offset = ValidScrollOffsetFor(fScrollOffset);
			if (offset != fScrollOffset)
				SetScrollOffset(offset);
		}
	}
}

// DataRect
//
// Returns the current data rect.
BRect
Scrollable::DataRect() const
{
	return fDataRect;
}

// SetScrollOffset
//
// Sets the scroll offset.
void
Scrollable::SetScrollOffset(BPoint offset)
{
	// adjust the supplied offset to be valid
	offset = ValidScrollOffsetFor(offset);
	if (fScrollOffset != offset) {
		BPoint oldOffset = fScrollOffset;
		fScrollOffset = offset;
		// notify ourselves
		ScrollOffsetChanged(oldOffset, fScrollOffset);
		// notify scroller
		if (fScrollSource)
			fScrollSource->ScrollOffsetChanged(oldOffset, fScrollOffset);
	}
}

// ScrollOffset
//
// Returns the current scroll offset.
BPoint
Scrollable::ScrollOffset() const
{
	return fScrollOffset;
}

// SetDataRect
//
// Sets the data rect.
void
Scrollable::SetDataRectAndScrollOffset(BRect dataRect, BPoint offset)
{
	if (fDataRect != dataRect && dataRect.IsValid()) {

		BRect oldDataRect = fDataRect;
		fDataRect = dataRect;
		// notify ourselves
		DataRectChanged(oldDataRect, fDataRect);
		// notify scroller
		if (fScrollSource) {
			fScrollSource->SetScrollingEnabled(false);
			fScrollSource->DataRectChanged(oldDataRect, fDataRect);
		}
		// adjust the scroll offset, if necessary
		offset = ValidScrollOffsetFor(offset);
		if (offset != fScrollOffset)
			SetScrollOffset(offset);

		if (fScrollSource)
			fScrollSource->SetScrollingEnabled(true);
	}
}

// ValidScrollOffsetFor
//
// Returns the valid scroll offset next to the supplied offset.
BPoint
Scrollable::ValidScrollOffsetFor(BPoint offset) const
{
	return ValidScrollOffsetFor(offset, fDataRect);
}

// ValidScrollOffsetFor
//
// Returns the valid scroll offset next to the supplied offset.
BPoint
Scrollable::ValidScrollOffsetFor(BPoint offset, const BRect& dataRect) const
{
	float maxX = max_c(dataRect.left, dataRect.right - fVisibleWidth);
	float maxY = max_c(dataRect.top, dataRect.bottom - fVisibleHeight);
	// adjust the offset to be valid
	if (offset.x < dataRect.left)
		offset.x = dataRect.left;
	else if (offset.x > maxX)
		offset.x = maxX;
	if (offset.y < dataRect.top)
		offset.y = dataRect.top;
	else if (offset.y > maxY)
		offset.y = maxY;
	return offset;
}

// SetVisibleSize
//
// Sets the visible size.
void
Scrollable::SetVisibleSize(float width, float height)
{
	if ((fVisibleWidth != width || fVisibleHeight != height) &&
		width >= 0 && height >= 0) {
		float oldWidth = fVisibleWidth;
		float oldHeight = fVisibleHeight;
		fVisibleWidth = width;
		fVisibleHeight = height;
		// notify ourselves
		VisibleSizeChanged(oldWidth, oldHeight, fVisibleWidth, fVisibleHeight);
		// notify scroller
		if (fScrollSource) {
			fScrollSource->VisibleSizeChanged(oldWidth, oldHeight,
											  fVisibleWidth, fVisibleHeight);
		}
		// adjust the scroll offset, if necessary
		BPoint offset = ValidScrollOffsetFor(fScrollOffset);
		if (offset != fScrollOffset)
			SetScrollOffset(offset);
	}
}

// VisibleBounds
//
// Returns the visible bounds, i.e. a rectangle of the visible size
// located at (0.0, 0.0).
BRect
Scrollable::VisibleBounds() const
{
	return BRect(0.0, 0.0, fVisibleWidth, fVisibleHeight);
}

// VisibleRect
//
// Returns the visible rect, i.e. a rectangle of the visible size located
// at the scroll offset.
BRect
Scrollable::VisibleRect() const
{
	BRect rect(0.0, 0.0, fVisibleWidth, fVisibleHeight);
	rect.OffsetBy(fScrollOffset);
	return rect;
}

// DataRectChanged
//
// Hook function. Implemented by derived classes to get notified when
// the data rect has changed.
void
Scrollable::DataRectChanged(BRect /*oldDataRect*/, BRect /*newDataRect*/)
{
}

// ScrollOffsetChanged
//
// Hook function. Implemented by derived classes to get notified when
// the scroll offset has changed.
void
Scrollable::ScrollOffsetChanged(BPoint /*oldOffset*/, BPoint /*newOffset*/)
{
}

// VisibleSizeChanged
//
// Hook function. Implemented by derived classes to get notified when
// the visible size has changed.
void
Scrollable::VisibleSizeChanged(float /*oldWidth*/, float /*oldHeight*/,
							   float /*newWidth*/, float /*newHeight*/)
{
}

// ScrollSourceChanged
//
// Hook function. Implemented by derived classes to get notified when
// the scroll source has changed.
void
Scrollable::ScrollSourceChanged(Scroller* /*oldSource*/,
								Scroller* /*newSource*/)
{
}


