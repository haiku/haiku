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
//					Adi Oanca <adioanca@myrealbox.com>
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
#define DEBUG_WINBORDER_CLICK

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

WinBorder::WinBorder(const BRect &r, const char *name, const int32 look, const int32 feel,
		const int32 flags, ServerWindow *win)
	: Layer(r, name, B_NULL_TOKEN, B_FOLLOW_NONE, flags, win)
{
	// unlike BViews, windows start off as hidden, so we need to tweak the hidecount
	// assignment made by Layer().
	_hidden			= true;

	_mbuttons		= 0;
	_kmodifiers		= 0;
	_win			= win;
	_update			= false;
	_hresizewin		= false;
	_vresizewin		= false;
	fLastMousePosition.Set(-1,-1);
	SetLevel();

	fMainWinBorder	= NULL;
	_decorator		= NULL;

	if (feel == B_NO_BORDER_WINDOW_LOOK)
	{
		_full			= _win->fTopLayer->_full;
		fDecFull		= NULL;
		fDecFullVisible	= NULL;
		fDecVisible		= NULL;
	}
	else
	{
		_decorator		= new_decorator(r, name, look, feel, flags, fDriver);
		fDecFull		= new BRegion();
		fDecVisible		= new BRegion();
		fDecFullVisible	= fDecVisible;

		_decorator->GetFootprint( fDecFull );
		
		// our full region is the union between decorator's region and fTopLayer's region
		_full			= _win->fTopLayer->_full;
		_full.Include( fDecFull );
	}

	// get a token
	_view_token	= border_token_handler.GetToken();

	STRACE(("WinBorder %s:\n",GetName()));
	STRACE(("\tFrame: (%.1f,%.1f,%.1f,%.1f)\n",r.left,r.top,r.right,r.bottom));
	STRACE(("\tWindow %s\n",win?win->Title():"NULL"));
}

WinBorder::~WinBorder(void)
{
STRACE(("WinBorder %s:~WinBorder()\n",GetName()));
	if (_decorator)	{
		delete _decorator;
		_decorator		= NULL;

		delete fDecFull;
		fDecFull		= NULL;

		delete fDecFullVisible; // NOTE: fDecFullVisible == fDecVisible
		fDecFullVisible	= NULL;
		fDecVisible		= NULL;
	}
}

void WinBorder::MouseDown(const BPoint &pt, const int32 &buttons, const int32 &modifiers)
{
	// user clicked on decorator
	if (fDecFullVisible->Contains(pt))
	{
		click_type	click;
		click = _decorator->Clicked(pt, buttons, modifiers);
		
		switch(click)
		{
			case DEC_CLOSE:
			{
				STRACE_CLICK(("WinBorder: Push Close Button\n"));
				_decorator->SetClose(true);
				_decorator->DrawClose();
				break;
			}
			case DEC_ZOOM:
			{
				STRACE_CLICK(("WinBorder: Push Zoom Button\n"));
				_decorator->SetZoom(true);
				_decorator->DrawZoom();
				break;
			}
			case DEC_MINIMIZE:
			{
				STRACE_CLICK(("WinBorder: Push Close Button\n"));
				_decorator->SetMinimize(true);
				_decorator->DrawMinimize();
				break;
			}
			case DEC_MOVETOBACK:
			{
				STRACE_CLICK(("WinBorder: MoveToBack\n"));
				MoveToBack();
				break;
			}
			default:{
				STRACE_CLICK(("WinBorder: MoveToFront\n"));
				MoveToFront();
				break;
			}
		}
	}
	// user clicked in window's area
	else
	{
		STRACE_CLICK(("WinBorder: MoveToFront 2\n"));
		bool		sendMessage = true;

		if (1 /* TODO: uncomment: ActiveLayer() != this*/)
		{
			MoveToFront();
			if (0 /* B_FIRST_CLICK? what's the name of that flaaaag ??? */){
				sendMessage = false;
			}
		}

		if (sendMessage)
		{
			Layer *targetLayer=_win->fTopLayer->LayerAt(pt);
			if(targetLayer)
			{
				BMessage		msg;

				// a tweak for converting a point into local coords. :-)
				BRect helpRect(pt.x, pt.y, pt.x+1, pt.y+1);
				msg.what = B_MOUSE_DOWN;
				msg.AddInt64("when", real_time_clock_usecs());
				msg.AddPoint("where", (targetLayer->ConvertFromTop(helpRect)).LeftTop() );
				msg.AddInt32("modifiers", modifiers);
				msg.AddInt32("buttons", buttons);
				msg.AddInt32("clicks", 1);
				
				_win->SendMessageToClient( &msg );
			}
		}
	}
		// this is important to determine how much we should resize or move the Layer(WinBorder)(window)	
	fLastMousePosition		= pt;
}

