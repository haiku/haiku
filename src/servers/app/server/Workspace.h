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
#include <Accelerant.h>

#include "RGBColor.h"

class WinBorder;
class RBGColor;
class RootLayer;

struct ListData
{
	WinBorder *layerPtr;
	ListData *upperItem;
	ListData *lowerItem;
};

class Workspace
{
public:
	Workspace(const uint32 colorspace, int32 ID, const RGBColor& BGColor,
		RootLayer *owner);
	~Workspace(void);
	
	bool AddLayerPtr(WinBorder *layer);
	bool RemoveLayerPtr(WinBorder *layer);
	bool HideSubsetWindows(WinBorder *layer);
	WinBorder *FocusLayer(void) const;
	WinBorder *FrontLayer(void) const;
	
	void MoveToBack(WinBorder *newLast);
	
	// The bottom item is the one which is visible
	WinBorder *GoToBottomItem(void);
	WinBorder *GoToUpperItem(void);
	WinBorder *GoToTopItem(void);
	WinBorder *GoToLowerItem(void);
	bool GoToItem(WinBorder *layer);
	
	void SetLocalSpace(const uint32 colorspace);
	uint32 LocalSpace(void) const;
	
	void SetBGColor(const RGBColor &c);
	RGBColor BGColor(void) const;
	
	int32 ID(void) const { return fID; }
	
	void GetSettings(const BMessage &msg);
	void GetDefaultSettings(void);
	void PutSettings(BMessage *msg, const int32 &index) const;
	static void PutDefaultSettings(BMessage *msg, const int32 &index);
	
	// debug methods
	void PrintToStream(void) const;
	void PrintItem(ListData *item) const;
	
	// TODO: Bad Style. There should be a more elegant way of doing this
	// .... private :-) - do not use!
	void SearchAndSetNewFront(WinBorder *preferred);
	void SearchAndSetNewFocus(WinBorder *preferred);
	void BringToFrontANormalWindow(WinBorder *layer);
	
	ListData *HasItem(WinBorder *layer);
	
private:
	
	void InsertItem(ListData *item, ListData *before);
	void RemoveItem(ListData *item);
	ListData *HasItem(ListData *item);
	
	ListData *FindPlace(ListData *pref);
	
	int32 fID;
	uint32 fSpace;
	RGBColor fBGColor;
	
	BLocker fOpLock;
	
	RootLayer *fOwner;
	
	// first visible onscreen
	ListData *fBottomItem;
	
	 // the last visible(or covered by other Layers)
	ListData *fTopItem;
	
	 // pointer to the currect element in the list
	ListData *fCurrentItem;
	
	 // the focus WinBorder - for keyboard events
	ListData *fFocusItem;
	
	 // the one the mouse can bring in front as possible(in its set)
	ListData *fFrontItem;
	
	// settings for each workspace -- example taken from R5's app_server_settings file
	display_timing fDisplayTiming;
	int16 fVirtualWidth;
	int16 fVirtualHeight;
	
	// TODO: find out what specific values need to be contained in fFlags as per R5's server
	// not to be confused with display_timing.flags
	uint32 fFlags;
};

#endif
