#include "DisplayDriver.h"
#include <View.h>
#include "LayerData.h"
#include "ColorUtils.h"
#include "BeDecorator.h"
#include "RGBColor.h"

//#define DEBUG_DECOR

#ifdef DEBUG_DECOR
#include <stdio.h>
#endif

BeDecorator::BeDecorator(BRect rect, int32 wlook, int32 wfeel, int32 wflags)
 : Decorator(rect,wlook,wfeel,wflags)
{
#ifdef DEBUG_DECOR
printf("BeDecorator()\n");
#endif
	taboffset=0;

	// These hard-coded assignments will go bye-bye when the system colors 
	// API is implemented

	// commented these out because they are taken care of by the SetFocus() call
	SetFocus(false);

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

	tab_highcol=colors->window_tab;
	tab_lowcol=colors->window_tab;
}

BeDecorator::~BeDecorator(void)
{
#ifdef DEBUG_DECOR
printf("~BeDecorator()\n");
#endif
}

click_type BeDecorator::Clicked(BPoint pt, int32 buttons, int32 modifiers)
{
	if(closerect.Contains(pt))
	{

#ifdef DEBUG_DECOR
printf("BeDecorator():Clicked() - Close\n");
#endif

		return CLICK_CLOSE;
	}

	if(zoomrect.Contains(pt))
	{

#ifdef DEBUG_DECOR
printf("BeDecorator():Clicked() - Zoom\n");
#endif

		return CLICK_ZOOM;
	}
	
	if(resizerect.Contains(pt) && look==WLOOK_DOCUMENT)
	{

#ifdef DEBUG_DECOR
printf("BeDecorator():Clicked() - Resize thumb\n");
#endif

		return CLICK_RESIZE;
	}

	// Clicking in the tab?
	if(tabrect.Contains(pt))
	{
		// Here's part of our window management stuff
		if(buttons==B_PRIMARY_MOUSE_BUTTON && !GetFocus())
			return CLICK_MOVETOFRONT;
		if(buttons==B_SECONDARY_MOUSE_BUTTON)
			return CLICK_MOVETOBACK;
		return CLICK_DRAG;
	}

	// We got this far, so user is clicking on the border?
	BRect brect(frame);
	brect.top+=19;
	BRect clientrect(brect.InsetByCopy(3,3));
	if(brect.Contains(pt) && !clientrect.Contains(pt))
	{
#ifdef DEBUG_DECOR
printf("BeDecorator():Clicked() - Drag\n");
#endif		
		if(resizerect.Contains(pt))
			return CLICK_RESIZE;
		
		return CLICK_DRAG;
	}

	// Guess user didn't click anything
#ifdef DEBUG_DECOR
printf("BeDecorator():Clicked()\n");
#endif
	return CLICK_NONE;
}

void BeDecorator::_DoLayout(void)
{
	// Here we determine the size of every rectangle that we use
	// internally when we are given the size of the client rectangle.
	
	// Current version simply makes everything fit inside the rect
	// instead of building around it. This will change.
	
#ifdef DEBUG_DECOR
printf("BeDecorator()::_DoLayout()"); rect.PrintToStream();
#endif
	tabrect=frame;
	resizerect=frame;
	borderrect=frame;
	closerect=frame;

	closerect.left+=(look==WLOOK_FLOATING)?2:4;
	closerect.top+=(look==WLOOK_FLOATING)?6:4;
	closerect.right=closerect.left+10;
	closerect.bottom=closerect.top+10;

	borderrect.top+=19;
	
	resizerect.top=resizerect.bottom-18;
	resizerect.left=resizerect.right-18;
	
	tabrect.bottom=tabrect.top+18;
/*	if(GetTitle())
	{
		float titlewidth=closerect.right
			+driver->StringWidth(layer->name->String(),
			layer->name->Length())
			+35;
		tabrect.right=(titlewidth<frame.right -1)?titlewidth:frame.right;
	}
	else
*/		tabrect.right=tabrect.left+tabrect.Width()/2;

	if(look==WLOOK_FLOATING)
		tabrect.top+=4;

	zoomrect=tabrect;
	zoomrect.top+=(look==WLOOK_FLOATING)?2:4;
	zoomrect.right-=4;
	zoomrect.bottom-=4;
	zoomrect.left=zoomrect.right-10;
	zoomrect.bottom=zoomrect.top+10;
	
	textoffset=(look==WLOOK_FLOATING)?5:7;
}

