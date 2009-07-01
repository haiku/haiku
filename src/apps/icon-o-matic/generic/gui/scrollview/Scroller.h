/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */
#ifndef SCROLLER_H
#define SCROLLER_H

#include <Rect.h>

class Scrollable;

class Scroller {
 public:
								Scroller();
	virtual						~Scroller();

			void				SetScrollTarget(Scrollable* target);
			Scrollable*			ScrollTarget() const;

			void				SetDataRect(BRect dataRect);
			BRect				DataRect() const;

			void				SetScrollOffset(BPoint offset);
			BPoint				ScrollOffset() const;

			void				SetVisibleSize(float width, float height);
			BRect				VisibleBounds() const;
			BRect				VisibleRect() const;

	virtual	void				SetScrollingEnabled(bool enabled);
			bool				IsScrollingEnabled() const
									{ return fScrollingEnabled; }

	virtual	bool				IsScrolling() const;

protected:
	virtual	void				DataRectChanged(BRect oldDataRect,
												BRect newDataRect);
	virtual	void				ScrollOffsetChanged(BPoint oldOffset,
													BPoint newOffset);
	virtual	void				VisibleSizeChanged(float oldWidth,
												   float oldHeight,
												   float newWidth,
												   float newHeight);
	virtual	void				ScrollTargetChanged(Scrollable* oldTarget,
													Scrollable* newTarget);

 protected:
			Scrollable*			fScrollTarget;
			bool				fScrollingEnabled;

	friend class Scrollable;
};


#endif	// SCROLLER_H
