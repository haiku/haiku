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
TokenHandler	border_token_handler;

bool			gMouseDown = false;

//---------------------------------------------------------------------------
WinBorder::WinBorder(const BRect &r, const char *name, const int32 look, const int32 feel,
		const int32 flags, ServerWindow *win, DisplayDriver *driver)
	: Layer(r, name, B_NULL_TOKEN, B_FOLLOW_NONE, flags, driver)
{
	// unlike BViews, windows start off as hidden
	_hidden			= true;
	_serverwin		= win;

	fMouseButtons	= 0;
	fKeyModifiers	= 0;
	fMainWinBorder	= NULL;
	fDecorator		= NULL;
	fDecFull		= NULL;

	fIsMoving		= false;
	fIsResizing		= false;
	fIsClosing		= false;
	fIsMinimizing	= false;
	fIsZooming		= false;

	fLastMousePosition.Set(-1,-1);
	SetLevel();
	fNewTopLayerFrame = &(win->fTopLayer->_frame);

	if (feel == B_NO_BORDER_WINDOW_LOOK){
	}
	else{
		fDecorator		= new_decorator(r, name, look, feel, flags, fDriver);
		fDecFull		= new BRegion();
		fDecorator->GetFootprint(fDecFull);
	}

	_full.MakeEmpty();

	STRACE(("WinBorder %s:\n",GetName()));
	STRACE(("\tFrame: (%.1f,%.1f,%.1f,%.1f)\n",r.left,r.top,r.right,r.bottom));
	STRACE(("\tWindow %s\n",win?win->Title():"NULL"));
}
//---------------------------------------------------------------------------
WinBorder::~WinBorder(void)
{
STRACE(("WinBorder(%s):~WinBorder()\n",GetName()));
	if (fDecorator)	{
		delete fDecorator;
		fDecorator		= NULL;

		delete fDecFull;
		fDecFull		= NULL;
	}
}
//---------------------------------------------------------------------------
void WinBorder::RebuildFullRegion(void){
STRACE(("WinBorder(%s):~RebuildFullRegion()\n",GetName()));
	BRegion			topLayerFull;
	Layer			*topLayer = _serverwin->fTopLayer;
	topLayerFull.Set( ConvertToTop(*fNewTopLayerFrame) );
	fNewTopLayerFrame = &(_serverwin->fTopLayer->_frame);

// TODO: Convert to screen coordinates!
	LayerData	*ld;
	ld			= topLayer->_layerdata;
	do{
			// clip to user region
		if(ld->clipReg)
			topLayerFull.IntersectWith( ld->clipReg );
	} while( (ld = ld->prevState) );

		// clip to user picture region
	if(topLayer->clipToPicture)
		if(topLayer->clipToPictureInverse) {
			topLayerFull.Exclude( topLayer->clipToPicture );
		}
		else{
			topLayerFull.IntersectWith( topLayer->clipToPicture );
		}

	_full.MakeEmpty();
	_full		= topLayerFull;

	if (fDecorator){
		fDecFull->MakeEmpty();
		fDecorator->GetFootprint(fDecFull);
		_full.Include(fDecFull);
	}
}
//---------------------------------------------------------------------------
void WinBorder::MouseDown(const BPoint &pt, const int32 &buttons, const int32 &modifiers)
{
	// this is important to determine how much we should resize or move the Layer(WinBorder)(window)

	// user clicked on WinBorder's visible region, which is in fact decorator's.
	// so, if true, we find out if the user clicked the decorator.
	Layer	*target = LayerAt(pt);
	if (target == this){
		click_type		action;
		// find out where user clicked in Decorator
		action			= fDecorator->Clicked(pt, buttons, modifiers);
		switch(action){
/*
TODO: add methods like DrawCloseBtnDown(true/false) to let Decorator draw "down" buttoms
	for closing and zooming for example. Call them here!
*/
			case DEC_CLOSE:
				fIsClosing = true;
//				fDecorator->DrawClosingBtnDown(true);
STRACE_CLICK(("===> DEC_CLOSE\n"));
				break;
			case DEC_ZOOM:
				fIsZooming = true;
//				fDecorator->DrawZoomBtnDown(true);
STRACE_CLICK(("===> DEC_ZOOM\n"));
				break;
			case DEC_RESIZE:
				fIsResizing = true;
//				fDecorator->DrawResizingBtnDown(true);
STRACE_CLICK(("===> DEC_RESIZE\n"));
				break;
			case DEC_DRAG:
				fIsMoving = true;
//				fDecorator->DrawMovingBtnDown(true);
STRACE_CLICK(("===> DEC_DRAG\n"));
				break;
			case DEC_MOVETOBACK:
				GetRootLayer()->ActiveWorkspace()->MoveToBack(this);
				break;
			case DEC_NONE:
				debugger("WinBorder::MouseDown - Decorator should NOT return DEC_NONE\n");
				break;
			default:
				debugger("WinBorder::MouseDown - Decorator returned UNKNOWN code\n");
				break;
		}
	}
	else{
/*
TODO: implement!
	The problem here is that, there is no way to get the required data for the B_MOUSE_DOWN
	message. There should be a method like BWindow::CurrentMessage() to get the input message
	parameters. The problem is that Poller currently use PortLink message system, and there
	is no way to get the paramets we need(e.g. when, noOfClicks).
	This yelds PortLink messages from input_server to be replaced by regulat BMessages!!!
*/
/*		BMessage	msg;
		msg->what		= B_MOUSE_DOWN;
		msg->AddInt64("when", when);
		msg->AddPoint("where", where);
		msg->AddInt32("modifiers", modifiers);
		msg->AddInt32("buttons", buttons);
		msg->AddInt32("clicks", noOfClicks);
		
		msg->AddInt32("token", token); ??? // Have a look into BLooper::task_looper()!!!
		Window()->SendMessageToClient(msg);
*/
	}
	
	fLastMousePosition		= pt;
}
//---------------------------------------------------------------------------
void WinBorder::MouseMoved(const BPoint &pt, const int32 &buttons)
{
	if (fIsMoving){
STRACE_CLICK(("===> Moving...\n"));
		BPoint		offset = pt;
		offset		-= fLastMousePosition;
		MoveBy(offset.x, offset.y);
		goto MMend;
	}
	if (fIsResizing){
STRACE_CLICK(("===> Resizing...\n"));
		BPoint		offset = pt;
		offset		-= fLastMousePosition;
		ResizeBy(offset.x, offset.y);
		goto MMend;
	}
	if (fIsZooming){
/*
TODO: implement!
	Add what you need to the Decorator API.

		if (fDecorator->GetZoomRegion().Contains(pt)){
			// do nothing! Mouse still inside the zooming region.
		}
		else{
			fDecorator->DrawZoomBtnDown(false);
		}
		goto MMend;
*/
	}
	if (fIsClosing){
/*
TODO: implement!
	Add what you need to the Decorator API.

		if (fDecorator->GetZoomRegion().Contains(pt)){
			// do nothing! Mouse still inside the zooming region.
		}
		else{
			fDecorator->DrawCloseBtnDown(false);
		}
		goto MMend;
*/
	}

	MMend:
	fLastMousePosition		= pt;
}
//---------------------------------------------------------------------------
void WinBorder::MouseUp(const BPoint &pt, const int32 &modifiers)
{
	if (fIsMoving){
		fIsMoving	= false;
//		DrawMovingBtnDown(false);
		return;
	}
	if (fIsResizing){
		fIsResizing	= false;
//		DrawResisingBtnDown(false);
		return;
	}
	if (fIsZooming){
		fIsZooming	= false;
//		DrawZoomBtnDown(false);
		return;
	}
	if (fIsClosing){
		fIsClosing	= false;
//		DrawCloseBtnDown(false);
		return;
	}
}
//---------------------------------------------------------------------------
void WinBorder::HighlightDecorator(const bool &active)
{
	fDecorator->SetFocus(active);
}
//---------------------------------------------------------------------------
void WinBorder::Draw(const BRect &r)
{
STRACE(("WinBorder(%s)::Draw()\n", GetName()));
	if(fDecorator){
		// decorator is allowed to draw in its entire visible region, not just in the update one.
		fUpdateReg		= _visible;
		fUpdateReg.IntersectWith(fDecFull);
		// restrict Decorator drawing to the update region only.
		fDriver->ConstrainClippingRegion(&fUpdateReg);
/*
fUpdateReg.PrintToStream();
RGBColor		c(128, 56, 98);
//fDriver->FillRect(r, c);
fDriver->FillRect(fUpdateReg.Frame(), c);
snooze(1000000);
*/
// TODO: pass 'r' not as you do now!!! Let Decorator object handle update problems
		fDecorator->Draw(fUpdateReg.Frame());

		// remove the additional clipping region.
		fDriver->ConstrainClippingRegion(NULL);
	}
}
//---------------------------------------------------------------------------
void WinBorder::MoveBy(float x, float y)
{
STRACE(("WinBorder(%s)::MoveBy()\n", GetName()));
	if(fDecorator){
		fDecorator->MoveBy(x,y);
		fDecFull->OffsetBy(x,y);
	}
	
	Layer::MoveBy(x,y);
}
//---------------------------------------------------------------------------
void WinBorder::ResizeBy(float x, float y)
{
STRACE(("WinBorder(%s)::ResizeBy()\n", GetName()));
	if(fDecorator){
		fDecorator->ResizeBy(x,y);
	}
	BRect		*localRect = new BRect(_serverwin->fTopLayer->_frame);
	fNewTopLayerFrame			= localRect;
	// force topLayer's frame to resize
	fNewTopLayerFrame->right	+= x;
	fNewTopLayerFrame->bottom	+= y;

	Layer::ResizeBy(x,y);
	delete localRect;
}
//---------------------------------------------------------------------------
bool WinBorder::HasPoint(BPoint& pt) const{
	return _fullVisible.Contains(pt);
}
//---------------------------------------------------------------------------
void WinBorder::SetMainWinBorder(WinBorder *newMain){
	fMainWinBorder = newMain;
}
//---------------------------------------------------------------------------
WinBorder* WinBorder::MainWinBorder() const{
	return fMainWinBorder;
}
//---------------------------------------------------------------------------
void WinBorder::SetLevel(){
	switch(_serverwin->Feel()){
		case B_NORMAL_WINDOW_FEEL:
			_level	= B_NORMAL_FEEL;
			break;
		case B_FLOATING_SUBSET_WINDOW_FEEL:
			_level	= B_FLOATING_SUBSET_FEEL;
			break;
		case B_FLOATING_APP_WINDOW_FEEL:
			_level	= B_FLOATING_APP_FEEL;
			break;
		case B_FLOATING_ALL_WINDOW_FEEL:
			_level	= B_FLOATING_ALL_FEEL;
			break;
		case B_MODAL_SUBSET_WINDOW_FEEL:
			_level	= B_MODAL_SUBSET_FEEL;
			break;
		case B_MODAL_APP_WINDOW_FEEL:
			_level	= B_MODAL_APP_FEEL;
			break;
		case B_MODAL_ALL_WINDOW_FEEL:
			_level	= B_MODAL_ALL_FEEL;
			break;
		case B_SYSTEM_LAST:
		case B_SYSTEM_FIRST:
// TODO: uncomment later when this code makes its way into the real server!
//			if(_win->ServerTeamID() != _win->ClientTeamID())
//				_win->QuietlySetFeel(B_NORMAL_WINDOW_FEEL);
//			else
				_level	= _serverwin->Feel();
			break;
		default:
			_serverwin->QuietlySetFeel(B_NORMAL_WINDOW_FEEL);
			_level	= B_NORMAL_FEEL;
			break;
	}
}
//---------------------------------------------------------------------------
void WinBorder::AddToSubsetOf(WinBorder* main){
STRACE(("WinBorder(%s)::AddToSubsetOf()\n", GetName()));
	if (!main || (main && !(main->GetRootLayer())))
		return;

	if (main->Window()->fWinFMWList.HasItem(this) || !(desktop->HasWinBorder(this)))
		return;

	if (main->Window()->Feel() == B_NORMAL_WINDOW_FEEL
			&& ( Window()->Feel() == B_FLOATING_SUBSET_WINDOW_FEEL
				|| Window()->Feel() == B_MODAL_SUBSET_WINDOW_FEEL)
		)
	{
			// if the main window is hidden also hide this one.
		if(main->IsHidden())
			_hidden = true;
			// add to main window's subset
		main->Window()->fWinFMWList.AddItem(this);
			// set this member accordingly
		fMainWinBorder = main;
			// because this window is in a subset it should appear in the
			// workspaces its main window appears in.
		Window()->QuietlySetWorkspaces(main->Window()->Workspaces());
			// this is a *modal* window, so add it to main windows workspaces.
		if (Window()->Feel() == B_MODAL_SUBSET_WINDOW_FEEL){
			RootLayer		*rl = main->GetRootLayer();
			rl->fMainLock.Lock();
			rl->AddWinBorderToWorkspaces(this, main->Window()->Workspaces());
			rl->fMainLock.Unlock();
		}
			// this a *floating* window so if the main window is 'front',
			// 	add it to workspace.
		if ( !(main->IsHidden()) && Window()->Feel() == B_FLOATING_SUBSET_WINDOW_FEEL){
			RootLayer		*rl = main->GetRootLayer();

			desktop->fGeneralLock.Lock();
			STRACE(("WinBorder(%s)::AddToSubsetOf(%s) - General lock acquired\n", GetName(), main->GetName()));
			rl->fMainLock.Lock();
			STRACE(("WinBorder(%s)::AddToSubsetOf(%s) - Main lock acquired\n", GetName(), main->GetName()));

			for(int32 i = 0; i < rl->WorkspaceCount(); i++){
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
//---------------------------------------------------------------------------
void WinBorder::RemoveFromSubsetOf(WinBorder* main){
STRACE(("WinBorder(%s)::RemoveFromSubsetOf()\n", GetName()));
	RootLayer		*rl = main->GetRootLayer();

	desktop->fGeneralLock.Lock();
	STRACE(("WinBorder(%s)::RemoveFromSubsetOf(%s) - General lock acquired\n", GetName(), main->GetName()));
	rl->fMainLock.Lock();
	STRACE(("WinBorder(%s)::RemoveFromSubsetOf(%s) - Main lock acquired\n", GetName(), main->GetName()));
		// remove from main window's subset list.
	if(main->Window()->fWinFMWList.RemoveItem(this)){
		int32	count = main->GetRootLayer()->WorkspaceCount();
		for(int32 i=0; i < count; i++){
			if(main->Window()->Workspaces() & (0x00000001 << i)){
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
//---------------------------------------------------------------------------
void WinBorder::PrintToStream(){
	printf("\t%s", GetName());
		if (_level == B_FLOATING_SUBSET_FEEL)
			printf("\t%s", "B_FLOATING_SUBSET_WINDOW_FEEL");
		if (_level == B_FLOATING_APP_FEEL)
			printf("\t%s", "B_FLOATING_APP_WINDOW_FEEL");
		if (_level == B_FLOATING_ALL_FEEL)
			printf("\t%s", "B_FLOATING_ALL_WINDOW_FEEL");
		if (_level == B_MODAL_SUBSET_FEEL)
			printf("\t%s", "B_MODAL_SUBSET_WINDOW_FEEL");
		if (_level == B_MODAL_APP_FEEL)
			printf("\t%s", "B_MODAL_APP_WINDOW_FEEL");
		if (_level == B_MODAL_ALL_FEEL)
			printf("\t%s", "B_MODAL_ALL_WINDOW_FEEL");
		if (_level == B_NORMAL_FEEL)
			printf("\t%s", "B_NORMAL_WINDOW_FEEL");

	printf("\t%s\n", _hidden?"hidden" : "not hidden");
}
//---------------------------------------------------------------------------
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
