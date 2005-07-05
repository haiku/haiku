//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc. All rights reserved.
//  Distributed under the terms of the MIT license.
//
//	File Name:		Desktop.cpp
//	Author:			Adi Oanca <adioanca@cotty.iren.ro>
//					Stephan Aßmus <superstippi@gmx.de>
//	Description:	Class used to encapsulate desktop management
//
//------------------------------------------------------------------------------
#include <stdio.h>

#include <Message.h>
#include <Region.h>

#include "AppServer.h"
#include "DisplayDriverPainter.h"
#include "Globals.h"
#include "Layer.h"
#include "RootLayer.h"
#include "ServerConfig.h"
#include "ServerScreen.h"
#include "ServerApp.h"
#include "ServerWindow.h"
#include "WinBorder.h"
#include "Workspace.h"

#ifdef __HAIKU__
#define USE_ACCELERANT 1
#else
#define USE_ACCELERANT 0
#endif

#if USE_ACCELERANT
  #include "AccelerantHWInterface.h"
#else
  #include "ViewHWInterface.h"
#endif

#include "Desktop.h"

//#define DEBUG_DESKTOP

#ifdef DEBUG_DESKTOP
	#define STRACE(a) printf(a)
#else
	#define STRACE(a) /* nothing */
#endif


Desktop::Desktop()
	: fWinBorderList(64),
	  fWinLock("desktop window list lock"),
	  fRootLayerList(2),
	  fActiveRootLayer(NULL),
	  fScreenList(2),
	  fActiveScreen(NULL),
	  fMouseMode(B_NORMAL_MOUSE),
	  fFFMouseMode(false)
{
	// init scrollbar info
	fScrollBarInfo.proportional = true;
	fScrollBarInfo.double_arrows = false;
	// look of the knob (R5: (0, 1, 2), 1 = default)
	fScrollBarInfo.knob = 1;
	fScrollBarInfo.min_knob_size = 15;

	// init menu info
	fMenuInfo.font_size = 12.0;
// TODO: ...
//	fMenuInfo.f_family;
//	fMenuInfo.f_style;
//	fMenuInfo.background_color = gColorSet->menu_background;
	// look of the separator (R5: (0, 1, 2), default ?)
	fMenuInfo.separator = 0;
	fMenuInfo.click_to_open = true;
	fMenuInfo.triggers_always_shown = false;
}


Desktop::~Desktop()
{
	for (int32 i = 0; WinBorder *border = (WinBorder *)fWinBorderList.ItemAt(i); i++)
		delete border;

	for (int32 i = 0; RootLayer *rootLayer = (RootLayer *)fRootLayerList.ItemAt(i); i++)
		delete rootLayer;

	for (int32 i = 0; Screen *screen = (Screen *)fScreenList.ItemAt(i); i++)
		delete screen;
}


void
Desktop::Init()
{
	HWInterface *interface = NULL;

	// Eventually we will loop through drivers until
	// one can't initialize in order to support multiple monitors.
	// For now, we'll just load one and be done with it.
	
	bool initDrivers = true;
	while (initDrivers) {

#if USE_ACCELERANT
		  interface = new AccelerantHWInterface();
#else
		  interface = new ViewHWInterface();
#endif

		_AddGraphicsCard(interface);
		initDrivers = false;
	}

	if (fScreenList.CountItems() < 1) {
		delete this;
		return;
	}

	InitMode();

	SetActiveRootLayerByIndex(0);
}


void
Desktop::_AddGraphicsCard(HWInterface* interface)
{
	Screen *screen = new Screen(interface, fScreenList.CountItems() + 1);
		// The interface is now owned by the screen

	if (screen->Initialize() >= B_OK && fScreenList.AddItem((void*)screen)) {

		// TODO: be careful of screen initialization - monitor may not support 640x480
#if __HAIKU__
		screen->SetMode(1400, 1050, B_RGB32, 60.f);
#else
		screen->SetMode(800, 600, B_RGB32, 60.f);
#endif

	} else {
		delete screen;
	}
}


void
Desktop::InitMode()
{
	// this is init mode for n-SS.
	fActiveScreen = (Screen *)fScreenList.ItemAt(0);

	for (int32 i = 0; i < fScreenList.CountItems(); i++) {
		Screen *screen = (Screen *)fScreenList.ItemAt(i);

		char name[32];
		sprintf(name, "RootLayer %ld", i + 1);

		RootLayer *rootLayer = new RootLayer(name, 4, this, GetDisplayDriver());
		rootLayer->SetScreens(&screen, 1, 1);
		rootLayer->RunThread();

		fRootLayerList.AddItem(rootLayer);
	}
}


//---------------------------------------------------------------------------
//					Methods for multiple monitors.
//---------------------------------------------------------------------------


inline void
Desktop::SetActiveRootLayerByIndex(int32 listIndex)
{
	RootLayer *rootLayer = RootLayerAt(listIndex);

	if (rootLayer != NULL)
		SetActiveRootLayer(rootLayer);
}


