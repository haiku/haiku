//------------------------------------------------------------------------------
//	Copyright (c) 2001-2003, OpenBeOS
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
//	File Name:		RootLayer.h
//	Author:			Gabe Yoder <gyoder@stny.rr.com>
//					DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Class used for the top layer of each workspace's Layer tree
//  
//------------------------------------------------------------------------------
#ifndef _ROOTLAYER_H_
#define _ROOTLAYER_H_

#include <List.h>
#include <Locker.h>

#include "Layer.h"
#include "FMWList.h"

class RGBColor;
class Workspace;
class Screen;
class WinBorder;
class Desktop;
class DisplayDriver;

/*!
	\class RootLayer RootLayer.h
	\brief Class used for the top layer of each workspace's Layer tree
	
	RootLayers are used to head up the top of each Layer tree and reimplement certain 
	Layer functions to act accordingly. There is only one for each workspace class.
	
*/
class RootLayer : public Layer
{
public:
	RootLayer(const char *name, int32 workspaceCount, Desktop *desktop, 
			DisplayDriver *driver);
	virtual	~RootLayer(void);
	
	virtual	void MoveBy(float x, float y);
	virtual	void ResizeBy(float x, float y);
	
	// For the active workspaces
	virtual	Layer *VirtualTopChild(void) const;
	virtual	Layer *VirtualLowerSibling(void) const;
	virtual	Layer *VirtualUpperSibling(void) const;
	virtual	Layer *VirtualBottomChild(void) const;

	void ReadWorkspaceData(const char *path);
	void SaveWorkspaceData(const char *path);
	
	void AddWinBorder(WinBorder *winBorder);
	void RemoveWinBorder(WinBorder *winBorder);
	void ChangeWorkspacesFor(WinBorder *winBorder, uint32 newWorkspaces);
	bool SetFrontWinBorder(WinBorder *winBorder);
	
	void SetScreens(Screen *screen[], int32 rows, int32 columns);
	Screen **Screens(void);
	bool SetScreenResolution(int32 width, int32 height, uint32 colorspace);
	int32 ScreenRows(void) const { return fRows; }
	int32 ScreenColumns(void) const { return fColumns; }
	
	void SetWorkspaceCount(const int32 count);
	int32 WorkspaceCount(void) const;
	Workspace *WorkspaceAt(const int32 index) const;
	void SetActiveWorkspaceByIndex(const int32 index);
	void SetActiveWorkspace(Workspace *ws);
	int32 ActiveWorkspaceIndex(void) const;
	Workspace *ActiveWorkspace(void) const;
	
	void SetBGColor(const RGBColor &col);
	RGBColor BGColor(void) const;
	
	void AddWinBorderToWorkspaces(WinBorder *winBorder, uint32 wks);
	
	void PrintToStream(void);
	
	// "Private" to app_server :-) - they should not be used
	void RemoveAppWindow(WinBorder *wb);
	
	FMWList fMainFMWList;
	BLocker fMainLock;

private:
	Desktop *fDesktop;
	
	BList fScreenPtrList;
	int32 fRows;
	int32 fColumns;
	int32 fScreenXResolution;
	int32 fScreenYResolution;
	uint32 fColorSpace;
	
	BList fWorkspaceList;
	Workspace *fActiveWorkspace;
};

#endif
