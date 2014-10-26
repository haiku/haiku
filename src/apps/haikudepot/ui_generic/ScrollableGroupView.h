/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SCROLLABLE_GROUP_VIEW_H
#define SCROLLABLE_GROUP_VIEW_H


#include <GroupView.h>

//! A group view to hold items in a vertically scrollable area. Needs to
// be embedded into a BScrollView with vertical scrollbar to work. Get the
// BGroupLayout with GroupLayout() and add or remove items/views to the layout.
class ScrollableGroupView : public BGroupView {
public:
								ScrollableGroupView();

	virtual	BSize				MinSize();

protected:
	virtual	void				DoLayout();
};


#endif // SCROLLABLE_GROUP_VIEW_H
