/*
 * Copyright 2007-2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 *	Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 *		Ingo Weinhold (ingo_weinhold@gmx.de)
 */
#ifndef SMART_TAB_VIEW_H
#define SMART_TAB_VIEW_H


#include <TabView.h>


class BButton;
class BPopUpMenu;
class BScrollView;


class SmartTabView : public BTabView {
public:
			class Listener;

public:
								SmartTabView(BRect frame, const char* name,
									button_width width = B_WIDTH_AS_USUAL,
									uint32 resizingMode = B_FOLLOW_ALL,
									uint32 flags = B_FULL_UPDATE_ON_RESIZE
										| B_WILL_DRAW | B_NAVIGABLE_JUMP
										| B_FRAME_EVENTS | B_NAVIGABLE);
								SmartTabView(const char* name,
									button_width width = B_WIDTH_AS_USUAL,
									uint32 flags = B_FULL_UPDATE_ON_RESIZE
										| B_WILL_DRAW | B_NAVIGABLE_JUMP
										| B_FRAME_EVENTS | B_NAVIGABLE
										| B_SUPPORTS_LAYOUT);
	virtual						~SmartTabView();

			void				SetInsets(float left, float top, float right,
									float bottom);

	virtual void				MouseDown(BPoint where);

	virtual void				AttachedToWindow();
	virtual void				AllAttached();

	virtual	void				Select(int32 tab);

	virtual	void				AddTab(BView* target, BTab* tab = NULL);
	virtual BTab*				RemoveTab(int32 index);
			void				MoveTab(int32 index, int32 newIndex);

	virtual BRect				DrawTabs();

			void				SetScrollView(BScrollView* scrollView);

			void				SetListener(Listener* listener)
									{ fListener = listener; }

private:
			int32				_ClickedTabIndex(const BPoint& point);

private:
			BRect				fInsets;
			BScrollView*		fScrollView;
			Listener*			fListener;
			BButton*			fFullScreenButton;
};


class SmartTabView::Listener {
public:
	virtual						~Listener();

	virtual	void				TabSelected(SmartTabView* tabView, int32 index);
	virtual	void				TabDoubleClicked(SmartTabView* tabView,
									BPoint point, int32 index);
	virtual	void				TabMiddleClicked(SmartTabView* tabView,
									BPoint point, int32 index);
	virtual	void				TabRightClicked(SmartTabView* tabView,
									BPoint point, int32 index);
};


#endif // SMART_TAB_VIEW_H
