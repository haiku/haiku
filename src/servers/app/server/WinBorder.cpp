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
#include "View.h"	// for mouse button defines
#include "ServerWindow.h"
#include "Decorator.h"
#include "DisplayDriver.h"
#include "Desktop.h"
#include "WinBorder.h"
#include "AppServer.h"	// for new_decorator()

// TODO: Document this file completely

//#define DEBUG_WINBORDER
//#define DEBUG_WINBORDER_MOUSE
//#define DEBUG_WINBORDER_CLICK

#ifdef DEBUG_WINBORDER
#include <stdio.h>
#endif

#ifdef DEBUG_WINBORDER_MOUSE
#include <stdio.h>
#endif

#ifdef DEBUG_WINBORDER_CLICK
#include <stdio.h>
#endif

namespace winborder_private
{
	bool is_moving_window=false;
	bool is_resizing_window=false;
	bool is_sliding_tab=false;
	WinBorder *active_winborder=NULL;
};

extern ServerWindow *active_serverwindow;

bool is_moving_window(void) { return winborder_private::is_moving_window; }
void set_is_moving_window(bool state) { winborder_private::is_moving_window=state; }
bool is_resizing_window(void) { return winborder_private::is_resizing_window; }
void set_is_resizing_window(bool state) { winborder_private::is_resizing_window=state; }
bool is_sliding_tab(void) { return winborder_private::is_sliding_tab; }
void set_is_sliding_tab(bool state) { winborder_private::is_sliding_tab=state; }
WinBorder * get_active_winborder(void) { return winborder_private::active_winborder; }
void set_active_winborder(WinBorder *win) { winborder_private::active_winborder=win; }

WinBorder::WinBorder(const BRect &r, const char *name, const int32 look, const int32 feel,
	const int32 flags, ServerWindow *win)
 : Layer(r,name,B_FOLLOW_NONE,flags,win)
{
	// unlike BViews, windows start off as hidden, so we need to tweak the hidecount
	// assignment made by Layer().
	_hidecount=1;
	_mbuttons=0;
	_kmodifiers=0;
	_win=win;
	if(_win)
		_frame=_win->_frame;

	_mousepos.Set(0,0);
	_update=false;

	_title=new BString(name);
	_hresizewin=false;
	_vresizewin=false;
	_driver=GetGfxDriver(ActiveScreen());
	_decorator=new_decorator(r,name,look,feel,flags,GetGfxDriver(ActiveScreen()));
	_decorator->SetDriver(_driver);
	_decorator->SetTitle(name);
	
#ifdef DEBUG_WINBORDER
printf("WinBorder %s:\n",_title->String());
printf("\tFrame: (%.1f,%.1f,%.1f,%.1f)\n",r.left,r.top,r.right,r.bottom);
printf("\tWindow %s\n",win?win->Title():"NULL");
#endif
}

