/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef WINDOW_STACK_TEST_H
#define WINDOW_STACK_TEST_H

#include <Box.h>
#include <Button.h>
#include <GroupLayoutBuilder.h>
#include <ListView.h>
#include <StringItem.h>
#include <StringView.h>


class BWindow;


class WindowListItem : public BStringItem
{
public:
					WindowListItem(const char* text, BWindow* window);

	BWindow*		Window() { return fWindow; }

private:
	BWindow* 		fWindow;
};


class MainView : public BBox
{
public:
						MainView();
	virtual				~MainView() {}

	virtual	void		AttachedToWindow();
	virtual	void		MessageReceived(BMessage* message);

private:
		BStringView*	fStackedWindowsLabel;
		BListView*		fStackedWindowsList;
		BButton*		fGetWindowsButton;
		BButton*		fAddWindowButton;
		BButton*		fRemoveWindowButton;
};


#endif