inline void
Desktop::SetActiveRootLayer(RootLayer *rootLayer)
{
	if (fActiveRootLayer == rootLayer)
		return;

	fActiveRootLayer = rootLayer;
}


//---------------------------------------------------------------------------
//				Methods for layer(WinBorder) manipulation.
//---------------------------------------------------------------------------


void
Desktop::AddWinBorder(WinBorder *winBorder)
{
	if (!winBorder)
		return;

	// R2: how to determine the RootLayer to which this window should be added???
	// for now, use ActiveRootLayer() because we only have one instance.

	int32 feel = winBorder->Feel();

	// we are ServerApp thread, we need to lock RootLayer here.
	ActiveRootLayer()->Lock();

	// we're playing with window list. lock first.
	Lock();

	if (fWinBorderList.HasItem(winBorder)) {
		Unlock();
		ActiveRootLayer()->Unlock();
		debugger("AddWinBorder: WinBorder already in Desktop list\n");
		return;
	}

	// we have a new window. store a record of it.
	fWinBorderList.AddItem(winBorder);

	// add FLOATING_APP windows to the local list of all normal windows.
	// This is to keep the order all floating windows (app or subset) when we go from
	// one normal window to another.
	if (feel == B_FLOATING_APP_WINDOW_FEEL || feel == B_NORMAL_WINDOW_FEEL) {
		WinBorder *wb = NULL;
		int32 count = fWinBorderList.CountItems();
		int32 feelToLookFor = (feel == B_NORMAL_WINDOW_FEEL ?
			B_FLOATING_APP_WINDOW_FEEL : B_NORMAL_WINDOW_FEEL);

		for (int32 i = 0; i < count; i++) {
			wb = (WinBorder *)fWinBorderList.ItemAt(i);

			if (wb->App()->ClientTeam() == winBorder->App()->ClientTeam()
				&& wb->Feel() == feelToLookFor) {
				// R2: RootLayer comparison is needed.
				feel == B_NORMAL_WINDOW_FEEL ?
					winBorder->fSubWindowList.AddWinBorder(wb) :
					wb->fSubWindowList.AddWinBorder(winBorder);
			}
		}
	}

	// add application's list of modal windows.
	if (feel == B_MODAL_APP_WINDOW_FEEL) {
		winBorder->App()->fAppSubWindowList.AddWinBorder(winBorder);
	}

	// send WinBorder to be added to workspaces
	ActiveRootLayer()->AddWinBorder(winBorder);

	// hey, unlock!
	Unlock();

	ActiveRootLayer()->Unlock();
}


void
Desktop::RemoveWinBorder(WinBorder *winBorder)
{
	if (!winBorder)
		return;

	// we are ServerApp thread, we need to lock RootLayer here.
	ActiveRootLayer()->Lock();

	// we're playing with window list. lock first.
	Lock();

	// remove from main WinBorder list.
	if (fWinBorderList.RemoveItem(winBorder)) {
		int32 feel = winBorder->Feel();

		// floating app/subset and modal_subset windows require special atention because
		// they are/may_be added to the list of a lot normal windows.
		if (feel == B_FLOATING_SUBSET_WINDOW_FEEL
			|| feel == B_MODAL_SUBSET_WINDOW_FEEL
			|| feel == B_FLOATING_APP_WINDOW_FEEL)
		{
			WinBorder *wb = NULL;
			int32 count = fWinBorderList.CountItems();

			for (int32 i = 0; i < count; i++) {
				wb = (WinBorder*)fWinBorderList.ItemAt(i);

				if (wb->Feel() == B_NORMAL_WINDOW_FEEL
					&& wb->App()->ClientTeam() == winBorder->App()->ClientTeam()) {
					// R2: RootLayer comparison is needed. We'll see.
					wb->fSubWindowList.RemoveItem(winBorder);
				}
			}
		}

		// remove from application's list
		if (feel == B_MODAL_APP_WINDOW_FEEL) {
			winBorder->App()->fAppSubWindowList.RemoveItem(winBorder);
		}
	} else {
		Unlock();
		ActiveRootLayer()->Unlock();
		debugger("RemoveWinBorder: WinBorder not found in Desktop list\n");
		return;
	}

	// Tell to winBorder's RootLayer about this.
	ActiveRootLayer()->RemoveWinBorder(winBorder);

	Unlock();
	ActiveRootLayer()->Unlock();
}


