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
//	File Name:		RootLayer.cpp
//	Author:			Gabe Yoder <gyoder@stny.rr.com>
//					DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@myrealbox.com>
//	Description:	Class used for the top layer of each workspace's Layer tree
//  
//------------------------------------------------------------------------------

#include <stdio.h>
#include <Window.h>
#include <List.h>

#include "Globals.h"
#include "RootLayer.h"
#include "Layer.h"
#include "Workspace.h"
#include "ServerScreen.h"
#include "WinBorder.h"
#include "ServerWindow.h"
#include "ServerApp.h"
#include "Desktop.h"
#include "FMWList.h"
#include "DisplayDriver.h"

//#define DEBUG_ROOTLAYER

#ifdef DEBUG_ROOTLAYER
	#define STRACE(a) printf(a)
#else
	#define STRACE(a) /* nothing */
#endif

//#define DISPLAYDRIVER_TEST_HACK


RootLayer::RootLayer(const char *name, int32 workspaceCount, 
		Desktop *desktop, DisplayDriver *driver)
 : Layer(BRect(0,0,0,0), name, 0, B_FOLLOW_ALL, B_WILL_DRAW, driver)
{
	fDesktop = desktop;
	
	//NOTE: be careful about this one.
	fRootLayer = this;
	fActiveWorkspace = NULL;
	fRows = 0;
	fColumns = 0;
	
	// TODO: read these 3 from a configuration file.
	fScreenXResolution = 0;
	fScreenYResolution = 0;
	fColorSpace = B_RGB32;
	
	fViewToken = 0; // is this used for WinBorders?
	fHidden	= false;
	
	SetWorkspaceCount(workspaceCount);
}

RootLayer::~RootLayer()
{
	// RootLayer object just uses Screen objects, it is not allowed to delete them.
}

void RootLayer::Draw(const BRect &r)
{
	// REMOVEME: This method is here because of DisplayDriver testing ONLY. 
	// It should be removed when testing is finished

	STRACE(("*RootLayer(%s)::Draw(r)\n", GetName()));
	
	RGBColor		c(51,102,152);
	fDriver->FillRect(r, c);
	
	STRACE(("#RootLayer(%s)::Draw(r) END\n", GetName()));
	
#ifdef DISPLAYDRIVER_TEST_HACK
	DisplayDriver	*_driver = fDriver;
	int8 pattern[8];
	int8 pattern2[8];
	memset(pattern,255,8);
	memset(pattern2,128+64+32+16,8);
	BRect r1(100,100,1500,1100);
	BPoint pts[4];
	pts[0].x = 200;
	pts[0].y = 200;
	pts[1].x = 400;
	pts[1].y = 1000;
	pts[2].x = 600;
	pts[2].y = 400;
	pts[3].x = 1200;
	pts[3].y = 800;
	BPoint triangle[3];
	BRect triangleRect(100,100,400,300);
	triangle[0].x = 100;
	triangle[0].y = 100;
	triangle[1].x = 100;
	triangle[1].y = 300;
	triangle[2].x = 400;
	triangle[2].y = 300;
	BPoint polygon[6];
	BRect polygonRect(100,100,300,400);
	polygon[0].x = 100;
	polygon[0].y = 100;
	polygon[1].x = 100;
	polygon[1].y = 400;
	polygon[2].x = 200;
	polygon[2].y = 300;
	polygon[3].x = 300;
	polygon[3].y = 400;
	polygon[4].x = 300;
	polygon[4].y = 100;
	polygon[5].x = 200;
	polygon[5].y = 200;
	
	fLayerData->highcolor.SetColor(255,0,0,255);
	fLayerData->lowcolor.SetColor(255,255,255,255);
	_driver->FillRect(r1,fLayerData,pattern);
	
	fLayerData->highcolor.SetColor(255,255,0,255);
	_driver->StrokeLine(BPoint(100,100),BPoint(1500,1100),fLayerData,pattern);
	
	fLayerData->highcolor.SetColor(0,0,255,255);
	_driver->StrokeBezier(pts,fLayerData,pattern);
	_driver->StrokeArc(BRect(200,300,400,600),30,270,fLayerData,pattern);
	_driver->StrokeEllipse(BRect(200,700,400,900),fLayerData,pattern);
	_driver->StrokeRect(BRect(650,1000,750,1090),fLayerData,pattern);
	_driver->StrokeRoundRect(BRect(200,1000,600,1090),30,40,fLayerData,pattern);
//	_driver->StrokePolygon(polygon,6,polygonRect,fLayerData,pattern);
//	_driver->StrokeTriangle(triangle,triangleRect,fLayerData,pattern);
	fLayerData->highcolor.SetColor(255,0,255,255);
	_driver->FillArc(BRect(1250,300,1450,600),30,270,fLayerData,pattern);
//	_driver->FillBezier(pts,fLayerData,pattern);
	_driver->FillEllipse(BRect(800,300,1200,600),fLayerData,pattern);
	_driver->FillRoundRect(BRect(800,1000,1200,1090),30,40,fLayerData,pattern2);
	_driver->FillPolygon(polygon,6,polygonRect,fLayerData,pattern);
//	_driver->FillTriangle(triangle,triangleRect,fLayerData,pattern);
	
#endif // end DISPLAYDRIVER_TEST_HACK

}

