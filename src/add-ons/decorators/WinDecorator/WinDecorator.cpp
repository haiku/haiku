#include "DisplayDriver.h"
#include <View.h>
#include "LayerData.h"
#include "ColorUtils.h"
#include "WinDecorator.h"
#include "RGBColor.h"

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

	// These hard-coded assignments will go bye-bye when the system colors 
	// API is implemented
	tab_highcol.SetColor(100,100,255);
	tab_lowcol.SetColor(40,0,255);

	button_highercol.SetColor(255,255,255);
	button_lowercol.SetColor(0,0,0);
	
	button_highcol=button_lowercol.MakeBlendColor(button_highercol,0.85);
	button_midcol=button_lowercol.MakeBlendColor(button_highercol,0.75);
	button_lowcol=button_lowercol.MakeBlendColor(button_highercol,0.5);

	frame_highercol.SetColor(216,216,216);
	frame_lowercol.SetColor(110,110,110);

	textcol.SetColor(0,0,0);
	
	frame_highcol=frame_lowercol.MakeBlendColor(frame_highercol,0.75);
	frame_midcol=frame_lowercol.MakeBlendColor(frame_highercol,0.5);
	frame_lowcol=frame_lowercol.MakeBlendColor(frame_highercol,0.25);

	_DoLayout();
	
	textoffset=5;
	solidhigh=0xFFFFFFFFFFFFFFFFLL;
	solidlow=0;
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

		return CLICK_CLOSE;
	}

	if(_zoomrect.Contains(pt))
	{

#ifdef DEBUG_DECOR
printf("WinDecorator():Clicked() - Zoom\n");
#endif

		return CLICK_ZOOM;
	}
	
	// Clicking in the tab?
	if(_tabrect.Contains(pt))
	{
		// Here's part of our window management stuff
		if(buttons==B_PRIMARY_MOUSE_BUTTON && !GetFocus())
			return CLICK_MOVETOFRONT;
		return CLICK_DRAG;
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
		return CLICK_RESIZE;
	}

	// Guess user didn't click anything
#ifdef DEBUG_DECOR
printf("WinDecorator():Clicked()\n");
#endif
	return CLICK_NONE;
}

