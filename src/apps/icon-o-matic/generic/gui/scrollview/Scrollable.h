/*
 * Copyright 2001-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
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

			void				SetDataRect(BRect dataRect,
									bool validateScrollOffset = true);
			BRect				DataRect() const;

	virtual	void				SetScrollOffset(BPoint offset);
			BPoint				ScrollOffset() const;

			void				SetDataRectAndScrollOffset(BRect dataRect,
									BPoint offset);

			BPoint				ValidScrollOffsetFor(BPoint offset) const;
			BPoint				ValidScrollOffsetFor(BPoint offset,
									const BRect& dataRect) const;

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
};


#endif	// SCROLLABLE_H

