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

WinDecorator::WinDecorator(SRect rect, int32 wlook, int32 wfeel, int32 wflags)
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

click_type WinDecorator::Clicked(SPoint pt, int32 buttons, int32 modifiers)
{
	if(closerect.Contains(pt))
	{

#ifdef DEBUG_DECOR
printf("WinDecorator():Clicked() - Close\n");
#endif

		return CLICK_CLOSE;
	}

	if(zoomrect.Contains(pt))
	{

#ifdef DEBUG_DECOR
printf("WinDecorator():Clicked() - Zoom\n");
#endif

		return CLICK_ZOOM;
	}
	
	// Clicking in the tab?
	if(tabrect.Contains(pt))
	{
		// Here's part of our window management stuff
		if(buttons==B_PRIMARY_MOUSE_BUTTON && !GetFocus())
			return CLICK_MOVETOFRONT;
		return CLICK_DRAG;
	}

	// We got this far, so user is clicking on the border?
	SRect srect(frame);
	srect.top+=19;
	SRect clientrect(srect.InsetByCopy(3,3));
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
	tabrect=frame;
	borderrect=frame;

	borderrect.top+=19;
	
	tabrect.bottom=tabrect.top+18;

	zoomrect=tabrect;
	zoomrect.top+=4;
	zoomrect.right-=4;
	zoomrect.bottom-=4;
	zoomrect.left=zoomrect.right-10;
	zoomrect.bottom=zoomrect.top+10;
	
	closerect=zoomrect;
	zoomrect.OffsetBy(0-zoomrect.Width()-4,0);
	
	minimizerect=zoomrect;
	minimizerect.OffsetBy(0-zoomrect.Width()-2,0);
}

void WinDecorator::MoveBy(float x, float y)
{
	MoveBy(SPoint(x,y));
}

void WinDecorator::MoveBy(SPoint pt)
{
	// Move all internal rectangles the appropriate amount
	frame.OffsetBy(pt);
	closerect.OffsetBy(pt);
	tabrect.OffsetBy(pt);
	borderrect.OffsetBy(pt);
	zoomrect.OffsetBy(pt);
	minimizerect.OffsetBy(pt);
}
/*
SRegion * WinDecorator::GetFootprint(void)
{
	// This function calculates the decorator's footprint in coordinates
	// relative to the layer. This is most often used to set a WindowBorder
	// object's visible region.
	
	SRegion *reg=new SRegion(borderrect);
	reg->Include(tabrect);
	return reg;
}
*/

