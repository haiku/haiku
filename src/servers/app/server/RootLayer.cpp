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
#include <Message.h>
#include <File.h>

#include "Globals.h"
#include "RootLayer.h"
#include "Layer.h"
#include "Workspace.h"
#include "ServerScreen.h"
#include "WinBorder.h"
#include "ServerWindow.h"
#include "ServerApp.h"
#include "Desktop.h"
#include "ServerConfig.h"
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
	
	// TODO: this should eventually be replaced by a method to convert the monitor
	// number to an index in the name, i.e. workspace_settings_1 for screen #1
	ReadWorkspaceData(WORKSPACE_DATA_LIST);

	// TODO: read these 3 from a configuration file.
	fScreenXResolution = 0;
	fScreenYResolution = 0;
	fColorSpace = B_RGB32;

	// easy way to identify this class.
	fClassID = AS_ROOTLAYER_CLASS;
	fHidden	= false;
	
}

RootLayer::~RootLayer()
{
	// RootLayer object just uses Screen objects, it is not allowed to delete them.
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

void RootLayer::ReadWorkspaceData(const char *path)
{
	BMessage msg, settings;
	BFile file(path,B_READ_ONLY);
	char string[20];
	
	if(file.InitCheck()==B_OK && msg.Unflatten(&file)==B_OK)
	{
		int32 count;
		
		if(msg.FindInt32("workspace_count",&count)!=B_OK)
			count=9;
		
		SetWorkspaceCount(count);
		
		for(int32 i=0; i<count; i++)
		{
			Workspace *ws=(Workspace*)fWorkspaceList.ItemAt(i);
			if(!ws)
				continue;
			
			sprintf(string,"workspace %ld",i);
			
			if(msg.FindMessage(string,&settings)==B_OK)
			{
				ws->GetSettings(settings);
				settings.MakeEmpty();
			}
			else
				ws->GetDefaultSettings();
		}
	}
	else
	{
		SetWorkspaceCount(9);
		
		for(int32 i=0; i<9; i++)
		{
			Workspace *ws=(Workspace*)fWorkspaceList.ItemAt(i);
			if(!ws)
				continue;
			
			ws->GetDefaultSettings();
		}
	}
}

void RootLayer::SaveWorkspaceData(const char *path)
{
	BMessage msg,dummy;
	BFile file(path,B_READ_WRITE | B_CREATE_FILE);
	
	if(file.InitCheck()!=B_OK)
	{
		printf("ERROR: Couldn't save workspace data in RootLayer\n");
		return;
	}
	
	char string[20];
	int32 count=fWorkspaceList.CountItems();

	if(msg.Unflatten(&file)==B_OK)
	{
		// if we were successful in unflattening the file, it means we're
		// going to need to save over the existing data
		for(int32 i=0; i<count; i++)
		{
			sprintf(string,"workspace %ld",i);
			if(msg.FindMessage(string,&dummy)==B_OK)
				msg.RemoveName(string);
		}
	}
		
	for(int32 i=0; i<count; i++)
	{
		sprintf(string,"workspace %ld",i);

		Workspace *ws=(Workspace*)fWorkspaceList.ItemAt(i);
		
		if(!ws)
		{
			dummy.MakeEmpty();
			ws->PutSettings(&dummy,i);
			msg.AddMessage(string,&dummy);
		}
		else
		{
			// We're not supposed to have this happen, but we'll suck it up, anyway. :P
			Workspace::PutDefaultSettings(&msg,i);
		}
	}
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
		workspacePtr	= fWorkspaceList.ItemAt(i);
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
	for (int j=count; j < fWorkspaceList.CountItems(); j++)
	{
		Workspace	*ws = NULL;
		ws = static_cast<Workspace*>(fWorkspaceList.ItemAt(j));
		if (ws)
			delete ws;
		else
		{
			STRACE(("\nSERVER: PANIC: in RootLayer::SetWorkspaceCount()\n\n"));
			return;
		}
	}
	
	fWorkspaceList = newWSPtrList;

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
	return fWorkspaceList.CountItems();
}

Workspace* RootLayer::WorkspaceAt(const int32 index) const
{
	Workspace *ws = NULL;
	ws = static_cast<Workspace*>(fWorkspaceList.ItemAt(index-1));
	
	return ws;
}

void RootLayer::SetActiveWorkspaceByIndex(const int32 index)
{
	Workspace *ws = NULL;
	ws = static_cast<Workspace*>(fWorkspaceList.ItemAt(index-1));
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
	for(int32 i=0; i<fWorkspaceList.CountItems(); i++)
	{
		printf("\t~~~Workspace: %ld\n", ((Workspace*)fWorkspaceList.ItemAt(i))->ID());
		((Workspace*)fWorkspaceList.ItemAt(i))->PrintToStream();
		printf("~~~~~~~~\n");
	}
	printf("Active Workspace: %ld\n", fActiveWorkspace? fActiveWorkspace->ID(): -1);
}
