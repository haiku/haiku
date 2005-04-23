//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Workspace.h
//	Author:			Adi Oanca <adioanca@cotty.iren.com>
//	Description:	Tracks workspaces
//  
//  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//  Notes:			IMPORTANT WARNING
//					This object does not use any locking mechanism. It is designed
//					to be used only by RootLayer class. DO NOT USE from another class!
//------------------------------------------------------------------------------
#ifndef _WORKSPACE_H_
#define _WORKSPACE_H_

#include <SupportDefs.h>
#include <Locker.h>
#include <Accelerant.h>

#include "RGBColor.h"

class WinBorder;

struct ListData
{
	bool isFree;
	WinBorder *layerPtr;
	ListData *upperItem;
	ListData *lowerItem;
};

class Workspace
{
public:
								Workspace(	const int32 ID,
											const uint32 colorspace,
											const RGBColor& BGColor);
								~Workspace(void);

			int32				ID(void) const { return fID; }
	
			void				AddWinBorder(WinBorder *winBorder);
			void				RemoveWinBorder(WinBorder *winBorder);
			bool				HasWinBorder(const WinBorder *winBorder) const;

			WinBorder*			Focus(void) const;
			WinBorder*			Front(void) const;
			bool				GetWinBorderList(void **list, int32 *itemCount ) const;
	
			bool				MoveToBack(WinBorder *newLast);
			bool				MoveToFront(WinBorder *newFront, bool doNotDisturb = false);

			bool				HideWinBorder(WinBorder *winBorder);
			bool				ShowWinBorder(WinBorder *winBorder, bool userBusy = false);

			// resolution related methods.
			void				SetLocalSpace(const uint32 colorspace);
			uint32				LocalSpace(void) const;
	
			void				SetBGColor(const RGBColor &c);
			RGBColor			BGColor(void) const;

			// settings related methods	
			void				GetSettings(const BMessage &msg);
			void				GetDefaultSettings(void);
			void				PutSettings(BMessage *msg, const int32 &index) const;
	static	void				PutDefaultSettings(BMessage *msg, const int32 &index);
	
			// debug methods
			void				PrintToStream(void) const;
			void				PrintItem(ListData *item) const;
	
private:
			void				InsertItem(ListData *item, ListData *before);
			void				RemoveItem(ListData *item);
			ListData*			HasItem(const ListData *item, int32 *index = NULL) const;
			ListData*			HasItem(const WinBorder *layer, int32 *index = NULL) const;
			int32				IndexOf(const ListData *item) const;

			bool				placeToBack(ListData *newLast);
			void				placeInFront(ListData *item, const bool userBusy);

			bool				removeAndPlaceBefore(const WinBorder *wb, ListData *beforeItem);
			bool				removeAndPlaceBefore(ListData *item, ListData *beforeItem);

			WinBorder*			searchFirstMainWindow(WinBorder *wb) const;
			WinBorder*			searchANormalWindow(WinBorder *wb) const;

			bool				windowHasVisibleModals(const WinBorder *winBorder) const;
			ListData*			putModalsInFront(ListData *item);
			void				putFloatingInFront(ListData *item);
			void				saveFloatingWindows(ListData *itemNormal);

			ListData*			findNextFront() const;

	class MemoryPool
	{
	public:
					MemoryPool();
					~MemoryPool();
		ListData*	GetCleanMemory(WinBorder* winborder);
		void		ReleaseMemory(ListData* mem);
	private:
		void		expandBuffer(int32 start);
		ListData	*buffer;
		int32		count;
	};

			int32				fID;
			uint32				fSpace;
			RGBColor			fBGColor;

			// first visible onscreen
			ListData			*fBottomItem;
	
			// the last visible(or covered by other Layers)
			ListData			*fTopItem;
	
			// the focus WinBorder - for keyboard events
			ListData			*fFocusItem;
	
			// pointer for which "big" actions are intended
			ListData			*fFrontItem;
	
			// settings for each workspace -- example taken from R5's app_server_settings file
			display_timing		fDisplayTiming;
			int16				fVirtualWidth;
			int16				fVirtualHeight;
	
			MemoryPool			fPool;
};

#endif
