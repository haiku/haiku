#include "DisplayDriver.h"
#include <View.h>
#include "LayerData.h"
#include "ColorUtils.h"
#include "WinDecorator.h"
#include "RGBColor.h"
#include "PatternHandler.h"

//#define DEBUG_DECOR

#ifdef DEBUG_DECOR
#include <stdio.h>
#endif

WinDecorator::WinDecorator(BRect rect, int32 wlook, int32 wfeel, int32 wflags)
 : Decorator(rect,wlook,wfeel,wflags)
{
#ifdef DEBUG_DECOR
printf("WinDecorator()\n");
#endif
	taboffset=0;

	frame_highcol.SetColor(255,255,255);
	frame_midcol.SetColor(216,216,216);
	frame_lowcol.SetColor(110,110,110);
	frame_lowercol.SetColor(0,0,0);

	_DoLayout();
	
	textoffset=5;
}

WinDecorator::~WinDecorator(void)
{
#ifdef DEBUG_DECOR
printf("~WinDecorator()\n");
#endif
}

click_type WinDecorator::Clicked(BPoint pt, int32 buttons, int32 modifiers)
{
	if(_closerect.Contains(pt))
	{

#ifdef DEBUG_DECOR
printf("WinDecorator():Clicked() - Close\n");
#endif

		return DEC_CLOSE;
	}

	if(_zoomrect.Contains(pt))
	{

#ifdef DEBUG_DECOR
printf("WinDecorator():Clicked() - Zoom\n");
#endif

		return DEC_ZOOM;
	}
	
	// Clicking in the tab?
	if(_tabrect.Contains(pt))
	{
		// Here's part of our window management stuff
		if(buttons==B_PRIMARY_MOUSE_BUTTON && !GetFocus())
			return DEC_MOVETOFRONT;
		return DEC_DRAG;
	}

	// We got this far, so user is clicking on the border?
	BRect srect(_frame);
	srect.top+=19;
	BRect clientrect(srect.InsetByCopy(3,3));
	if(srect.Contains(pt) && !clientrect.Contains(pt))
	{
#ifdef DEBUG_DECOR
printf("WinDecorator():Clicked() - Resize\n");
#endif		
		return DEC_RESIZE;
	}

	// Guess user didn't click anything
#ifdef DEBUG_DECOR
printf("WinDecorator():Clicked()\n");
#endif
	return DEC_NONE;
}

void WinDecorator::_DoLayout(void)
{
#ifdef DEBUG_DECOR
printf("WinDecorator()::_DoLayout()\n");
#endif
	_borderrect=_frame;
	_tabrect=_frame;

	_tabrect.InsetBy(4,4);
	_tabrect.bottom=_tabrect.top+19;

	_zoomrect=_tabrect;
	_zoomrect.top+=3;
	_zoomrect.right-=3;
	_zoomrect.bottom-=3;
	_zoomrect.left=_zoomrect.right-15;
	
	_closerect=_zoomrect;
	_zoomrect.OffsetBy(0-_zoomrect.Width()-3,0);
	
	_minimizerect=_zoomrect;
	_minimizerect.OffsetBy(0-_zoomrect.Width()-1,0);
}

void WinDecorator::MoveBy(float x, float y)
{
	MoveBy(BPoint(x,y));
}

void WinDecorator::MoveBy(BPoint pt)
{
	// Move all internal rectangles the appropriate amount
	_frame.OffsetBy(pt);
	_closerect.OffsetBy(pt);
	_tabrect.OffsetBy(pt);
	_borderrect.OffsetBy(pt);
	_zoomrect.OffsetBy(pt);
	_minimizerect.OffsetBy(pt);
}

void WinDecorator::GetFootprint(BRegion *region)
{
	// This function calculates the decorator's footprint in coordinates
	// relative to the layer. This is most often used to set a WinBorder
	// object's visible region.
	if(!region)
		return;
	
	region->Set(_borderrect);
	region->Include(_tabrect);
}

