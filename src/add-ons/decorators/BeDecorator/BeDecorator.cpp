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
//	File Name:		BeDecorator.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Decorator in the style of BeOS R5
//  
//------------------------------------------------------------------------------
#include <Rect.h>
#include "DisplayDriver.h"
#include <View.h>
#include "LayerData.h"
#include "ColorUtils.h"
#include "BeDecorator.h"
#include "RGBColor.h"

//#define DEBUG_DECORATOR


#define USE_VIEW_FILL_HACK

#ifdef DEBUG_DECORATOR
#include <stdio.h>
#endif

BeDecorator::BeDecorator(BRect rect, int32 wlook, int32 wfeel, int32 wflags)
 : Decorator(rect,wlook,wfeel,wflags)
{
	taboffset=0;
	titlepixelwidth=0;

	_SetFocus();

	// These hard-coded assignments will go bye-bye when the system _colors 
	// API is implemented
	frame_highercol.SetColor(216,216,216);
	frame_lowercol.SetColor(110,110,110);

	textcol.SetColor(0,0,0);
	
	frame_highcol=frame_lowercol.MakeBlendColor(frame_highercol,0.75);
	frame_midcol=frame_lowercol.MakeBlendColor(frame_highercol,0.5);
	frame_lowcol=frame_lowercol.MakeBlendColor(frame_highercol,0.25);

	_DoLayout();
	
	// This flag is used to determine whether or not we're moving the tab
	slidetab=false;
	solidhigh=0xFFFFFFFFFFFFFFFFLL;
	solidlow=0;

//	tab_highcol=_colors->window_tab;
//	tab_lowcol=_colors->window_tab;

#ifdef DEBUG_DECORATOR
printf("BeDecorator:\n");
printf("\tFrame (%.1f,%.1f,%.1f,%.1f)\n",rect.left,rect.top,rect.right,rect.bottom);
#endif
}

BeDecorator::~BeDecorator(void)
{
#ifdef DEBUG_DECORATOR
printf("BeDecorator: ~BeDecorator()\n");
#endif
}

click_type BeDecorator::Clicked(BPoint pt, int32 buttons, int32 modifiers)
{
#ifdef DEBUG_DECORATOR
printf("BeDecorator: Clicked\n");
printf("\tPoint: (%.1f,%.1f)\n",pt.x,pt.y);
printf("\tButtons:\n");
if(buttons==0)
	printf("\t\tNone\n");
else
{
	if(buttons & B_PRIMARY_MOUSE_BUTTON)
		printf("\t\tPrimary\n");
	if(buttons & B_SECONDARY_MOUSE_BUTTON)
		printf("\t\tSecondary\n");
	if(buttons & B_TERTIARY_MOUSE_BUTTON)
		printf("\t\tTertiary\n");
}
printf("\tModifiers:\n");
if(modifiers==0)
	printf("\t\tNone\n");
else
{
	if(modifiers & B_CAPS_LOCK)
		printf("\t\tCaps Lock\n");
	if(modifiers & B_NUM_LOCK)
		printf("\t\tNum Lock\n");
	if(modifiers & B_SCROLL_LOCK)
		printf("\t\tScroll Lock\n");
	if(modifiers & B_LEFT_COMMAND_KEY)
		printf("\t\t Left Command\n");
	if(modifiers & B_RIGHT_COMMAND_KEY)
		printf("\t\t Right Command\n");
	if(modifiers & B_LEFT_CONTROL_KEY)
		printf("\t\tLeft Control\n");
	if(modifiers & B_RIGHT_CONTROL_KEY)
		printf("\t\tRight Control\n");
	if(modifiers & B_LEFT_OPTION_KEY)
		printf("\t\tLeft Option\n");
	if(modifiers & B_RIGHT_OPTION_KEY)
		printf("\t\tRight Option\n");
	if(modifiers & B_LEFT_SHIFT_KEY)
		printf("\t\tLeft Shift\n");
	if(modifiers & B_RIGHT_SHIFT_KEY)
		printf("\t\tRight Shift\n");
	if(modifiers & B_MENU_KEY)
		printf("\t\tMenu\n");
}
#endif
	if(_closerect.Contains(pt))
		return CLICK_CLOSE;

	if(_zoomrect.Contains(pt))
		return CLICK_ZOOM;
	
	if(_resizerect.Contains(pt) && _look==B_DOCUMENT_WINDOW_LOOK)
		return CLICK_RESIZE;

	// Clicking in the tab?
	if(_tabrect.Contains(pt))
	{
		// Here's part of our window management stuff
//		if(buttons==B_PRIMARY_MOUSE_BUTTON && !GetFocus())
//			return CLICK_MOVETOFRONT;
		if(buttons==B_SECONDARY_MOUSE_BUTTON)
			return CLICK_MOVETOBACK;
		return CLICK_DRAG;
	}

	// We got this far, so user is clicking on the border?
	BRect brect(_frame);
	brect.top+=19;
	BRect clientrect(brect.InsetByCopy(3,3));
	if(brect.Contains(pt) && !clientrect.Contains(pt))
	{
		if(_resizerect.Contains(pt))
			return CLICK_RESIZE;
		
		return CLICK_DRAG;
	}

	// Guess user didn't click anything
	return CLICK_NONE;
}

