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
//	File Name:		Workspace.cpp
//	Author:			Adi Oanca <adioanca@mymail.ro>
//	Description:	Tracks workspaces
//  
//------------------------------------------------------------------------------
#include <stdio.h>
#include <Window.h>

#include "Workspace.h"
#include "Layer.h"
#include "WinBorder.h"
#include "ServerWindow.h"
#include "ServerApp.h"
#include "RGBColor.h"
#include "Globals.h"
#include "FMWList.h"
#include "RootLayer.h"
#include "Desktop.h"

//#define DEBUG_WORKSPACE

#ifdef DEBUG_WORKSPACE
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

#ifdef DEBUG_WORKSPACE
#	include <stdio.h>
#	define STRACESTREAM() PrintToStream()
#else
#	define STRACESTREAM() ;
#endif


Workspace::Workspace(const uint32 colorspace, int32 ID, const RGBColor& BGColor, RootLayer* owner){
	fID			= ID;
	fSpace		= colorspace;
	fBGColor	= BGColor;

	fOwner		= owner;
	
	fBottomItem	= NULL;
	fTopItem	= NULL;
	fCurrentItem= NULL;
	fFocusItem	= NULL;
	fFrontItem	= NULL;
}
//---------------------------------------------------------------------------
Workspace::~Workspace(){
//printf("~Workspace( %ld )\n", fID);
	ListData		*toast;
	while(fBottomItem){
		toast		= fBottomItem;
		fBottomItem	= fBottomItem->upperItem;
		
		delete toast;
	}
	fBottomItem		= NULL;
	fTopItem		= NULL;
	fCurrentItem	= NULL;
	fFocusItem		= NULL;
	fFrontItem		= NULL;
}
//---------------------------------------------------------------------------
/* Adds layer ptr to workspace's list of WinBorders. After the item is added
 *	a *smart* search is performed to set the new front WinBorder. Of course,
 *	the first candidate is 'layer', but if it is hidden or it has the
 *	B_AVOID_FRONT flag, surely it won't get that status.
 * This method also calls SearchAndSetNewFocus witch searches for a WinBorder,
 *	to give focus to, having as preferred 'layer'.
 * Remember, some windows having B_AVOID_FOCUS can't have the focus state.
 */
bool Workspace::AddLayerPtr(WinBorder* layer){
		// allocate a new item
	ListData		*item;
	item			= new ListData;
	
	item->layerPtr	= layer;
	item->upperItem	= NULL;
	item->lowerItem	= NULL;

	opLock.Lock();
		// insert 'item' at the end. It doesn't matter where we add it,
		// it will be placed correctly by SearchAndSetNewFront(item->layerPtr);
	InsertItem(item, NULL);
	opLock.Unlock();

STRACE(("\n*AddLayerPtr(%s) -", layer->GetName()));

		// this may happen in case of subset windows.
	if(!(layer->IsHidden())){
			// do a *smart* search and set the new 'front'
		SearchAndSetNewFront(layer);
			// do a *smart* search and set the new 'focus' + a redraw
		SetFocusLayer(layer);
	}
	
	return true;
}
/* Removes a WinBorder from workspace's list. It DOES NOT delete it!
 * If this window was the front/focus one it calls SearchAndSetNew(Front/Focus)
 *	to give the respective state to the window below her.
 */
