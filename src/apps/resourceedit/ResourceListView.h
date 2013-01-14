/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef RESOURCE_LIST_VIEW_H
#define RESOURCE_LIST_VIEW_H


#include <ColumnListView.h>


class ResourceListView : public BColumnListView {
public:
			ResourceListView(BRect rect, const char* name, uint32 resizingMode,
				uint32 drawFlags, border_style border = B_NO_BORDER,
				bool showHorizontalScrollbar = true);
			~ResourceListView();

	void	MouseDown(BPoint point);
	void	MessageReceived(BMessage* msg);

private:

};


#endif