void RootLayer::MoveBy(float x, float y)
{
}

void RootLayer::ResizeBy(float x, float y)
{
	// TODO: implement
}

Layer* RootLayer::VirtualTopChild() const
{
	return fActiveWorkspace->GoToTopItem();
}

Layer* RootLayer::VirtualLowerSibling() const
{
	return fActiveWorkspace->GoToLowerItem();
}

Layer* RootLayer::VirtualUpperSibling() const
{
	return fActiveWorkspace->GoToUpperItem();
}

Layer* RootLayer::VirtualBottomChild() const
{
	return fActiveWorkspace->GoToBottomItem();
}

void RootLayer::AddWinBorderToWorkspaces(WinBorder* winBorder, uint32 wks)
{
	if (!(fMainLock.IsLocked()))
		debugger("RootLayer::AddWinBorderToWorkspaces - fMainLock has to be locked!\n");

	if (wks == B_CURRENT_WORKSPACE)
	{
		ActiveWorkspace()->AddLayerPtr(winBorder);
		return;
	}
	
	for( int32 i=0; i < 32; i++)
	{
		if( wks & (0x00000001 << i) && i < WorkspaceCount())
			WorkspaceAt(i+1)->AddLayerPtr(winBorder);
	}
}

void RootLayer::AddWinBorder(WinBorder* winBorder)
{
	// The main job of this function, besides adding winBorder as a child, is to determine
	// in which workspaces to add this window.

	STRACE(("*RootLayer::AddWinBorder(%s)\n", winBorder->GetName()));
	desktop->fGeneralLock.Lock();
	
	STRACE(("*RootLayer::AddWinBorder(%s) - General lock acquired\n", winBorder->GetName()));
	fMainLock.Lock();

	STRACE(("*RootLayer::AddWinBorder(%s) - Main lock acquired\n", winBorder->GetName()));

	// in case we want to be added to the current workspace
	if (winBorder->Window()->Workspaces() == 0)
		winBorder->Window()->QuietlySetWorkspaces(0x00000001 << (ActiveWorkspaceIndex()-1));

	// add winBorder to the known list of WinBorders so we can keep track of it.
	AddChild(winBorder, winBorder->Window());

	// add winBorder to the desired workspaces
	switch(winBorder->Window()->Feel())
	{
		case B_MODAL_SUBSET_WINDOW_FEEL:
		{
printf("XXXXXXXX1: 21\n");
			// this kind of window isn't added anywhere. It will be added
			//	to main window's subset when winBorder::AddToSubsetOf(main)
			//	will be called.
			break;
		}
		case B_MODAL_APP_WINDOW_FEEL:
		{
printf("XXXXXXXX1: 22\n");
			// add to app's list of Floating/Modal windows (as opposed to the system's)
			winBorder->Window()->App()->fAppFMWList.AddItem(winBorder);

			// determine in witch workspaces to add this winBorder.
			uint32		wks = 0;

			// TODO: change later when you put this code into the server
			for (int32 i=0; i<WorkspaceCount(); i++)
			{
				Workspace	*ws = WorkspaceAt(i+1);
				for (WinBorder *wb = ws->GoToBottomItem(); wb!=NULL; wb = ws->GoToUpperItem())
				{
					if ( !(wb->IsHidden()) &&
						 winBorder->Window()->ClientTeamID() == wb->Window()->ClientTeamID())
					{
						wks = wks | winBorder->Window()->Workspaces();
						break;
					}
				}
			}
			// by using _bottomchild and _uppersibling.

			AddWinBorderToWorkspaces(winBorder, wks);
			break;
		}
				
		case B_MODAL_ALL_WINDOW_FEEL:
		{
printf("XXXXXXXX1: 23\n");
			// add to system's list of Floating/Modal Windows (as opposed to the app's list)
			fMainFMWList.AddItem(winBorder);
			
			// add this winBorder to all workspaces
			AddWinBorderToWorkspaces(winBorder, 0xffffffffUL);
			break;
		}

		case B_FLOATING_SUBSET_WINDOW_FEEL:
		{
printf("XXXXXXXX1: 24\n");
			// this kind of window isn't added anywhere. It *will* be added to WS's list
			//	when its main window will become the front one.
			//	Also, it will be added to MainWinBorder's list when
			//	winBorder::AddToSubset(main) is called.
			break;
		}
				
		case B_FLOATING_APP_WINDOW_FEEL:
		{
printf("XXXXXXXX1: 25\n");
			// add to app's list of Floating/Modal windows (as opposed to the system's)
			winBorder->Window()->App()->fAppFMWList.AddItem(winBorder);
			
			for (int32 i=0; i<WorkspaceCount(); i++){
				Workspace	*ws = WorkspaceAt(i+1);
				WinBorder	*wb = ws->FrontLayer();
				if(wb && wb->Window()->ClientTeamID() == winBorder->Window()->ClientTeamID()
					&& wb->Window()->Feel() == B_NORMAL_WINDOW_FEEL)
				{
					ws->AddLayerPtr(winBorder);
				}
			}

			break;
		}
				
		case B_FLOATING_ALL_WINDOW_FEEL:
		{
printf("XXXXXXXX1: 26\n");
			// add to system's list of Floating/Modal Windows (as opposed to the app's list)
			fMainFMWList.AddItem(winBorder);
			
			// add this winBorder to all workspaces
			AddWinBorderToWorkspaces(winBorder, 0xffffffffUL);
			break;
		}
		
		case B_NORMAL_WINDOW_FEEL:
		{
printf("XXXXXXXX1: 27\n");
			// add this winBorder to the specified workspaces
			AddWinBorderToWorkspaces(winBorder, winBorder->Window()->Workspaces());
			break;
		}
		case B_SYSTEM_LAST:
		case B_SYSTEM_FIRST:
		{
printf("XXXXXXXX1: 28\n");
			// add this winBorder to all workspaces
			AddWinBorderToWorkspaces(winBorder, 0xffffffffUL);
			break;
		}
		default:{
printf("XXXXXXXX1: 29\n");
			break;
		}
	}	// end switch(winborder->Feel())
printf("XXXXXXXX1: 4\n");	
	fMainLock.Unlock();
	STRACE(("*RootLayer::AddWinBorder(%s) - Main lock released\n", winBorder->GetName()));
	
	desktop->fGeneralLock.Unlock();
	STRACE(("*RootLayer::AddWinBorder(%s) - General lock released\n", winBorder->GetName()));
	
	STRACE(("#RootLayer::AddWinBorder(%s)\n", winBorder->GetName()));
}