//---------------------------------------------------------------------------
bool Workspace::RemoveLayerPtr(WinBorder* layer){
	if (!layer)
		return false;
STRACE(("\n*Workspace(%ld)::RemoveLayerPtr(%s)\n", ID(), layer->GetName()));
STRACE(("BEFORE ANY opperation:\n"));
STRACESTREAM();
		// search to see if this workspace has WinBorder's pointer in its list
	ListData	*item = NULL;

	opLock.Lock();
	if ((item = HasItem(layer))){
		ListData	*nextItem	= NULL;
		bool		wasFront	= false;
		bool		wasFocus	= false;
		
		wasFront	= FrontLayer() == layer;
		wasFocus	= FocusLayer() == layer;

			// prepare to set new front/focus if this layer was front/focus
		nextItem	= item->upperItem;

		if (wasFront)
			SearchAndSetNewFront(nextItem? nextItem->layerPtr: NULL);

			// remove some windows.
		if (item && item->layerPtr->fLevel == B_NORMAL_FEEL)
		{
			ListData	*listItem = item->lowerItem;
			while(listItem && (listItem->layerPtr->fLevel == B_FLOATING_SUBSET_FEEL
								|| listItem->layerPtr->fLevel == B_FLOATING_APP_FEEL
								|| listItem->layerPtr->fLevel == B_MODAL_SUBSET_FEEL))
			{
					// *carefuly* remove the item from the list
				ListData	*itemX = listItem;
				listItem	= listItem->lowerItem;
				RemoveItem(itemX);
				delete itemX;
			}
		}
		
		RemoveItem(item);
		delete item;
		STRACESTREAM();		
		opLock.Unlock();

			// reset some internal variables
		layer->SetMainWinBorder(NULL);
		// its RootLayer is set to NULL by Layer::RemoveChild(layer);

		STRACE(("Layer %s found and removed from Workspace No %ld\n", layer->GetName(), ID()));

		if (wasFocus)
			SetFocusLayer(nextItem? nextItem->layerPtr: NULL);
		else
			Invalidate();

		return true;
	}
	else{
		STRACE(("Layer %s NOT found in Workspace No %ld\n", layer->GetName(), ID()));
		opLock.Unlock();
		return false;
	}
}
//---------------------------------------------------------------------------
bool Workspace::HideSubsetWindows(WinBorder* layer){
	if (!layer)
		return false;

		// search to see if this workspace has WinBorder's pointer in its list
	ListData	*item = NULL;

	opLock.Lock();
	if ((item = HasItem(layer))){
		ListData	*nextItem = NULL;

			// prepare to set new front/focus if this layer was front/focus
		nextItem	= item->upperItem;

		SearchAndSetNewFront(nextItem? nextItem->layerPtr: NULL);
			// we don't care bout focus in this method!!!
		//SearchAndSetNewFocus(nextItem? nextItem->layerPtr: NULL);

			// remove some windows.
		if (item && item->layerPtr->fLevel == B_NORMAL_FEEL)
		{
			ListData	*listItem = item->lowerItem;
			while(listItem && (listItem->layerPtr->fLevel == B_FLOATING_SUBSET_FEEL
								|| listItem->layerPtr->fLevel == B_FLOATING_APP_FEEL
								|| listItem->layerPtr->fLevel == B_MODAL_SUBSET_FEEL))
			{
					// *carefuly* remove the item from the list
				ListData	*itemX = listItem;
				listItem	= listItem->lowerItem;
				RemoveItem(itemX);
				delete itemX;
			}
		}
		
		opLock.Unlock();

		return true;
	}
	else{
		opLock.Unlock();
		return false;
	}
}
//---------------------------------------------------------------------------
WinBorder* Workspace::SetFocusLayer(WinBorder* layer){
STRACE(("\n@Workspace(%ld)::SetFOCUSLayer( %s )\n", ID(), layer? layer->GetName(): "NULL"));

	if(!(desktop->fGeneralLock.IsLocked()))
		debugger("Workspace::SetFocusLayer - desktop->fGeneralLock must be LOCKED!\n");

	WinBorder		*previousFocus = FocusLayer();

	SearchAndSetNewFocus(layer);

	if (previousFocus != FocusLayer()){
		if (previousFocus)
			previousFocus->HighlightDecorator(false);

		if (FocusLayer()){
			FocusLayer()->HighlightDecorator(true);
		}
	}
STRACE(("\n#Workspace(%ld)::SetFOCUSLayer( %s ) ENDED\n", ID(), layer? layer->GetName(): "NULL"));
	return FocusLayer();
}
//---------------------------------------------------------------------------
WinBorder* Workspace::FocusLayer() const{
	return fFocusItem ? fFocusItem->layerPtr : NULL;
}
//---------------------------------------------------------------------------
WinBorder* Workspace::SetFrontLayer(WinBorder* layer){
STRACE(("\n@Workspace(%ld)::SetFrontLayer( %s )\n", ID(), layer? layer->GetName(): "NULL"));

	if(!(desktop->fGeneralLock.IsLocked()))
		debugger("Workspace::SetFRONTLayer - desktop->fGeneralLock must be LOCKED!\n");

	SearchAndSetNewFront(layer);
STRACESTREAM();
// TODO: remove???
	Invalidate();

 	return fFrontItem? fFrontItem->layerPtr: NULL;;
}
//---------------------------------------------------------------------------
WinBorder* Workspace::FrontLayer() const{
	return fFrontItem? fFrontItem->layerPtr: NULL;
}
//---------------------------------------------------------------------------
WinBorder* Workspace::GoToBottomItem(){
	fCurrentItem	= fBottomItem;
	return fCurrentItem ? fCurrentItem->layerPtr : NULL;
}
//---------------------------------------------------------------------------
WinBorder* Workspace::GoToUpperItem(){
	if (fCurrentItem){
		fCurrentItem	= fCurrentItem->upperItem;
		return fCurrentItem ? fCurrentItem->layerPtr : NULL;
	}
	else
		return NULL;
}
//---------------------------------------------------------------------------
WinBorder* Workspace::GoToTopItem(){
	fCurrentItem	= fTopItem;
	return fCurrentItem ? fCurrentItem->layerPtr : NULL;
}
//---------------------------------------------------------------------------
WinBorder* Workspace::GoToLowerItem(){
	if (fCurrentItem){
		fCurrentItem	= fCurrentItem->lowerItem;
		return fCurrentItem ? fCurrentItem->layerPtr : NULL;
	}
	else
		return NULL;
}
//---------------------------------------------------------------------------
bool Workspace::GoToItem(WinBorder* layer){
	if (!layer)
		return false;

	if (fCurrentItem && fCurrentItem->layerPtr == layer){
		return true;
	}
	
	for( ListData *item = fBottomItem; item != NULL; item = item->upperItem){
		if (item->layerPtr == layer){
			fCurrentItem	= item;
			return true;
		}
	}
	return false;
}
//---------------------------------------------------------------------------
WinBorder* Workspace::SearchWinBorder(BPoint pt){
// TODO: implement correctly once you have clipping code working
// For the moment, take windows from front to back and see in witch one 'pt' falls
	WinBorder		*target = NULL;
	opLock.Lock();

	for( WinBorder *wb = GoToBottomItem(); wb; wb = GoToUpperItem()){
//		wb->PrintToStream();
		if (!wb->IsHidden() && wb->HasPoint(pt)){
			STRACE(("%s - SELECTED!\n", wb->GetName()));
			target	= wb;
			break;
		}
	}
	opLock.Unlock();
	return target;
}
//---------------------------------------------------------------------------
void Workspace::Invalidate(){
//TODO: *****!*!*!*!*!*!*!**!***REMOVE this! For Test purposes only!
/*	opLock.Lock();
	if(fOwner->ActiveWorkspace() == this)
		fOwner->FullInvalidate(fOwner->Bounds());
	opLock.Unlock();*/
//----------------
}
//---------------------------------------------------------------------------
void Workspace::InsertItem(ListData* item, ListData* before){
		// insert before one other item;
	if (before){
		if (before->upperItem)
			before->upperItem->lowerItem	= item;
		item->upperItem		= before->upperItem;
		before->upperItem	= item;
		item->lowerItem		= before;

			// if we're inserting at top of the stack, change it accordingly.
		if(fTopItem == before)
			fTopItem = item;
	}
		// insert item at bottom.
	else{
		item->upperItem	= fBottomItem;
		if (fBottomItem)
			fBottomItem->lowerItem	= item;

		fBottomItem		= item;

		if (!fTopItem)
			fTopItem	= item;
	}
}
//---------------------------------------------------------------------------
void Workspace::RemoveItem(ListData* item){
	if (!item)
		return;

	if (fBottomItem == item){
		fBottomItem = item->upperItem;
	}
	else{
		item->lowerItem->upperItem	= item->upperItem;
	}
	if (fTopItem == item){
		fTopItem = item->lowerItem;
	}
	else{
		item->upperItem->lowerItem	= item->lowerItem;
	}

		// set all these to NULL to avoid confusion later.

	item->upperItem	= NULL;
	item->lowerItem	= NULL;
	
	if (fFocusItem == item){
		fFocusItem->layerPtr->HighlightDecorator(false);
		fFocusItem = NULL;
	}
	if (fFrontItem == item)
		fFrontItem = NULL;
	if (fCurrentItem == item)
		fCurrentItem = NULL;
}
//---------------------------------------------------------------------------
ListData* Workspace::HasItem(ListData* item){
	for (ListData *itemX = fBottomItem; itemX != NULL; itemX = itemX->upperItem){
		if (item == itemX){
			return itemX;
		}
	}
	return NULL;
}
//---------------------------------------------------------------------------
ListData* Workspace::HasItem(WinBorder* layer){
	for (ListData *item = fBottomItem; item != NULL; item = item->upperItem){
		if (item->layerPtr == layer){
			return item;
		}
	}
	return NULL;
}
/* This, also is a "smart" method. Firstly this funtionality was in
 *	SearchAndSetNewFront, but I've split it for reasons of clarity. Anyway,
 *	what is to be noticed is that those 2 methods work hand-in-hand.
 * What FindPlace() litelaly does, is *place* the 'pref' item into its
 *	>right< place! If 'pref->layerPtr'(the WinBorder in ListData structure)
 *	does not meet some conditions a search for another, valid, WinBorder is
 *	being made.
 *
 * NOTE: Do NOT be confussed! This method places the preferred window *ONLY*!
 *	Other windows that need to be placed before it, *WILL* be placed by almost
 *	all code found in SearchAndSetNewFront
 */
