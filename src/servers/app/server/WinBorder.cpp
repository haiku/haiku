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
//	File Name:		WinBorder.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@mymail.ro>
//	Description:	Layer subclass which handles window management
//  
//------------------------------------------------------------------------------
#include <Region.h>
#include <String.h>
#include <Locker.h>
#include <Debug.h>
#include <TokenSpace.h>
#include "PortMessage.h"
#include "View.h"	// for mouse button defines
#include "ServerWindow.h"
#include "Decorator.h"
#include "DisplayDriver.h"
#include "Desktop.h"
#include "WinBorder.h"
#include "AppServer.h"	// for new_decorator()
#include "TokenHandler.h"
#include "Globals.h"
#include "RootLayer.h"
#include "Workspace.h"

// TODO: Document this file completely

// Toggle general function call output
//#define DEBUG_WINBORDER

// toggle
//#define DEBUG_WINBORDER_MOUSE
//#define DEBUG_WINBORDER_CLICK

#ifdef DEBUG_WINBORDER
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

#ifdef DEBUG_WINBORDER_MOUSE
#	include <stdio.h>
#	define STRACE_MOUSE(x) printf x
#else
#	define STRACE_MOUSE(x) ;
#endif

#ifdef DEBUG_WINBORDER_CLICK
#	include <stdio.h>
#	define STRACE_CLICK(x) printf x
#else
#	define STRACE_CLICK(x) ;
#endif

//! TokenHandler object used to provide IDs for all WinBorder objects
TokenHandler border_token_handler;

bool gMouseDown = false;


WinBorder::WinBorder(const BRect &r, const char *name, const int32 look, const int32 feel,
		const int32 flags, ServerWindow *win, DisplayDriver *driver)
	: Layer(r, name, B_NULL_TOKEN, B_FOLLOW_NONE, flags, driver)
{
	// unlike BViews, windows start off as hidden
	fHidden			= true;
	fServerWin		= win;

	fMouseButtons	= 0;
	fKeyModifiers	= 0;
	fMainWinBorder	= NULL;
	fDecorator		= NULL;
	fTopLayer		= NULL;
	fAdFlags		= fAdFlags | B_LAYER_CHILDREN_DEPENDANT;
	fFlags			= B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE;

	fIsMoving		= false;
	fIsResizing		= false;
	fIsClosing		= false;
	fIsMinimizing	= false;
	fIsZooming		= false;

	fLastMousePosition.Set(-1,-1);
	SetLevel();

	if (feel!= B_NO_BORDER_WINDOW_LOOK)
		fDecorator = new_decorator(r, name, look, feel, flags, fDriver);

	STRACE(("WinBorder %s:\n",GetName()));
	STRACE(("\tFrame: (%.1f,%.1f,%.1f,%.1f)\n",r.left,r.top,r.right,r.bottom));
	STRACE(("\tWindow %s\n",win?win->Title():"NULL"));
}

WinBorder::~WinBorder(void)
{
	STRACE(("WinBorder(%s)::~WinBorder()\n",GetName()));
	if (fTopLayer){
		delete fTopLayer;
		fTopLayer = NULL;
	}

	if (fDecorator)	
	{
		delete fDecorator;
		fDecorator = NULL;
	}
}

void WinBorder::RebuildFullRegion(void)
{
	STRACE(("WinBorder(%s)::RebuildFullRegion()\n",GetName()));

	fFull.MakeEmpty();

	// Winborder holds Decorator's full regions. if any...
	if (fDecorator)
		fDecorator->GetFootprint(&fFull);
}