void WinBorder::MouseMoved(const BPoint &pt, const int32 &buttons)
{
	// Buffer data:
	// 1) int64 - time of mouse click
	// 2) float - x coordinate of mouse click
	// 3) float - y coordinate of mouse click
	// 4) int32 - buttons down
	
	click_type action = _decorator->Clicked(pt, _mbuttons, _kmodifiers);
	
	
	// If the user clicked a button and then moused away without lifting the button,
	// we don't want to trigger the button. Instead, we reset it to its original up state
	if(_decorator->GetClose() && action!=DEC_CLOSE)
	{
		_decorator->SetClose(false);
		_decorator->DrawClose();
	}
	if(_decorator->GetZoom() && action!=DEC_ZOOM)
	{
		_decorator->SetZoom(false);
		_decorator->DrawZoom();
	}
	if(_decorator->GetMinimize() && action!=DEC_MINIMIZE)
	{
		_decorator->SetMinimize(false);
		_decorator->DrawMinimize();
	}
	
	switch (action)
	{
		case DEC_DRAG:
		{
			STRACE_CLICK(("WinBorder: Drag\n"));
			BPoint		difference;
			difference	= pt - fLastMousePosition;
			
			MoveBy( difference.x, difference.y );
			break;
		}
		case DEC_RESIZE:
		{
			STRACE_CLICK(("WinBorder: Resize\n"));
			BPoint		difference;
			difference	= pt - fLastMousePosition;
			
			ResizeBy( difference.x, difference.y );
			break;
		}
		default:
		{
			BMessage msg;
			
			// a tweak for converting a point into local coords. :-)
			BRect helpRect(pt.x, pt.y, pt.x+1, pt.y+1);
			msg.what = B_MOUSE_MOVED;
			msg.AddInt64("when", real_time_clock_usecs());
			msg.AddPoint("where", (_win->fTopLayer->ConvertFromTop(helpRect)).LeftTop() );
			msg.AddInt32("buttons", buttons);
			
			_win->SendMessageToClient( &msg );
		}
	}
	// this is important to determine how much we should resize or move the 
	// Layer(WinBorder)(window)
	fLastMousePosition		= pt;
}

void WinBorder::MouseUp(const BPoint &pt, const int32 &modifiers)
{
	STRACE_MOUSE(("WinBorder %s: MouseUp() \n",GetName()));
	
	// buffer data:
	// 1) int64 - time of mouse click
	// 2) float - x coordinate of mouse click
	// 3) float - y coordinate of mouse click
	// 4) int32 - modifier keys down

	click_type action	=_decorator->Clicked(pt, _mbuttons, _kmodifiers);

	switch (action)
	{
		case DEC_CLOSE:
		{
			STRACE_CLICK(("WinBorder: Close\n"));
			
			if(_decorator->GetClose())
			{
				_decorator->SetClose(false);
				_decorator->DrawClose();
				
				BMessage msg(B_QUIT_REQUESTED);
				_win->SendMessageToClient(&msg);
			}
			break;
		}
		case DEC_ZOOM:
		{
			STRACE_CLICK(("WinBorder: Zoom\n"));

			if(_decorator->GetZoom())
			{
				_decorator->SetZoom(false);
				_decorator->DrawZoom();
				
				BMessage msg(B_ZOOM);
				_win->SendMessageToClient(&msg);
			}
			break;
		}
		case DEC_MINIMIZE:
		{
			STRACE_CLICK(("WinBoder: Minimize\n"));
			if(_decorator->GetMinimize())
			{
				_decorator->SetMinimize(false);
				_decorator->DrawMinimize();
				
				BMessage msg(B_MINIMIZE);
				_win->SendMessageToClient(&msg);
			}
			break;
		}
		default:
		{
			Layer *targetLayer=_win->fTopLayer->LayerAt(pt);
			if(targetLayer)
			{
				BMessage msg;
				
				// a tweak for converting a point into local coords. :-)
				BRect helpRect(pt.x, pt.y, pt.x+1, pt.y+1);
				msg.what = B_MOUSE_UP;
				msg.AddInt64("when", real_time_clock_usecs());
				msg.AddPoint("where", (targetLayer->ConvertFromTop(helpRect)).LeftTop() );
				msg.AddInt32("modifiers", modifiers);
				
				_win->SendMessageToClient( &msg );
			}
		}
	}
}

