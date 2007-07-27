/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 *	Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 */

#include "SmartTabView.h"

SmartTabView::SmartTabView(BRect frame, const char *name, button_width width, 
				uint32 resizingMode, uint32 flags)
	:
	BTabView(frame, name, width, resizingMode, flags)
{
}


SmartTabView::~SmartTabView()
{
}


void
SmartTabView::Select(int32 index)
{
	BTabView::Select(index);
	BTab *tab = TabAt(Selection());
	if (tab != NULL) {	
		BView *view = tab->View();
		if (view != NULL)
			view->ResizeTo(Bounds().Width(), Bounds().Height());
	}
}
	