WinBorder::~WinBorder(void)
{
#ifdef DEBUG_WINBORDER
printf("WinBorder %s:~WinBorder()\n",_title->String());
#endif
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
#ifdef DEBUG_WINBORDER_CLICK
printf("Click: MoveToBack\n");
#endif
			MakeTopChild();
			break;
		}
		case CLICK_MOVETOFRONT:
		{
#ifdef DEBUG_WINBORDER_CLICK
printf("Click: MoveToFront\n");
#endif
			MakeBottomChild();
			break;
		}
		case CLICK_CLOSE:
		{
#ifdef DEBUG_WINBORDER_CLICK
printf("Click: Close\n");
#endif
			_decorator->SetClose(true);
			_decorator->DrawClose();
			break;
		}
		case CLICK_ZOOM:
		{
#ifdef DEBUG_WINBORDER_CLICK
printf("Click: Zoom\n");
#endif
			_decorator->SetZoom(true);
			_decorator->DrawZoom();
			break;
		}
		case CLICK_MINIMIZE:
		{
#ifdef DEBUG_WINBORDER_CLICK
printf("Click: Minimize\n");
#endif
			_decorator->SetMinimize(true);
			_decorator->DrawMinimize();
			break;
		}
		case CLICK_DRAG:
		{
			if(buttons==B_PRIMARY_MOUSE_BUTTON)
			{
#ifdef DEBUG_WINBORDER_CLICK
printf("Click: Drag\n");
#endif
				MakeBottomChild();
				set_is_moving_window(true);
			}

			if(buttons==B_SECONDARY_MOUSE_BUTTON)
			{
#ifdef DEBUG_WINBORDER_CLICK
printf("Click: MoveToBack\n");
#endif
				MakeTopChild();
			}
			break;
		}
		case CLICK_SLIDETAB:
		{
#ifdef DEBUG_WINBORDER_CLICK
printf("Click: Slide Tab\n");
#endif
			set_is_sliding_tab(true);
			break;
		}
		case CLICK_RESIZE:
		{
			if(buttons==B_PRIMARY_MOUSE_BUTTON)
			{
#ifdef DEBUG_WINBORDER_CLICK
printf("Click: Resize\n");
#endif
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
#ifdef DEBUG_WINBORDER_CLICK
printf("ClickMove: Slide Tab\n");
#endif
		float dx=pt.x-_mousepos.x;

		if(dx!=0)
		{
			// SlideTab returns how much things were moved, and currently
			// supports just the x direction, so get the value so
			// we can invalidate the proper area.
			lock_layers();
			_parent->Invalidate(_decorator->SlideTab(dx,0));
			_parent->RequestDraw();
			_decorator->DrawTab();
			unlock_layers();
		}
	}


	if(is_moving_window())
	{
#ifdef DEBUG_WINBORDER_CLICK
printf("ClickMove: Drag\n");
#endif
//debugger("");
		float dx=pt.x-_mousepos.x,
			dy=pt.y-_mousepos.y;
		if(buttons!=0 && (dx!=0 || dy!=0))
		{
			BRect oldmoveframe=_win->_frame;
			_clientframe.OffsetBy(pt);
	
			_win->Lock();
			_win->_frame.OffsetBy(dx,dy);
			_win->Unlock();
			
			lock_layers();
			BRegion *reg=_decorator->GetFootprint();
			_driver->CopyRegion(reg,_win->_frame.LeftTop());
			
			BRegion reg2(oldmoveframe);
			MoveBy(dx,dy);
			_decorator->MoveBy(BPoint(dx, dy));
			reg->OffsetBy((int32)dx, (int32)dy);
			reg2.Exclude(reg);
			_parent->RebuildRegions();
			_parent->RequestDraw();
			
			delete reg;
			unlock_layers();
		}
	}


	if(is_resizing_window())
	{
#ifdef DEBUG_WINBORDER_CLICK
printf("ClickMove: Resize\n");
#endif
		float dx=pt.x-_mousepos.x,
			dy=pt.y-_mousepos.y;
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
	
#ifdef DEBUG_WINBORDER_MOUSE
printf("WinBorder %s: MouseUp unimplemented\n",_title->String());
#endif

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
			
			break;
		}
		case CLICK_ZOOM:
		{
			_decorator->SetZoom(false);
			_decorator->DrawZoom();
			
			// call zoom stuff here
			
			break;
		}
		case CLICK_MINIMIZE:
		{
			_decorator->SetMinimize(false);
			_decorator->DrawMinimize();
			
			// call minimize stuff here
			
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
#ifdef DEBUG_WINBORDER
printf("WinBorder %s: RequestDraw\n",_title->String());
#endif
	_decorator->Draw(r);
}

void WinBorder::RequestDraw(void)
{
#ifdef DEBUG_WINBORDER
printf("WinBorder %s::RequestDraw\n",_title->String());
#endif
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
#ifdef DEBUG_WINBORDER
printf("WinBorder %s: UpdateColors\n",_title->String());
#endif
}

void WinBorder::UpdateDecorator(void)
{
#ifdef DEBUG_WINBORDER
printf("WinBorder %s: UpdateDecorator\n",_title->String());
#endif
}

void WinBorder::UpdateFont(void)
{
#ifdef DEBUG_WINBORDER
printf("WinBorder %s: UpdateFont\n",_title->String());
#endif
}

void WinBorder::UpdateScreen(void)
{
#ifdef DEBUG_WINBORDER
printf("WinBorder %s: UpdateScreen\n",_title->String());
#endif
}