//---------------------------------------------------------------------------
ListData* Workspace::FindPlace(ListData* pref){
		// if we received a NULL value, we stil have to give 'front' state to some window...
	if (!pref)
		pref	= HasItem(fBottomItem);
	else
		pref	= HasItem(pref);

	ListData	*item = NULL;

		// search a window that is not hidden and does *not* have B_AVOID_FRONT flag.
	item		= pref;
	while(pref && item->lowerItem != pref && (pref->upperItem || pref->lowerItem)){
		if ( !(item->layerPtr->Window()->Flags() & B_AVOID_FRONT) && !(item->layerPtr->IsHidden()) )
			break;
		STRACE(("item: %s - pref: %s\n", item->layerPtr->GetName(), pref->layerPtr->GetName()));
		if (item == fTopItem)
			item = fBottomItem;
		else
			item = item->upperItem;
	}

		// if true, it means we have windows with B_AVOID_FRONT flag *only*,
		// or they are all hidden. Pick one, if you can.
	if (pref && item->lowerItem == pref){
		for (item = fBottomItem; item; item = item->upperItem){
			if ( !(item->layerPtr->IsHidden()) )
				break;
		}
	}

		// if the search took place or not...
	pref = item;

		// we have exhausted all possibilities to make pref a vaid pointer... so... exit now.
	if (!pref)
		return NULL;

		// temporally remove 'pref' to reinsert it later in the *right* place
	RemoveItem(pref);

		// we start searching its place
	ListData		*cursor	= fBottomItem;
	int32			feel	= pref->layerPtr->Window()->Feel();
	
	switch(feel){
		case B_NORMAL_WINDOW_FEEL:{
			while(cursor &&	cursor->layerPtr->fLevel > B_MODAL_APP_FEEL){
				cursor	= cursor->upperItem;
			}
			InsertItem(pref, cursor? cursor->lowerItem: NULL);
			break;
		}
		
		case B_SYSTEM_LAST:{
			while(cursor &&	cursor->layerPtr->fLevel > pref->layerPtr->fLevel){
				cursor	= cursor->upperItem;
			}
			InsertItem(pref, cursor? cursor->lowerItem: fTopItem);
			break;
		}
		
		case B_SYSTEM_FIRST:
		case B_FLOATING_ALL_WINDOW_FEEL:
		case B_MODAL_ALL_WINDOW_FEEL:
		case B_MODAL_APP_WINDOW_FEEL:{
			while(cursor &&	cursor->layerPtr->fLevel > pref->layerPtr->fLevel){
				cursor	= cursor->upperItem;
			}
			InsertItem(pref, cursor? cursor->lowerItem: NULL);
			break;
		}

			// place in front of: its main window, other subset windows and in front
			// of other application's floating windows.
			// NOTE that this happens only if its main window is the front most one.
		case B_FLOATING_SUBSET_WINDOW_FEEL:{
			for(cursor = fBottomItem; cursor; cursor = cursor->upperItem){
				if (cursor->layerPtr->fLevel <= pref->layerPtr->fLevel
					&& (cursor->layerPtr == pref->layerPtr->MainWinBorder()
						|| cursor->layerPtr->MainWinBorder() == pref->layerPtr->MainWinBorder())
					)
				{ break; }
				else if(pref->layerPtr->fLevel == B_FLOATING_SUBSET_FEEL
						&& cursor->layerPtr->fLevel == B_FLOATING_APP_FEEL)
				{ break; }
			}
			if (cursor)
				InsertItem(pref, cursor? cursor->lowerItem: NULL);
			break;
		}

			// place this SUBSET_MODAL behind APP_MODAL ones if they belong
			// to the same application OR, in front of all MODAL/NORMAL if
			// it belongs to another application
		case B_MODAL_SUBSET_WINDOW_FEEL:{
			for(cursor = fBottomItem; cursor; cursor = cursor->upperItem){
				if (cursor->layerPtr->fLevel <= pref->layerPtr->fLevel)
					break;
				else if(pref->layerPtr->fLevel == B_MODAL_SUBSET_FEEL
					&& cursor->layerPtr->fLevel == B_MODAL_APP_FEEL
					&& pref->layerPtr->Window()->ClientTeamID()
						!=  cursor->layerPtr->Window()->ClientTeamID())
					break;
			}
			if (cursor)
				InsertItem(pref, cursor? cursor->lowerItem: NULL);
			break;
		}

			// place in front of: its main window, other floating windows
			// NOTE that this happens only if its main window is the front most one.
		case B_FLOATING_APP_WINDOW_FEEL:{
			for(cursor = fBottomItem; cursor; cursor = cursor->upperItem){
				if (cursor->layerPtr->fLevel <= pref->layerPtr->fLevel
					&& pref->layerPtr->Window()->ClientTeamID() ==
						cursor->layerPtr->Window()->ClientTeamID())
				{ break; }
			}

			if (cursor)
				InsertItem(pref, cursor? cursor->lowerItem: NULL);
			break;
		}
	}
	
	return pref;
}
/* This method is the key to correct window arangement!!!
 * With the help of FindPlace it correctly aranges windows. FindPlace, only
 *	inserted the window in the correct place, this method also brings all
 *	other windows that are supposed to be in front of her - I'm talking here
 *	about floating and modal windows.
 * It also clerverly selects the new FINAL front window.
 */
