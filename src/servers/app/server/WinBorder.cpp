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
//	Description:	Layer subclass which handles window management
//  
//------------------------------------------------------------------------------
#include <Region.h>
#include <String.h>
#include <Locker.h>
#include <Region.h>
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

namespace winborder_private
{
	bool is_moving_window		= false;
	bool is_resizing_window		= false;
	bool is_sliding_tab			= false;
	WinBorder *active_winborder	= NULL;
};

//! TokenHandler object used to provide IDs for all WinBorder objects
TokenHandler	border_token_handler;

extern ServerWindow	*active_serverwindow;

bool	is_moving_window(void) { return winborder_private::is_moving_window; }
void	set_is_moving_window(bool state) { winborder_private::is_moving_window=state; }
bool	is_resizing_window(void) { return winborder_private::is_resizing_window; }
void	set_is_resizing_window(bool state) { winborder_private::is_resizing_window=state; }
bool	is_sliding_tab(void) { return winborder_private::is_sliding_tab; }
void	set_is_sliding_tab(bool state) { winborder_private::is_sliding_tab=state; }
void	set_active_winborder(WinBorder *win) { winborder_private::active_winborder=win; }
WinBorder*	get_active_winborder(void) { return winborder_private::active_winborder; }

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
	_mousepos.Set(0,0);

	_decorator		= NULL;

	if (feel == B_NO_BORDER_WINDOW_LOOK){
		_full			= _win->top_layer->_full;
		fDecFull		= NULL;
		fDecFullVisible	= NULL;
		fDecVisible		= NULL;
	}
	else{
		_decorator		= new_decorator(r, name, look, feel, flags, fDriver);
		fDecFull		= new BRegion();
		fDecVisible		= new BRegion();
		fDecFullVisible	= fDecVisible;

		_decorator->GetFootprint( fDecFull );

			// our full region is the union between decorator's region and top_layer's region
		_full			= _win->top_layer->_full;
		_full.Include( fDecFull );
	}

		// get a token
	_view_token		= border_token_handler.GetToken();

STRACE(("WinBorder %s:\n",_title->String()));
STRACE(("\tFrame: (%.1f,%.1f,%.1f,%.1f)\n",r.left,r.top,r.right,r.bottom));
STRACE(("\tWindow %s\n",win?win->Title():"NULL"));
}