void BeDecorator::MoveBy(float x, float y)
{
	MoveBy(BPoint(x,y));
}

void BeDecorator::MoveBy(BPoint pt)
{
	// Move all internal rectangles the appropriate amount
	frame.OffsetBy(pt);
	closerect.OffsetBy(pt);
	tabrect.OffsetBy(pt);
	resizerect.OffsetBy(pt);
	borderrect.OffsetBy(pt);
	zoomrect.OffsetBy(pt);
}
/*
SRegion * BeDecorator::GetFootprint(void)
{
	// This function calculates the decorator's footprint in coordinates
	// relative to the layer. This is most often used to set a WindowBorder
	// object's visible region.
	
	SRegion *reg=new SRegion(borderrect);
	reg->Include(tabrect);
	return reg;
}
*/

void BeDecorator::_DrawTitle(BRect r)
{
	// Designed simply to redraw the title when it has changed on
	// the client side.
/*	driver->SetDrawingMode(B_OP_OVER);
	rgb_color tmpcol=driver->HighColor();
	driver->SetHighColor(textcol.red,textcol.green,textcol.blue);
	driver->DrawString((char *)string,strlen(string),
		BPoint(closerect.right+textoffset,closerect.bottom-1));
	driver->SetHighColor(tmpcol.red,tmpcol.green,tmpcol.blue);
	driver->SetDrawingMode(B_OP_COPY);
*/
}

void BeDecorator::_SetFocus(void)
{
	// SetFocus() performs necessary duties for color swapping and
	// other things when a window is deactivated or activated.
	
	if(GetFocus())
	{
//		tab_highcol.SetColor(255,236,33);
//		tab_lowcol.SetColor(234,181,0);
		tab_highcol=colors->window_tab;
		tab_lowcol=colors->window_tab;

		button_highcol.SetColor(255,255,0);
		button_lowcol.SetColor(255,203,0);
	}
	else
	{
		tab_highcol.SetColor(235,235,235);
		tab_lowcol.SetColor(160,160,160);

		button_highcol.SetColor(229,229,229);
		button_lowcol.SetColor(153,153,153);
	}

}

void BeDecorator::Draw(BRect update)
{
	// We need to draw a few things: the tab, the resize thumb, the borders,
	// and the buttons

	_DrawTab(update);

	// Draw the top view's client area - just a hack :)
	RGBColor blue(100,100,255);

	layerdata.highcolor=blue;

	if(borderrect.Intersects(update))
		driver->FillRect(borderrect,&layerdata,(int8*)&solidhigh);
	
	_DrawFrame(update);
}

void BeDecorator::Draw(void)
{
	// Easy way to draw everything - no worries about drawing only certain
	// things

	// Draw the top view's client area - just a hack :)
	RGBColor blue(100,100,255);

	layerdata.highcolor=blue;

	driver->FillRect(borderrect,&layerdata,(int8*)&solidhigh);
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
	if(look==WLOOK_NO_BORDER)
		return;
	
	layerdata.highcolor=frame_lowcol;
	driver->StrokeRect(tabrect,&layerdata,(int8*)&solidhigh);

//	UpdateTitle(layer->name->String());

	layerdata.highcolor=colors->window_tab;
	driver->FillRect(tabrect.InsetByCopy(1,1),&layerdata,(int8*)&solidhigh);

	// Draw the buttons if we're supposed to	
	if(!(flags & NOT_CLOSABLE))
		_DrawClose(closerect);
	if(!(flags & NOT_ZOOMABLE))
		_DrawZoom(zoomrect);
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
		layerdata.highcolor=tmpcol;
		driver->StrokeLine(BPoint(r.left,r.top+i),
			BPoint(r.left+i,r.top),&layerdata,(int8*)&solidhigh);

		SetRGBColor(&tmpcol, uint8(halfcol.red-(i*rstep)),
			uint8(halfcol.green-(i*gstep)),
			uint8(halfcol.blue-(i*bstep)));

		layerdata.highcolor=tmpcol;
		driver->StrokeLine(BPoint(r.left+steps,r.top+i),
			BPoint(r.left+i,r.top+steps),&layerdata,(int8*)&solidhigh);

	}

//	layerdata.highcolor=startcol;
//	driver->FillRect(r,&layerdata,(int8*)&solidhigh);
	layerdata.highcolor=frame_lowcol;
	driver->StrokeRect(r,&layerdata,(int8*)&solidhigh);
}