//---------------------------------------------------------------------------
void Workspace::SearchAndSetNewFront(WinBorder* preferred){
STRACE(("*WS(%ld)::SASNF(%s)\n", ID(), preferred? preferred->GetName(): "NULL"));
opLock.Lock();
		// the new front must not be the same as the previous one.
	if (fFrontItem && fFrontItem->layerPtr == preferred && !(preferred->IsHidden())){
STRACE(("-WS(%ld)::SASNF(%s) - opperation not needed! Workspace data:",
		ID(), preferred? preferred->GetName(): "NULL"));
STRACESTREAM();
STRACE(("#WS(%ld)::SASNF(%s) ENDED 1\n", ID(), preferred? preferred->GetName(): "NULL"));
		opLock.Unlock();
		return;
	}

		// properly place this 'preferred' WinBorder.
	ListData		*lastInserted;
	lastInserted	= FindPlace(HasItem(preferred));
	preferred		= lastInserted? lastInserted->layerPtr: NULL;
STRACE(("-WS(%ld)::SASNF(%s) - after FindPlace...", ID(), preferred? preferred->GetName(): "NULL"));
STRACESTREAM();

		// if the new front layer is the same... there is no point continuing
	if(fFrontItem == lastInserted){
STRACE(("-WS(%ld)::SASNF(%s) - new front layer is the same as the old one. Stop!", ID(), preferred? preferred->GetName(): "NULL"));
STRACESTREAM();
STRACE(("#WS(%ld)::SASNF(%s) ENDED 2\n", ID(), preferred? preferred->GetName(): "NULL"));
		opLock.Unlock();
		return;
	}

if(!lastInserted){
	STRACE(("PAAAAANIC: Workspace::SASNF(): 'lastInserted' IS NULL\n"));	
}
else{
	STRACE(("\n&&&&Processing for: %s\n", lastInserted->layerPtr->GetName()));
}

	if (lastInserted && !(lastInserted->layerPtr->IsHidden())){
		int32		prefFeel = preferred->Window()->Feel();

		if (prefFeel == B_NORMAL_WINDOW_FEEL){
STRACE((" NORMAL Window '%s' -", preferred? preferred->GetName(): "NULL"));
			if (fFrontItem){
				// if they are in the same team...
			if (preferred->Window()->ClientTeamID() == fFrontItem->layerPtr->Window()->ClientTeamID()){
STRACE((" SAME TeamID\n"));
					// collect subset windows that are common to application's windows...
					// NOTE: A subset window *can* be added to more than just one window.
				FMWList	commonFMW;
				FMWList	appFMW;
				FMWList	finalFMWList;
				int32	count, i;
					// collect floating and modal windows spread across the workspace only if
					// they are in our window's subset
					// * also remove them, for repositioning, later.
				ListData	*listItem = fTopItem;
				while(listItem){
					int32	feel = listItem->layerPtr->Window()->Feel();
					if (feel == B_FLOATING_SUBSET_WINDOW_FEEL || feel == B_MODAL_SUBSET_WINDOW_FEEL){
						if(preferred->Window()->fWinFMWList.HasItem(listItem->layerPtr)){
							commonFMW.AddItem(listItem->layerPtr);
								// *carefully* remove the item from the list
							ListData	*item = listItem;
							listItem	= listItem->lowerItem;
							RemoveItem(item);
						}
							// also remove floating windows. Those, SURELY belong to
							// 'fFrontItem' - this being the *old* front window.
						else if (feel == B_FLOATING_SUBSET_WINDOW_FEEL){
								// *carefully* remove the item from the list
							ListData	*item = listItem;
							listItem	= listItem->lowerItem;
							RemoveItem(item);
						}
						else{
							listItem	= listItem->lowerItem;
						}
					}
						// ALSO collect application's floating and modal windows,
						// for reinsertion, later.
					else if (feel == B_FLOATING_APP_WINDOW_FEEL || feel == B_MODAL_APP_WINDOW_FEEL){
						if(listItem->layerPtr->Window()->ClientTeamID() ==
							preferred->Window()->ClientTeamID())
						{
							appFMW.AddItem(listItem->layerPtr);
								// *carefully* remove the item from the list
							ListData	*item = listItem;
							listItem	= listItem->lowerItem;
							RemoveItem(item);
						}
						else
							listItem	= listItem->lowerItem;
					}
					else
						listItem	= listItem->lowerItem;
				}

					// put in the final list, items that are not found in the common list
				count	= preferred->Window()->fWinFMWList.CountItems();
				for (i=0; i<count; i++){
					void	*item = preferred->Window()->fWinFMWList.ItemAt(i);
					if (commonFMW.HasItem(item)){
					}
					else
						finalFMWList.AddItem(item);
				}

					// colapse the 2 lists
				finalFMWList.AddFMWList(&commonFMW);
				finalFMWList.AddFMWList(&appFMW);

					// insert windows found in 'finalFMWList' in Workspace's list.
					// IF *one* modal is found, do not add floating ones!
					
					// see if the last WinBorder in the list is a modal window.
					// OR if the last one in Workspace's list is a modal window...
				WinBorder	*wb = (WinBorder*)finalFMWList.LastItem();
				bool		lastIsModal = false;
				if (	(wb &&
						(wb->Window()->Feel() == B_MODAL_SUBSET_WINDOW_FEEL
							|| wb->Window()->Feel() == B_MODAL_APP_WINDOW_FEEL))
					||	(fBottomItem &&
						(fBottomItem->layerPtr->Window()->Feel() == B_MODAL_ALL_WINDOW_FEEL
							|| fBottomItem->layerPtr->Window()->Feel() == B_SYSTEM_FIRST))
					)
					{ lastIsModal = true; }

					// this variable will help us in deciding the new front WinBorder.
				WinBorder		*finalPreferred = preferred;
					// now insert items found in finalFMWList into Workspace
				count	= finalFMWList.CountItems();
				for(i=0; i<count; i++){
					WinBorder			*wb = (WinBorder*)finalFMWList.ItemAt(i);
					if(wb->Window()->Feel() == B_FLOATING_SUBSET_WINDOW_FEEL
							|| wb->Window()->Feel() == B_FLOATING_APP_WINDOW_FEEL)
					{
							// set its new MainWinBorder
						if(wb->Window()->Feel() == B_FLOATING_SUBSET_WINDOW_FEEL)
							wb->SetMainWinBorder(preferred);
							// don't add if the last WinBorder is a modal one.
						if (lastIsModal)
							continue;
					}
					else{
						if(wb->Window()->Feel() == B_MODAL_SUBSET_WINDOW_FEEL)
							wb->SetMainWinBorder(preferred);
							// if this modal window is not hidden, give it the front status.
						if (!(wb->IsHidden()))
							finalPreferred		= wb;
					}

						// insert item just after the last inserted one.
					ListData			*newItem = new ListData();
					newItem->layerPtr	= wb;
					newItem->lowerItem	= lastInserted->lowerItem;
					newItem->upperItem	= lastInserted;
					if (lastInserted->lowerItem)
						lastInserted->lowerItem->upperItem	= newItem;
					lastInserted->lowerItem		= newItem;
					
					if(lastInserted == fBottomItem)
						fBottomItem = newItem;

					lastInserted		= newItem;
				}
				preferred		= finalPreferred;
			}
			else{
STRACE((" DIFERRENT TeamID\n"));
					// remove front window's floating(_SUBSET_/_APP_) windows, if any.
				ListData	*listItem = fFrontItem->lowerItem;
				while(listItem && (listItem->layerPtr->fLevel == B_FLOATING_SUBSET_FEEL
									|| listItem->layerPtr->fLevel == B_FLOATING_APP_FEEL))
				{		// *carefully* remove the item from the list
					ListData	*item = listItem;
					listItem	= listItem->lowerItem;
					RemoveItem(item);
				}
					// now, jump to et1: to make a 'clean' selecting of
					// windows to be inserted
				fFrontItem = NULL;
				goto et1;
			} // END: else - if diferrent ClientTeamID()s.
			} // if (fFrontItem) && NORMAL_WINDOW
			else{
STRACE((" NO previous FRONT Item\n"));
				et1:
				FMWList		finalFMWList;
				int32		count, i;
				ListData	*listItem = fTopItem;
					// remove window's subset and application's *modal* windows spread
					// across Workspace's list, to be insert later, in the corect order.
				while(listItem){
					int32		feel = listItem->layerPtr->Window()->Feel();
					if ((feel == B_MODAL_SUBSET_WINDOW_FEEL
							&& preferred->Window()->fWinFMWList.HasItem(listItem->layerPtr))
						|| (feel == B_MODAL_APP_WINDOW_FEEL
							&& preferred->Window()->ClientTeamID() ==
								listItem->layerPtr->Window()->ClientTeamID()
							))
					{
							// *carefully* remove the item from the list
						ListData	*item = listItem;
						listItem	= listItem->lowerItem;
						RemoveItem(item);
					}
					else
						listItem	= listItem->lowerItem;
				}
					// add to the final list window's subset windows. (..._SUBSET_...)
				finalFMWList.AddFMWList(&(preferred->Window()->fWinFMWList));
					// also add application's ones. (..._APP_...)
				finalFMWList.AddFMWList(&(preferred->Window()->App()->fAppFMWList));
					// insert windows found in 'finalFMWList' in Workspace's list.
					// IF *one* modal is found, do not add floating ones!
					
					// see if the last WinBorder in the list is a modal window.
				WinBorder	*wb = (WinBorder*)finalFMWList.LastItem();
				bool		lastIsModal = false;
				if (	(wb &&
						(wb->Window()->Feel() == B_MODAL_SUBSET_WINDOW_FEEL
							|| wb->Window()->Feel() == B_MODAL_APP_WINDOW_FEEL))
					||	(fBottomItem &&
						(fBottomItem->layerPtr->Window()->Feel() == B_MODAL_ALL_WINDOW_FEEL
							|| fBottomItem->layerPtr->Window()->Feel() == B_SYSTEM_FIRST))
					)
					{ lastIsModal = true; }

					// this variable will help us in deciding the new front WinBorder.
				WinBorder		*finalPreferred = preferred;
					// now insert items found in finalFMWList into Workspace
				count	= finalFMWList.CountItems();
				for(i=0; i<count; i++){
					WinBorder			*wb = (WinBorder*)finalFMWList.ItemAt(i);
						// do not insert floating windows if the last is a modal one.
					if(wb->Window()->Feel() == B_FLOATING_SUBSET_WINDOW_FEEL
							|| wb->Window()->Feel() == B_FLOATING_APP_WINDOW_FEEL)
					{
							// set its new MainWinBorder
						if(wb->Window()->Feel() == B_FLOATING_SUBSET_WINDOW_FEEL)
							wb->SetMainWinBorder(preferred);
							// don't add if the last WinBorder is a modal one.
						if (lastIsModal)
							continue;
					}
					else{
						if(wb->Window()->Feel() == B_MODAL_SUBSET_WINDOW_FEEL)
							wb->SetMainWinBorder(preferred);
							// if this modal window is not hidden, give it the front status.
						if (!(wb->IsHidden()))
							finalPreferred		= wb;
					}
						// insert item just after the last inserted one.
					ListData			*newItem = new ListData();
					newItem->layerPtr	= wb;
					newItem->lowerItem	= lastInserted->lowerItem;
					newItem->upperItem	= lastInserted;
					if (lastInserted->lowerItem)
						lastInserted->lowerItem->upperItem	= newItem;
					lastInserted->lowerItem		= newItem;
					
					if(lastInserted == fBottomItem)
						fBottomItem = newItem;

					lastInserted		= newItem;
				}
				preferred		= finalPreferred;
			} // else - if(fFrontItem);
		} // END: if _NORMAL_ window.
		else if(	prefFeel == B_FLOATING_SUBSET_WINDOW_FEEL
				 || prefFeel == B_FLOATING_APP_WINDOW_FEEL
				 || prefFeel == B_FLOATING_ALL_WINDOW_FEEL)
		{
STRACE((" FLOATING Window '%s'\n", preferred? preferred->GetName(): "NULL"));
			// Do nothing! FindPlace() was called and it has corectly placed our window
			// We don't have to do noting here.
		}
		else if(	prefFeel == B_MODAL_SUBSET_WINDOW_FEEL
				 || prefFeel == B_MODAL_APP_WINDOW_FEEL)
		{
STRACE((" MODAL_APP/SUBSET Window '%s'\n", preferred? preferred->GetName(): "NULL"));
			FMWList		finalMWList;
			int32		count = 0, i;
			int32		prefIndex = -1;

			if (fFrontItem && fFrontItem->layerPtr->fLevel == B_NORMAL_FEEL)
			{
					// remove front window's floating(_SUBSET_/_APP_) windows, if any.
				ListData	*listItem = fFrontItem->lowerItem;
				while(listItem && (listItem->layerPtr->fLevel == B_FLOATING_SUBSET_FEEL
									|| listItem->layerPtr->fLevel == B_FLOATING_APP_FEEL))
				{		// *carefully* remove the item from the list
					ListData	*item = listItem;
					listItem	= listItem->lowerItem;
					RemoveItem(item);
				}
			}

				// If this is a MODAL_SUBSET window, search it's place in its MainWinBorder
				//	window subset, and then take all modals after it; then continue talking
				//	modal windoes *only*, from application's set.
				// If it is a MODAL_APP only search an take windows in/from application's set
			if (prefFeel == B_MODAL_SUBSET_WINDOW_FEEL){
				prefIndex	= preferred->MainWinBorder()->Window()->fWinFMWList.IndexOf(preferred);
				count		= preferred->MainWinBorder()->Window()->fWinFMWList.CountItems();
				if (prefIndex >= 0){
					WinBorder	*wb = NULL;
						// add modal windows from its main window subset - windows that are
						// positioned after 'preferred'.
					FMWList		*listPtr = &(preferred->MainWinBorder()->Window()->fWinFMWList);
					for(i = prefIndex+1; i < count; i++){
							// they *all* are modal windows.
						wb		= (WinBorder*)listPtr->ItemAt(i);
						wb->SetMainWinBorder(preferred);
						finalMWList.AddItem(wb);
							// remove those windows(if in there), for correct re-insertion later.
						ListData	*ld = NULL;
						if((ld = HasItem(wb)))
							RemoveItem(ld);
					}
						// add modal windows that are found in application's subset
					count	= preferred->Window()->App()->fAppFMWList.CountItems();
					listPtr	= &(preferred->Window()->App()->fAppFMWList);
					for(i = 0; i < count; i++){
						wb		= (WinBorder*)listPtr->ItemAt(i);
						if (wb->Window()->Feel() == B_MODAL_APP_WINDOW_FEEL){
							finalMWList.AddItem(wb);
								// remove those windows(if in there), for correct re-insertion later.
							ListData	*ld = NULL;
							if((ld = HasItem(wb)))
								RemoveItem(ld);
						}
					}
				}
			}
			else if(prefFeel == B_MODAL_APP_WINDOW_FEEL){
				prefIndex	= preferred->Window()->App()->fAppFMWList.IndexOf(preferred);
				count		= preferred->Window()->App()->fAppFMWList.CountItems();
				if (prefIndex >= 0){
					WinBorder	*wb = NULL;
						// add modal windows from application's subset - windows that are
						// positioned after 'preferred'.
					FMWList		*listPtr = &(preferred->Window()->App()->fAppFMWList);
					for(i = prefIndex+1; i < count; i++){
						wb		= (WinBorder*)listPtr->ItemAt(i);
							// they *all* are modal windows.
						finalMWList.AddItem(wb);
								// remove those windows(if in there), for correct re-insertion later.
						ListData	*ld = NULL;
						if((ld = HasItem(wb)))
							RemoveItem(ld);
					}
				}
			}
		
				// insert windows found in 'finalMWList' in Workspace's list, after "preferred"
			count		= finalMWList.CountItems();
				// this varable will help us designate the future front.
			WinBorder	*finalPreferred = preferred;
			for(i=0; i<count; i++){
				WinBorder			*wb = (WinBorder*)finalMWList.ItemAt(i);
					// wb is surely a modal so... ATM make it the front one.
				if(!(wb->IsHidden()))
					finalPreferred	= wb;

					// insert item just after the last inserted one.
				ListData			*newItem = new ListData();
				newItem->layerPtr	= wb;
				newItem->lowerItem	= lastInserted->lowerItem;
				newItem->upperItem	= lastInserted;
				if (lastInserted->lowerItem)
					lastInserted->lowerItem->upperItem	= newItem;
				lastInserted->lowerItem		= newItem;
				
				if(lastInserted == fBottomItem){
					fBottomItem = newItem;
				}

				lastInserted		= newItem;
			}
			preferred = finalPreferred;
		}
		else if(prefFeel == B_MODAL_ALL_WINDOW_FEEL
				|| prefFeel == B_SYSTEM_FIRST)
		{
STRACE((" MODAL ALL/SYSTEM FIRST Window '%s'\n", preferred? preferred->GetName(): "NULL"));
				// remove all application's floating windows.
			if (fFrontItem && fFrontItem->layerPtr->fLevel == B_NORMAL_FEEL)
			{
				ListData	*listItem = fFrontItem->lowerItem;
				while(listItem &&
						(listItem->layerPtr->fLevel == B_FLOATING_SUBSET_FEEL
						|| listItem->layerPtr->fLevel == B_FLOATING_APP_FEEL))
				{
						// *carefully* remove the item from the list
					ListData	*item = listItem;
					listItem		= listItem->lowerItem;
					RemoveItem(item);
				}
			}
		}
		else if(prefFeel == B_SYSTEM_LAST){
			; // Do Nothing.
		}
		else{
			// We ***should NOT*** reach this point!
			STRACE(("SERVER: PANIC: \"%s\": What kind of window is this???\n", preferred? preferred->GetName(): "NULL"));
		}
	}
	else
		STRACE((" The window IS Hidden\n"));

	ListData	*exFrontItem	= fFrontItem;
	ListData	*newFrontItem	= NULL;
	if(preferred && fBottomItem){
		int32		feel = fBottomItem->layerPtr->Window()->Feel();

			// if preferred is one of these *don't* give front state to it!
		if(preferred->fLevel == B_FLOATING_SUBSET_FEEL
			|| preferred->fLevel == B_FLOATING_APP_FEEL
			|| preferred->fLevel == B_FLOATING_ALL_FEEL)
		{
			newFrontItem	= exFrontItem;
		}
			// if the last in workspace's list is one of these, GIVE focus to it!
		else if((feel == B_SYSTEM_FIRST || feel == B_MODAL_ALL_WINDOW_FEEL)
				&& !(fBottomItem->layerPtr->IsHidden()) )
		{
			newFrontItem	= fBottomItem;
		}
			// the SYSTEM_LAST will get the front status, only if it's the only
			// WinBorder in this workspace.
		else if (preferred->fLevel == B_SYSTEM_LAST
				 && !(preferred->IsHidden()) )
		{
			if(fBottomItem->layerPtr == preferred)
				newFrontItem	= HasItem(preferred);
		}
			// this is in case of MODAL[APP/SUBSET] and NORMAL windows
		else if ( !(preferred->IsHidden()) ){
			newFrontItem	= HasItem(preferred);
		}
		else{
			newFrontItem	= NULL;
		}
	}
	else{ newFrontItem	= NULL; }
	
	if (fFrontItem != newFrontItem){
		fFrontItem = newFrontItem;
// TODO: call a method like... WinBorder::MakeFront(true);
	}
opLock.Unlock();
STRACE(("#WS(%ld)::SASNF(%s) ENDED! Workspace data...", ID(), preferred? preferred->GetName(): "NULL"));
STRACESTREAM();
}
/* This method performs a simple opperation. It searched the new focus window
 *	by walking from front to back, cheking for some things, until all variables
 *	correspond.
 */
