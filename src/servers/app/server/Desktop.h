//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
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
//	File Name:		Desktop.cpp
//	Author:			Adi Oanca <adioanca@cotty.iren.ro>
//	Description:	Class used to encapsulate desktop management
//
//------------------------------------------------------------------------------
#ifndef _DESKTOP_H_
#define _DESKTOP_H_

#include <Locker.h>
#include <List.h>
#include <Menu.h>
#include <InterfaceDefs.h>

class RootLayer;
class Screen;
class Layer;
class BMessage;
class WinBorder;
class DisplayDriver;
class BPortLink;

class Desktop
{
public:
	// startup methods
	Desktop(void);
	virtual ~Desktop(void);
	void Init(void);

	// 1-BigScreen or n-SmallScreens
	void InitMode(void);
	
	// Methods for multiple monitors.
	Screen *ScreenAt(int32 index) const;
	int32 ScreenCount(void) const;
	Screen *ActiveScreen(void) const;
	
	void SetActiveRootLayerByIndex(int32 listIndex);
	void SetActiveRootLayer(RootLayer *rl);
	RootLayer *RootLayerAt(int32 index);
	RootLayer *ActiveRootLayer(void) const;
	int32 ActiveRootLayerIndex(void) const;
	int32 CountRootLayers(void) const;
	DisplayDriver *GetDisplayDriver(void) const;
	
	// Methods for layer(WinBorder) manipulation.
	void AddWinBorder(WinBorder *winBorder);
	void RemoveWinBorder(WinBorder *winBorder);
	void AddWinBorderToSubset(WinBorder *winBorder, WinBorder *toWinBorder);
	void RemoveWinBorderFromSubset(WinBorder *winBorder, WinBorder *fromWinBorder);
	bool HasWinBorder(WinBorder *winBorder);

	WinBorder* FindWinBorderByServerWindowTokenAndTeamID(int32 token, team_id teamID);
	// get list of registed windows
	const BList& WindowList() const
	{
		if (!IsLocked())
			debugger("You must lock before getting registered windows list\n");
		return fWinBorderList;
	}

	// locking with regards to registered windows list
	bool Lock() { return fWinLock.Lock(); }
	void Unlock() { return fWinLock.Unlock(); }
	bool IsLocked() const { return fWinLock.IsLocked(); }

	// Methods for various desktop stuff handled by the server
	void SetScrollBarInfo(const scroll_bar_info &info);
	scroll_bar_info		ScrollBarInfo(void) const;
	
	void SetMenuInfo(const menu_info &info);
	menu_info MenuInfo(void) const;
	
	void UseFFMouse(const bool &useffm);
	bool FFMouseInUse(void) const;
	void SetFFMouseMode(const mode_mouse &value);
	mode_mouse FFMouseMode(void) const;
	
	// Debugging methods
	void PrintToStream(void);
	void PrintVisibleInRootLayerNo(int32 no);
	
private:
	void		AddDriver(DisplayDriver *driver);

	BList		fWinBorderList;
	BLocker		fWinLock;

	BList		fRootLayerList;
	RootLayer	*fActiveRootLayer;

	BList		fScreenList;
	Screen		*fActiveScreen;
	
	scroll_bar_info fScrollBarInfo;
	menu_info	fMenuInfo;
	mode_mouse	fMouseMode;
	bool		fFFMouseMode;
};

extern Desktop *desktop;

#endif // _DESKTOP_H_
