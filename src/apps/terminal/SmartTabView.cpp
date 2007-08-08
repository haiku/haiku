/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 *	Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 */

#include "SmartTabView.h"

#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <Window.h>

#include <stdio.h>

const static uint32 kCloseTab = 'ClTb';

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
	bool handled = false;

	if (CountTabs() > 1) {
		int32 tabIndex = _ClickedTabIndex(point);
		if (tabIndex >= 0) {		
			int32 buttons;
			Window()->CurrentMessage()->FindInt32("buttons", &buttons);
			if (buttons & B_SECONDARY_MOUSE_BUTTON) {	
				BMessage *message = new BMessage(kCloseTab);
				message->AddInt32("index", tabIndex);
				
				BPopUpMenu *popUpMenu = new BPopUpMenu("tab menu");
				popUpMenu->AddItem(new BMenuItem("Close Tab", message));
				popUpMenu->SetAsyncAutoDestruct(true);
				popUpMenu->SetTargetForItems(BMessenger(this));

				// TODO: I thought I'd get a "sticky" menu with this call...
				// Bug in our implementation or I just have to use the "other"
				// Go() method?	
				popUpMenu->Go(ConvertToScreen(point), true, true, true);
								
				handled = true;		
			} else if (buttons & B_TERTIARY_MOUSE_BUTTON) {
				RemoveAndDeleteTab(tabIndex);
				handled = true;
			}
		}	
	}

	if (!handled)
		BTabView::MouseDown(point);
		
}


void
SmartTabView::AttachedToWindow()
{
	BTabView::AttachedToWindow();
}


void
SmartTabView::AllAttached()
{
	BTabView::AllAttached();
}


void
SmartTabView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kCloseTab:
		{
			int32 tabIndex = 0;
			if (message->FindInt32("index", &tabIndex) == B_OK)
				RemoveAndDeleteTab(tabIndex);
			break;
		}		
		default:
			BTabView::MessageReceived(message);
			break;	
	}
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
SmartTabView::RemoveAndDeleteTab(int32 index)
{
	// Select another tab
	if (index == Selection()) {
		if (index > 0)
			Select(index - 1);
		else if (index < CountTabs())
			Select(index + 1);
	}	
	BTab *tab = RemoveTab(index);		
	delete tab;	
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


int32
SmartTabView::_ClickedTabIndex(const BPoint &point)
{
	if (point.y <= TabHeight()) {
		for (int32 i = 0; i < CountTabs(); i++) {
			if (TabFrame(i).Contains(point))
				return i;
		}
	}

	return -1;
}