//---------------------------------------------------------------------------
void Workspace::SearchAndSetNewFocus(WinBorder* preferred){
STRACE(("*WS(%ld)::SASNFocus(%s)\n", ID(), preferred? preferred->GetName(): "NULL"));
	opLock.Lock();

	if(!preferred)
		preferred	= fBottomItem? fBottomItem->layerPtr : NULL;

	bool		selectOthers = false;
	ListData	*item = NULL;

	for(item = fBottomItem; item != NULL; item = item->upperItem){
			// if this WinBorder doesn't want to have focus... get to the next one
		if (item->layerPtr->Window()->Flags() & B_AVOID_FOCUS)
			continue;

		if (preferred && item->layerPtr == preferred){
				// our preffered one is hidden so... select another one
			if (preferred && preferred->IsHidden()){
				selectOthers = true;
				continue;
			}
			else{
				break;
			}
		}

		if (item->layerPtr->fLevel == B_SYSTEM_FIRST || item->layerPtr->fLevel == B_MODAL_ALL_FEEL)
		{
			break;
		}

		if (item->layerPtr->fLevel == B_MODAL_APP_FEEL
			 && (preferred && preferred->Window()->ClientTeamID() == item->layerPtr->Window()->ClientTeamID()))
		{
			break;
		}

		if (item->layerPtr->fLevel == B_MODAL_SUBSET_FEEL
			&& (preferred && item->layerPtr->MainWinBorder() == preferred))
		{
			break;
		}

			// select one window, other than a system_last one!
		if (selectOthers && item->layerPtr->fLevel != B_SYSTEM_LAST){
			break;
		}
	}

		// there are no windows below us to select. take the one from above us.
	if(selectOthers && !item){
			// there HAS to be valid
		item 	= HasItem(preferred);
		if (item)
			item= item->lowerItem;
	}

		// there are NO windows in front of us, and no regular window below us
		// maybe there is a system_last window...
	if(!item){
		item = fTopItem;
	}
	
	if (item != fFocusItem){
		// TODO: item->ColorTabInGrey & send message to client
		// TODO: item->ColorTabInYellow & send message to client
		// TODO: Rebuild & Redraw.
		fFocusItem		= item;
	}
	
	opLock.Unlock();
}
//---------------------------------------------------------------------------
void Workspace::BringToFrontANormalWindow(WinBorder* layer){
	switch (layer->Window()->Feel()){
		case B_FLOATING_SUBSET_WINDOW_FEEL:
		case B_MODAL_SUBSET_WINDOW_FEEL:{
			SearchAndSetNewFront(layer->MainWinBorder());
			break;
		}
		case B_FLOATING_APP_WINDOW_FEEL:
		case B_MODAL_APP_WINDOW_FEEL:{
			opLock.Lock();
			ListData	*item	= fBottomItem;
			team_id		tid		= layer->Window()->ClientTeamID();
			while(item){
				if(item->layerPtr->Window()->ClientTeamID() == tid
					&& item->layerPtr->Window()->Feel() == B_NORMAL_WINDOW_FEEL)
				{
					break;
				}
				item	= item->upperItem;
			}
			if(item){
				SearchAndSetNewFront(item->layerPtr);
			}
			opLock.Unlock();
			break;
		}
		default:{
			// in case of MODAL/FLOATING_ALL or _NORMAL_ or SYSTEM_FIRST/LAST do nothing!
		}
	}
}
/* The method moves a window to the back of its subset.
 */