void WinDecorator::_DoLayout(void)
{
#ifdef DEBUG_DECOR
printf("WinDecorator()::_DoLayout()\n");
#endif
	_tabrect=_frame;
	_borderrect=_frame;

	_borderrect.top+=19;
	
	_tabrect.bottom=_tabrect.top+18;

	_zoomrect=_tabrect;
	_zoomrect.top+=4;
	_zoomrect.right-=4;
	_zoomrect.bottom-=4;
	_zoomrect.left=_zoomrect.right-10;
	_zoomrect.bottom=_zoomrect.top+10;
	
	_closerect=_zoomrect;
	_zoomrect.OffsetBy(0-_zoomrect.Width()-4,0);
	
	_minimizerect=_zoomrect;
	_minimizerect.OffsetBy(0-_zoomrect.Width()-2,0);
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
/*
SRegion * WinDecorator::GetFootprint(void)
{
	// This function calculates the decorator's footprint in coordinates
	// relative to the layer. This is most often used to set a WindowBorder
	// object's visible region.
	
	SRegion *reg=new SRegion(_borderrect);
	reg->Include(_tabrect);
	return reg;
}
*/

void WinDecorator::_DrawTitle(BRect r)
{
	// Designed simply to redraw the title when it has changed on
	// the client side.
/*	if(title)
	{
		_driver->SetDrawingMode(B_OP_OVER);
		rgb_color tmpcol=_driver->HighColor();
		_driver->SetHighColor(textcol.red,textcol.green,textcol.blue);
		_driver->DrawString((char *)string,strlen(string),
			BPoint(_frame.left+textoffset,_closerect.bottom-1));
		_driver->SetHighColor(tmpcol.red,tmpcol.green,tmpcol.blue);
		_driver->SetDrawingMode(B_OP_COPY);
	}
*/
}

void WinDecorator::_SetFocus(void)
{
	// SetFocus() performs necessary duties for color swapping and
	// other things when a window is deactivated or activated.
	
	if(GetFocus())
	{
		tab_highcol.SetColor(100,100,255);
		tab_lowcol.SetColor(40,0,255);
	}
	else
	{
		tab_highcol.SetColor(220,220,220);
		tab_lowcol.SetColor(128,128,128);
	}
}

void WinDecorator::Draw(BRect update)
{
#ifdef DEBUG_DECOR
printf("WinDecorator::Draw(): "); update.PrintToStream();
#endif
	// Draw the top view's client area - just a hack :)
	RGBColor blue(100,100,255);

	_layerdata.highcolor=blue;

	if(_borderrect.Intersects(update))
		_driver->FillRect(_borderrect,&_layerdata,(int8*)&solidhigh);
	
	_DrawFrame(update);
	_DrawTab(update);
}

void WinDecorator::Draw(void)
{
#ifdef DEBUG_DECOR
printf("WinDecorator::Draw()\n");
#endif

	// Draw the top view's client area - just a hack :)
	RGBColor blue(100,100,255);

	_layerdata.highcolor=blue;

	_driver->FillRect(_borderrect,&_layerdata,(int8*)&solidhigh);
	DrawFrame();

	DrawTab();
}

void WinDecorator::_DrawZoom(BRect r)
{
	DrawBlendedRect(r,GetZoom());

	// Draw the Zoom box
	_layerdata.highcolor=textcol;
	_driver->StrokeRect(r.InsetByCopy(2,2),&_layerdata,(int8*)&solidhigh);
}

void WinDecorator::_DrawClose(BRect r)
{
	// Just like DrawZoom, but for a close button
	DrawBlendedRect(r,GetClose());
	
	// Draw the X
	_layerdata.highcolor=textcol;
	_driver->StrokeLine(BPoint(_closerect.left+2,_closerect.top+2),BPoint(_closerect.right-2,
		_closerect.bottom-2),&_layerdata,(int8*)&solidhigh);
	_driver->StrokeLine(BPoint(_closerect.right-2,_closerect.top+2),BPoint(_closerect.left+2,
		_closerect.bottom-2),&_layerdata,(int8*)&solidhigh);
}

void WinDecorator::_DrawMinimize(BRect r)
{
	// Just like DrawZoom, but for a Minimize button
	DrawBlendedRect(r,GetMinimize());

	_layerdata.highcolor=textcol;
	_driver->StrokeLine(BPoint(_minimizerect.left+2,_minimizerect.bottom-2),BPoint(_minimizerect.right-2,
		_minimizerect.bottom-2),&_layerdata,(int8*)&solidhigh);
}

void WinDecorator::_DrawTab(BRect r)
{
	// If a window has a tab, this will draw it and any buttons which are
	// in it.
	if(_look==B_NO_BORDER_WINDOW_LOOK)
		return;
	
	_layerdata.highcolor=frame_lowcol;
	_driver->StrokeRect(_tabrect,&_layerdata,(int8*)&solidhigh);

//	UpdateTitle(layer->name->String());

	_layerdata.highcolor=tab_lowcol;
	_driver->FillRect(_tabrect.InsetByCopy(1,1),&_layerdata,(int8*)&solidhigh);

	// Draw the buttons if we're supposed to	
	if(!(_flags & B_NOT_CLOSABLE))
		_DrawClose(_closerect);
	if(!(_flags & B_NOT_ZOOMABLE))
		_DrawZoom(_zoomrect);
}

void WinDecorator::DrawBlendedRect(BRect r, bool down)
{
	// This bad boy is used to draw a rectangle with a gradient.
	// Note that it is not part of the Decorator API - it's specific
	// to just the WinDecorator. Called by DrawZoom and DrawClose

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

void WinDecorator::_DrawFrame(BRect rect)
{
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
}

extern "C" float get_decorator_version(void)
{
	return 1.00;
}

extern "C" Decorator *instantiate_decorator(BRect rect, int32 wlook, int32 wfeel, int32 wflags)
{
	return new WinDecorator(rect,wlook,wfeel,wflags);
}
