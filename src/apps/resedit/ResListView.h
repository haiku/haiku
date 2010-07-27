/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#ifndef RESLISTVIEW_H
#define RESLISTVIEW_H

#include "ColumnListView.h"

class ResListView : public BColumnListView
{
public:
			ResListView(const BRect &frame, const char *name, int32 resize, int32 flags,
						border_style border);
	void	MessageDropped(BMessage *msg, BPoint pt);
};

#endif