void RootLayer::RemoveWinBorder(WinBorder* winBorder)
{
	// This method does 3 things:
	// 1) Removes MODAL/SUBSET windows from system/app/window subset list.
	// 2) Removes this window from any workspace it appears in.
	// 3) Removes this window from RootLayer's list of children.

	desktop->fGeneralLock.Lock();
	fMainLock.Lock();
	
	int32 feel = winBorder->Window()->Feel();
	if(feel == B_MODAL_SUBSET_WINDOW_FEEL || feel == B_FLOATING_SUBSET_WINDOW_FEEL)
	{
		desktop->RemoveSubsetWindow(winBorder);
	}
	else
	if (feel == B_MODAL_APP_WINDOW_FEEL || feel == B_FLOATING_APP_WINDOW_FEEL)
	{
		RemoveAppWindow(winBorder);
	}
	else
	if(feel == B_MODAL_ALL_WINDOW_FEEL || feel == B_FLOATING_ALL_WINDOW_FEEL
			|| feel == B_SYSTEM_FIRST || feel == B_SYSTEM_LAST)
	{
		if(feel == B_MODAL_ALL_WINDOW_FEEL || feel == B_FLOATING_ALL_WINDOW_FEEL)
			fMainFMWList.RemoveItem(winBorder);

		int32 count = WorkspaceCount();
		for(int32 i=0; i < count; i++)
			WorkspaceAt(i+1)->RemoveLayerPtr(winBorder);
	}
	else
	{	// for B_NORMAL_WINDOW_FEEL
		uint32 workspaces = winBorder->Window()->Workspaces();
		int32 count = WorkspaceCount();
		for( int32 i=0; i < 32 && i < count; i++)
		{
			if( workspaces & (0x00000001UL << i))
				WorkspaceAt(i+1)->RemoveLayerPtr(winBorder);
		}
	}
	
	RemoveChild(winBorder);
	
	fMainLock.Unlock();
	desktop->fGeneralLock.Unlock();
}