void WinBorder::Show(){
	if( !_hidden )
		return;
	
	_hidden		= false;
}

void WinBorder::Hide(){
	if ( _hidden )
		return;

	_hidden		= true;
}

/*!
	\brief Function to pass focus value on to decorator
	\param active Focus flag
*/
void WinBorder::SetFocus(const bool &active)
{
	_decorator->SetFocus(active);
}

void WinBorder::RebuildRegions( const BRect& r ){
	/* WinBorder is a little bit special. It doesn't have a visible region
		in which to do its drawings. Instead the whole visible region is split
		between decorator and top_layer.
	 */

		// if we're in the rebuild area, rebuild our v.r.
	if ( _full.Intersects( r ) ){
		_visible			= _full;

		if (_parent){
			_visible.IntersectWith( &(_parent->_visible) );
				// exclude from parent's visible area.
			if ( !(_hidden) )
				_parent->_visible.Exclude( &(_visible) );
		}

		_fullVisible		= _visible;
	}
	else{
			// our visible region will stay the same

			// exclude our FULL visible region from parent's visible region.
		if ( !(_hidden) && _parent )
			_parent->_visible.Exclude( &(_fullVisible) );

			// we're not in the rebuild area so our children's v.r.s are OK. 
		return;
	}

		// rebuild top_layer:
	if ( _win->fTopLayer->_full.Intersects( r ) ){
			// build top_layer's visible region by intersecting its _full with winborder's _visible region.
		_win->fTopLayer->_visible		= _win->fTopLayer->_full;
		_win->fTopLayer->_visible.IntersectWith( &(_visible) );

			// then exclude it from winborder's _visible... 
		_visible.Exclude( &(_win->fTopLayer->_visible) );

		_win->fTopLayer->_fullVisible	= _win->fTopLayer->_visible;

			// Rebuild regions for children...
		for(Layer *lay = _win->fTopLayer->_bottomchild; lay != NULL; lay = lay->_uppersibling){
			if ( !(lay->_hidden) ){
				lay->RebuildRegions( r );
			}
		}
	}
	else{
		_visible.Exclude( &(_win->fTopLayer->_fullVisible) );
	}

		// rebuild decorator.
	if (_decorator){
		if ( fDecFull->Intersects( r ) ){
			*fDecVisible			= *fDecFull;
			fDecVisible->IntersectWith( &(_visible) );

			_visible.Exclude( fDecVisible );

				// !!!! Pointer assignement !!!!
			fDecFullVisible		= fDecVisible;
		}
		else{
			_visible.Exclude( fDecFullVisible );
		}
	}
}

void WinBorder::Draw(const BRect &r)
{
//TODO: REMOVE this! For Test purposes only!
	STRACE(("*WinBorder(%s)::Draw()\n", GetName()));
	_decorator->Draw();
	STRACE(("#WinBorder(%s)::Draw() ENDED\n", GetName()));
	return;
//----------------

		// draw the decorator
	BRegion			reg(r);
	if (_decorator){
		reg.IntersectWith( fDecVisible );
		if (reg.CountRects() > 0){
			_decorator->Draw( reg.Frame() );
		}
	}

		// draw the top_layer
	reg.Set( r );
	reg.IntersectWith( &(_win->fTopLayer->_visible) );
	if (reg.CountRects() > 0){
		_win->fTopLayer->RequestClientUpdate( reg.Frame() );
	}
}

