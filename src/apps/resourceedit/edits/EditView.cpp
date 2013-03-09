/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "EditView.h"


EditView::EditView(const char* name)
	:
	BView(BRect(0, 0, 0, 0), name, 0, 0)
{
	// Implemented by subclasses.
}


EditView::~EditView()
{
	// Implemented by subclasses.
}


void
EditView::AttachTo(BView* view)
{
	while (true) {
		BView* child = ChildAt(0);

		if (child == NULL)
			break;

		child->RemoveSelf();
	}

	// Implemented by subclasses.
}


void
EditView::Edit(ResourceRow* row)
{
	fRow = row;
	// Implemented by subclasses.
}


void
EditView::Commit()
{
	// Implemented by subclasses.
}
