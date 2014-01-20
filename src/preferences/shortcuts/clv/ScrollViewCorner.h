/*
 * Copyright 1999-2009 Jeremy Friesner
 * Copyright 2009-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		John Scipione, jscipione@gmail.com
 */
#ifndef SCROLL_VIEW_CORNER_H
#define SCROLL_VIEW_CORNER_H


/*
 * If you have a BScrollView with horizontal and vertical sliders that isn't
 * seated to the lower-right corner of a B_DOCUMENT_WINDOW, there's a "hole"
 * between the sliders that needs to be filled.  You can use this to fill it.
 * In general, it looks best to set the ScrollViewCorner color to
 * BeInactiveControlGrey if the vertical BScrollBar is inactive, and the color
 * to BeBackgroundGrey if the vertical BScrollBar is active.  Have a look at
 * Demo3 of ColumnListView to see what I mean if this is unclear.
 */


class ScrollViewCorner : public BView
{
public:
								ScrollViewCorner(float left, float top);
	virtual						~ScrollViewCorner();

	virtual	void				Draw(BRect updateRect);
};


#endif	// SCROLL_VIEW_CORNER_H