void WinDecorator::_DrawTitle(BRect r)
{
	_drawdata.highcolor=_colors->window_tab_text;
	_drawdata.lowcolor=(GetFocus())?_colors->window_tab:_colors->inactive_window_tab;

	int32 titlecount=_ClipTitle((_minimizerect.left-5)-(_tabrect.left+5));
	BString titlestr=GetTitle();
	if(titlecount<titlestr.CountChars())
	{
		titlestr.Truncate(titlecount-1);
		titlestr+="...";
		titlecount+=2;
	}
	_driver->DrawString(titlestr.String(),titlecount,
		BPoint(_tabrect.left+5,_closerect.bottom-1),&_drawdata);
}

void WinDecorator::_SetFocus(void)
{
	// SetFocus() performs necessary duties for color swapping and
	// other things when a window is deactivated or activated.
	
	if(GetFocus())
	{
//		tab_highcol.SetColor(100,100,255);
//		tab_lowcol.SetColor(40,0,255);
		tab_highcol=_colors->window_tab;
		textcol=_colors->window_tab_text;
	}
	else
	{
//		tab_highcol.SetColor(220,220,220);
//		tab_lowcol.SetColor(128,128,128);
		tab_highcol=_colors->inactive_window_tab;
		textcol=_colors->inactive_window_tab_text;
	}
}

void WinDecorator::Draw(BRect update)
{
#ifdef DEBUG_DECOR
printf("WinDecorator::Draw(): "); update.PrintToStream();
#endif
	// Draw the top view's client area - just a hack :)
//	RGBColor blue(100,100,255);
//	_drawdata.highcolor=blue;

	_driver->FillRect(_borderrect,_colors->document_background);

	if(_borderrect.Intersects(update))
		_driver->FillRect(_borderrect,_colors->document_background);
	
	_DrawFrame(update);
	_DrawTab(update);
}

void WinDecorator::Draw(void)
{
#ifdef DEBUG_DECOR
printf("WinDecorator::Draw()\n");
#endif

	// Draw the top view's client area - just a hack :)
//	RGBColor blue(100,100,255);
//	_drawdata.highcolor=blue;

	_driver->FillRect(_borderrect,_colors->document_background);
	_driver->FillRect(_borderrect,_colors->document_background);
	DrawFrame();

	DrawTab();
}

void WinDecorator::_DrawZoom(BRect r)
{
	DrawBeveledRect(r,GetZoom());
	
	// Draw the Zoom box

	BRect rect(r);
	rect.InsetBy(2,2);
	rect.InsetBy(1,0);
	rect.bottom--;
	rect.right--;
	
	if(GetZoom())
		rect.OffsetBy(1,1);

	_drawdata.highcolor.SetColor(0,0,0);
	_driver->StrokeRect(rect,_drawdata.highcolor);
	rect.InsetBy(1,1);
	_driver->StrokeLine(rect.LeftTop(),rect.RightTop(),_drawdata.highcolor);
	
}

void WinDecorator::_DrawClose(BRect r)
{
	// Just like DrawZoom, but for a close button
	DrawBeveledRect(r,GetClose());
	
	// Draw the X

	BRect rect(r);
	rect.InsetBy(4,4);
	rect.right--;
	rect.top--;
	
	if(GetClose())
		rect.OffsetBy(1,1);

	_drawdata.highcolor.SetColor(0,0,0);
	_driver->StrokeLine(rect.LeftTop(),rect.RightBottom(),_drawdata.highcolor);
	_driver->StrokeLine(rect.RightTop(),rect.LeftBottom(),_drawdata.highcolor);
	rect.OffsetBy(1,0);
	_driver->StrokeLine(rect.LeftTop(),rect.RightBottom(),_drawdata.highcolor);
	_driver->StrokeLine(rect.RightTop(),rect.LeftBottom(),_drawdata.highcolor);
}

void WinDecorator::_DrawMinimize(BRect r)
{
	// Just like DrawZoom, but for a Minimize button
	DrawBeveledRect(r,GetMinimize());

	_drawdata.highcolor=textcol;
	BRect rect(r.left+5,r.bottom-4,r.right-5,r.bottom-3);
	if(GetMinimize())
		rect.OffsetBy(1,1);
	
	_drawdata.highcolor.SetColor(0,0,0);
	_driver->StrokeRect(rect,_drawdata.highcolor);
}

