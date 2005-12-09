/*
 * Copyright (c) 2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "DesktopSettings.h"
#include "WindowLayer.h"


const BPoint kInvalidWindowPosition = BPoint(INFINITY, INFINITY);


window_anchor::window_anchor()
	 :
	 next(NULL),
	 previous(NULL),
	 position(kInvalidWindowPosition)
{
}


//	#pragma mark -


WindowList::WindowList(int32 index)
	:
	fIndex(index),
	fFirstWindow(NULL),
	fLastWindow(NULL)
{
}


WindowList::~WindowList()
{
}


void
WindowList::SetIndex(int32 index)
{
	fIndex = index;
}


void
WindowList::AddWindow(WindowLayer* window, WindowLayer* before)
{
	window_anchor& windowAnchor = window->Anchor(fIndex);

	if (before != NULL) {
		window_anchor& beforeAnchor = before->Anchor(fIndex);

		// add view before this one
		windowAnchor.next = before;
		windowAnchor.previous = beforeAnchor.previous;
		if (windowAnchor.previous != NULL)
			windowAnchor.previous->Anchor(fIndex).next = window;

		beforeAnchor.previous = window;
		if (fFirstWindow == before)
			fFirstWindow = window;
	} else {
		// add view to the end of the list
		if (fLastWindow != NULL) {
			fLastWindow->Anchor(fIndex).next = window;
			windowAnchor.previous = fLastWindow;
		} else {
			fFirstWindow = window;
			windowAnchor.previous = NULL;
		}

		windowAnchor.next = NULL;
		fLastWindow = window;
	}

	if (fIndex < kMaxWorkspaces)
		window->SetWorkspaces(window->Workspaces() | (1UL << fIndex));
}


void
WindowList::RemoveWindow(WindowLayer* window)
{
	window_anchor& windowAnchor = window->Anchor(fIndex);

	if (fFirstWindow == window) {
		// it's the first child
		fFirstWindow = windowAnchor.next;
	} else {
		// it must have a previous sibling, then
		windowAnchor.previous->Anchor(fIndex).next = windowAnchor.next;
	}

	if (fLastWindow == window) {
		// it's the last child
		fLastWindow = windowAnchor.previous;
	} else {
		// then it must have a next sibling
		windowAnchor.next->Anchor(fIndex).previous = windowAnchor.previous;
	}

	if (fIndex < kMaxWorkspaces)
		window->SetWorkspaces(window->Workspaces() & ~(1UL << fIndex));

	windowAnchor.previous = NULL;
	windowAnchor.next = NULL;
}


bool
WindowList::HasWindow(WindowLayer* window) const
{
	if (window == NULL)
		return false;

	return window->Anchor(fIndex).next != NULL
		|| window->Anchor(fIndex).previous != NULL
		|| fFirstWindow == window
		|| fLastWindow == window;
}

