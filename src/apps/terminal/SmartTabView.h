/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 *	Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 */

#ifndef __SMARTTABVIEW_H
#define __SMARTTABVIEW_H

#include <View.h>

class BTabView;
class SmartTabView : public BView {
public:
	SmartTabView(BRect frame, const char *name,
			button_width width = B_WIDTH_AS_USUAL, 
			uint32 resizingMode = B_FOLLOW_ALL,
			uint32 flags = B_FULL_UPDATE_ON_RESIZE |
					B_WILL_DRAW | B_NAVIGABLE_JUMP |
					B_FRAME_EVENTS | B_NAVIGABLE);
	virtual ~SmartTabView();
	
	virtual BView *ContainerView();
	virtual BView *ViewForTab(int32);

	virtual	void Select(int32 tab);
	virtual int32 Selection() const;

	virtual	void AddTab(BView *target, BTab *tab = NULL);
	virtual BTab* RemoveTab(int32 index);
	virtual int32 CountTabs() const;	
	
	//virtual void Draw(BRect rect);

private:
	BTabView *fTabView;
	BView *fView;
};

#endif // __SMARTTABVIEW_H

