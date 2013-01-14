/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ResourceListView.h"

#include "Constants.h"

#include <ColumnListView.h>
#include <Entry.h>


ResourceListView::ResourceListView(BRect rect, const char* name,
	uint32 resizingMode, uint32 drawFlags, border_style border,
	bool showHorizontalScrollbar)
	:
	BColumnListView(rect, name, resizingMode, drawFlags, border,
		showHorizontalScrollbar)
{
	//SetMouseTrackingEnabled(true);
}


ResourceListView::~ResourceListView()
{

}


void
ResourceListView::MouseDown(BPoint point)
{
	PRINT(("MouseDown()"));
}


void
ResourceListView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_SIMPLE_DATA:
		{
			entry_ref ref;
			int32 n = 0;

			// TODO: Implement D&D adding of files.

			while (msg->FindRef("refs", n++, &ref) == B_OK) {
				PRINT(("%s\n", ref.name));
				// ...
			}

			break;
		}
		default:
			BColumnListView::MessageReceived(msg);
	}
}