//---------------------------------------------------------------------------
void Workspace::MoveToBack(WinBorder* newLast){
STRACE(("\n!MoveToBack(%s) -", newLast? newLast->GetName(): "NULL"));
		// does the list have this element?
	ListData	*item = HasItem(newLast);
	if(item){
		ListData	*listItem = NULL;

		switch(item->layerPtr->fLevel){
			case B_FLOATING_ALL_FEEL:{
STRACE((" B_FLOATING_ALL_FEEL window\n"));
					// search the place where we should insert it later
				listItem		= item->upperItem;
				while(listItem && (listItem->layerPtr->fLevel == item->layerPtr->fLevel))
					listItem = listItem->upperItem;

				break;
			}

				// those 2 flags form a 'virtual' set, so even if FLOATING_APP
				// has a greater priority than FLOATING_SUBSET, it is still moved
				// behind FLOATING_SUBSET.
			case B_FLOATING_SUBSET_FEEL:
			case B_FLOATING_APP_FEEL:{
STRACE((" B_FLOATING_SUBSET_FEEL/B_FLOATING_APP_FEEL window\n"));
					// search the place where we should insert it later
				listItem		= item->upperItem;
				while(listItem &&
						(listItem->layerPtr->fLevel == B_FLOATING_SUBSET_FEEL
						|| listItem->layerPtr->fLevel == B_FLOATING_APP_FEEL))
					listItem = listItem->upperItem;

				break;
			}

				// the same like with floating window. Those 2 and NORMAL form a'virtual' set.
			case B_MODAL_SUBSET_FEEL:
			case B_MODAL_APP_FEEL:{
STRACE((" B_MODAL_SUBSET_FEEL/B_MODAL_APP_FEEL window\n"));
					// search the place where we should insert it later
				listItem		= item->upperItem;
				while(listItem &&
						(listItem->layerPtr->fLevel == B_MODAL_SUBSET_FEEL
						|| listItem->layerPtr->fLevel == B_MODAL_APP_FEEL
						|| listItem->layerPtr->fLevel == B_NORMAL_FEEL))
					listItem = listItem->upperItem;

				break;
			}

				// here is the simplest, we use the priority levels.
			case B_NORMAL_FEEL:{
STRACE((" B_NORMAL_FEEL window\n"));
				listItem		= item->upperItem;
				while(listItem && (listItem->layerPtr->fLevel >= item->layerPtr->fLevel)){
					listItem = listItem->upperItem;
				}
												
				break;
			}
		}
		
		if ((listItem && item->lowerItem == listItem->lowerItem)
			|| (listItem == NULL && item == fTopItem))
		{
			// do nothing! The window will be in the same position it is now.
		}
		else{
				// if it's a normal window, first remove any floating windows,
				// it or its application has
			if(newLast->fLevel == B_NORMAL_FEEL && fFrontItem->layerPtr == newLast){
				ListData	*iter = item->lowerItem;
				while(iter &&
						(iter->layerPtr->fLevel == B_FLOATING_SUBSET_FEEL
						|| iter->layerPtr->fLevel == B_FLOATING_APP_FEEL))
				{
						// *carefully* remove the item from the list
					ListData	*itemX = iter;
					iter		= iter->lowerItem;
					RemoveItem(itemX);
				}

			}

			WinBorder	*nextPreferred = item->upperItem? item->upperItem->layerPtr: NULL;
			bool		wasFront = fFrontItem->layerPtr == newLast;
			bool		wasFocus = fFocusItem->layerPtr == newLast;
				// remove item
			RemoveItem(item);
				// insert in the new, right' position.
			InsertItem(item, listItem? listItem->lowerItem: fTopItem);

			if (wasFront)
				SearchAndSetNewFront(nextPreferred);
			if (wasFocus)
				SearchAndSetNewFocus(nextPreferred);
		}
STRACESTREAM();
	}
	else
		STRACE((" this operation was not needed\n"));
}
//---------------------------------------------------------------------------
void Workspace::SetLocalSpace(const uint32 colorspace){
	fSpace		= colorspace;
}
//---------------------------------------------------------------------------
uint32 Workspace::LocalSpace() const{
	return fSpace;
}
//---------------------------------------------------------------------------
void Workspace::SetBGColor(const RGBColor &c){
	fBGColor	= c;
}
//---------------------------------------------------------------------------
RGBColor Workspace::BGColor(void) const{
	return fBGColor;
}
//---------------------------------------------------------------------------
// Debug methods!!!
//---------------------------------------------------------------------------
void Workspace::PrintToStream() const{
	printf("\nWorkspace %ld hierarchy shown from back to front:\n", fID);
	for (ListData *item = fTopItem; item != NULL; item = item->lowerItem){
		WinBorder		*wb = (WinBorder*)item->layerPtr;
		printf("\tName: %s\t%s", wb->GetName(), wb->IsHidden()?"Hidden\t": "NOT Hidden");
		if (wb->Window()->Feel() == B_FLOATING_SUBSET_WINDOW_FEEL)
			printf("\t%s\n", "B_FLOATING_SUBSET_WINDOW_FEEL");
		if (wb->Window()->Feel() == B_FLOATING_APP_WINDOW_FEEL)
			printf("\t%s\n", "B_FLOATING_APP_WINDOW_FEEL");
		if (wb->Window()->Feel() == B_FLOATING_ALL_WINDOW_FEEL)
			printf("\t%s\n", "B_FLOATING_ALL_WINDOW_FEEL");
		if (wb->Window()->Feel() == B_MODAL_SUBSET_WINDOW_FEEL)
			printf("\t%s\n", "B_MODAL_SUBSET_WINDOW_FEEL");
		if (wb->Window()->Feel() == B_MODAL_APP_WINDOW_FEEL)
			printf("\t%s\n", "B_MODAL_APP_WINDOW_FEEL");
		if (wb->Window()->Feel() == B_MODAL_ALL_WINDOW_FEEL)
			printf("\t%s\n", "B_MODAL_ALL_WINDOW_FEEL");
		if (wb->Window()->Feel() == B_NORMAL_WINDOW_FEEL)
			printf("\t%s\n", "B_NORMAL_WINDOW_FEEL");
		if (wb->Window()->Feel() == B_SYSTEM_LAST)
			printf("\t%s\n", "B_SYSTEM_LAST");
		if (wb->Window()->Feel() == B_SYSTEM_FIRST)
			printf("\t%s\n", "B_SYSTEM_FIRST");
	}
	printf("Focus Layer:\t%s\n", fFocusItem? fFocusItem->layerPtr->GetName(): "NULL");
	printf("Front Layer:\t%s\n", fFrontItem? fFrontItem->layerPtr->GetName(): "NULL");
//	printf("Current Layer:\t%s\n", fCurrentItem? fCurrentItem->layerPtr->GetName(): "NULL");
//	printf("Top Layer:\t%s\n", fTopItem? fTopItem->layerPtr->GetName(): "NULL");
//	printf("Bottom Layer:\t%s\n", fBottomItem? fBottomItem->layerPtr->GetName(): "NULL");
}
//---------------------------------------------------------------------------
void Workspace::PrintItem(ListData *item) const{
	printf("ListData members:\n");
	if (item){
		printf("WinBorder:\t%s\n", item->layerPtr? item->layerPtr->GetName(): "NULL");
		printf("UpperItem:\t%s\n", item->upperItem? item->upperItem->layerPtr->GetName(): "NULL");
		printf("LowerItem:\t%s\n", item->lowerItem? item->lowerItem->layerPtr->GetName(): "NULL");
	}
	else{
		printf("NULL item!!!\n");
	}
}
