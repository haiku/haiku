//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	Author:			Adi Oanca <adioanca@myrealbox.com>
//	Description:	Tracks workspaces
//  
//------------------------------------------------------------------------------
#ifndef _WORKSPACE_H_
#define _WORKSPACE_H_

#include <SupportDefs.h>
#include <Locker.h>

#include "RGBColor.h"

class WinBorder;
class RBGColor;
class RootLayer;

struct ListData{
	WinBorder	*layerPtr;
	ListData	*upperItem;
	ListData	*lowerItem;
};

class Workspace
{
public:
								Workspace(const uint32 colorspace,
											int32 ID,
											const RGBColor& BGColor,
											RootLayer* owner);
								~Workspace();

			bool				AddLayerPtr(WinBorder* layer);
			bool				RemoveLayerPtr(WinBorder* layer);
			bool				HideSubsetWindows(WinBorder* layer);
			WinBorder*			SetFocusLayer(WinBorder* layer);
			WinBorder*			FocusLayer() const;
			WinBorder*			SetFrontLayer(WinBorder* layer);
			WinBorder*			FrontLayer() const;

			void				MoveToBack(WinBorder* newLast);

			WinBorder*			GoToBottomItem(); // the one that's visible.
			WinBorder*			GoToUpperItem();
			WinBorder*			GoToTopItem();
			WinBorder*			GoToLowerItem();
			bool				GoToItem(WinBorder* layer);

			WinBorder*			SearchWinBorder(BPoint pt);
			void				Invalidate();

			void				SetLocalSpace(const uint32 colorspace);
			uint32				LocalSpace() const;

			void				SetBGColor(const RGBColor &c);
			RGBColor			BGColor(void) const;
			
			int32				ID() const { return fID; }

//			void				SetFlags(const uint32 flags);
//			uint32				Flags(void) const;
	// debug methods
			void				PrintToStream() const;
			void				PrintItem(ListData *item) const;

// .... private :-) - do not use!
			void				SearchAndSetNewFront(WinBorder* preferred);
			void				SearchAndSetNewFocus(WinBorder* preferred);
			void				BringToFrontANormalWindow(WinBorder* layer);

			ListData*			HasItem(ListData* item);
			ListData*			HasItem(WinBorder* layer);

private:

			void				InsertItem(ListData* item, ListData* before);
			void				RemoveItem(ListData* item);

			ListData*			FindPlace(ListData* pref);

			int32				fID;
			uint32				fSpace;
//			uint32				fFlags;
			RGBColor			fBGColor;
			
			BLocker				opLock;

			RootLayer			*fOwner;
			
			ListData			*fBottomItem, // first visible onscreen
								*fTopItem, // the last visible(or covered by other Layers)
								*fCurrentItem, // pointer to the currect element in the list
								*fFocusItem, // the focus WinBorder - for keyboard events
								*fFrontItem; // the one the mouse can bring in front as possible(in its set)
};

#endif
