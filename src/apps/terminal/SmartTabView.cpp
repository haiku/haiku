/*
 * Copyright 2007-2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 *	Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 *		Ingo Weinhold (ingo_weinhold@gmx.de)
 */


/*!	The SmartTabView class is a BTabView descendant that hides the tab bar
	as long as there is only a single tab.
	Furthermore, it provides a tab context menu, as well as allowing you to
	close buttons with the middle mouse button.
*/


#include "SmartTabView.h"

#include <stdio.h>

#include <BitmapButton.h>
#include <Button.h>
#include <Catalog.h>
#include <Locale.h>
#include <Message.h>
#include <Messenger.h>
#include <Screen.h>
#include <ScrollView.h>
#include <Window.h>

#include "TermConst.h"
#include "WindowIcon.h"


// #pragma mark - SmartTabView


SmartTabView::SmartTabView(BRect frame, const char* name, button_width width,
		uint32 resizingMode, uint32 flags)
	:
	BTabView(frame, name, width, resizingMode, flags),
	fInsets(0, 0, 0, 0),
	fScrollView(NULL),
	fListener(NULL)
{
	// Resize the container view to fill the complete tab view for single-tab
	// mode. Later, when more than one tab is added, we shrink the container
	// view again.
	frame.OffsetTo(B_ORIGIN);
	ContainerView()->MoveTo(frame.LeftTop());
	ContainerView()->ResizeTo(frame.Width(), frame.Height());

	BRect buttonRect(frame);
	buttonRect.left = frame.right - B_V_SCROLL_BAR_WIDTH + 1;
	buttonRect.bottom = frame.top + TabHeight() - 1;
	fFullScreenButton = new BBitmapButton(kWindowIconBits, kWindowIconWidth,
		kWindowIconHeight, kWindowIconFormat, new BMessage(FULLSCREEN));
	fFullScreenButton->SetResizingMode(B_FOLLOW_TOP | B_FOLLOW_RIGHT);
	fFullScreenButton->MoveTo(buttonRect.LeftTop());
	fFullScreenButton->ResizeTo(buttonRect.Width(), buttonRect.Height());
	fFullScreenButton->Hide();

	AddChild(fFullScreenButton);
}


SmartTabView::~SmartTabView()
{
}


void
SmartTabView::SetInsets(float left, float top, float right, float bottom)
{
	fInsets.left = left;
	fInsets.top = top;
	fInsets.right = right;
	fInsets.bottom = bottom;
}