void WinBorder::MouseDown(PortMessage *msg, bool sendMessage)
{
	if (!(Window()->IsLocked()))
		debugger("you must lock the attached ServerWindow object\n\t before calling WinBorder::MouseDown()\n");

	// this is important to determine how much we should resize or move the Layer(WinBorder)(window)

	// user clicked on WinBorder's visible region, which is in fact decorator's.
	// so, if true, we find out if the user clicked the decorator.

	// Attached data:
	// 1) int64 - time of mouse click
	// 2) float - x coordinate of mouse click
	// 3) float - y coordinate of mouse click
	// 4) int32 - modifier keys down
	// 5) int32 - buttons down
	// 6) int32 - clicks

	int64 time;
	BPoint pt;
	int32 modifierkeys;
	int32 buttons;
	int32 clicks;
	
	msg->Read<int64>(&time);
	msg->Read<float>(&pt.x);
	msg->Read<float>(&pt.y);
	msg->Read<int32>(&modifierkeys);
	msg->Read<int32>(&buttons);
	msg->Read<int32>(&clicks);

	Layer	*target = LayerAt(pt);
	if (target == this)
	{
		click_type action;

		// find out where user clicked in Decorator
		action = fDecorator->Clicked(pt, buttons, modifierkeys);
		switch(action)
		{
			case DEC_CLOSE:
			{
				fIsClosing = true;
				fDecorator->SetClose(true);
				fDecorator->DrawClose();
				STRACE_CLICK(("===> DEC_CLOSE\n"));
				break;
			}
			case DEC_ZOOM:
			{
				fIsZooming = true;
				fDecorator->SetZoom(true);
				fDecorator->DrawZoom();
				STRACE_CLICK(("===> DEC_ZOOM\n"));
				break;
			}
			case DEC_MINIMIZE:
			{
				fIsMinimizing = true;
				fDecorator->SetMinimize(true);
				fDecorator->DrawMinimize();
				STRACE_CLICK(("===> DEC_MINIMIZE\n"));
				break;
			}
			case DEC_RESIZE:
			{
				fIsResizing = true;
				STRACE_CLICK(("===> DEC_RESIZE\n"));
				break;
			}
			case DEC_DRAG:
			{
				fIsMoving = true;
				STRACE_CLICK(("===> DEC_DRAG\n"));
				break;
			}
			case DEC_MOVETOBACK:
			{
				GetRootLayer()->ActiveWorkspace()->MoveToBack(this);
				break;
			}
			case DEC_NONE:
			{
				debugger("WinBorder::MouseDown - Decorator should NOT return DEC_NONE\n");
				break;
			}
			default:
			{
				debugger("WinBorder::MouseDown - Decorator returned UNKNOWN code\n");
				break;
			}
		}
	}
	else if (sendMessage){
		BMessage msg;
		msg.what= B_MOUSE_DOWN;
		msg.AddInt64("when", time);
		msg.AddPoint("where", pt);
		msg.AddInt32("modifiers", modifierkeys);
		msg.AddInt32("buttons", buttons);
		msg.AddInt32("clicks", clicks);
		
		// TODO: figure out how to specify the target
		// msg.AddInt32("token", token);
		Window()->SendMessageToClient(&msg);
	}
	
	// Just to clean up any mess we've made. :)
	msg->Rewind();
	
	fLastMousePosition = pt;
}

void WinBorder::MouseMoved(PortMessage *msg)
{
	if (!(Window()->IsLocked()))
		debugger("you must lock the attached ServerWindow object\n\t before calling WinBorder::MouseMoved()\n");

	BPoint pt;
	int64 dummy;
	int32 buttons;
	
	msg->Read<int64>(&dummy);
	msg->Read<float>(&pt.x);
	msg->Read<float>(&pt.y);
	msg->Read<int32>(&buttons);
	msg->Rewind();
	
	if (fIsMoving)
	{
		STRACE_CLICK(("===> Moving...\n"));
		BPoint		offset = pt;
		offset		-= fLastMousePosition;
		MoveBy(offset.x, offset.y);
	}
	else
	if (fIsResizing)
	{
		STRACE_CLICK(("===> Resizing...\n"));
		BPoint		offset = pt;
		offset		-= fLastMousePosition;
		ResizeBy(offset.x, offset.y);
	}
	else
	{
		// Do a click test only if we have to, which would be now. :)
		click_type location=fDecorator->Clicked(pt,buttons,fKeyModifiers);
		
		if (fIsZooming && location!=DEC_ZOOM)
		{
			fDecorator->SetZoom(false);
			fDecorator->DrawZoom();
		}
		else
		if (fIsClosing && location!=DEC_CLOSE)
		{
			fDecorator->SetClose(false);
			fDecorator->DrawClose();
		}
		else
		if(fIsMinimizing && location!=DEC_MINIMIZE)
		{
			fDecorator->SetMinimize(false);
			fDecorator->DrawMinimize();
		}
	}
	fLastMousePosition = pt;
}

void WinBorder::MouseUp(PortMessage *msg)
{
	if (!(Window()->IsLocked()))
		debugger("you must lock the attached ServerWindow object\n\t before calling WinBorder::MouseUp()\n");

	if (fIsMoving)
	{
		fIsMoving	= false;
		return;
	}
	if (fIsResizing)
	{
		fIsResizing	= false;
		return;
	}
	if (fIsZooming)
	{
		fIsZooming	= false;
		fDecorator->SetZoom(false);
		fDecorator->DrawZoom();
		Window()->Zoom();
		return;
	}
	if (fIsClosing)
	{
		fIsClosing	= false;
		fDecorator->SetClose(false);
		fDecorator->DrawClose();
		Window()->Quit();
		return;
	}
	if(fIsMinimizing)
	{
		fIsMinimizing = false;
		fDecorator->SetMinimize(false);
		fDecorator->DrawMinimize();
		Window()->Minimize(true);
		return;
	}
}

void WinBorder::HighlightDecorator(const bool &active)
{
	STRACE(("Decorator->Highlight\n"));
	fDecorator->SetFocus(active);
}

