/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered
trademarks of Be Incorporated in the United States and other countries. Other
brand product names are registered trademarks or trademarks of their respective
holders.
All rights reserved.
*/

#ifndef _DESKBAR_MENU_H_
#define _DESKBAR_MENU_H_


#include "NavMenu.h"


class TBarView;

enum recent_type {
	kRecentDocuments = 0,
	kRecentApplications,
	kRecentFolders,
	kRecentAppDocuments
};

class TRecentsMenu : public BNavMenu {
	public:
		TRecentsMenu(const char* name, TBarView* bar, int32 which,
			const char* signature = NULL, entry_ref* appRef = NULL);
		virtual ~TRecentsMenu();

		void			DetachedFromWindow();
		void			ResetTargets();

		int32			RecentsCount();
		bool			RecentsEnabled();

	private:
		virtual	bool	StartBuildingItemList();
		virtual	bool	AddNextItem();
				bool	AddRecents(int32 count);
		virtual	void	DoneBuildingItemList();
		virtual	void	ClearMenuBuildingState();

	private:
		int32			fWhich;
		entry_ref*		fAppRef;
		char*			fSignature;
		int32			fRecentsCount;
		bool			fRecentsEnabled;

		int32 			fItemIndex;
		BMessage		fRecentList;

		TBarView*		fBarView;
};


inline int32
TRecentsMenu::RecentsCount()
{
	return fRecentsCount;
}


inline bool
TRecentsMenu::RecentsEnabled()
{
	return fRecentsEnabled;
}


class TDeskbarMenu : public BNavMenu {
	public:
		TDeskbarMenu(TBarView* bar);

		void			AttachedToWindow();
		void			DetachedFromWindow();

		void			ResetTargets();

		static BMessenger DefaultTarget();

	private:
		enum State {
			kStart,
			kAddingRecents,
			kAddingDeskbarMenu,
			kDone
		};
	protected:
		BPoint			ScreenLocation();

		bool			AddStandardDeskbarMenuItems();

	private:
		virtual bool	StartBuildingItemList();
		virtual void	DoneBuildingItemList();
		virtual bool	AddNextItem();
		virtual void	ClearMenuBuildingState();

		// to keep track of the menu building state
		State 			fAddState;
		TBarView*		fBarView;
};


#endif	/* _DESKBAR_MENU_H_ */