void WinBorder::MoveBy(float x, float y)
{
	BRegion		oldFullVisible( _fullVisible );
//	BPoint		oldFullVisibleOrigin( _fullVisible.Frame().LeftTop() );

	_frame.OffsetBy(x, y);
	_full.OffsetBy(x, y);

	_win->fTopLayer->_frame.OffsetBy(x, y);
	_win->fTopLayer->MoveRegionsBy(x, y);

	if (_decorator){
			// allow decorator to make its internal calculations.
		_decorator->MoveBy(x, y);

		fDecFull->OffsetBy(x, y);
	}

	if ( !_hidden ){
			// to clear the area occupied on its parent _visible
		if (_fullVisible.CountRects() > 0){
			_hidden		= true;
			_parent->RebuildChildRegions( _fullVisible.Frame(), this );
			_hidden		= false;
		}
		_parent->RebuildChildRegions( _full.Frame(), this );
	}

		// REDRAWING CODE:

	if ( !(_hidden) )
	{
			/* The region(on screen) that will be invalidated.
			 *	It is composed by:
			 *		the regions that were visible, and now they aren't +
			 *		the regions that are now visible, and they were not visible before.
			 *	(oldFullVisible - _fullVisible) + (_fullVisible - oldFullVisible)
			 */
		BRegion			clipReg;

			// first offset the old region so we can do the correct operations.
		oldFullVisible.OffsetBy(x, y);

			// + (oldFullVisible - _fullVisible)
		if ( oldFullVisible.CountRects() > 0 ){
			BRegion		tempReg( oldFullVisible );
			tempReg.Exclude( &_fullVisible );

			if (tempReg.CountRects() > 0){
				clipReg.Include( &tempReg );
			}
		}

			// + (_fullVisible - oldFullVisible)
		if ( _fullVisible.CountRects() > 0 ){
			BRegion		tempReg( _fullVisible );
			tempReg.Exclude( &oldFullVisible );

			if (tempReg.CountRects() > 0){
				clipReg.Include( &tempReg );
			}
		}

			// there is no point in redrawing what already is visible. So just copy
			// on-screen pixels to layer's new location.
		if ( (oldFullVisible.CountRects() > 0) && (_fullVisible.CountRects() > 0) ){
			BRegion		tempReg( oldFullVisible );
			tempReg.IntersectWith( &_fullVisible );

			if (tempReg.CountRects() > 0){
// TODO: when you have such a method in DisplayDriver/Clipper, uncomment!
				//fDriver->CopyBits( &tempReg, oldFullVisibleOrigin, oldFullVisibleOrigin.OffsetByCopy(x,y) );
			}
		}

			// invalidate 'clipReg' so we can see the results of this move.
		if (clipReg.CountRects() > 0){
			Invalidate( clipReg );
		}
	}
}

void WinBorder::ResizeBy(float x, float y)
{
	BRegion		oldFullVisible( _fullVisible );

	_frame.right	= _frame.right + x;
	_frame.bottom	= _frame.bottom + y;

	_win->fTopLayer->ResizeRegionsBy(x, y);

	_full			= _win->fTopLayer->_full;

	if (_decorator){
			// allow decorator to make its internal calculations.
		_decorator->ResizeBy(x, y);

		_decorator->GetFootprint( fDecFull );

		_full.Include( fDecFull );
	}

	if ( !_hidden ){
		_parent->RebuildChildRegions( _full.Frame(), this );
	}


		// REDRAWING CODE:

	if ( !(_hidden) )
	{
			/* The region(on screen) that will be invalidated.
			 *	It is composed by:
			 *		the regions that were visible, and now they aren't +
			 *		the regions that are now visible, and they were not visible before.
			 *	(oldFullVisible - _fullVisible) + (_fullVisible - oldFullVisible)
			 */
		BRegion			clipReg;

			// + (oldFullVisible - _fullVisible)
		if ( oldFullVisible.CountRects() > 0 ){
			BRegion		tempReg( oldFullVisible );
			tempReg.Exclude( &_fullVisible );

			if (tempReg.CountRects() > 0){
				clipReg.Include( &tempReg );
			}
		}

			// + (_fullVisible - oldFullVisible)
		if ( _fullVisible.CountRects() > 0 ){
			BRegion		tempReg( _fullVisible );
			tempReg.Exclude( &oldFullVisible );

			if (tempReg.CountRects() > 0){
				clipReg.Include( &tempReg );
			}
		}

			// invalidate 'clipReg' so we can see the results of this resize operation.
		if (clipReg.CountRects() > 0){
			Invalidate( clipReg );
		}
	}
}

