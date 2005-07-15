/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */

/**	Class used to encapsulate desktop management */


#include <stdio.h>

#include <Message.h>
#include <Region.h>

#include <WindowInfo.h>
#include <ServerProtocol.h>

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
#	define USE_ACCELERANT 1
#else
#	define USE_ACCELERANT 0
#endif

#if USE_ACCELERANT
#	include "AccelerantHWInterface.h"
#else
#	include "ViewHWInterface.h"
#endif

#include "Desktop.h"

//#define DEBUG_DESKTOP

#ifdef DEBUG_DESKTOP
#	define STRACE(a) printf(a)
#else
#	define STRACE(a) /* nothing */
#endif


Desktop::Desktop()
	: fWinBorderList(64),
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

	delete fRootLayer;
}


void
Desktop::Init()
{
	fVirtualScreen.RestoreConfiguration(*this);

	// TODO: temporary workaround, fActiveScreen will be removed
	fActiveScreen = fVirtualScreen.ScreenAt(0);

	// TODO: add user identity to the name
	char name[32];
	sprintf(name, "RootLayer %d", 1);

	fRootLayer = new RootLayer(name, 4, this, GetDisplayDriver());
	fRootLayer->RunThread();
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
Desktop::FindWinBorderByClientToken(int32 token, team_id teamID)
{
	BAutolock locker(this);

	WinBorder *wb;
	for (int32 i = 0; (wb = (WinBorder *)fWinBorderList.ItemAt(i)); i++) {
		if (wb->Window()->ClientToken() == token
			&& wb->Window()->ClientTeam() == teamID)
			return wb;
	}

	return NULL;
}


void
Desktop::WriteWindowList(team_id team, BPrivate::LinkSender& sender)
{
	BAutolock locker(this);

	// compute the number of windows

	int32 count = 0;
	if (team >= B_OK) {
		for (int32 i = 0; i < fWinBorderList.CountItems(); i++) {
			WinBorder* border = (WinBorder*)fWinBorderList.ItemAt(i);

			if (border->Window()->ClientTeam() == team)
				count++;
		}
	} else
		count = fWinBorderList.CountItems();

	// write list

	sender.StartMessage(SERVER_TRUE);
	sender.Attach<int32>(count);

	for (int32 i = 0; i < fWinBorderList.CountItems(); i++) {
		WinBorder* border = (WinBorder*)fWinBorderList.ItemAt(i);

		if (team >= B_OK && border->Window()->ClientTeam() != team)
			continue;

		sender.Attach<int32>(border->Window()->ServerToken());
	}

	sender.Flush();
}


void
Desktop::WriteWindowInfo(int32 serverToken, BPrivate::LinkSender& sender)
{
	BAutolock locker(this);
	BAutolock tokenLocker(BPrivate::gDefaultTokens);

	ServerWindow* window;
	if (BPrivate::gDefaultTokens.GetToken(serverToken,
			B_SERVER_TOKEN, (void**)&window) != B_OK) {
		sender.StartMessage(B_ENTRY_NOT_FOUND);
		sender.Flush();
		return;
	}

	window_info info;
	window->GetInfo(info);

	int32 length = window->Title() ? strlen(window->Title()) : 0;

	sender.StartMessage(B_OK);
	sender.Attach<int32>(sizeof(window_info) + length + 1);
	sender.Attach(&info, sizeof(window_info));
	if (length > 0)
		sender.Attach(window->Title(), length + 1);
	else
		sender.Attach<char>('\0');
	sender.Flush();
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