void BeDecorator::_DoLayout(void)
{
//debugger("");
#ifdef DEBUG_DECORATOR
printf("BeDecorator: Do Layout\n");
#endif
	// Here we determine the size of every rectangle that we use
	// internally when we are given the size of the client rectangle.
	
	// Current version simply makes everything fit inside the rect
	// instead of building around it. This will change.
	
	_tabrect=_frame;
	_resizerect=_frame;
	_borderrect=_frame;
	_closerect=_frame;

	textoffset=(_look==B_FLOATING_WINDOW_LOOK)?5:7;

	_closerect.left+=(_look==B_FLOATING_WINDOW_LOOK)?2:4;
	_closerect.top+=(_look==B_FLOATING_WINDOW_LOOK)?6:4;
	_closerect.right=_closerect.left+10;
	_closerect.bottom=_closerect.top+10;

	_borderrect.top+=19;
	
	_resizerect.top=_resizerect.bottom-18;
	_resizerect.left=_resizerect.right-18;
	
	_tabrect.bottom=_tabrect.top+18;
	if(strlen(GetTitle())>1)
	{
		if(_driver)
			titlepixelwidth=_driver->StringWidth(GetTitle(),_TitleWidth(), &_layerdata);
		else
			titlepixelwidth=10;
		
		if(_closerect.right+textoffset+titlepixelwidth+35< _frame.Width()-1)
			_tabrect.right=_tabrect.left+titlepixelwidth;
	}
	else
		_tabrect.right=_tabrect.left+_tabrect.Width()/2;

	if(_look==B_FLOATING_WINDOW_LOOK)
		_tabrect.top+=4;

	_zoomrect=_tabrect;
	_zoomrect.top+=(_look==B_FLOATING_WINDOW_LOOK)?2:4;
	_zoomrect.right-=4;
	_zoomrect.bottom-=4;
	_zoomrect.left=_zoomrect.right-10;
	_zoomrect.bottom=_zoomrect.top+10;
	
}

void BeDecorator::MoveBy(float x, float y)
{
	MoveBy(BPoint(x,y));
}

void BeDecorator::MoveBy(BPoint pt)
{
#ifdef DEBUG_DECORATOR
printf("BeDecorator: Move By (%.1f, %.1f)\n",pt.x,pt.y);
#endif
	// Move all internal rectangles the appropriate amount
	_frame.OffsetBy(pt);
	_closerect.OffsetBy(pt);
	_tabrect.OffsetBy(pt);
	_resizerect.OffsetBy(pt);
	_borderrect.OffsetBy(pt);
	_zoomrect.OffsetBy(pt);
}

BRegion * BeDecorator::GetFootprint(void)
{
#ifdef DEBUG_DECORATOR
printf("BeDecorator: Get Footprint\n");
#endif
	// This function calculates the decorator's footprint in coordinates
	// relative to the layer. This is most often used to set a WinBorder
	// object's visible region.
	
	BRegion *reg=new BRegion(_borderrect);
	reg->Include(_tabrect);
	return reg;
}

