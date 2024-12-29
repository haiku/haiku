/*
 * Copyright 2016-2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm, darkwyrm@earthlink.net
 *		John Scipione, jscipione@gmail.com
 */
#ifndef _COLOR_LIST_VIEW_H
#define _COLOR_LIST_VIEW_H


#include <ListView.h>


namespace BPrivate {


class BColorListView : public BListView {
public:
	enum {
		B_MESSAGE_SET_CURRENT_COLOR		= 'sccl',
		B_MESSAGE_SET_COLOR				= 'sclr'
	};

								BColorListView(const char* name,
									list_view_type type = B_SINGLE_SELECTION_LIST,
									uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);
	virtual						~BColorListView();

	virtual	bool				InitiateDrag(BPoint where, int32 index,
									bool wasSelected);
	virtual	void				MessageReceived(BMessage* message);
};


} // namespace BPrivate


#endif // _COLOR_LIST_VIEW_H
