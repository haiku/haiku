/*
 * Copyright (c) 2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef WINDOW_LIST_H
#define WINDOW_LIST_H


#include <SupportDefs.h>
#include <Point.h>


class WindowLayer;


class WindowList {
	public:
		WindowList(int32 index = 0);
		~WindowList();

		void SetIndex(int32 index);
		int32 Index() const { return fIndex; }

		WindowLayer* FirstWindow() { return fFirstWindow; }
		WindowLayer* LastWindow() { return fLastWindow; }

		void AddWindow(WindowLayer* window, WindowLayer* before = NULL);
		void RemoveWindow(WindowLayer* window);

		bool HasWindow(WindowLayer* window) const;

	private:
		int32			fIndex;
		WindowLayer*	fFirstWindow;
		WindowLayer*	fLastWindow;
};

enum window_lists {
	kAllWindowList = 32,
	kSubsetList,
	kWorkingList,
};

struct window_anchor {
	window_anchor();

	WindowLayer*	next;
	WindowLayer*	previous;
	BPoint			position;
};

extern const BPoint kInvalidWindowPosition;

#endif	// WINDOW_LIST_H
