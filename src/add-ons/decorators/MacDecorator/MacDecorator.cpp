#include "DisplayDriver.h"
#include <View.h>
#include "LayerData.h"
#include "ColorUtils.h"
#include "MacDecorator.h"
#include "RGBColor.h"
#include "PatternHandler.h"

//#define DEBUG_DECOR

#ifdef DEBUG_DECOR
#include <stdio.h>
#endif

MacDecorator::MacDecorator(BRect rect, int32 wlook, int32 wfeel, int32 wflags)
 : Decorator(rect,wlook,wfeel,wflags)
{
#ifdef DEBUG_DECOR
printf("MacDecorator()\n");
#endif
	taboffset=0;

	frame_highcol.SetColor(255,255,255);
	frame_midcol.SetColor(216,216,216);
	frame_lowcol.SetColor(110,110,110);
	frame_lowercol.SetColor(0,0,0);
	
	button_highcol.SetColor(232,232,232);
	button_lowcol.SetColor(128,128,128);
	
	textcol.SetColor(0,0,0);
	inactive_textcol.SetColor(100,100,100);
		
	_DoLayout();
	
	textoffset=5;
}

MacDecorator::~MacDecorator(void)
{
#ifdef DEBUG_DECOR
printf("~MacDecorator()\n");
#endif
}

click_type MacDecorator::Clicked(BPoint pt, int32 buttons, int32 modifiers)
{
	if(_closerect.Contains(pt))
	{

#ifdef DEBUG_DECOR
printf("MacDecorator():Clicked() - Close\n");
#endif

		return CLICK_CLOSE;
	}

	if(_zoomrect.Contains(pt))
	{

#ifdef DEBUG_DECOR
printf("MacDecorator():Clicked() - Zoom\n");
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
printf("MacDecorator():Clicked() - Resize\n");
#endif		
		return CLICK_RESIZE;
	}

	// Guess user didn't click anything
#ifdef DEBUG_DECOR
printf("MacDecorator():Clicked()\n");
#endif
	return CLICK_NONE;
}

void MacDecorator::_DoLayout(void)
{
#ifdef DEBUG_DECOR
printf("MacDecorator()::_DoLayout()\n");
#endif
	_borderrect=_frame;
	_borderrect.top+=19;
	_tabrect=_frame;

	_tabrect.bottom=_tabrect.top+19;

	_zoomrect=_tabrect;
	_zoomrect.left=_zoomrect.right-12;
	_zoomrect.bottom=_zoomrect.top+12;
	_zoomrect.OffsetBy(-4,4);
	
	_closerect=_zoomrect;
	_minimizerect=_zoomrect;

	_closerect.OffsetTo(_tabrect.left+4,_tabrect.top+4);
	
	_zoomrect.OffsetBy(0-(_zoomrect.Width()+4),0);
	if(GetTitle() && _driver)
	{
		titlepixelwidth=_driver->StringWidth(GetTitle(),strlen(GetTitle()),&_layerdata);
		
		if(titlepixelwidth<(_zoomrect.left-_closerect.right-10))
		{
			// start with offset from closerect.right
			textoffset=int(((_zoomrect.left-5)-(_closerect.right+5))/2);
			textoffset-=int(titlepixelwidth/2);
			
			// now make it the offset from _tabrect.left
			textoffset+=int(_closerect.right+5-_tabrect.left);
		}
		else
			textoffset=int(_closerect.right)+5;
	}
	else
	{
		textoffset=0;
		titlepixelwidth=0;
	}
}

void MacDecorator::MoveBy(float x, float y)
{
	MoveBy(BPoint(x,y));
}

void MacDecorator::MoveBy(BPoint pt)
{
	// Move all internal rectangles the appropriate amount
	_frame.OffsetBy(pt);
	_closerect.OffsetBy(pt);
	_tabrect.OffsetBy(pt);
	_borderrect.OffsetBy(pt);
	_zoomrect.OffsetBy(pt);
	_minimizerect.OffsetBy(pt);
}

void MacDecorator::_DrawTitle(BRect r)
{
	if(GetFocus())
		_layerdata.highcolor=textcol;
	else
		_layerdata.highcolor=inactive_textcol;
		
	_layerdata.lowcolor=frame_midcol;

	int32 titlecount=_ClipTitle((_zoomrect.left-5)-(_closerect.right+5));
	BString titlestr=GetTitle();
	if(titlecount<titlestr.CountChars())
	{
		titlestr.Truncate(titlecount-1);
		titlestr+="...";
		titlecount+=2;
	}
	_driver->DrawString(titlestr.String(),titlecount,
		BPoint(_tabrect.left+textoffset,_closerect.bottom-1),&_layerdata);
}

