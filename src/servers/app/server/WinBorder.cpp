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
	_hidden		= true;
	_mbuttons	= 0;
	_kmodifiers	= 0;
	_win		= win;

	_mousepos.Set(0,0);
	_update		= false;

	_title		= new BString(name);
	_hresizewin	= false;
	_vresizewin	= false;
	_driver		= GetGfxDriver(ActiveScreen());
	_decorator	= new_decorator(r,name,look,feel,flags,GetGfxDriver(ActiveScreen()));
	_decorator->GetFootprint(_visible);

	*_full		= *_visible;
	*_invalid	= *_visible;
	_frame		= _visible->Frame();
	
		// I tend to think it is not needed, so if it isn't, DW, please remove it.
	_view_token	= border_token_handler.GetToken();

	_decorator->SetDriver(_driver);
	_decorator->SetTitle(name);
	

	STRACE(("WinBorder %s:\n",_title->String()));
	STRACE(("\tFrame: (%.1f,%.1f,%.1f,%.1f)\n",r.left,r.top,r.right,r.bottom));
	STRACE(("\tWindow %s\n",win?win->Title():"NULL"));
}

WinBorder::~WinBorder(void)
{
	STRACE(("WinBorder %s:~WinBorder()\n",_title->String()));
	delete _title;
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
	int8 *index=buffer; index+=sizeof(int64);
	float x=*((float*)index); index+=sizeof(float);
	float y=*((float*)index); index+=sizeof(float);
	int32 modifiers=*((int32*)index); index+=sizeof(int32);
	int32 buttons=*((int32*)index);

	BPoint pt(x,y);

	_mbuttons=buttons;
	_kmodifiers=modifiers;
	click_type click=_decorator->Clicked(pt, _mbuttons, _kmodifiers);
	_mousepos=pt;

	switch(click)
	{
		case CLICK_MOVETOBACK:
		{
			STRACE_CLICK(("Click: MoveToBack\n"));
			MakeTopChild();
			break;
		}
		case CLICK_MOVETOFRONT:
		{
			STRACE_CLICK(("Click: MoveToFront\n"));
			MakeBottomChild();
			break;
		}
		case CLICK_CLOSE:
		{
			STRACE_CLICK(("Click: Close\n"));
			_decorator->SetClose(true);
			_decorator->DrawClose();
			break;
		}
		case CLICK_ZOOM:
		{
			STRACE_CLICK(("Click: Zoom\n"));
			_decorator->SetZoom(true);
			_decorator->DrawZoom();
			break;
		}
		case CLICK_MINIMIZE:
		{
			STRACE_CLICK(("Click: Minimize\n"));
			_decorator->SetMinimize(true);
			_decorator->DrawMinimize();
			break;
		}
		case CLICK_DRAG:
		{
			if(buttons==B_PRIMARY_MOUSE_BUTTON)
			{
			STRACE_CLICK(("Click: Drag\n"));
				MakeBottomChild();
				set_is_moving_window(true);
			}

			if(buttons==B_SECONDARY_MOUSE_BUTTON)
			{
				STRACE_CLICK(("Click: MoveToBack\n"));
				MakeTopChild();
			}
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
			if(buttons==B_PRIMARY_MOUSE_BUTTON)
			{
				STRACE_CLICK(("Click: Resize\n"));
				set_is_resizing_window(true);
			}
			break;
		}
		case CLICK_NONE:
		{
			break;
		}
		default:
		{
			break;
		}
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
			_parent->RequestDraw();
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
			_driver->CopyRegion(&reg,reg2.Frame().LeftTop());

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
			
			_parent->RebuildRegions();
			_parent->RequestDraw();
			
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
			_parent->RequestDraw();
			unlock_layers();
			_decorator->ResizeBy(dx,dy);
			_decorator->Draw();
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
	if(_decorator)
		_decorator->SetFocus(active);
}

void WinBorder::RequestDraw(const BRect &r)
{
	STRACE(("WinBorder %s: RequestDraw(BRect)\n",_title->String()));
	#ifdef DEBUG_WINBORDER
		PrintToStream();
	#endif

	_decorator->Draw(r);
	delete _invalid;
	_invalid = NULL;
}

void WinBorder::RequestDraw(void)
{
	STRACE(("WinBorder %s::RequestDraw()\n",_title->String()));
	#ifdef DEBUG_WINBORDER
		PrintToStream();
	#endif
	
	if(_invalid)
	{
		for(int32 i=0;i<_invalid->CountRects();i++)
			_decorator->Draw(_invalid->RectAt(i));
		
		delete _invalid;
		_invalid = NULL;
	}
	else
		_decorator->Draw();
}
/*
void WinBorder::MoveBy(BPoint pt)
{
}

void WinBorder::MoveBy(float x, float y)
{
}

void WinBorder::ResizeBy(BPoint pt)
{
}

void WinBorder::ResizeBy(float x, float y)
{
}
*/
void WinBorder::UpdateColors(void)
{
	gui_colorset.Lock();
	_decorator->SetColors(gui_colorset);
	gui_colorset.Unlock();

	// Tell the decorator to update all visible regions of the window border	
	if(_visible)
		for(int32 i=0; i<_visible->CountRects(); i++)
			_decorator->Draw(_visible->RectAt(i));
}

void WinBorder::UpdateDecorator(void)
{
	STRACE(("WinBorder %s: UpdateDecorator unimplemented\n",_title->String()));

	// TODO: Implement
	
	// What needs done here is to delete the current decorator and do a lot of the
	// decorator-related setup done in the constructor. This definitely needs to be
	// done *after* all the silliness with figuring out the clipping code is taken care of
}

void WinBorder::UpdateFont(void)
{
	STRACE(("WinBorder %s: UpdateFont unimplemented\n",_title->String()));

	// TODO: Implement
	
	// What needs done here is to first call _decorator->SetFont(). Following that, 
	// GetFootprint() will need to be called to obtain changes in the decorator's size due to
	// changes in the font, whether they are face, size, or whatever. Thus, the WinBorder will
	// need to do some region-related recalculation, as well.
}

void WinBorder::UpdateScreen(void)
{
	STRACE(("WinBorder %s: UpdateScreen unimplemented\n",_title->String()));
}