void RootLayer::ChangeWorkspacesFor(WinBorder* winBorder, uint32 newWorkspaces)
{
	// only normal windows are affected by this change
	if(!winBorder->fLevel != B_NORMAL_FEEL)
		return;

	uint32 oldWorkspaces = winBorder->Window()->Workspaces();
	for(int32 i=0; i < WorkspaceCount(); i++)
	{
		if ((oldWorkspaces & (0x00000001 << i)) && (newWorkspaces & (0x00000001 << i)))
		{
			// do nothing.
		}
		else
		if(oldWorkspaces & (0x00000001 << i))
		{
			WorkspaceAt(i+1)->RemoveLayerPtr(winBorder);
		}
		else
		if (newWorkspaces & (0x00000001 << i))
		{
			WorkspaceAt(i+1)->AddLayerPtr(winBorder);
		}
	}
}

bool RootLayer::SetFrontWinBorder(WinBorder* winBorder)
{
	if(!winBorder)
		return false;
	
	STRACE(("*RootLayer::SetFrontWinBorder(%s)\n", winBorder? winBorder->GetName():"NULL"));
	
	fMainLock.Lock();
	STRACE(("*RootLayer::SetFrontWinBorder(%s) - main lock acquired\n", winBorder? winBorder->GetName():"NULL"));
	
	if (!winBorder)
	{
		ActiveWorkspace()->SetFrontLayer(NULL);
		return true;
	}
	
	uint32 workspaces	= winBorder->Window()->Workspaces();
	int32 count		= WorkspaceCount();
	int32 newWorkspace= 0;
	
	if (workspaces & (0x00000001UL << (ActiveWorkspaceIndex()-1)) )
	{
		newWorkspace = ActiveWorkspaceIndex();
	}
	else
	{
		int32	i;
		for( i = 0; i < 32 && i < count; i++)
		{
			if( workspaces & (0x00000001UL << i))
			{
				newWorkspace	= i+1;
				break;
			}
		}
		
		if (i == count || i == 32)
			newWorkspace = ActiveWorkspaceIndex();
	}

	if(newWorkspace != ActiveWorkspaceIndex())
	{
		WorkspaceAt(newWorkspace)->SetFrontLayer(winBorder);
		SetActiveWorkspaceByIndex(newWorkspace);
	}
	else
	{
		ActiveWorkspace()->SetFrontLayer(winBorder);
	}
	
	fMainLock.Unlock();
	STRACE(("*RootLayer::SetFrontWinBorder(%s) - main lock released\n", winBorder? winBorder->GetName():"NULL"));
	STRACE(("#RootLayer::SetFrontWinBorder(%s)\n", winBorder? winBorder->GetName():"NULL"));
	return true;
}