bool WinBorder::HasPoint(BPoint pt) const{
	return _full.Contains(pt);
}

void WinBorder::MoveToBack(){
// TODO: take care of focus.
	if (this == _parent->_topchild)
		return;

	if (_uppersibling){
		_uppersibling->_lowersibling	= _lowersibling;
	}
	else{
		_parent->_topchild				= _lowersibling;
	}
	
	if (_lowersibling){
		_lowersibling->_uppersibling	= _uppersibling;
	}
	else{
		_parent->_bottomchild			= _uppersibling;
	}
	
	this->_lowersibling		= _parent->_topchild;
	this->_uppersibling		= NULL;
	_parent->_topchild		= this;

		// cache WinBorder's fullVisible region for use later
	BRegion			cachedFullVisible = _fullVisible;
		// rebuild only that area the encloses the full visible part of this layer.
	_parent->RebuildChildRegions(_fullVisible.Frame(), this);
		// we now have <= fullVisible region, so invalidate the parts that are not common.
	cachedFullVisible.Exclude(&_fullVisible);
	Invalidate( cachedFullVisible );
}

void WinBorder::MoveToFront(){
// TODO: take care of focus.
	if (this == _parent->_bottomchild)
		return;

	if (_uppersibling){
		_uppersibling->_lowersibling	= _lowersibling;
	}
	else{
		_parent->_topchild				= _lowersibling;
	}
	
	if (_lowersibling){
		_lowersibling->_uppersibling	= _uppersibling;
	}
	else{
		_parent->_bottomchild			= _uppersibling;
	}
	
	this->_lowersibling		= NULL;
	this->_uppersibling		= _parent->_bottomchild;
	_parent->_bottomchild	= this;

		// cache WinBorder's fullVisible region for use later
	BRegion			cachedFullVisible	= _fullVisible;
		// rebuild the area that this WinBorder will occupy.
	_parent->RebuildChildRegions(_full.Frame(), this);
		// make a copy of the fullVisible region because it needs
		// to be modified for invalidating a minimum area.
	BRegion			tempFullVisible		= _fullVisible;
		// invalidate only the difference between the present and
		// the old fullVisible regions. We only need to update that area.
	tempFullVisible.Exclude(&cachedFullVisible);
	Invalidate( tempFullVisible );
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
	switch(_win->Feel()){
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
				_level	= _win->Feel();
			break;
		default:
			_win->QuietlySetFeel(B_NORMAL_WINDOW_FEEL);
			_level	= B_NORMAL_FEEL;
			break;
	}
}
//---------------------------------------------------------------------------
void WinBorder::AddToSubsetOf(WinBorder* main){
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
/*		if (Window()->Feel() == B_FLOATING_SUBSET_WINDOW_FEEL)
			printf("\t%s", "B_FLOATING_SUBSET_WINDOW_FEEL");
		if (Window()->Feel() == B_FLOATING_APP_WINDOW_FEEL)
			printf("\t%s", "B_FLOATING_APP_WINDOW_FEEL");
		if (Window()->Feel() == B_FLOATING_ALL_WINDOW_FEEL)
			printf("\t%s", "B_FLOATING_ALL_WINDOW_FEEL");
		if (Window()->Feel() == B_MODAL_SUBSET_WINDOW_FEEL)
			printf("\t%s", "B_MODAL_SUBSET_WINDOW_FEEL");
		if (Window()->Feel() == B_MODAL_APP_WINDOW_FEEL)
			printf("\t%s", "B_MODAL_APP_WINDOW_FEEL");
		if (Window()->Feel() == B_MODAL_ALL_WINDOW_FEEL)
			printf("\t%s", "B_MODAL_ALL_WINDOW_FEEL");
		if (Window()->Feel() == B_NORMAL_WINDOW_FEEL)
			printf("\t%s", "B_NORMAL_WINDOW_FEEL");
*/
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
//	_full.PrintToStream();
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