WinBorder::~WinBorder(void)
{
STRACE(("WinBorder %s:~WinBorder()\n",_title->String()));
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

void WinBorder::MouseDown(int8 *buffer)
{
	// Buffer data:
	// 1) int64 - time of mouse click
	// 2) float - x coordinate of mouse click
	// 3) float - y coordinate of mouse click
	// 4) int32 - modifier keys down
	// 5) int32 - buttons down
	// 6) int32 - clicks
	int8	*index		= buffer;			index+=sizeof(int64);
	float	x			= *((float*)index);	index+=sizeof(float);
	float	y			= *((float*)index);	index+=sizeof(float);
	int32	modifiers	= *((int32*)index);	index+=sizeof(int32);
	int32	buttons		= *((int32*)index);

	BPoint pt(x,y);

		// user clicked on decorator
	if (fDecFullVisible->Contains(pt)){
		click_type	click;
		click		= _decorator->Clicked(pt, buttons, modifiers);
		
		switch(click){
			case CLICK_MOVETOBACK:
			{
				STRACE_CLICK(("Click: MoveToBack\n"));
				MoveToBack();
				break;
			}
			case CLICK_MOVETOFRONT:
			{
				STRACE_CLICK(("Click: MoveToFront\n"));
				MoveToFront();
				break;
			}
			case CLICK_CLOSE:
			{
				STRACE_CLICK(("Click: Close\n"));
				RemoveSelf();
				break;
			}
			case CLICK_ZOOM:
			{
				STRACE_CLICK(("Click: Zoom\n"));
				// TODO: implement
				// if (actual_coods != max_zoom_coords)
				// MoveBy(X, Y);
				// ResizeBy(X, Y);
				break;
			}
			case CLICK_MINIMIZE:
			{
				STRACE_CLICK(("Click: Minimize\n"));
				if ( !IsHidden() ){
					Hide();
				}
				break;
			}
			case CLICK_DRAG:
			{
				STRACE_CLICK(("Click: Drag\n"));
				set_is_moving_window(true);
				break;
			}
			case CLICK_SLIDETAB:
			{
				STRACE_CLICK(("Click: Slide Tab\n"));
				set_is_sliding_tab(true);
				break;
			}
			case CLICK_RESIZE:
			{
				STRACE_CLICK(("Click: Resize\n"));
				set_is_resizing_window(true);
				break;
			}
			case CLICK_NONE:
			default:
			{
				break;
			}
		}
	}
		// user clicked in window's area
	else{
		/*
		TODO: implement!!!
		NOTE: send that message to BViews registered for receiving mouse&keyboard events.
		SendMouseMessage( B_MOUSE_DOWN );
		*/
	}
}

void WinBorder::MouseMoved(int8 *buffer)
{
	// Buffer data:
	// 1) int64 - time of mouse click
	// 2) float - x coordinate of mouse click
	// 3) float - y coordinate of mouse click
	// 4) int32 - buttons down
	int8 *index=buffer; index+=sizeof(int64);
	float x=*((float*)index); index+=sizeof(float);
	float y=*((float*)index); index+=sizeof(float);
	int32 buttons=*((int32*)index);

	BPoint pt(x,y);
	click_type click=_decorator->Clicked(pt, _mbuttons, _kmodifiers);

	if(click!=CLICK_CLOSE && _decorator->GetClose())
	{
		_decorator->SetClose(false);
		_decorator->Draw();
	}	

	if(click!=CLICK_ZOOM && _decorator->GetZoom())
	{
		_decorator->SetZoom(false);
		_decorator->Draw();
	}	

	if(click!=CLICK_MINIMIZE && _decorator->GetMinimize())
	{
		_decorator->SetMinimize(false);
		_decorator->Draw();
	}	

	if(is_sliding_tab())
	{
STRACE_CLICK(("ClickMove: Slide Tab\n"));
		float dx=pt.x-_mousepos.x;
		float dy=pt.y-_mousepos.y;		

		if(dx != 0 || dy != 0)
		{
			// SlideTab returns how much things were moved, and currently
			// supports just the x direction, so get the value so
			// we can invalidate the proper area.
			lock_layers();
			_parent->Invalidate(_decorator->SlideTab(dx,dy));
//			_parent->RequestDraw();
//			_decorator->DrawTab();
			unlock_layers();
		}
	}


	if(is_moving_window())
	{
		// We are moving the window. Because speed is of the essence, we need to handle a lot
		// of stuff which we might otherwise not need to.

STRACE_CLICK(("ClickMove: Drag\n"));
		float	dx	= pt.x - _mousepos.x,
				dy	= pt.y - _mousepos.y;
		if(buttons!=0 && (dx!=0 || dy!=0))
		{
			// 2) Offset necessary data members
			_clientframe.OffsetBy(dx,dy);

			_win->Lock();
			_win->_frame.OffsetBy(dx,dy);
			_win->Unlock();

			lock_layers();

			// Move the window decorator's footprint and remove the area occupied
			// by the new location so we know what areas to invalidate.
			
			// The original location
			BRegion reg;
			_decorator->GetFootprint(&reg);
			
			// The new location
			BRegion reg2(reg);
			reg2.OffsetBy((int32)dx, (int32)dy);

			MoveBy(dx,dy);
			_decorator->MoveBy(BPoint(dx, dy));

			// 3) quickly move the window
//			_driver->CopyRegion(&reg,reg2.Frame().LeftTop());

			// 4) Invalidate only the areas which we can't redraw directly
			for(int32 i=0; i<reg2.CountRects();i++)
				reg.Exclude(reg2.RectAt(i));
			
			// TODO: DW's notes to self
			// As of right now, dragging the window is extremely slow despite the use
			// of CopyRegion. The reason is because of the redraw taken. When Invalidate() is
			// called the RootLayer invalidates things properly for itself, but the area which
			// should be invalidated for the first of the two windows in my test case is not
			// made dirty, so the entire first window is redrawn with RequestDraw being 
			// restructured as it is. Additionally, the second window is redrawn for the same reason
			// when in fact it shouldn't be redrawn at all.
			
			// Solution:
			// Figure out what the exact usage of Layer::Invalidate() should be (parent coordinates,
			//  layer's coordinates, etc) and set things right. Secondly, nuke the invalid region
			// in this call so that when RequestDraw is called, this WinBorder doesn't redraw itself
			
			_parent->Invalidate(reg);
			
//			_parent->RebuildRegions();
//			_parent->RequestDraw();
			
			unlock_layers();
		}
	}


	if(is_resizing_window())
	{
STRACE_CLICK(("ClickMove: Resize\n"));
		float	dx	= pt.x - _mousepos.x,
				dy	= pt.y - _mousepos.y;
		if(buttons!=0 && (dx!=0 || dy!=0))
 		{
			_clientframe.right+=dx;
			_clientframe.bottom+=dy;
	
			_win->Lock();
			_win->_frame.right+=dx;
			_win->_frame.bottom+=dy;
			_win->Unlock();
	
			lock_layers();
			ResizeBy(dx,dy);
//			_parent->RequestDraw();
			unlock_layers();
			_decorator->ResizeBy(dx,dy);
//			_decorator->Draw();
		}
	}

	_mousepos=pt;

}

void WinBorder::MouseUp(int8 *buffer)
{
STRACE_MOUSE(("WinBorder %s: MouseUp() \n",_title->String()));
	// buffer data:
	// 1) int64 - time of mouse click
	// 2) float - x coordinate of mouse click
	// 3) float - y coordinate of mouse click
	// 4) int32 - modifier keys down
	int8 *index=buffer; index+=sizeof(int64);
	float x=*((float*)index); index+=sizeof(float);
	float y=*((float*)index); index+=sizeof(float);
	int32 modifiers=*((int32*)index);
	BPoint pt(x,y);
	
	_mbuttons=0;
	_kmodifiers=modifiers;

	set_is_moving_window(false);
	set_is_resizing_window(false);
	set_is_sliding_tab(false);

	click_type click=_decorator->Clicked(pt, _mbuttons, _kmodifiers);

	switch(click)
	{
		case CLICK_CLOSE:
		{
			_decorator->SetClose(false);
			_decorator->DrawClose();
			
			// call close window stuff here
			STRACE_MOUSE(("WinBorder %s: MouseUp:CLICK_CLOSE unimplemented\n",_title->String()));
			
			break;
		}
		case CLICK_ZOOM:
		{
			_decorator->SetZoom(false);
			_decorator->DrawZoom();
			
			// call zoom stuff here
			STRACE_MOUSE(("WinBorder %s: MouseUp:CLICK_ZOOM unimplemented\n",_title->String()));
			
			break;
		}
		case CLICK_MINIMIZE:
		{
			_decorator->SetMinimize(false);
			_decorator->DrawMinimize();
			
			// call minimize stuff here
			STRACE_MOUSE(("WinBorder %s: MouseUp:CLICK_MINIMIZE unimplemented\n",_title->String()));
			
		}
		default:
		{
			break;
		}
	}
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
		in witch to do its drawings. Instead the hole visible region is split
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
	if ( _win->top_layer->_full.Intersects( r ) ){
			// build top_layer's visible region by intersecting its _full with winborder's _visible region.
		_win->top_layer->_visible		= _win->top_layer->_full;
		_win->top_layer->_visible.IntersectWith( &(_visible) );

			// then exclude it from winborder's _visible... 
		_visible.Exclude( &(_win->top_layer->_visible) );

		_win->top_layer->_fullVisible	= _win->top_layer->_visible;

			// Rebuild regions for children...
		for(Layer *lay = _win->top_layer->_bottomchild; lay != NULL; lay = lay->_uppersibling){
			if ( !(lay->_hidden) ){
				lay->RebuildRegions( r );
			}
		}
	}
	else{
		_visible.Exclude( &(_win->top_layer->_fullVisible) );
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
	reg.IntersectWith( &(_win->top_layer->_visible) );
	if (reg.CountRects() > 0){
		_win->top_layer->RequestClientUpdate( reg.Frame() );
	}
}

void WinBorder::MoveBy(float x, float y)
{
	BRegion		oldFullVisible( _fullVisible );
//	BPoint		oldFullVisibleOrigin( _fullVisible.Frame().LeftTop() );

	_frame.OffsetBy(x, y);
	_full.OffsetBy(x, y);

	_win->top_layer->_frame.OffsetBy(x, y);
	_win->top_layer->MoveRegionsBy(x, y);

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

	_win->top_layer->ResizeRegionsBy(x, y);

	_full			= _win->top_layer->_full;

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

void WinBorder::MoveToBack(){
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

void WinBorder::UpdateColors(void)
{
STRACE(("WinBorder %s: UpdateColors unimplemented\n",_title->String()));
}

void WinBorder::UpdateDecorator(void)
{
STRACE(("WinBorder %s: UpdateDecorator unimplemented\n",_title->String()));
}

void WinBorder::UpdateFont(void)
{
STRACE(("WinBorder %s: UpdateFont unimplemented\n",_title->String()));
}

void WinBorder::UpdateScreen(void)
{
STRACE(("WinBorder %s: UpdateScreen unimplemented\n",_title->String()));
}
