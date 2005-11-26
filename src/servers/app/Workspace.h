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

class WindowLayer;

struct ListData
{
	bool isFree;
	WindowLayer *layerPtr;
	ListData *upperItem;
	ListData *lowerItem;
};

class Workspace {
	public:
		class State {
			public:
							State() : Front(NULL), Focus(NULL), WindowList(50) { }

				void		PrintToStream();

				WindowLayer* Front;
				WindowLayer* Focus;
				BList		WindowList;
		};
								Workspace(	const int32 ID,
											const uint32 colorspace,
											const RGBColor& BGColor);
								~Workspace();

			int32				ID() const { return fID; }

			void				AddWindowLayer(WindowLayer *winBorder);
			void				RemoveWindowLayer(WindowLayer *winBorder);
			bool				HasWindowLayer(const WindowLayer *winBorder) const;

			WindowLayer*			Focus() const;
			WindowLayer*			Front() const;
			WindowLayer*			Active() const;
			void				GetState(Workspace::State *state) const;
			bool				AttemptToSetFront(WindowLayer *newFront);
			int32				AttemptToSetFocus(WindowLayer *newFocus);
			bool				AttemptToMoveToBack(WindowLayer *newBack);
			bool				AttemptToActivate(WindowLayer *toActivate);

			bool				GetWindowLayerList(void **list, int32 *itemCount ) const;

			bool				MoveToBack(WindowLayer *newLast);
			bool				MoveToFront(WindowLayer *newFront, bool doNotDisturb = false);

			bool				HideWindowLayer(WindowLayer *winBorder);
			bool				ShowWindowLayer(WindowLayer *winBorder, bool userBusy = false);

			// resolution related methods.
			status_t			SetDisplayMode(const display_mode &mode);
			status_t			GetDisplayMode(display_mode &mode) const;
	
			void				SetBGColor(const RGBColor &c);
			RGBColor			BGColor(void) const;

			// settings related methods	
			void				GetSettings(const BMessage &msg);
			void				GetDefaultSettings(void);
			void				PutSettings(BMessage *msg, const uint8 &index) const;
	static	void				PutDefaultSettings(BMessage *msg, const uint8 &index);
	
			// debug methods
			void				PrintToStream(void) const;
			void				PrintItem(ListData *item) const;
	
private:
			void				InsertItem(ListData *item, ListData *before);
			void				RemoveItem(ListData *item);
			ListData*			HasItem(const ListData *item, int32 *index = NULL) const;
			ListData*			HasItem(const WindowLayer *layer, int32 *index = NULL) const;
			int32				IndexOf(const ListData *item) const;

			bool				placeToBack(ListData *newLast);
			void				placeInFront(ListData *item, const bool userBusy);

			int32				_SetFocus(ListData *newFocusItem);

			bool				removeAndPlaceBefore(const WindowLayer *wb, ListData *beforeItem);
			bool				removeAndPlaceBefore(ListData *item, ListData *beforeItem);

			WindowLayer*			searchFirstMainWindow(WindowLayer *wb) const;
			WindowLayer*			searchANormalWindow(WindowLayer *wb) const;

			bool				windowHasVisibleModals(const WindowLayer *winBorder) const;
			ListData*			putModalsInFront(ListData *item);
			void				putFloatingInFront(ListData *item);
			void				saveFloatingWindows(ListData *itemNormal);

			ListData*			findNextFront() const;

	class MemoryPool
	{
	public:
					MemoryPool();
					~MemoryPool();
		ListData*	GetCleanMemory(WindowLayer* winborder);
		void		ReleaseMemory(ListData* mem);
	private:
		void		expandBuffer(int32 start);
		ListData	*buffer;
		int32		count;
	};

			int32				fID;
			RGBColor			fBGColor;

			// first visible onscreen
			ListData			*fBottomItem;

			// the last visible(or covered by other Layers)
			ListData			*fTopItem;

			// the focus WindowLayer - for keyboard events
			ListData			*fFocusItem;

			// pointer for which "big" actions are intended
			ListData			*fFrontItem;

			// settings for each workspace
			display_mode		fDisplayMode;
			
			MemoryPool			fPool;
};

#endif
