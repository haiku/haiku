#include "DisplayDriver.h"
#include <View.h>
#include "LayerData.h"
#include "ColorUtils.h"
#include "DefaultDecorator.h"
#include "RGBColor.h"

DefaultDecorator::DefaultDecorator(BRect rect, int32 wlook, int32 wfeel, int32 wflags)
 : Decorator(rect,wlook,wfeel,wflags)
{
	taboffset=0;

	// These hard-coded assignments will go bye-bye when the system _colors 
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

	tab_highcol=_colors->window_tab;
	tab_lowcol=_colors->window_tab;
}

DefaultDecorator::~DefaultDecorator(void)
{
}

click_type DefaultDecorator::Clicked(BPoint pt, int32 buttons, int32 modifiers)
{
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
		if(buttons==B_PRIMARY_MOUSE_BUTTON && !GetFocus())
			return CLICK_MOVETOFRONT;
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

void DefaultDecorator::_DoLayout(void)
{
	// Here we determine the size of every rectangle that we use
	// internally when we are given the size of the client rectangle.
	
	// Current version simply makes everything fit inside the rect
	// instead of building around it. This will change.
	
	_tabrect=_frame;
	_resizerect=_frame;
	_borderrect=_frame;
	_closerect=_frame;

	_closerect.left+=(_look==B_FLOATING_WINDOW_LOOK)?2:4;
	_closerect.top+=(_look==B_FLOATING_WINDOW_LOOK)?6:4;
	_closerect.right=_closerect.left+10;
	_closerect.bottom=_closerect.top+10;

	_borderrect.top+=19;
	
	_resizerect.top=_resizerect.bottom-18;
	_resizerect.left=_resizerect.right-18;
	
	_tabrect.bottom=_tabrect.top+18;
/*	if(GetTitle())
	{
		float titlewidth=_closerect.right
			+_driver->StringWidth(layer->name->String(),
			layer->name->Length())
			+35;
		_tabrect.right=(titlewidth<_frame.right -1)?titlewidth:_frame.right;
	}
	else
*/		_tabrect.right=_tabrect.left+_tabrect.Width()/2;

	if(_look==B_FLOATING_WINDOW_LOOK)
		_tabrect.top+=4;

	_zoomrect=_tabrect;
	_zoomrect.top+=(_look==B_FLOATING_WINDOW_LOOK)?2:4;
	_zoomrect.right-=4;
	_zoomrect.bottom-=4;
	_zoomrect.left=_zoomrect.right-10;
	_zoomrect.bottom=_zoomrect.top+10;
	
	textoffset=(_look==B_FLOATING_WINDOW_LOOK)?5:7;
}

void DefaultDecorator::MoveBy(float x, float y)
{
	MoveBy(BPoint(x,y));
}

void DefaultDecorator::MoveBy(BPoint pt)
{
	// Move all internal rectangles the appropriate amount
	_frame.OffsetBy(pt);
	_closerect.OffsetBy(pt);
	_tabrect.OffsetBy(pt);
	_resizerect.OffsetBy(pt);
	_borderrect.OffsetBy(pt);
	_zoomrect.OffsetBy(pt);
}

BRegion * DefaultDecorator::GetFootprint(void)
{
	// This function calculates the decorator's footprint in coordinates
	// relative to the layer. This is most often used to set a WinBorder
	// object's visible region.
	
	BRegion *reg=new BRegion(_borderrect);
	reg->Include(_tabrect);
	return reg;
}

void DefaultDecorator::_DrawTitle(BRect r)
{
	// Designed simply to redraw the title when it has changed on
	// the client side.
/*	_driver->SetDrawingMode(B_OP_OVER);
	rgb_color tmpcol=_driver->HighColor();
	_driver->SetHighColor(textcol.red,textcol.green,textcol.blue);
	_driver->DrawString((char *)string,strlen(string),
		BPoint(_closerect.right+textoffset,_closerect.bottom-1));
	_driver->SetHighColor(tmpcol.red,tmpcol.green,tmpcol.blue);
	_driver->SetDrawingMode(B_OP_COPY);
*/
}

void DefaultDecorator::_SetFocus(void)
{
	// SetFocus() performs necessary duties for color swapping and
	// other things when a window is deactivated or activated.
	
	if(GetFocus())
	{
		tab_highcol=_colors->window_tab;
		tab_lowcol=_colors->window_tab;

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

void DefaultDecorator::Draw(BRect update)
{
	// We need to draw a few things: the tab, the resize thumb, the borders,
	// and the buttons

	_DrawTab(update);

	// Draw the top view's client area - just a hack :)
	RGBColor blue(100,100,255);

	_layerdata.highcolor=blue;

	if(_borderrect.Intersects(update))
		_driver->FillRect(_borderrect,&_layerdata,(int8*)&solidhigh);
	
	_DrawFrame(update);
}

void DefaultDecorator::Draw(void)
{
	// Easy way to draw everything - no worries about drawing only certain
	// things

	// Draw the top view's client area - just a hack :)
	RGBColor blue(100,100,255);

	_layerdata.highcolor=blue;

	_driver->FillRect(_borderrect,&_layerdata,(int8*)&solidhigh);
	DrawFrame();

	DrawTab();

}

void DefaultDecorator::_DrawZoom(BRect r)
{
	// If this has been implemented, then the decorator has a Zoom button
	// which should be drawn based on the state of the member zoomstate
	BRect zr=r;
	zr.left+=zr.Width()/3;
	zr.top+=zr.Height()/3;

	DrawBlendedRect(zr,GetZoom());
	DrawBlendedRect(zr.OffsetToCopy(r.LeftTop()),GetZoom());
}

void DefaultDecorator::_DrawClose(BRect r)
{
	// Just like DrawZoom, but for a close button
	DrawBlendedRect(r,GetClose());
}

void DefaultDecorator::_DrawTab(BRect r)
{
	// If a window has a tab, this will draw it and any buttons which are
	// in it.
	if(_look==B_NO_BORDER_WINDOW_LOOK)
		return;
	
	_layerdata.highcolor=frame_lowcol;
	_driver->StrokeRect(_tabrect,&_layerdata,(int8*)&solidhigh);

//	UpdateTitle(layer->name->String());

	_layerdata.highcolor=_colors->window_tab;
	_driver->FillRect(_tabrect.InsetByCopy(1,1),&_layerdata,(int8*)&solidhigh);

	// Draw the buttons if we're supposed to	
	if(!(_flags & B_NOT_CLOSABLE))
		_DrawClose(_closerect);
	if(!(_flags & B_NOT_ZOOMABLE))
		_DrawZoom(_zoomrect);
}

void DefaultDecorator::DrawBlendedRect(BRect r, bool down)
{
	// This bad boy is used to draw a rectangle with a gradient.
	// Note that it is not part of the Decorator API - it's specific
	// to just the DefaultDecorator. Called by DrawZoom and DrawClose

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

void DefaultDecorator::_DrawFrame(BRect rect)
{
	// Duh, draws the window frame, I think. ;)

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
