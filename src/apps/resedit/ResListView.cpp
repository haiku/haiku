/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#include "ResListView.h"

ResListView::ResListView(const BRect &frame, const char *name, int32 resize, int32 flags,
						border_style border)
	:  BColumnListView(frame,name,resize,flags,border)
{
}


void
ResListView::MessageDropped(BMessage *msg, BPoint pt)
{
	if (!Parent())
		return;
	
	BMessenger messenger(Parent());
	msg->what = B_REFS_RECEIVED;
	messenger.SendMessage(msg);
}