void BeDecorator::_DrawFrame(BRect rect)
{
	// Duh, draws the window frame, I think. ;)

	if(look==WLOOK_NO_BORDER)
		return;
	
	BRect r=borderrect;

	layerdata.highcolor=frame_midcol;
	driver->StrokeLine(BPoint(r.left,r.top),BPoint(r.right-1,r.top),
		&layerdata,(int8*)&solidhigh);
	layerdata.highcolor=frame_lowcol;
	driver->StrokeLine(BPoint(r.left,r.top),BPoint(r.left,r.bottom),
		&layerdata,(int8*)&solidhigh);
	layerdata.highcolor=frame_lowercol;
	driver->StrokeLine(BPoint(r.right,r.bottom),BPoint(r.right,r.top),
		&layerdata,(int8*)&solidhigh);
	layerdata.highcolor=frame_lowercol;
	driver->StrokeLine(BPoint(r.right,r.bottom),BPoint(r.left,r.bottom),
		&layerdata,(int8*)&solidhigh);

	r.InsetBy(1,1);
	layerdata.highcolor=frame_highercol;
	driver->StrokeLine(BPoint(r.left,r.top),BPoint(r.right-1,r.top),
		&layerdata,(int8*)&solidhigh);
	layerdata.highcolor=frame_highercol;
	driver->StrokeLine(BPoint(r.left,r.top),BPoint(r.left,r.bottom),
		&layerdata,(int8*)&solidhigh);
	layerdata.highcolor=frame_midcol;
	driver->StrokeLine(BPoint(r.right,r.bottom),BPoint(r.right,r.top),
		&layerdata,(int8*)&solidhigh);
	layerdata.highcolor=frame_midcol;
	driver->StrokeLine(BPoint(r.right,r.bottom),BPoint(r.left,r.bottom),
		&layerdata,(int8*)&solidhigh);
	
	r.InsetBy(1,1);
	layerdata.highcolor=frame_highcol;
	driver->StrokeRect(r,&layerdata,(int8*)&solidhigh);

	r.InsetBy(1,1);
	layerdata.highcolor=frame_lowercol;
	driver->StrokeLine(BPoint(r.left,r.top),BPoint(r.right-1,r.top),
		&layerdata,(int8*)&solidhigh);
	layerdata.highcolor=frame_lowercol;
	driver->StrokeLine(BPoint(r.left,r.top),BPoint(r.left,r.bottom),
		&layerdata,(int8*)&solidhigh);
	layerdata.highcolor=frame_highercol;
	driver->StrokeLine(BPoint(r.right,r.bottom),BPoint(r.right,r.top),
		&layerdata,(int8*)&solidhigh);
	layerdata.highcolor=frame_highercol;
	driver->StrokeLine(BPoint(r.right,r.bottom),BPoint(r.left,r.bottom),
		&layerdata,(int8*)&solidhigh);
	driver->StrokeRect(borderrect,&layerdata,(int8*)&solidhigh);

	// Draw the resize thumb if we're supposed to
	if(!(flags & NOT_RESIZABLE))
	{
		r=resizerect;

		int32 w=r.IntegerWidth(),  h=r.IntegerHeight();
		
		// This code is strictly for B_DOCUMENT_WINDOW looks
		if(look==WLOOK_DOCUMENT)
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
				layerdata.highcolor.SetColor(uint8(startcol.red-(i*rstep)),
					uint8(startcol.green-(i*gstep)),
					uint8(startcol.blue-(i*bstep)));
				
				driver->StrokeLine(BPoint(r.left,r.top+i),
					BPoint(r.left+i,r.top),&layerdata,(int8*)&solidhigh);
		
				layerdata.highcolor.SetColor(uint8(halfcol.red-(i*rstep)),
					uint8(halfcol.green-(i*gstep)),
					uint8(halfcol.blue-(i*bstep)));
				driver->StrokeLine(BPoint(r.left+steps,r.top+i),
					BPoint(r.left+i,r.top+steps),&layerdata,(int8*)&solidhigh);			
			}
			layerdata.highcolor=frame_lowercol;
			driver->StrokeRect(r,&layerdata,(int8*)&solidhigh);
		}
		else
		{
			layerdata.highcolor=frame_lowercol;
			driver->StrokeLine(BPoint(r.right,r.top),BPoint(r.right-3,r.top),
				&layerdata,(int8*)&solidhigh);
			driver->StrokeLine(BPoint(r.left,r.bottom),BPoint(r.left,r.bottom-3),
				&layerdata,(int8*)&solidhigh);
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
