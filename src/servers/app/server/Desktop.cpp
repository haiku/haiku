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
#include <stdio.h>
#include <Region.h>
#include <Message.h>

#include "AccelerantDriver.h"
#include "AppServer.h"
#include "Desktop.h"
#include "DisplayDriver.h"
#include "Globals.h"
#include "Layer.h"
#include "RootLayer.h"
#include "ServerConfig.h"
#include "ServerScreen.h"
#include "ServerWindow.h"
#include "ViewDriver.h"
#include "DirectDriver.h"
#include "WinBorder.h"
#include "Workspace.h"

//#define DEBUG_DESKTOP

#ifdef DEBUG_DESKTOP
	#define STRACE(a) printf(a)
#else
	#define STRACE(a) /* nothing */
#endif

Desktop::Desktop(void)
{
	fActiveRootLayer	= NULL;
	fActiveScreen		= NULL;
}

Desktop::~Desktop(void)
{
	void	*ptr;

	for(int32 i=0; (ptr=fWinBorderList.ItemAt(i)); i++)
		delete (WinBorder*)ptr;

	for(int32 i=0; (ptr=fRootLayerList.ItemAt(i)); i++)
		delete (RootLayer*)ptr;

	for(int32 i=0; (ptr=fScreenList.ItemAt(i)); i++)
		delete (Screen*)ptr;

}

void Desktop::Init(void)
{
	DisplayDriver	*driver = NULL;
	int32 driverCount = 0;
	bool initDrivers = true;

	while(initDrivers)
	{

		if(DISPLAYDRIVER == HWDRIVER)
		{
			// If we're using the AccelerantDriver for rendering, eventually we will loop through
			// drivers until one can't initialize in order to support multiple monitors. For now,
			// we'll just load one and be done with it.
			driver = new AccelerantDriver();
		}
		else if(DISPLAYDRIVER == DIRECTDRIVER)
		{
			// Eventually, it would be nice to do away with ViewDriver and replace it with
			// one which uses a double-buffered BDirectWindow as a rendering context
			driver = new DirectDriver();
		}
		else
		{
			// Eventually, it would be nice to do away with ViewDriver and replace it with
			// one which uses a double-buffered BDirectWindow as a rendering context
			driver = new ViewDriver();
		}

		if(driver->Initialize())
		{
			driverCount++;

			Screen		*sc = new Screen(driver, BPoint(640, 480), B_RGB32, driverCount);

			// TODO: be careful, of screen initialization - monitor may not support 640x480
			fScreenList.AddItem(sc);

			if ( (DISPLAYDRIVER != HWDRIVER) && (driverCount == 1) )
				initDrivers	= false;
		}
		else
		{
			driver->Shutdown();
			delete	driver;
			driver	= NULL;
			initDrivers	= false;
		}
	}

	if (driverCount < 1){
		delete this;
		return;
	}
	
	InitMode();

	SetActiveRootLayerByIndex(0);
}

void Desktop::InitMode(void)
{
	// this is init mode for n-SS.
	fActiveScreen	= fScreenList.ItemAt(0)? (Screen*)fScreenList.ItemAt(0): NULL;
	for (int32 i=0; i<fScreenList.CountItems(); i++)
	{
		char		name[32];
		sprintf(name, "RootLayer %ld", i+1);

		Screen		*screens[1];		
		screens[0]	= (Screen*)fScreenList.ItemAt(i);
		
		RootLayer	*rl = new RootLayer(name, 4, this, GetDisplayDriver());
		rl->SetScreens(screens, 1, 1);
		rl->RunThread();
		
		fRootLayerList.AddItem(rl);
	}
}

//---------------------------------------------------------------------------
//					Methods for multiple monitors.
//---------------------------------------------------------------------------


Screen* Desktop::ScreenAt(int32 index) const
{
	Screen	*sc= static_cast<Screen*>(fScreenList.ItemAt(index));

	return sc;
}

int32 Desktop::ScreenCount(void) const
{
	return fScreenList.CountItems();
}

Screen* Desktop::ActiveScreen(void) const
{
	return fActiveScreen;
}

void Desktop::SetActiveRootLayerByIndex(int32 listIndex)
{
	RootLayer	*rl=RootLayerAt(listIndex);

	if (rl)
		SetActiveRootLayer(rl);
}

void Desktop::SetActiveRootLayer(RootLayer* rl)
{
	if (fActiveRootLayer == rl)
		return;

	fActiveRootLayer	= rl;
}

RootLayer* Desktop::ActiveRootLayer(void) const
{
	return fActiveRootLayer;
}

int32 Desktop::ActiveRootLayerIndex(void) const
{
	int32		rootLayerCount = CountRootLayers(); 
	for(int32 i=0; i<rootLayerCount; i++)
	{
		if(fActiveRootLayer == (RootLayer*)(fRootLayerList.ItemAt(i)))
			return i;
	}
	return -1;
}

RootLayer* Desktop::RootLayerAt(int32 index)
{
	RootLayer	*rl=static_cast<RootLayer*>(fRootLayerList.ItemAt(index));

	return rl;
}

int32 Desktop::CountRootLayers() const
{
	return fRootLayerList.CountItems();
}

DisplayDriver* Desktop::GetDisplayDriver() const
{
	return ScreenAt(0)->DDriver();
}

//---------------------------------------------------------------------------
//				Methods for layer(WinBorder) manipulation.
//---------------------------------------------------------------------------