void MacDecorator::GetFootprint(BRegion *region)
{
	// This function calculates the decorator's footprint in coordinates
	// relative to the layer. This is most often used to set a WinBorder
	// object's visible region.
	if(!region)
		return;
	
	region->Set(_borderrect);
	region->Include(_tabrect);
}

void MacDecorator::Draw(BRect update)
{
#ifdef DEBUG_DECOR
printf("MacDecorator::Draw(): "); update.PrintToStream();
#endif
	// Draw the top view's client area - just a hack :)
	_layerdata.highcolor=_colors->document_background;
	_driver->FillRect(_borderrect,&_layerdata,pat_solidhigh);

	if(_borderrect.Intersects(update))
		_driver->FillRect(_borderrect,&_layerdata,pat_solidhigh);
	
	_DrawFrame(update);
	_DrawTab(update);
}

void MacDecorator::Draw(void)
{
#ifdef DEBUG_DECOR
printf("MacDecorator::Draw()\n");
#endif

	// Draw the top view's client area - just a hack :)
//	RGBColor blue(100,100,255);
//	_layerdata.highcolor=blue;

	_layerdata.highcolor=_colors->document_background;
	_driver->FillRect(_borderrect,&_layerdata,pat_solidhigh);

	_driver->FillRect(_borderrect,&_layerdata,pat_solidhigh);
	DrawFrame();

	DrawTab();
}

void MacDecorator::_DrawZoom(BRect r)
{
	bool down=GetClose();
	
	// Just like DrawZoom, but for a close button
	BRect rect(r);

	BPoint pt(r.LeftTop()),pt2(r.RightTop());
	
	pt2.x--;
	_layerdata.highcolor.SetColor(136,136,136);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	
	pt2=r.LeftBottom();
	pt2.y--;
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	
	pt=r.RightBottom();
	pt2=r.RightTop();
	pt2.y++;
	_layerdata.highcolor.SetColor(255,255,255);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	
	pt2=r.LeftBottom();
	pt2.x++;
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);

	rect.InsetBy(1,1);
	_layerdata.highcolor.SetColor(0,0,0);
	_driver->StrokeRect(rect,&_layerdata,pat_solidhigh);
	
	rect.InsetBy(1,1);
	DrawBlendedRect(rect,down);
	rect.InsetBy(1,1);
	DrawBlendedRect(rect,!down);
	
	rect.top+=2;
	rect.left--;
	rect.right++;
	
	_layerdata.highcolor.SetColor(0,0,0);
	_driver->StrokeLine(rect.LeftTop(),rect.RightTop(),&_layerdata,pat_solidhigh);
}

void MacDecorator::_DrawClose(BRect r)
{
	bool down=GetClose();
	
	// Just like DrawZoom, but for a close button
	BRect rect(r);

	BPoint pt(r.LeftTop()),pt2(r.RightTop());
	
	pt2.x--;
	_layerdata.highcolor.SetColor(136,136,136);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	
	pt2=r.LeftBottom();
	pt2.y--;
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	
	pt=r.RightBottom();
	pt2=r.RightTop();
	pt2.y++;
	_layerdata.highcolor.SetColor(255,255,255);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	
	pt2=r.LeftBottom();
	pt2.x++;
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);

	rect.InsetBy(1,1);
	_layerdata.highcolor.SetColor(0,0,0);
	_driver->StrokeRect(rect,&_layerdata,pat_solidhigh);
	
	rect.InsetBy(1,1);
	DrawBlendedRect(rect,down);
	rect.InsetBy(1,1);
	DrawBlendedRect(rect,!down);
	
//	rect.top+=4;
//	rect.left++;
//	rect.right--;
	
//	_layerdata.highcolor.SetColor(0,0,0);
//	_driver->StrokeLine(rect.LeftTop(),rect.RightTop(),&_layerdata,pat_solidhigh);
}