void RootLayer::SetScreens(Screen *screen[], int32 rows, int32 columns)
{
	// NOTE: All screens *must* have the same resolution
	fScreenPtrList.MakeEmpty();
	BRect	newFrame(0, 0, 0, 0);
	for (int32 i=0; i < rows; i++)
	{
		if (i==0)
		{
			for(int32 j=0; j < columns; j++)
			{
				fScreenPtrList.AddItem(screen[i*rows + j]);
				newFrame.right += screen[i*rows + j]->Resolution().x;
			}
		}
		newFrame.bottom		+= screen[i*rows]->Resolution().y;
	}
	
	newFrame.right	-= 1;
	newFrame.bottom	-= 1;
	
	fFrame = newFrame;
	fRows = rows;
	fColumns = columns;
	fScreenXResolution = (int32)(screen[0]->Resolution().x);
	fScreenYResolution = (int32)(screen[0]->Resolution().y);

	// NOTE: a RebuildFullRegion() followed by FullInvalidate() are required after calling 
	// this method!
}

Screen **RootLayer::Screens()
{
	return (Screen**)fScreenPtrList.Items();
}

bool RootLayer::SetScreenResolution(int32 width, int32 height, uint32 colorspace)
{
	if (fScreenXResolution == width && fScreenYResolution == height &&
		fColorSpace == colorspace)
		return false;
	
	bool accepted = true;
	
	for (int i=0; i < fScreenPtrList.CountItems(); i++)
	{
		Screen *screen;
		screen = static_cast<Screen*>(fScreenPtrList.ItemAt(i));
		
		if ( !(screen->SupportsResolution(BPoint(width, height), colorspace)) )
			accepted = false;
	}
	
	if (accepted)
	{
		for (int i=0; i < fScreenPtrList.CountItems(); i++)
		{
			Screen *screen;
			screen = static_cast<Screen*>(fScreenPtrList.ItemAt(i));
			
			screen->SetResolution(BPoint(width, height), colorspace);
		}
		
		Screen **screens = (Screen**)fScreenPtrList.Items();
		SetScreens(screens, fRows, fColumns);
		
		return true;
	}
	
	return false;
}