void
Desktop::AddWinBorderToSubset(WinBorder *winBorder, WinBorder *toWinBorder)
{
	// NOTE: we can safely lock the entire method body, because this method is called from
	//		 RootLayer's thread only.

	// we're playing with window list. lock first.
	Lock();

	if (!winBorder || !toWinBorder
		|| !fWinBorderList.HasItem(winBorder)
		|| !fWinBorderList.HasItem(toWinBorder)) {
		Unlock();
		debugger("AddWinBorderToSubset: NULL WinBorder or not found in Desktop list\n");
		return;
	}

	if ((winBorder->Feel() == B_FLOATING_SUBSET_WINDOW_FEEL
			|| winBorder->Feel() == B_MODAL_SUBSET_WINDOW_FEEL)
		&& toWinBorder->Feel() == B_NORMAL_WINDOW_FEEL
		&& toWinBorder->App()->ClientTeam() == winBorder->App()->ClientTeam()
		&& !toWinBorder->fSubWindowList.HasItem(winBorder)) {
		// add to normal_window's list
		toWinBorder->fSubWindowList.AddWinBorder(winBorder);
	} else {
		Unlock();
		debugger("AddWinBorderToSubset: you must add a subset_window to a normal_window's subset with the same team_id\n");
		return;
	}

	// send WinBorder to be added to workspaces, if not already in there.
	ActiveRootLayer()->AddSubsetWinBorder(winBorder, toWinBorder);

	Unlock();
}


void
Desktop::RemoveWinBorderFromSubset(WinBorder *winBorder, WinBorder *fromWinBorder)
{
	// NOTE: we can safely lock the entire method body, because this method is called from
	//		 RootLayer's thread only.

	// we're playing with window list. lock first.
	Lock();

	if (!winBorder || !fromWinBorder
		|| !fWinBorderList.HasItem(winBorder)
		|| !fWinBorderList.HasItem(fromWinBorder)) {
		Unlock();
		debugger("RemoveWinBorderFromSubset: NULL WinBorder or not found in Desktop list\n");
		return;
	}

	// remove WinBorder from workspace, if needed - some other windows may still have it in their subset
	ActiveRootLayer()->RemoveSubsetWinBorder(winBorder, fromWinBorder);

	if (fromWinBorder->Feel() == B_NORMAL_WINDOW_FEEL) {
		//remove from this normal_window's subset.
		fromWinBorder->fSubWindowList.RemoveItem(winBorder);
	} else {
		Unlock();
		debugger("RemoveWinBorderFromSubset: you must remove a subset_window from a normal_window's subset\n");
		return;
	}

	Unlock();
}


void
Desktop::SetWinBorderFeel(WinBorder *winBorder, uint32 feel)
{
	// NOTE: this method is called from RootLayer thread only

	// we're playing with window list. lock first.
	Lock();

	RemoveWinBorder(winBorder);
	winBorder->QuietlySetFeel(feel);
	AddWinBorder(winBorder);

	Unlock();
}


WinBorder *
Desktop::FindWinBorderByServerWindowTokenAndTeamID(int32 token, team_id teamID)
{
	WinBorder *wb;

	Lock();
	for (int32 i = 0; (wb = (WinBorder *)fWinBorderList.ItemAt(i)); i++) {
		if (wb->Window()->ClientToken() == token
			&& wb->Window()->ClientTeam() == teamID)
			break;
	}
	Unlock();

	return wb;
}


//---------------------------------------------------------------------------
//				Methods for various desktop stuff handled by the server
//---------------------------------------------------------------------------


void
Desktop::SetScrollBarInfo(const scroll_bar_info &info)
{
	fScrollBarInfo = info;
}


scroll_bar_info
Desktop::ScrollBarInfo(void) const
{
	return fScrollBarInfo;
}


void
Desktop::SetMenuInfo(const menu_info &info)
{
	fMenuInfo = info;
}


menu_info
Desktop::MenuInfo(void) const
{
	return fMenuInfo;
}


void
Desktop::UseFFMouse(const bool &useffm)
{
	fFFMouseMode = useffm;
}


bool
Desktop::FFMouseInUse(void) const
{
	return fFFMouseMode;
}


void
Desktop::SetFFMouseMode(const mode_mouse &value)
{
	fMouseMode = value;
}


mode_mouse
Desktop::FFMouseMode(void) const
{
	return fMouseMode;
}


void
Desktop::PrintToStream(void)
{
	printf("RootLayer List:\n=======\n");

	for (int32 i = 0; i < fRootLayerList.CountItems(); i++) {
		printf("\t%s\n", ((RootLayer*)fRootLayerList.ItemAt(i))->Name());
		((RootLayer*)fRootLayerList.ItemAt(i))->PrintToStream();
		printf("-------\n");
	}

	printf("=======\nActive RootLayer: %s\n",
		fActiveRootLayer ? fActiveRootLayer->Name() : "NULL");
//	printf("Active WinBorder: %s\n", fActiveWinBorder? fActiveWinBorder->Name(): "NULL");

	printf("Screen List:\n");
	for (int32 i = 0; i < fScreenList.CountItems(); i++)
		printf("\t%ld\n", ((Screen*)fScreenList.ItemAt(i))->ScreenNumber());
}


void
Desktop::PrintVisibleInRootLayerNo(int32 no)
{
}