void MacDecorator::_DrawMinimize(BRect r)
{
	bool down=GetClose();
	
	// Just like DrawZoom, but for a close button
	BRect rect(r);

	BPoint pt(r.LeftTop()),pt2(r.RightTop());
	
	pt2.x--;
	_layerdata.highcolor.SetColor(136,136,136);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	
	pt2=r.LeftBottom();
	pt2.y--;
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	
	pt=r.RightBottom();
	pt2=r.RightTop();
	pt2.y++;
	_layerdata.highcolor.SetColor(255,255,255);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	
	pt2=r.LeftBottom();
	pt2.x++;
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);

	rect.InsetBy(1,1);
	_layerdata.highcolor.SetColor(0,0,0);
	_driver->StrokeRect(rect,&_layerdata,pat_solidhigh);
	
	rect.InsetBy(1,1);
	DrawBlendedRect(rect,down);
	rect.InsetBy(1,1);
	DrawBlendedRect(rect,!down);
	
	rect.top+=4;
	rect.bottom-=4;
	rect.InsetBy(-2,0);
	
	_layerdata.highcolor.SetColor(0,0,0);
	_driver->StrokeRect(rect,&_layerdata,pat_solidhigh);
}

void MacDecorator::_DrawTab(BRect r)
{
	// If a window has a tab, this will draw it and any buttons which are
	// in it.
	if(_look==B_NO_BORDER_WINDOW_LOOK)
		return;
	
//	_layerdata.highcolor=frame_lowcol;
//	_driver->StrokeRect(_tabrect,&_layerdata,pat_solidhigh);

//	UpdateTitle(layer->name->String());
	BRect rect(_tabrect);
	_layerdata.highcolor.SetColor(frame_midcol);
	_driver->FillRect(rect,&_layerdata,pat_solidhigh);
	
	
	if(GetFocus())
	{
		_layerdata.highcolor.SetColor(frame_lowercol);
		_driver->StrokeLine(rect.LeftTop(),rect.RightTop(),&_layerdata,pat_solidhigh);
		_driver->StrokeLine(rect.LeftTop(),rect.LeftBottom(),&_layerdata,pat_solidhigh);
		_driver->StrokeLine(rect.RightBottom(),rect.RightTop(),&_layerdata,pat_solidhigh);
	
		rect.InsetBy(1,1);
		rect.bottom++;
		
		_layerdata.highcolor.SetColor(frame_highcol);
		_driver->StrokeLine(rect.LeftTop(),rect.RightTop(),&_layerdata,pat_solidhigh);
		_driver->StrokeLine(rect.LeftTop(),rect.LeftBottom(),&_layerdata,pat_solidhigh);
		_layerdata.highcolor.SetColor(frame_lowcol);
		_driver->StrokeLine(rect.RightBottom(),rect.RightTop(),&_layerdata,pat_solidhigh);
		
		// Draw the neat little lines on either side of the title if there's room
		if((_tabrect.left+textoffset)>(_closerect.right+5))
		{
			// Left side

			BPoint pt(_closerect.right+5,_closerect.top),
				pt2(_tabrect.left+textoffset-5,_closerect.top);
			_layerdata.highcolor.SetColor(frame_highcol);
			for(int32 i=0;i<6;i++)
			{
				_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
				pt.y+=2;
				pt2.y+=2;
			}
			
			pt.Set(_closerect.right+6,_closerect.top+1),
				pt2.Set(_tabrect.left+textoffset-4,_closerect.top+1);
			_layerdata.highcolor.SetColor(frame_lowcol);
			for(int32 i=0;i<6;i++)
			{
				_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
				pt.y+=2;
				pt2.y+=2;
			}
			
			// Right side
			
			pt.Set(_tabrect.left+textoffset+titlepixelwidth+6,_zoomrect.top),
				pt2.Set(_zoomrect.left-6,_zoomrect.top);
			if(pt.x<pt2.x)
			{
				_layerdata.highcolor.SetColor(frame_highcol);
				for(int32 i=0;i<6;i++)
				{
					_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
					pt.y+=2;
					pt2.y+=2;
				}
				pt.Set(_tabrect.left+textoffset+titlepixelwidth+7,_zoomrect.top+1),
					pt2.Set(_zoomrect.left-5,_zoomrect.top+1);
				_layerdata.highcolor.SetColor(frame_lowcol);
				for(int32 i=0;i<6;i++)
				{
					_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
					pt.y+=2;
					pt2.y+=2;
				}
			}
		}
		
		// Draw the buttons if we're supposed to	
		if(!(_flags & B_NOT_CLOSABLE))
			_DrawClose(_closerect);
		if(!(_flags & B_NOT_ZOOMABLE))
			_DrawZoom(_zoomrect);
	}
	
}