void BeDecorator::_DrawTitle(BRect r)
{
	// Designed simply to redraw the title when it has changed on
	// the client side.
	_layerdata.highcolor=_colors->window_tab_text;
	_layerdata.lowcolor=(GetFocus())?_colors->window_tab:_colors->inactive_window_tab;

	int32 titlecount=_ClipTitle((_zoomrect.left-5)-(_closerect.right+textoffset));
	BString titlestr=GetTitle();
	if(titlecount<titlestr.CountChars())
	{
		titlestr.Truncate(titlecount-1);
		titlestr+="...";
		titlecount+=2;
	}
	_driver->DrawString(titlestr.String(),titlecount,
		BPoint(_closerect.right+textoffset,_closerect.bottom+1),&_layerdata);
}

void BeDecorator::_SetFocus(void)
{
	// SetFocus() performs necessary duties for color swapping and
	// other things when a window is deactivated or activated.
	
	if(GetFocus())
	{
		button_highcol.SetColor(tint_color(_colors->window_tab.GetColor32(),B_LIGHTEN_2_TINT));
		button_lowcol.SetColor(tint_color(_colors->window_tab.GetColor32(),B_DARKEN_2_TINT));
		textcol=_colors->window_tab_text;
	}
	else
	{
		button_highcol.SetColor(tint_color(_colors->inactive_window_tab.GetColor32(),B_LIGHTEN_2_TINT));
		button_lowcol.SetColor(tint_color(_colors->inactive_window_tab.GetColor32(),B_DARKEN_2_TINT));
		textcol=_colors->inactive_window_tab_text;
	}
}

void BeDecorator::Draw(BRect update)
{
#ifdef DEBUG_DECORATOR
printf("BeDecorator: Draw(%.1f,%.1f,%.1f,%.1f)\n",update.left,update.top,update.right,update.bottom);
#endif
	// We need to draw a few things: the tab, the resize thumb, the borders,
	// and the buttons

	_DrawTab(update);

	// Draw the top view's client area - just a hack :)
	_layerdata.highcolor=_colors->document_background;

	if(_borderrect.Intersects(update))
		_driver->FillRect(_borderrect,&_layerdata,(int8*)&solidhigh);
	
	_DrawFrame(update);
}

void BeDecorator::Draw(void)
{
	// Easy way to draw everything - no worries about drawing only certain
	// things

	// Draw the top view's client area - just a hack :)
	_layerdata.highcolor=_colors->document_background;

	_driver->FillRect(_borderrect,&_layerdata,(int8*)&solidhigh);
	DrawFrame();

	DrawTab();

}

void BeDecorator::_DrawZoom(BRect r)
{
	// If this has been implemented, then the decorator has a Zoom button
	// which should be drawn based on the state of the member zoomstate
	BRect zr=r;
	zr.left+=zr.Width()/3;
	zr.top+=zr.Height()/3;

	DrawBlendedRect(zr,GetZoom());
	DrawBlendedRect(zr.OffsetToCopy(r.LeftTop()),GetZoom());
}

void BeDecorator::_DrawClose(BRect r)
{
	// Just like DrawZoom, but for a close button
	DrawBlendedRect(r,GetClose());
}

void BeDecorator::_DrawTab(BRect r)
{
	// If a window has a tab, this will draw it and any buttons which are
	// in it.
	if(_look==B_NO_BORDER_WINDOW_LOOK)
		return;
	
	_layerdata.highcolor=(GetFocus())?_colors->window_tab:_colors->inactive_window_tab;
	_driver->FillRect(_tabrect,&_layerdata,(int8*)&solidhigh);
	_layerdata.highcolor=frame_lowcol;
	_driver->StrokeLine(_tabrect.LeftBottom(),_tabrect.RightBottom(),&_layerdata,(int8*)&solidhigh);

	_DrawTitle(_tabrect);

	// Draw the buttons if we're supposed to	
	if(!(_flags & B_NOT_CLOSABLE))
		_DrawClose(_closerect);
	if(!(_flags & B_NOT_ZOOMABLE))
		_DrawZoom(_zoomrect);
}

