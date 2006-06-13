/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#ifndef SCROLLABLE_H
#define SCROLLABLE_H

#include <Point.h>
#include <Rect.h>

class Scroller;

class Scrollable {
 public:
								Scrollable();
	virtual						~Scrollable();

			void				SetScrollSource(Scroller* source);
			Scroller*			ScrollSource() const;

			void				SetDataRect(BRect dataRect);
			BRect				DataRect() const;

			void				SetScrollOffset(BPoint offset);
			BPoint				ScrollOffset() const;

			void				SetVisibleSize(float width, float height);
			BRect				VisibleBounds() const;
			BRect				VisibleRect() const;

protected:
	virtual	void				DataRectChanged(BRect oldDataRect,
												BRect newDataRect);
	virtual	void				ScrollOffsetChanged(BPoint oldOffset,
													BPoint newOffset);
	virtual	void				VisibleSizeChanged(float oldWidth,
												   float oldHeight,
												   float newWidth,
												   float newHeight);
	virtual	void				ScrollSourceChanged(Scroller* oldSource,
													Scroller* newSource);

 private:
			BRect				fDataRect;
			BPoint				fScrollOffset;
			float				fVisibleWidth;
			float				fVisibleHeight;
			Scroller*			fScrollSource;

			BPoint				_ValidScrollOffsetFor(BPoint offset) const;
};


#endif	// SCROLLABLE_H

