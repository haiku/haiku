#include "Layer.h"
#include "DisplayDriver.h"
#include <View.h>
#include "WinDecorator.h"
#include "ColorUtils.h"

//#define DEBUG_DECOR

#ifdef DEBUG_DECOR
#include <stdio.h>
#endif

WinDecorator::WinDecorator(Layer *lay, uint32 dflags, uint32 wlook)
 : Decorator(lay, dflags, wlook)
{
#ifdef DEBUG_DECOR
printf("WinDecorator()\n");
#endif
	zoomstate=false;
	closestate=false;
	minstate=false;
	taboffset=0;

	// These hard-coded assignments will go bye-bye when the system colors 
	// API is implemented

	SetRGBColor(&tab_highcol,100,100,255);
	SetRGBColor(&tab_lowcol,40,0,255);

	SetRGBColor(&button_highercol,255,255,255);
	SetRGBColor(&button_lowercol,0,0,0);
	
	button_highcol=MakeBlendColor(button_lowercol,button_highercol,0.85);
	button_midcol=MakeBlendColor(button_lowercol,button_highercol,0.75);
	button_lowcol=MakeBlendColor(button_lowercol,button_highercol,0.5);

 	SetRGBColor(&frame_highercol,216,216,216);
	SetRGBColor(&frame_lowercol,110,110,110);

	SetRGBColor(&textcol,0,0,0);
	
	frame_highcol=MakeBlendColor(frame_lowercol,frame_highercol,0.75);
	frame_midcol=MakeBlendColor(frame_lowercol,frame_highercol,0.5);
	frame_lowcol=MakeBlendColor(frame_lowercol,frame_highercol,0.25);

	Resize(lay->frame);
	
	textoffset=5;
}

WinDecorator::~WinDecorator(void)
{
#ifdef DEBUG_DECOR
printf("~WinDecorator()\n");
#endif
}

click_type WinDecorator::Clicked(BPoint pt, uint32 buttons)
{
	// Clicked is a hit-testing function which is called by each
	// decorator object's respective WindowBorder. When a click or
	// mouse message is received, we must return to the WindowBorder
	// what the click meant to us and the appropriate action for it
	// to take.

	// the case order is important - we go from smallest to largest
	// for efficiency's sake.
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
	
	if(minrect.Contains(pt))
	{

#ifdef DEBUG_DECOR
printf("WinDecorator():Clicked() - Minimize\n");
#endif

		return CLICK_MINIMIZE;
	}

	// Clicking in the tab?
	if(tabrect.Contains(pt))
	{
		// Here's part of our window management stuff
		if(buttons==B_PRIMARY_MOUSE_BUTTON && !focused)
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
printf("WinDecorator():Clicked() - Resize\n");
#endif		
		if(brect.Contains(pt))
			return CLICK_RESIZE;
	}

	// Guess user didn't click anything
#ifdef DEBUG_DECOR
printf("WinDecorator():Clicked()\n");
#endif
	return CLICK_NONE;
}

void WinDecorator::Resize(BRect rect)
{
	// Here we determine the size of every rectangle that we use
	// internally when we are given the size of the client rectangle.
	
	// Current version simply makes everything fit inside the rect
	// instead of building around it. This will change.
	
#ifdef DEBUG_DECOR
printf("WinDecorator()::Resize()"); rect.PrintToStream();
#endif
	frame=rect;
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
	
	minrect=zoomrect;
	minrect.OffsetBy(0-zoomrect.Width()-2,0);

}

void WinDecorator::MoveBy(BPoint pt)
{
	// Move all internal rectangles the appropriate amount
	frame.OffsetBy(pt);
	closerect.OffsetBy(pt);
	tabrect.OffsetBy(pt);
	borderrect.OffsetBy(pt);
	zoomrect.OffsetBy(pt);
	minrect.OffsetBy(pt);
}

BPoint WinDecorator::GetMinimumSize(void)
{
	// This is currently unused, but minsize is the minimum size the
	// window may take.
	return minsize;
}

void WinDecorator::SetFlags(uint32 wflags)
{
	dflags=wflags;
}

void WinDecorator::UpdateFont(void)
{
	// This will be updated once font capability is implemented
}