void
SmartTabView::MouseDown(BPoint point)
{
	bool handled = false;

	if (CountTabs() > 1) {
		int32 tabIndex = _ClickedTabIndex(point);
		int32 buttons = 0;
		int32 clickCount = 0;
		Window()->CurrentMessage()->FindInt32("buttons", &buttons);
		Window()->CurrentMessage()->FindInt32("clicks", &clickCount);

		if ((buttons & B_PRIMARY_MOUSE_BUTTON) != 0 && clickCount == 2) {
			if (fListener != NULL)
				fListener->TabDoubleClicked(this, point, tabIndex);
		} else if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
			if (fListener != NULL)
				fListener->TabRightClicked(this, point, tabIndex);
			handled = true;
		} else if ((buttons & B_TERTIARY_MOUSE_BUTTON) != 0) {
			if (fListener != NULL)
				fListener->TabMiddleClicked(this, point, tabIndex);
			handled = true;
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
SmartTabView::Select(int32 index)
{
	BTabView::Select(index);
	BView *view = ViewForTab(index);
	if (view != NULL) {
		view->MoveTo(fInsets.LeftTop());
		view->ResizeTo(ContainerView()->Bounds().Width()
				- fInsets.left - fInsets.right,
			ContainerView()->Bounds().Height() - fInsets.top - fInsets.bottom);
	}

	if (fListener != NULL)
		fListener->TabSelected(this, index);
}


void
SmartTabView::AddTab(BView* target, BTab* tab)
{
	if (target == NULL)
		return;

	BTabView::AddTab(target, tab);

	if (CountTabs() == 1) {
		// Call select on the tab, since
		// we're resizing the contained view
		// inside that function
		Select(0);
	} else if (CountTabs() == 2) {
		// Need to resize the view, since we're switching from "normal"
		// to tabbed mode
		ContainerView()->ResizeBy(0, -TabHeight());
		ContainerView()->MoveBy(0, TabHeight());
		fFullScreenButton->Show();

		// Make sure the content size stays the same, but take special care
		// of full screen mode
		BScreen screen(Window());
		if (Window()->DecoratorFrame().Height() + 2 * TabHeight()
				< screen.Frame().Height()) {
			if (Window()->Frame().bottom + TabHeight()
				> screen.Frame().bottom - 5) {
				Window()->MoveBy(0, -TabHeight());
			}

			Window()->ResizeBy(0, TabHeight());
		}

		// Adapt scroll bar if there is one
		if (fScrollView != NULL) {
			BScrollBar* bar = fScrollView->ScrollBar(B_VERTICAL);
			if (bar != NULL) {
				bar->ResizeBy(0, -1);
				bar->MoveBy(0, 1);
			}
		}

		SetBorder(B_NO_BORDER);
	}

	Invalidate(TabFrame(CountTabs() - 1).InsetByCopy(-2, -2));
}


BTab*
SmartTabView::RemoveTab(int32 index)
{
	if (CountTabs() == 2) {
		// Hide the tab bar again by resizing the container view

		// Make sure the content size stays the same, but take special care
		// of full screen mode
		BScreen screen(Window());
		if (Window()->DecoratorFrame().Height() + 2 * TabHeight()
				< screen.Frame().Height()) {
			if (Window()->Frame().bottom
				> screen.Frame().bottom - 5 - TabHeight()) {
				Window()->MoveBy(0, TabHeight());
			}
			Window()->ResizeBy(0, -TabHeight());
		}

		// Adapt scroll bar if there is one
		if (fScrollView != NULL) {
			BScrollBar* bar = fScrollView->ScrollBar(B_VERTICAL);
			if (bar != NULL) {
				bar->ResizeBy(0, 1);
				bar->MoveBy(0, -1);
			}
		}

		ContainerView()->MoveBy(0, -TabHeight());
		ContainerView()->ResizeBy(0, TabHeight());
		fFullScreenButton->Hide();
	}

	return BTabView::RemoveTab(index);
}


void
SmartTabView::MoveTab(int32 index, int32 newIndex)
{
	// check the indexes
	int32 count = CountTabs();
	if (index == newIndex || index < 0 || newIndex < 0 || index >= count
		|| newIndex >= count) {
		return;
	}

	// Remove the tab from its position and add it to the end. Then cycle the
	// tabs starting after the new position (save the tab already moved) to the
	// end.
	BTab* tab = BTabView::RemoveTab(index);
	BTabView::AddTab(tab->View(), tab);

	for (int32 i = newIndex; i < count - 1; i++) {
		tab = BTabView::RemoveTab(newIndex);
		BTabView::AddTab(tab->View(), tab);
	}
}


BRect
SmartTabView::DrawTabs()
{
	if (CountTabs() > 1)
		return BTabView::DrawTabs();

	return BRect();
}


/*!	If you have a vertical scroll view that overlaps with the menu bar, it will
	be resized automatically when the tabs are hidden/shown.
*/
void
SmartTabView::SetScrollView(BScrollView* scrollView)
{
	fScrollView = scrollView;
}


int32
SmartTabView::_ClickedTabIndex(const BPoint& point)
{
	if (point.y <= TabHeight()) {
		for (int32 i = 0; i < CountTabs(); i++) {
			if (TabFrame(i).Contains(point))
				return i;
		}
	}

	return -1;
}


// #pragma mark - Listener


SmartTabView::Listener::~Listener()
{
}


void
SmartTabView::Listener::TabSelected(SmartTabView* tabView, int32 index)
{
}


void
SmartTabView::Listener::TabDoubleClicked(SmartTabView* tabView, BPoint point,
	int32 index)
{
}


void
SmartTabView::Listener::TabMiddleClicked(SmartTabView* tabView, BPoint point,
	int32 index)
{
}


void
SmartTabView::Listener::TabRightClicked(SmartTabView* tabView, BPoint point,
	int32 index)
{
}
