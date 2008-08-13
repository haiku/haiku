/*
 * Copyright (c) 2005-2008, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef WINDOW_LIST_H
#define WINDOW_LIST_H


#include <SupportDefs.h>
#include <Point.h>


class Window;


class WindowList {
public:
					WindowList(int32 index = 0);
					~WindowList();

			void	SetIndex(int32 index);
			int32	Index() const { return fIndex; }

			Window*	FirstWindow() const { return fFirstWindow; }
			Window*	LastWindow() const { return fLastWindow; }

			void	AddWindow(Window* window, Window* before = NULL);
			void	RemoveWindow(Window* window);

			bool	HasWindow(Window* window) const;
			bool	ValidateWindow(Window* window) const;

			int32	Count() const;
						// O(n)

private:
	int32			fIndex;
	Window*			fFirstWindow;
	Window*			fLastWindow;
};

enum window_lists {
	kAllWindowList = 32,
	kSubsetList,
	kFocusList,
	kWorkingList,

	kListCount
};

struct window_anchor {
	window_anchor();

	Window*	next;
	Window*	previous;
	BPoint	position;
};

extern const BPoint kInvalidWindowPosition;

#endif	// WINDOW_LIST_H