void MacDecorator::DrawBlendedRect(BRect r, bool down)
{
	// This bad boy is used to draw a rectangle with a gradient.
	// Note that it is not part of the Decorator API - it's specific
	// to just the BeDecorator. Called by DrawZoom and DrawClose

	// Actually just draws a blended square

	rgb_color tmpcol,halfcol, startcol, endcol;
	float rstep,gstep,bstep,i;

	BRect rect(r);

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
	
	int32 w=rect.IntegerWidth(),  h=rect.IntegerHeight();
	int steps=(w<h)?w:h;
	
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
		_driver->StrokeLine(BPoint(rect.left,rect.top+i),
			BPoint(rect.left+i,rect.top),&_layerdata,pat_solidhigh);

		SetRGBColor(&tmpcol, uint8(halfcol.red-(i*rstep)),
			uint8(halfcol.green-(i*gstep)),
			uint8(halfcol.blue-(i*bstep)));

		_layerdata.highcolor=tmpcol;
		_driver->StrokeLine(BPoint(rect.left+steps,rect.top+i),
			BPoint(rect.left+i,rect.top+steps),&_layerdata,pat_solidhigh);

	}
}

/*
void MacDecorator::_SetColors(void)
{
	_SetFocus();
}
*/
void MacDecorator::_DrawFrame(BRect rect)
{
	if(_look==B_NO_BORDER_WINDOW_LOOK)
		return;

	BRect r=_borderrect;
	BPoint pt,pt2,topleftpt,toprightpt;
	
	pt=r.LeftTop();
	pt2=r.LeftBottom();

	// Draw the left side of the frame
	_layerdata.highcolor.SetColor(frame_lowercol);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	pt.x++;
	pt2.x++;
	pt2.y--;
	
	_layerdata.highcolor.SetColor(frame_highcol);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	pt.x++;
	pt2.x++;
	pt2.y--;
	
	_layerdata.highcolor.SetColor(frame_midcol);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	pt.x++;
	pt2.x++;
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	pt.x++;
	pt2.x++;
	pt2.y--;

	_layerdata.highcolor.SetColor(frame_lowcol);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	pt.x++;
	pt.y+=2;
	topleftpt=pt;
	pt2.x++;
	pt2.y--;

	_layerdata.highcolor.SetColor(frame_lowercol);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);


	pt=r.RightTop();
	pt2=r.RightBottom();
	
	// Draw the right side of the frame
	_layerdata.highcolor.SetColor(frame_lowercol);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	pt.x--;
	pt2.x--;

	_layerdata.highcolor.SetColor(frame_lowcol);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	pt.x--;
	pt2.x--;

	_layerdata.highcolor.SetColor(frame_midcol);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	pt.x--;
	pt2.x--;
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	pt.x--;
	pt2.x--;

	_layerdata.highcolor.SetColor(frame_highcol);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	pt.x--;
	pt.y+=2;
	toprightpt=pt;
	pt2.x--;
	
	_layerdata.highcolor.SetColor(frame_lowercol);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);

	// Draw the top side of the frame that is not in the tab
	_layerdata.highcolor.SetColor(frame_lowercol);
	_driver->StrokeLine(topleftpt,toprightpt,&_layerdata,pat_solidhigh);
	topleftpt.y--;
	toprightpt.x++;
	toprightpt.y--;

	_layerdata.highcolor.SetColor(frame_lowcol);
	_driver->StrokeLine(topleftpt,toprightpt,&_layerdata,pat_solidhigh);
	
	pt=r.RightTop();
	pt2=r.RightBottom();


	pt=r.LeftBottom();
	pt2=r.RightBottom();
	
	// Draw the bottom side of the frame
	_layerdata.highcolor.SetColor(frame_lowercol);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	pt.x++;
	pt.y--;
	pt2.x--;
	pt2.y--;

	_layerdata.highcolor.SetColor(frame_lowcol);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	pt.x++;
	pt.y--;
	pt2.x--;
	pt2.y--;

	_layerdata.highcolor.SetColor(frame_midcol);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	pt.x++;
	pt.y--;
	pt2.x--;
	pt2.y--;

	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	pt.x++;
	pt.y--;
	pt2.x--;
	pt2.y--;

	_layerdata.highcolor.SetColor(frame_highcol);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	pt.x+=2;
	pt.y--;
	pt2.x--;
	pt2.y--;
	
	_layerdata.highcolor.SetColor(frame_lowercol);
	_driver->StrokeLine(pt,pt2,&_layerdata,pat_solidhigh);
	pt.y--;
	pt2.x--;
	pt2.y--;
}

extern "C" float get_decorator_version(void)
{
	return 1.00;
}

extern "C" Decorator *instantiate_decorator(BRect rect, int32 wlook, int32 wfeel, int32 wflags)
{
	return (new MacDecorator(rect,wlook,wfeel,wflags));
}