void WinDecorator::UpdateTitle(const char *string)
{
	// Designed simply to redraw the title when it has changed on
	// the client side.
	if(string)
	{
		driver->SetDrawingMode(B_OP_OVER);
		rgb_color tmpcol=driver->HighColor();
		driver->SetHighColor(textcol.red,textcol.green,textcol.blue);
		driver->DrawString((char *)string,strlen(string),
			BPoint(frame.left+textoffset,closerect.bottom-1));
		driver->SetHighColor(tmpcol.red,tmpcol.green,tmpcol.blue);
		driver->SetDrawingMode(B_OP_COPY);
	}

}

void WinDecorator::SetFocus(bool bfocused)
{
	// SetFocus() performs necessary duties for color swapping and
	// other things when a window is deactivated or activated.
	
	focused=bfocused;

	if(focused)
	{
		SetRGBColor(&tab_highcol,100,100,255);
		SetRGBColor(&tab_lowcol,40,0,255);
	}
	else
	{
		SetRGBColor(&tab_highcol,220,220,220);
		SetRGBColor(&tab_lowcol,128,128,128);
	}
}

void WinDecorator::Draw(BRect update)
{
	// We need to draw a few things: the tab, the resize thumb, the borders,
	// and the buttons

	if(tabrect.Intersects(update))
		DrawTab();

	// Draw the top view's client area - just a hack :)
	rgb_color blue={100,100,255,255};

	if(borderrect.Intersects(update))
		driver->FillRect(borderrect,blue);
	
	DrawFrame();
}

void WinDecorator::Draw(void)
{
	// Easy way to draw everything - no worries about drawing only certain
	// things

	DrawTab();

	// Draw the top view's client area - just a hack :)
	rgb_color blue={100,100,255,255};
	driver->FillRect(borderrect,blue);

	DrawFrame();
}

void WinDecorator::DrawZoom(BRect r)
{
	// If this has been implemented, then the decorator has a Zoom button
	// which should be drawn based on the state of the member zoomstate
	DrawBlendedRect(r,zoomstate);

	// Draw the Zoom box
	driver->StrokeRect(r.InsetByCopy(2,2),textcol);
}

void WinDecorator::DrawClose(BRect r)
{
	// Just like DrawZoom, but for a close button
	DrawBlendedRect(r,closestate);
	
	// Draw the X
	driver->StrokeLine(BPoint(closerect.left+2,closerect.top+2),BPoint(closerect.right-2,
		closerect.bottom-2),textcol);
	driver->StrokeLine(BPoint(closerect.right-2,closerect.top+2),BPoint(closerect.left+2,
		closerect.bottom-2),textcol);
}

void WinDecorator::DrawMinimize(BRect r)
{
	// Just like DrawZoom, but for a Minimize button
	DrawBlendedRect(r,minstate);

	driver->StrokeLine(BPoint(minrect.left+2,minrect.bottom-2),BPoint(minrect.right-2,
		minrect.bottom-2),textcol);
}

void WinDecorator::DrawTab(void)
{
	// If a window has a tab, this will draw it and any buttons which are
	// in it.
	if(dlook==WLOOK_NO_BORDER)
		return;
	
	rgb_color tmpcol;
	float rstep,gstep,bstep;

	int steps=tabrect.IntegerWidth();
	rstep=float(tab_highcol.red-tab_lowcol.red)/steps;
	gstep=float(tab_highcol.green-tab_lowcol.green)/steps;
	bstep=float(tab_highcol.blue-tab_lowcol.blue)/steps;

	driver->StrokeRect(tabrect,frame_lowcol);

	driver->BeginLineArray(steps);
	for(float i=1;i<steps; i++)
	{
		SetRGBColor(&tmpcol, uint8(tab_highcol.red-(i*rstep)),
			uint8(tab_highcol.green-(i*gstep)),
			uint8(tab_highcol.blue-(i*bstep)));
		
		driver->AddLine(BPoint(tabrect.left+i,tabrect.top+1),
			BPoint(tabrect.left+i,tabrect.bottom-1),tmpcol);
	}
	driver->EndLineArray();
	UpdateTitle(layer->name->String());

	// Draw the buttons if we're supposed to	
	if(!(dflags & NOT_CLOSABLE))
		DrawClose(closerect);
	if(!(dflags & NOT_ZOOMABLE))
		DrawZoom(zoomrect);
	if(!(dflags & NOT_MINIMIZABLE))
		DrawMinimize(minrect);
}