void WinDecorator::_DrawTitle(SRect r)
{
	// Designed simply to redraw the title when it has changed on
	// the client side.
/*	if(title)
	{
		driver->SetDrawingMode(B_OP_OVER);
		rgb_color tmpcol=driver->HighColor();
		driver->SetHighColor(textcol.red,textcol.green,textcol.blue);
		driver->DrawString((char *)string,strlen(string),
			BPoint(frame.left+textoffset,closerect.bottom-1));
		driver->SetHighColor(tmpcol.red,tmpcol.green,tmpcol.blue);
		driver->SetDrawingMode(B_OP_COPY);
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

void WinDecorator::Draw(SRect update)
{
#ifdef DEBUG_DECOR
printf("WinDecorator::Draw(): "); update.PrintToStream();
#endif
	// Draw the top view's client area - just a hack :)
	RGBColor blue(100,100,255);

	layerdata.highcolor=blue;

	if(borderrect.Intersects(update))
		driver->FillRect(borderrect,&layerdata,(int8*)&solidhigh);
	
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

	layerdata.highcolor=blue;

	driver->FillRect(borderrect,&layerdata,(int8*)&solidhigh);
	DrawFrame();

	DrawTab();
}

void WinDecorator::_DrawZoom(SRect r)
{
	DrawBlendedRect(r,GetZoom());

	// Draw the Zoom box
	layerdata.highcolor=textcol;
	driver->StrokeRect(r.InsetByCopy(2,2),&layerdata,(int8*)&solidhigh);
}

void WinDecorator::_DrawClose(SRect r)
{
	// Just like DrawZoom, but for a close button
	DrawBlendedRect(r,GetClose());
	
	// Draw the X
	layerdata.highcolor=textcol;
	driver->StrokeLine(SPoint(closerect.left+2,closerect.top+2),SPoint(closerect.right-2,
		closerect.bottom-2),&layerdata,(int8*)&solidhigh);
	driver->StrokeLine(SPoint(closerect.right-2,closerect.top+2),SPoint(closerect.left+2,
		closerect.bottom-2),&layerdata,(int8*)&solidhigh);
}

void WinDecorator::_DrawMinimize(SRect r)
{
	// Just like DrawZoom, but for a Minimize button
	DrawBlendedRect(r,GetMinimize());

	layerdata.highcolor=textcol;
	driver->StrokeLine(SPoint(minimizerect.left+2,minimizerect.bottom-2),SPoint(minimizerect.right-2,
		minimizerect.bottom-2),&layerdata,(int8*)&solidhigh);
}

void WinDecorator::_DrawTab(SRect r)
{
	// If a window has a tab, this will draw it and any buttons which are
	// in it.
	if(look==WLOOK_NO_BORDER)
		return;
	
	layerdata.highcolor=frame_lowcol;
	driver->StrokeRect(tabrect,&layerdata,(int8*)&solidhigh);

//	UpdateTitle(layer->name->String());

	layerdata.highcolor=tab_lowcol;
	driver->FillRect(tabrect.InsetByCopy(1,1),&layerdata,(int8*)&solidhigh);

	// Draw the buttons if we're supposed to	
	if(!(flags & NOT_CLOSABLE))
		_DrawClose(closerect);
	if(!(flags & NOT_ZOOMABLE))
		_DrawZoom(zoomrect);
}

void WinDecorator::DrawBlendedRect(SRect r, bool down)
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
		layerdata.highcolor=tmpcol;
		driver->StrokeLine(SPoint(r.left,r.top+i),
			SPoint(r.left+i,r.top),&layerdata,(int8*)&solidhigh);

		SetRGBColor(&tmpcol, uint8(halfcol.red-(i*rstep)),
			uint8(halfcol.green-(i*gstep)),
			uint8(halfcol.blue-(i*bstep)));

		layerdata.highcolor=tmpcol;
		driver->StrokeLine(SPoint(r.left+steps,r.top+i),
			SPoint(r.left+i,r.top+steps),&layerdata,(int8*)&solidhigh);

	}

//	layerdata.highcolor=startcol;
//	driver->FillRect(r,&layerdata,(int8*)&solidhigh);
	layerdata.highcolor=frame_lowcol;
	driver->StrokeRect(r,&layerdata,(int8*)&solidhigh);
}

void WinDecorator::_DrawFrame(SRect rect)
{
	if(look==WLOOK_NO_BORDER)
		return;
	
	SRect r=borderrect;
	
	layerdata.highcolor=frame_midcol;
	driver->StrokeLine(SPoint(r.left,r.top),SPoint(r.right-1,r.top),
		&layerdata,(int8*)&solidhigh);
	layerdata.highcolor=frame_lowcol;
	driver->StrokeLine(SPoint(r.left,r.top),SPoint(r.left,r.bottom),
		&layerdata,(int8*)&solidhigh);
	layerdata.highcolor=frame_lowercol;
	driver->StrokeLine(SPoint(r.right,r.bottom),SPoint(r.right,r.top),
		&layerdata,(int8*)&solidhigh);
	layerdata.highcolor=frame_lowercol;
	driver->StrokeLine(SPoint(r.right,r.bottom),SPoint(r.left,r.bottom),
		&layerdata,(int8*)&solidhigh);

	r.InsetBy(1,1);
	layerdata.highcolor=frame_highercol;
	driver->StrokeLine(SPoint(r.left,r.top),SPoint(r.right-1,r.top),
		&layerdata,(int8*)&solidhigh);
	layerdata.highcolor=frame_highercol;
	driver->StrokeLine(SPoint(r.left,r.top),SPoint(r.left,r.bottom),
		&layerdata,(int8*)&solidhigh);
	layerdata.highcolor=frame_midcol;
	driver->StrokeLine(SPoint(r.right,r.bottom),SPoint(r.right,r.top),
		&layerdata,(int8*)&solidhigh);
	layerdata.highcolor=frame_midcol;
	driver->StrokeLine(SPoint(r.right,r.bottom),SPoint(r.left,r.bottom),
		&layerdata,(int8*)&solidhigh);
	
	r.InsetBy(1,1);
	layerdata.highcolor=frame_highcol;
	driver->StrokeRect(r,&layerdata,(int8*)&solidhigh);

	r.InsetBy(1,1);
	layerdata.highcolor=frame_lowercol;
	driver->StrokeLine(SPoint(r.left,r.top),SPoint(r.right-1,r.top),
		&layerdata,(int8*)&solidhigh);
	layerdata.highcolor=frame_lowercol;
	driver->StrokeLine(SPoint(r.left,r.top),SPoint(r.left,r.bottom),
		&layerdata,(int8*)&solidhigh);
	layerdata.highcolor=frame_highercol;
	driver->StrokeLine(SPoint(r.right,r.bottom),SPoint(r.right,r.top),
		&layerdata,(int8*)&solidhigh);
	layerdata.highcolor=frame_highercol;
	driver->StrokeLine(SPoint(r.right,r.bottom),SPoint(r.left,r.bottom),
		&layerdata,(int8*)&solidhigh);
}

extern "C" float get_decorator_version(void)
{
	return 1.00;
}

extern "C" Decorator *instantiate_decorator(SRect rect, int32 wlook, int32 wfeel, int32 wflags)
{
	return new WinDecorator(rect,wlook,wfeel,wflags);
}