void WinBorder::Draw(const BRect &r)
{
	#ifdef DEBUG_WINBORDER
	printf("WinBorder(%s)::Draw() : ", GetName()));
	r.PrintToStream();
	#endif
	
	// if we have a visible region, it is decorator's one.
	if(fDecorator)
	{
/*
		//fUpdateReg.PrintToStream();
		RGBColor		c(128, 56, 98);
		//fDriver->FillRect(r, c);
		fDriver->FillRect(fUpdateReg.Frame(), c);
		snooze(1000000);
*/		
		fDecorator->Draw(fUpdateReg.Frame());
	}

	// clear background, *only IF* our view color is different to B_TRANSPARENT_COLOR!
	// TODO: DO That!
	// TODO: UNcomment
	// TODO: UPDATE  code 
	/*
	BMessage msg;
	msg.what = _UPDATE_;
	msg.AddInt32("_token", fViewToken);
	msg.AddRect("_rect", ConvertFromTop(reg.Frame()) );

	// for test purposes only!
	msg.AddRect("_rect2", reg.Frame());

	fServerWin->SendMessageToClient( &msg );
*/
}

void WinBorder::MoveBy(float x, float y)
{
	STRACE(("WinBorder(%s)::MoveBy()\n", GetName()));
	if(fDecorator)
		fDecorator->MoveBy(x,y);

	Layer::MoveBy(x,y);
}

void WinBorder::ResizeBy(float x, float y)
{
	STRACE(("WinBorder(%s)::ResizeBy()\n", GetName()));
	if(fDecorator)
		fDecorator->ResizeBy(x,y);

	Layer::ResizeBy(x,y);
}

bool WinBorder::HasPoint(BPoint& pt) const
{
	return fFullVisible.Contains(pt);
}

void WinBorder::SetMainWinBorder(WinBorder *newMain)
{
	fMainWinBorder = newMain;
}

WinBorder* WinBorder::MainWinBorder() const{
	return fMainWinBorder;
}

void WinBorder::SetLevel()
{
	switch(fServerWin->Feel())
	{
		case B_NORMAL_WINDOW_FEEL:
			fLevel	= B_NORMAL_FEEL;
			break;
		case B_FLOATING_SUBSET_WINDOW_FEEL:
			fLevel	= B_FLOATING_SUBSET_FEEL;
			break;
		case B_FLOATING_APP_WINDOW_FEEL:
			fLevel	= B_FLOATING_APP_FEEL;
			break;
		case B_FLOATING_ALL_WINDOW_FEEL:
			fLevel	= B_FLOATING_ALL_FEEL;
			break;
		case B_MODAL_SUBSET_WINDOW_FEEL:
			fLevel	= B_MODAL_SUBSET_FEEL;
			break;
		case B_MODAL_APP_WINDOW_FEEL:
			fLevel	= B_MODAL_APP_FEEL;
			break;
		case B_MODAL_ALL_WINDOW_FEEL:
			fLevel	= B_MODAL_ALL_FEEL;
			break;
		case B_SYSTEM_LAST:
		case B_SYSTEM_FIRST:

			// TODO: uncomment later when this code makes its way into the real server!
//			if(_win->ServerTeamID() != _win->ClientTeamID())
//				_win->QuietlySetFeel(B_NORMAL_WINDOW_FEEL);
//			else
				fLevel	= fServerWin->Feel();
			break;
		default:
			fServerWin->QuietlySetFeel(B_NORMAL_WINDOW_FEEL);
			fLevel	= B_NORMAL_FEEL;
			break;
	}
}