void BeDecorator::_SetColors(void)
{
	_SetFocus();
}

void BeDecorator::DrawBlendedRect(BRect r, bool down)
{
	// This bad boy is used to draw a rectangle with a gradient.
	// Note that it is not part of the Decorator API - it's specific
	// to just the BeDecorator. Called by DrawZoom and DrawClose

	// Actually just draws a blended square
	int32 w=r.IntegerWidth(),  h=r.IntegerHeight();

	rgb_color tmpcol,halfcol, startcol, endcol;
	float rstep,gstep,bstep,i;

	int steps=(w<h)?w:h;

	if(down)
	{
		startcol=button_lowcol.GetColor32();
		endcol=button_highcol.GetColor32();
	}
	else
	{
		startcol=button_highcol.GetColor32();
		endcol=button_lowcol.GetColor32();
	}

	halfcol=MakeBlendColor(startcol,endcol,0.5);

	rstep=float(startcol.red-halfcol.red)/steps;
	gstep=float(startcol.green-halfcol.green)/steps;
	bstep=float(startcol.blue-halfcol.blue)/steps;

	for(i=0;i<=steps; i++)
	{
		SetRGBColor(&tmpcol, uint8(startcol.red-(i*rstep)),
			uint8(startcol.green-(i*gstep)),
			uint8(startcol.blue-(i*bstep)));
		_layerdata.highcolor=tmpcol;
		_driver->StrokeLine(BPoint(r.left,r.top+i),
			BPoint(r.left+i,r.top),&_layerdata,(int8*)&solidhigh);

		SetRGBColor(&tmpcol, uint8(halfcol.red-(i*rstep)),
			uint8(halfcol.green-(i*gstep)),
			uint8(halfcol.blue-(i*bstep)));

		_layerdata.highcolor=tmpcol;
		_driver->StrokeLine(BPoint(r.left+steps,r.top+i),
			BPoint(r.left+i,r.top+steps),&_layerdata,(int8*)&solidhigh);

	}

//	_layerdata.highcolor=startcol;
//	_driver->FillRect(r,&_layerdata,(int8*)&solidhigh);
	_layerdata.highcolor=frame_lowcol;
	_driver->StrokeRect(r,&_layerdata,(int8*)&solidhigh);
}