void RootLayer::SetWorkspaceCount(const int32 count)
{
	STRACE(("*RootLayer::SetWorkspaceCount(%ld)\n", count));
	
	if ((count < 1 && count > 32) || count == WorkspaceCount())
		return;
	
	int32 	exActiveWorkspaceIndex = ActiveWorkspaceIndex();
	
	BList newWSPtrList;
	void *workspacePtr;
	
	fMainLock.Lock();
	STRACE(("*RootLayer::SetWorkspaceCount(%ld) - main lock acquired\n", count));
	
	for(int i=0; i < count; i++)
	{
		workspacePtr	= fWSPtrList.ItemAt(i);
		if (workspacePtr)
			newWSPtrList.AddItem(workspacePtr);
		else
		{
			Workspace	*ws;
			ws = new Workspace(fColorSpace, i+1, BGColor(), this);
			newWSPtrList.AddItem(ws);
		}
	}
	
	// delete other Workspace objects if the count is smaller than current one.
	for (int j=count; j < fWSPtrList.CountItems(); j++)
	{
		Workspace	*ws = NULL;
		ws = static_cast<Workspace*>(fWSPtrList.ItemAt(j));
		if (ws)
			delete ws;
		else
		{
			STRACE(("\nSERVER: PANIC: in RootLayer::SetWorkspaceCount()\n\n"));
			return;
		}
	}
	
	fWSPtrList = newWSPtrList;

	fMainLock.Unlock();
	STRACE(("*RootLayer::SetWorkspaceCount(%ld) - main lock released\n", count));
	
	if (exActiveWorkspaceIndex > count)
		SetActiveWorkspaceByIndex(count); 
	
	// if true, this is the first time this method is called.
	if (exActiveWorkspaceIndex == -1)
		SetActiveWorkspaceByIndex(1); 		
	
	STRACE(("#RootLayer::SetWorkspaceCount(%ld)\n", count));
}

int32 RootLayer::WorkspaceCount() const
{
	return fWSPtrList.CountItems();
}

Workspace* RootLayer::WorkspaceAt(const int32 index) const
{
	Workspace *ws = NULL;
	ws = static_cast<Workspace*>(fWSPtrList.ItemAt(index-1));
	
	return ws;
}

void RootLayer::SetActiveWorkspaceByIndex(const int32 index)
{
	Workspace *ws = NULL;
	ws = static_cast<Workspace*>(fWSPtrList.ItemAt(index-1));
	if (ws)
		SetActiveWorkspace(ws);
}

void RootLayer::SetActiveWorkspace(Workspace *ws)
{
	if (fActiveWorkspace == ws || !ws)
		return;
	
	fActiveWorkspace	= ws;
}

int32 RootLayer::ActiveWorkspaceIndex() const{
	if (fActiveWorkspace)
		return fActiveWorkspace->ID();
	else
		return -1;
}

Workspace* RootLayer::ActiveWorkspace() const
{
	return fActiveWorkspace;
}

void RootLayer::SetBGColor(const RGBColor &col)
{
	ActiveWorkspace()->SetBGColor(col);
	
	fLayerData->viewcolor	= col;
}

RGBColor RootLayer::BGColor(void) const
{
	return fLayerData->viewcolor;
}

void RootLayer::RemoveAppWindow(WinBorder *wb)
{
	wb->Window()->App()->fAppFMWList.RemoveItem(wb);

	int32 count = WorkspaceCount();
	for(int32 i=0; i < count; i++)
		WorkspaceAt(i+1)->RemoveLayerPtr(wb);
}

void RootLayer::PrintToStream()
{
	printf("\nRootLayer '%s' internals:\n", GetName());
	printf("Screen list:\n");
	for(int32 i=0; i<fScreenPtrList.CountItems(); i++)
		printf("\t%ld\n", ((Screen*)fScreenPtrList.ItemAt(i))->ScreenNumber());

	printf("Screen rows: %ld\nScreen columns: %ld\n", fRows, fColumns);
	printf("Resolution for all Screens: %ldx%ldx%ld\n", fScreenXResolution, fScreenYResolution, fColorSpace);
	printf("Workspace list:\n");
	for(int32 i=0; i<fWSPtrList.CountItems(); i++)
	{
		printf("\t~~~Workspace: %ld\n", ((Workspace*)fWSPtrList.ItemAt(i))->ID());
		((Workspace*)fWSPtrList.ItemAt(i))->PrintToStream();
		printf("~~~~~~~~\n");
	}
	printf("Active Workspace: %ld\n", fActiveWorkspace? fActiveWorkspace->ID(): -1);
}