void WinDecorator::_DrawTab(BRect r)
{
	// If a window has a tab, this will draw it and any buttons which are
	// in it.
	if(_look==B_NO_BORDER_WINDOW_LOOK)
		return;
	
//	_driver->StrokeRect(_tabrect,frame_lowcol);

//	UpdateTitle(layer->name->String());

	_driver->FillRect(_tabrect,tab_highcol);

	// Draw the buttons if we're supposed to	
	if(!(_flags & B_NOT_CLOSABLE))
		_DrawClose(_closerect);
	if(!(_flags & B_NOT_ZOOMABLE))
		_DrawZoom(_zoomrect);
}

void WinDecorator::DrawBeveledRect(BRect r, bool down)
{
	RGBColor higher,high,mid,low,lower;
	
	if(down)
	{
		lower.SetColor(255,255,255);
		low.SetColor(216,216,216);
		mid.SetColor(192,192,192);
		high.SetColor(128,128,128);
		higher.SetColor(0,0,0);
	}
	else
	{
		higher.SetColor(255,255,255);
		high.SetColor(216,216,216);
		mid.SetColor(192,192,192);
		low.SetColor(128,128,128);
		lower.SetColor(0,0,0);
	}

	BRect rect(r);
	BPoint pt;

	// Top highlight
	_drawdata.highcolor=higher;
	_driver->StrokeLine(rect.LeftTop(),rect.RightTop(),higher);

	// Left highlight
	_driver->StrokeLine(rect.LeftTop(),rect.LeftBottom(),higher);

	// Right shading
	pt=rect.RightTop();
	pt.y++;
	_driver->StrokeLine(pt,rect.RightBottom(),lower);
	
	// Bottom shading
	pt=rect.LeftBottom();
	pt.x++;
	_driver->StrokeLine(pt,rect.RightBottom(),lower);

	rect.InsetBy(1,1);

	// Top inside highlight
	_driver->StrokeLine(rect.LeftTop(),rect.RightTop(),higher);

	// Left inside highlight
	_driver->StrokeLine(rect.LeftTop(),rect.LeftBottom(),higher);

	// Right inside shading
	pt=rect.RightTop();
	pt.y++;
	_driver->StrokeLine(pt,rect.RightBottom(),lower);
	
	// Bottom inside shading
	pt=rect.LeftBottom();
	pt.x++;
	_driver->StrokeLine(pt,rect.RightBottom(),lower);
	
	rect.InsetBy(1,1);

	_driver->FillRect(rect,mid);
}

void WinDecorator::_SetColors(void)
{
	_SetFocus();
}

void WinDecorator::_DrawFrame(BRect rect)
{
	if(_look==B_NO_BORDER_WINDOW_LOOK)
		return;

	BRect r=_borderrect;
	
	_drawdata.highcolor.SetColor(255,0,0);
	_driver->StrokeRect(r,_drawdata.highcolor);
	
	BPoint pt;

	pt=r.RightTop();
	pt.x--;
	_driver->StrokeLine(r.LeftTop(),pt,frame_midcol);
	pt=r.LeftBottom();
	pt.y--;
	_driver->StrokeLine(r.LeftTop(),pt,frame_midcol);

	_driver->StrokeLine(r.RightTop(),r.RightBottom(),frame_lowercol);
	_driver->StrokeLine(r.LeftBottom(),r.RightBottom(),frame_lowercol);
	
	r.InsetBy(1,1);
	pt=r.RightTop();
	pt.x--;
	_driver->StrokeLine(r.LeftTop(),pt,frame_highcol);
	pt=r.LeftBottom();
	pt.y--;
	_driver->StrokeLine(r.LeftTop(),pt,frame_highcol);

	_driver->StrokeLine(r.RightTop(),r.RightBottom(),frame_lowcol);
	_driver->StrokeLine(r.LeftBottom(),r.RightBottom(),frame_lowcol);
	
	r.InsetBy(1,1);
	_driver->StrokeRect(r,frame_midcol);
	r.InsetBy(1,1);
	_driver->StrokeRect(r,frame_midcol);
}

extern "C" float get_decorator_version(void)
{
	return 1.00;
}

extern "C" Decorator *instantiate_decorator(BRect rect, int32 wlook, int32 wfeel, int32 wflags)
{
	return new WinDecorator(rect,wlook,wfeel,wflags);
}