void BeDecorator::_DrawFrame(BRect rect)
{
	// Duh, draws the window frame, I think. ;)

#ifdef USE_VIEW_FILL_HACK
_driver->FillRect(_borderrect,&_layerdata,(int8*)&solidhigh);
#endif

	if(_look==B_NO_BORDER_WINDOW_LOOK)
		return;
	
	BRect r=_borderrect;

	_layerdata.highcolor=frame_midcol;
	_driver->StrokeLine(BPoint(r.left,r.top),BPoint(r.right-1,r.top),
		&_layerdata,(int8*)&solidhigh);
	_layerdata.highcolor=frame_lowcol;
	_driver->StrokeLine(BPoint(r.left,r.top),BPoint(r.left,r.bottom),
		&_layerdata,(int8*)&solidhigh);
	_layerdata.highcolor=frame_lowercol;
	_driver->StrokeLine(BPoint(r.right,r.bottom),BPoint(r.right,r.top),
		&_layerdata,(int8*)&solidhigh);
	_layerdata.highcolor=frame_lowercol;
	_driver->StrokeLine(BPoint(r.right,r.bottom),BPoint(r.left,r.bottom),
		&_layerdata,(int8*)&solidhigh);

	r.InsetBy(1,1);
	_layerdata.highcolor=frame_highercol;
	_driver->StrokeLine(BPoint(r.left,r.top),BPoint(r.right-1,r.top),
		&_layerdata,(int8*)&solidhigh);
	_layerdata.highcolor=frame_highercol;
	_driver->StrokeLine(BPoint(r.left,r.top),BPoint(r.left,r.bottom),
		&_layerdata,(int8*)&solidhigh);
	_layerdata.highcolor=frame_midcol;
	_driver->StrokeLine(BPoint(r.right,r.bottom),BPoint(r.right,r.top),
		&_layerdata,(int8*)&solidhigh);
	_layerdata.highcolor=frame_midcol;
	_driver->StrokeLine(BPoint(r.right,r.bottom),BPoint(r.left,r.bottom),
		&_layerdata,(int8*)&solidhigh);
	
	r.InsetBy(1,1);
	_layerdata.highcolor=frame_highcol;
	_driver->StrokeRect(r,&_layerdata,(int8*)&solidhigh);

	r.InsetBy(1,1);
	_layerdata.highcolor=frame_lowercol;
	_driver->StrokeLine(BPoint(r.left,r.top),BPoint(r.right-1,r.top),
		&_layerdata,(int8*)&solidhigh);
	_layerdata.highcolor=frame_lowercol;
	_driver->StrokeLine(BPoint(r.left,r.top),BPoint(r.left,r.bottom),
		&_layerdata,(int8*)&solidhigh);
	_layerdata.highcolor=frame_highercol;
	_driver->StrokeLine(BPoint(r.right,r.bottom),BPoint(r.right,r.top),
		&_layerdata,(int8*)&solidhigh);
	_layerdata.highcolor=frame_highercol;
	_driver->StrokeLine(BPoint(r.right,r.bottom),BPoint(r.left,r.bottom),
		&_layerdata,(int8*)&solidhigh);
	_driver->StrokeRect(_borderrect,&_layerdata,(int8*)&solidhigh);

	// Draw the resize thumb if we're supposed to
	if(!(_flags & B_NOT_RESIZABLE))
	{
		r=_resizerect;

		int32 w=r.IntegerWidth(),  h=r.IntegerHeight();
		
		// This code is strictly for B_DOCUMENT_WINDOW looks
		if(_look==B_DOCUMENT_WINDOW_LOOK)
		{
			rgb_color halfcol, startcol, endcol;
			float rstep,gstep,bstep,i;
			
			int steps=(w<h)?w:h;
		
			startcol=frame_highercol.GetColor32();
			endcol=frame_lowercol.GetColor32();
		
			halfcol=frame_highercol.MakeBlendColor(frame_lowercol,0.5).GetColor32();
		
			rstep=(startcol.red-halfcol.red)/steps;
			gstep=(startcol.green-halfcol.green)/steps;
			bstep=(startcol.blue-halfcol.blue)/steps;

			for(i=0;i<=steps; i++)
			{
				_layerdata.highcolor.SetColor(uint8(startcol.red-(i*rstep)),
					uint8(startcol.green-(i*gstep)),
					uint8(startcol.blue-(i*bstep)));
				
				_driver->StrokeLine(BPoint(r.left,r.top+i),
					BPoint(r.left+i,r.top),&_layerdata,(int8*)&solidhigh);
		
				_layerdata.highcolor.SetColor(uint8(halfcol.red-(i*rstep)),
					uint8(halfcol.green-(i*gstep)),
					uint8(halfcol.blue-(i*bstep)));
				_driver->StrokeLine(BPoint(r.left+steps,r.top+i),
					BPoint(r.left+i,r.top+steps),&_layerdata,(int8*)&solidhigh);			
			}
			_layerdata.highcolor=frame_lowercol;
			_driver->StrokeRect(r,&_layerdata,(int8*)&solidhigh);
		}
		else
		{
			_layerdata.highcolor=frame_lowercol;
			_driver->StrokeLine(BPoint(r.right,r.top),BPoint(r.right-3,r.top),
				&_layerdata,(int8*)&solidhigh);
			_driver->StrokeLine(BPoint(r.left,r.bottom),BPoint(r.left,r.bottom-3),
				&_layerdata,(int8*)&solidhigh);
		}
	}
}

extern "C" float get_decorator_version(void)
{
	return 1.00;
}

extern "C" Decorator *instantiate_decorator(BRect rect, int32 wlook, int32 wfeel, int32 wflags)
{
	return new BeDecorator(rect,wlook,wfeel,wflags);
}