void WinBorder::AddToSubsetOf(WinBorder* main)
{
	STRACE(("WinBorder(%s)::AddToSubsetOf()\n", GetName()));
	if (!main || (main && !(main->GetRootLayer())))
		return;

	if (main->Window()->fWinFMWList.HasItem(this) || !(desktop->HasWinBorder(this)))
		return;

	if (main->Window()->Feel() == B_NORMAL_WINDOW_FEEL && 
		(Window()->Feel()==B_FLOATING_SUBSET_WINDOW_FEEL ||	Window()->Feel()==B_MODAL_SUBSET_WINDOW_FEEL) )
	{
		// if the main window is hidden also hide this one.
		if(main->IsHidden())
			fHidden = true;

		// add to main window's subset
		main->Window()->fWinFMWList.AddItem(this);

		// set this member accordingly
		fMainWinBorder = main;

		// because this window is in a subset it should appear in the
		// workspaces its main window appears in.
		Window()->QuietlySetWorkspaces(main->Window()->Workspaces());

		// this is a modal window, so add it to main window's workspaces.
		if (Window()->Feel() == B_MODAL_SUBSET_WINDOW_FEEL)
		{
			RootLayer		*rl = main->GetRootLayer();
			rl->fMainLock.Lock();
			rl->AddWinBorderToWorkspaces(this, main->Window()->Workspaces());
			rl->fMainLock.Unlock();
		}

		// this a *floating* window so if the main window is 'front', and add it to workspace.
		if ( !(main->IsHidden()) && Window()->Feel() == B_FLOATING_SUBSET_WINDOW_FEEL)
		{
			RootLayer		*rl = main->GetRootLayer();

			desktop->fGeneralLock.Lock();
			STRACE(("WinBorder(%s)::AddToSubsetOf(%s) - General lock acquired\n", GetName(), main->GetName()));

			rl->fMainLock.Lock();
			STRACE(("WinBorder(%s)::AddToSubsetOf(%s) - Main lock acquired\n", GetName(), main->GetName()));

			for(int32 i = 0; i < rl->WorkspaceCount(); i++)
			{
				Workspace	*ws = rl->WorkspaceAt(i+1);
				if(ws->FrontLayer() == main)
					ws->AddLayerPtr(this);
			}

			rl->fMainLock.Unlock();
			STRACE(("WinBorder(%s)::AddToSubsetOf(%s) - Main lock released\n", GetName(), main->GetName()));

			desktop->fGeneralLock.Unlock();
			STRACE(("WinBorder(%s)::AddToSubsetOf(%s) - General lock released\n", GetName(), main->GetName()));
		}
	}
}

void WinBorder::RemoveFromSubsetOf(WinBorder* main)
{
	STRACE(("WinBorder(%s)::RemoveFromSubsetOf()\n", GetName()));
	RootLayer		*rl = main->GetRootLayer();

	desktop->fGeneralLock.Lock();
	STRACE(("WinBorder(%s)::RemoveFromSubsetOf(%s) - General lock acquired\n", GetName(), main->GetName()));
	rl->fMainLock.Lock();
	STRACE(("WinBorder(%s)::RemoveFromSubsetOf(%s) - Main lock acquired\n", GetName(), main->GetName()));

	// remove from main window's subset list.
	if(main->Window()->fWinFMWList.RemoveItem(this))
	{
		int32	count = main->GetRootLayer()->WorkspaceCount();
		for(int32 i=0; i < count; i++)
		{
			if(main->Window()->Workspaces() & (0x00000001 << i))
			{
				Workspace	*ws = main->GetRootLayer()->WorkspaceAt(i+1);

				// if its main window is in 'i' workspaces, remove it from
				// workspace 'i' if it's in there...
				ws->RemoveLayerPtr(this);
			}
		}
	}
	fMainWinBorder	= NULL;

	rl->fMainLock.Unlock();
	STRACE(("WinBorder(%s)::RemoveFromSubsetOf(%s) - Main lock released\n", GetName(), main->GetName()));

	desktop->fGeneralLock.Unlock();
	STRACE(("WinBorder(%s)::RemoveFromSubsetOf(%s) - General lock released\n", GetName(), main->GetName()));
}

void WinBorder::PrintToStream()
{
	printf("\t%s", GetName());

	if (fLevel == B_FLOATING_SUBSET_FEEL)
		printf("\t%s", "B_FLOATING_SUBSET_WINDOW_FEEL");

	if (fLevel == B_FLOATING_APP_FEEL)
		printf("\t%s", "B_FLOATING_APP_WINDOW_FEEL");

	if (fLevel == B_FLOATING_ALL_FEEL)
		printf("\t%s", "B_FLOATING_ALL_WINDOW_FEEL");

	if (fLevel == B_MODAL_SUBSET_FEEL)
		printf("\t%s", "B_MODAL_SUBSET_WINDOW_FEEL");

	if (fLevel == B_MODAL_APP_FEEL)
		printf("\t%s", "B_MODAL_APP_WINDOW_FEEL");

	if (fLevel == B_MODAL_ALL_FEEL)
		printf("\t%s", "B_MODAL_ALL_WINDOW_FEEL");

	if (fLevel == B_NORMAL_FEEL)
		printf("\t%s", "B_NORMAL_WINDOW_FEEL");

	printf("\t%s\n", fHidden?"hidden" : "not hidden");
}

void WinBorder::UpdateColors(void)
{
	STRACE(("WinBorder %s: UpdateColors unimplemented\n",GetName()));
}

void WinBorder::UpdateDecorator(void)
{
	STRACE(("WinBorder %s: UpdateDecorator unimplemented\n",GetName()));
}

void WinBorder::UpdateFont(void)
{
	STRACE(("WinBorder %s: UpdateFont unimplemented\n",GetName()));
}

void WinBorder::UpdateScreen(void)
{
	STRACE(("WinBorder %s: UpdateScreen unimplemented\n",GetName()));
}