void WinDecorator::DrawBlendedRect(BRect r, bool down)
{
	// This bad boy is used to draw a rectangle with a gradient.
	// Note that it is not part of the Decorator API - it's specific
	// to just the WinDecorator. Called by DrawZoom and DrawClose
	BRect zr=r;
	rgb_color startcol,endcol;
	rgb_color start2col,end2col;

	if(down)
	{
		startcol=button_lowercol;
		start2col=button_lowcol;
		endcol=button_highercol;
		end2col=button_highcol;
	}
	else
	{
		startcol=button_highercol;
		start2col=button_highcol;
		end2col=button_lowcol;
		endcol=button_lowercol;
	}
	driver->BeginLineArray(8);
	driver->AddLine(BPoint(zr.left,zr.top),BPoint(zr.right-1,zr.top),startcol);
	driver->AddLine(BPoint(zr.left,zr.top),BPoint(zr.left,zr.bottom-1),startcol);
	driver->AddLine(BPoint(zr.right,zr.top),BPoint(zr.right,zr.bottom),endcol);
	driver->AddLine(BPoint(zr.left,zr.bottom),BPoint(zr.right,zr.bottom),endcol);

	zr.InsetBy(1,1);
	driver->AddLine(BPoint(zr.left,zr.top),BPoint(zr.right-1,zr.top),start2col);
	driver->AddLine(BPoint(zr.left,zr.top),BPoint(zr.left,zr.bottom-1),start2col);
	driver->AddLine(BPoint(zr.right,zr.top),BPoint(zr.right,zr.bottom),end2col);
	driver->AddLine(BPoint(zr.left,zr.bottom),BPoint(zr.right,zr.bottom),end2col);
	driver->EndLineArray();	
	zr.InsetBy(1,1);
	driver->FillRect(zr,button_midcol);
}

void WinDecorator::DrawFrame(void)
{
	// Duh, draws the window frame, I think. ;)

	if(dlook==WLOOK_NO_BORDER)
		return;
	
	BRect r=borderrect;
	
	driver->BeginLineArray(12);
	driver->AddLine(BPoint(r.left,r.top),BPoint(r.right-1,r.top),
		frame_midcol);
	driver->AddLine(BPoint(r.left,r.top),BPoint(r.left,r.bottom),
		frame_lowcol);
	driver->AddLine(BPoint(r.right,r.bottom),BPoint(r.right,r.top),
		frame_lowercol);
	driver->AddLine(BPoint(r.right,r.bottom),BPoint(r.left,r.bottom),
		frame_lowercol);

	r.InsetBy(1,1);
	driver->AddLine(BPoint(r.left,r.top),BPoint(r.right-1,r.top),
		frame_highercol);
	driver->AddLine(BPoint(r.left,r.top),BPoint(r.left,r.bottom),
		frame_highercol);
	driver->AddLine(BPoint(r.right,r.bottom),BPoint(r.right,r.top),
		frame_midcol);
	driver->AddLine(BPoint(r.right,r.bottom),BPoint(r.left,r.bottom),
		frame_midcol);
	
	r.InsetBy(1,1);
	driver->StrokeRect(r,frame_highcol);

	r.InsetBy(1,1);
	driver->AddLine(BPoint(r.left,r.top),BPoint(r.right-1,r.top),
		frame_lowercol);
	driver->AddLine(BPoint(r.left,r.top),BPoint(r.left,r.bottom),
		frame_lowercol);
	driver->AddLine(BPoint(r.right,r.bottom),BPoint(r.right,r.top),
		frame_highercol);
	driver->AddLine(BPoint(r.right,r.bottom),BPoint(r.left,r.bottom),
		frame_highercol);
	driver->EndLineArray();
}

void WinDecorator::SetLook(uint32 wlook)
{
	dlook=wlook;
}

void WinDecorator::CalculateBorders(void)
{
	// Here we calculate the size of the border on each side - how much the
	// decorator's visible area "sticks out" past the client rectangle.
	switch(dlook)
	{
		case WLOOK_NO_BORDER:
		{
			bsize.Set(0,0,0,0);
			break;
		}
		case WLOOK_TITLED:
		case WLOOK_DOCUMENT:
		case WLOOK_BORDERED:
		case WLOOK_MODAL:
		case WLOOK_FLOATING:
		{
			bsize.top=18;
			break;
		}
		default:
		{
			break;
		}
	}
}
