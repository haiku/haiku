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

//#define DEBUG_WINBORDER
//#define DEBUG_WINBORDER_MOUSE

#ifdef DEBUG_WINBORDER
#include <stdio.h>
#endif

#ifdef DEBUG_WINBORDER_MOUSE
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
WinBorder * get_active_winborder(void) { return winborder_private::active_winborder; }
void set_active_winborder(WinBorder *win) { winborder_private::active_winborder=win; }

WinBorder::WinBorder(BRect r, const char *name, int32 look, int32 feel, int32 flags, ServerWindow *win)
 : Layer(r,name,B_FOLLOW_NONE,flags,win)
{
	_mbuttons=0;
	_win=win;
	if(_win)
		_frame=_win->_frame;

	_mousepos.Set(0,0);
	_update=false;

	_title=new BString(name);
	_hresizewin=false;
	_vresizewin=false;

	_decorator=new_decorator(r,name,look,feel,flags,GetGfxDriver());
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
#ifdef DEBUG_WINBORDER_MOUSE
printf("WinBorder %s: MouseDown unimplemented\n",_title->String());
#endif
/*	_mbuttons=buttons;
	kmodifiers=modifiers;
	click_type click=_decorator->Clicked(pt, _mbuttons, kmodifiers);
	_mousepos=pt;

	switch(click)
	{
		case CLICK_MOVETOBACK:
		{
			MoveToBack();
			break;
		}
		case CLICK_MOVETOFRONT:
		{
			MoveToFront();
			break;
		}
		case CLICK_CLOSE:
		{
			_decorator->SetClose(true);
			_decorator->Draw();
			break;
		}
		case CLICK_ZOOM:
		{
			_decorator->SetZoom(true);
			_decorator->Draw();
			break;
		}
		case CLICK_MINIMIZE:
		{
			_decorator->SetMinimize(true);
			_decorator->Draw();
			break;
		}
		case CLICK_DRAG:
		{
			if(buttons==B_PRIMARY_MOUSE_BUTTON)
				is_moving_window=true;

			if(buttons==B_SECONDARY_MOUSE_BUTTON)
				MoveToBack();
			break;
		}
		case CLICK_SLIDETAB:
		{
			is_sliding_tab=true;
			break;
		}
		case CLICK_RESIZE:
		{
			if(buttons==B_PRIMARY_MOUSE_BUTTON)
				is_resizing_window=true;
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
	if(click!=CLICK_NONE)
		ActivateWindow(_win);
*/
}

void WinBorder::MouseMoved(int8 *buffer)
{
#ifdef DEBUG_WINBORDER_MOUSE
printf("WinBorder %s: MouseMoved unimplemented\n",_title->String());
#endif
/*	_mbuttons=buttons;
	kmodifiers=modifiers;
	click_type click=_decorator->Clicked(pt, _mbuttons, kmodifiers);
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

	if(is_sliding_tab)
	{
		float dx=pt.x-_mousepos.x;

		if(dx!=0)
		{
			// SlideTab returns how much things were moved, and currently
			// supports just the x direction, so get the value so
			// we can invalidate the proper area.
			layerlock->Lock();
			parent->Invalidate(_decorator->SlideTab(dx,0));
			parent->RequestDraw();
			_decorator->DrawTab();
			layerlock->Unlock();
		}
	}
	if(is_moving_window)
	{
		float dx=pt.x-_mousepos.x,
			dy=pt.y-_mousepos.y;
		if(dx!=0 || dy!=0)
		{
			BRect oldmoveframe=_win->_frame;
			clientframe.OffsetBy(pt);
	
			_win->Lock();
			_win->_frame.OffsetBy(dx,dy);
			_win->Unlock();
			
			layerlock->Lock();
//			InvalidateLowerSiblings(oldmoveframe);
			parent->Invalidate(oldmoveframe);
			MoveBy(dx,dy);
			parent->RequestDraw();
			_decorator->MoveBy(BPoint(dx, dy));
			_decorator->Draw();
			layerlock->Unlock();
		}
	}
	if(is_resizing_window)
	{
		float dx=pt.x-_mousepos.x,
			dy=pt.y-_mousepos.y;
		if(dx!=0 || dy!=0)
		{
			clientframe.right+=dx;
			clientframe.bottom+=dy;
	
			_win->Lock();
			_win->_frame.right+=dx;
			_win->_frame.bottom+=dy;
			_win->Unlock();
	
			layerlock->Lock();
			ResizeBy(dx,dy);
			parent->RequestDraw();
			layerlock->Unlock();
			_decorator->ResizeBy(dx,dy);
			_decorator->Draw();
		}
	}
	_mousepos=pt;
*/
}

void WinBorder::MouseUp(int8 *buffer)
{
#ifdef DEBUG_WINBORDER_MOUSE
printf("WinBorder %s: MouseUp unimplmented\n",_title->String());
#endif
/*
	_mbuttons=buttons;
	kmodifiers=modifiers;
	is_moving_window=false;
	is_resizing_window=false;
	is_sliding_tab=false;

	click_type click=_decorator->Clicked(pt, _mbuttons, kmodifiers);

	switch(click)
	{
		case CLICK_CLOSE:
		{
			_decorator->SetClose(false);
			_decorator->Draw();
			
			// call close window stuff here
			
			break;
		}
		case CLICK_ZOOM:
		{
			_decorator->SetZoom(false);
			_decorator->Draw();
			
			// call zoom stuff here
			
			break;
		}
		case CLICK_MINIMIZE:
		{
			_decorator->SetMinimize(false);
			_decorator->Draw();
			
			// call minimize stuff here
			
		}
		default:
		{
			break;
		}
	}
*/
}

void WinBorder::Draw(BRect update_rect)
{
#ifdef DEBUG_WINBORDER
printf("WinBorder %s: Draw\n",_title->String());
#endif
	if(_update && _visible!=NULL)
		_is_updating=true;

	_decorator->Draw(update_rect);

	if(_update && _visible!=NULL)
		_is_updating=false;
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

printf("WinBorder %s::RequestDraw\n",_title->String());
	if(_invalid)
	{
//printf("drew something\n");
//		for(int32 i=0; i<invalid->CountRects();i++)
//			_decorator->Draw(ConvertToTop(invalid->RectAt(i)));
			_decorator->Draw();
		delete _invalid;
		_invalid=NULL;
		_is_dirty=false;
	}
}

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
