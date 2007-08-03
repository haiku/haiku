/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 *	Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 */

#include "SmartTabView.h"

#include <Message.h>
#include <Window.h>

#include <stdio.h>

SmartTabView::SmartTabView(BRect frame, const char *name, button_width width, 
				uint32 resizingMode, uint32 flags)
	:
	BTabView(frame, name, width, resizingMode, flags)
{
	// See BTabView::_InitObject() to see why we are doing this
	ContainerView()->MoveBy(-3, -TabHeight() - 3);
	ContainerView()->ResizeBy(10, TabHeight() + 9);
}


SmartTabView::~SmartTabView()
{
}


void
SmartTabView::MouseDown(BPoint point)
{
	int32 buttons;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);

	if (CountTabs() > 1 && point.y <= TabHeight() && buttons & B_TERTIARY_MOUSE_BUTTON) {
		for (int32 i = 0; i < CountTabs(); i++) {
			if (TabFrame(i).Contains(point)) {
				// Select another tab				
				if (i > 0)
					Select(i - 1);
				else if (i < CountTabs())
					Select(i + 1);
				BTab *tab = RemoveTab(i);		
				delete tab;		
				return;
			}
		}
	} else 
		BTabView::MouseDown(point);
}


void
SmartTabView::AllAttached()
{
	BTabView::AllAttached();
}


void
SmartTabView::Select(int32 index)
{
	BTabView::Select(index);
	BView *view = ViewForTab(index);
	if (view != NULL) {
		view->ResizeTo(ContainerView()->Bounds().Width(), ContainerView()->Bounds().Height());
	}
}


void
SmartTabView::AddTab(BView *target, BTab *tab)
{
	if (target == NULL)
		return;

	if (CountTabs() == 1) {
		ContainerView()->ResizeBy(0, -TabHeight());
		ContainerView()->MoveBy(0, TabHeight());	
	}
	BTabView::AddTab(target, tab);
	
	Invalidate(TabFrame(CountTabs() - 1).InsetByCopy(-2, -2));
}


BTab *
SmartTabView::RemoveTab(int32 index)
{
	BTab *tab = BTabView::RemoveTab(index);
	if (CountTabs() == 1) {
		ContainerView()->MoveBy(0, -TabHeight());		
		ContainerView()->ResizeBy(0, TabHeight());	
	}
	return tab;
}


BRect
SmartTabView::DrawTabs()
{
	if (CountTabs() > 1)
		return BTabView::DrawTabs();
	return BRect();
}


