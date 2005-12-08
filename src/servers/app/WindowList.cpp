/*
 * Copyright (c) 2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


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
}


void
WindowList::RemoveWindow(WindowLayer* window)
{
// TODO: Axel, the same window can be removed in the same list
// more than once, would that be a bug?
	window_anchor& windowAnchor = window->Anchor(fIndex);

	if (fFirstWindow == window) {
		// it's the first child
		fFirstWindow = windowAnchor.next;
	} else {
		// it must have a previous sibling if it was not
		// previously removed from this list
		if (windowAnchor.previous)
			windowAnchor.previous->Anchor(fIndex).next = windowAnchor.next;
	}

	if (fLastWindow == window) {
		// it's the last child
		fLastWindow = windowAnchor.previous;
	} else {
		// it must have a next sibling if it was not
		// previously removed from this list
		if (windowAnchor.next)
			windowAnchor.next->Anchor(fIndex).previous = windowAnchor.previous;
	}

	windowAnchor.previous = NULL;
	windowAnchor.next = NULL;
}