void Desktop::AddWinBorder(WinBorder* winBorder)
{
	desktop->fGeneralLock.Lock();

	if(fWinBorderList.HasItem(winBorder))
		return;

	// special case for Tracker background window.
	if (winBorder->Level() == B_SYSTEM_LAST)
	{
		// it's added in all RottLayers
		for(int32 i=0; i<fRootLayerList.CountItems(); i++)
			((RootLayer*)fRootLayerList.ItemAt(i))->AddWinBorder(winBorder);
	}
	else
		// other windows are added to the current RootLayer only.
		ActiveRootLayer()->AddWinBorder(winBorder);

	// add that pointer to user winboder list so that we can keep track of them.		
	fLayerLock.Lock();
	fWinBorderList.AddItem(winBorder);
	fLayerLock.Unlock();

	desktop->fGeneralLock.Unlock();
}

void Desktop::RemoveWinBorder(WinBorder* winBorder)
{
	desktop->fGeneralLock.Lock();

	if(winBorder->Level() == B_SYSTEM_LAST)
	{
		for(int32 i=0; i<fRootLayerList.CountItems(); i++)
			((RootLayer*)fRootLayerList.ItemAt(i))->RemoveWinBorder(winBorder);
	}
	else
		winBorder->GetRootLayer()->RemoveWinBorder(winBorder);

	fLayerLock.Lock();
	fWinBorderList.RemoveItem(winBorder);
	fLayerLock.Unlock();

	desktop->fGeneralLock.Unlock();
}

bool Desktop::HasWinBorder(WinBorder* winBorder)
{
	return fWinBorderList.HasItem(winBorder);
}

//---------------------------------------------------------------------------
//				Methods for various desktop stuff handled by the server
//---------------------------------------------------------------------------
void Desktop::SetScrollBarInfo(const scroll_bar_info &info)
{
	fScrollBarInfo	= info;
}

scroll_bar_info Desktop::ScrollBarInfo(void) const
{
	return fScrollBarInfo;
}

void Desktop::SetMenuInfo(const menu_info &info)
{
	fMenuInfo	= info;
}

menu_info Desktop::MenuInfo(void) const
{
	return fMenuInfo;
}

void Desktop::UseFFMouse(const bool &useffm)
{
	fFFMouseMode	= useffm;
}

bool Desktop::FFMouseInUse(void) const
{
	return fFFMouseMode;
}

void Desktop::SetFFMouseMode(const mode_mouse &value)
{
	fMouseMode	= value;
}

mode_mouse Desktop::FFMouseMode(void) const
{
	return fMouseMode;
}

void Desktop::RemoveSubsetWindow(WinBorder* wb)
{
	WinBorder		*winBorder = NULL;

	fLayerLock.Lock();
	int32			count = fWinBorderList.CountItems();
	for(int32 i=0; i < count; i++)
	{
		winBorder	= static_cast<WinBorder*>(fWinBorderList.ItemAt(i));
		if (winBorder->Level() == B_NORMAL_FEEL)
			winBorder->Window()->fWinFMWList.RemoveItem(wb);
	}
	fLayerLock.Unlock();

	RootLayer *rl = winBorder->GetRootLayer();

	if (!fGeneralLock.IsLocked())
		debugger("Desktop::RemoveWinBorder() - fGeneralLock must be locked!\n");
	if (!(rl->fMainLock.IsLocked()))
		debugger("Desktop::RemoveWinBorder() - fMainLock must be locked!\n");
		
	int32 countWKs = rl->WorkspaceCount();
	for (int32 i=0; i < countWKs; i++)
		rl->WorkspaceAt(i+1)->RemoveWinBorder(wb);

}

void Desktop::PrintToStream(void)
{
	printf("RootLayer List:\n=======\n");
	for(int32 i=0; i<fRootLayerList.CountItems(); i++)
	{
		printf("\t%s\n", ((RootLayer*)fRootLayerList.ItemAt(i))->GetName());
		((RootLayer*)fRootLayerList.ItemAt(i))->PrintToStream();
		printf("-------\n");
	}
	printf("=======\nActive RootLayer: %s\n", fActiveRootLayer? fActiveRootLayer->GetName(): "NULL");
//	printf("Active WinBorder: %s\n", fActiveWinBorder? fActiveWinBorder->Name(): "NULL");
	
	printf("Screen List:\n");
	for(int32 i=0; i<fScreenList.CountItems(); i++)
		printf("\t%ld\n", ((Screen*)fScreenList.ItemAt(i))->ScreenNumber());
}

WinBorder* Desktop::FindWinBorderByServerWindowTokenAndTeamID(int32 token, team_id teamID)
{
	WinBorder*		wb;
	fLayerLock.Lock();
	for (int32 i = 0; (wb = (WinBorder*)fWinBorderList.ItemAt(i)); i++)
	{
		if (wb->Window()->ClientToken() == token
			&& wb->Window()->ClientTeamID() == teamID)
			break;
	}
	fLayerLock.Unlock();
	
	return wb;
}

void Desktop::PrintVisibleInRootLayerNo(int32 no)
{
	if (no<0 || no>=fRootLayerList.CountItems())
		return;

	printf("Visible windows in RootLayer %ld, Workspace %ld\n",
		ActiveRootLayerIndex(), ActiveRootLayer()->ActiveWorkspaceIndex());
	WinBorder *wb = NULL;
	Workspace *ws = ActiveRootLayer()->ActiveWorkspace();
	for(wb = (WinBorder*)ws->GoToTopItem(); wb != NULL; wb = (WinBorder*)ws->GoToLowerItem())
	{
		if (!wb->IsHidden())
			wb->PrintToStream();
	}
}
