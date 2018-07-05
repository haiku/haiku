/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */
#ifndef COLORWHICH_LIST_VIEW_H
#define COLORWHICH_LIST_VIEW_H


#include <ListView.h>


class ColorWhichListView : public BListView
{
public:
								ColorWhichListView(const char* name,
									list_view_type type
										= B_SINGLE_SELECTION_LIST,
									uint32 flags = B_WILL_DRAW
										| B_FRAME_EVENTS | B_NAVIGABLE);
	virtual						~ColorWhichListView();

	virtual	bool				InitiateDrag(BPoint where, int32 index,
									bool wasSelected);
	virtual	void				MessageReceived(BMessage* message);
};


#endif	// COLORWHICH_LIST_VIEW_H
