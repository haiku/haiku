#include "Layer.h"
#include "DisplayDriver.h"
#include <View.h>
#include "BeDecorator.h"
#include "ColorUtils.h"

//#define DEBUG_DECOR

#ifdef DEBUG_DECOR
#include <stdio.h>
#endif

BeDecorator::BeDecorator(Layer *lay, uint32 dflags, uint32 wlook)
 : Decorator(lay, dflags, wlook)
{
#ifdef DEBUG_DECOR
printf("BeDecorator()\n");
#endif
	zoomstate=false;
	closestate=false;
	taboffset=0;

	// These hard-coded assignments will go bye-bye when the system colors 
	// API is implemented

	// commented these out because they are taken care of by the SetFocus() call
	SetFocus(false);
//	SetRGBColor(&tab_highcol,255,236,33);
//	SetRGBColor(&tab_lowcol,234,181,0);

//	SetRGBColor(&button_highcol,255,255,0);
//	SetRGBColor(&button_lowcol,255,203,0);

	SetRGBColor(&frame_highercol,216,216,216);
	SetRGBColor(&frame_lowercol,110,110,110);

	SetRGBColor(&textcol,0,0,0);
	
	frame_highcol=MakeBlendColor(frame_lowercol,frame_highercol,0.75);
	frame_midcol=MakeBlendColor(frame_lowercol,frame_highercol,0.5);
	frame_lowcol=MakeBlendColor(frame_lowercol,frame_highercol,0.25);

	Resize(lay->frame);
	
	// We need to modify the visible rectangle because we have tabbed windows
	if(lay->visible)
	{
		delete lay->visible;
		lay->visible=GetFootprint();
	}

	// This flag is used to determine whether or not we're moving the tab
	slidetab=false;
}

BeDecorator::~BeDecorator(void)
{
#ifdef DEBUG_DECOR
printf("~BeDecorator()\n");
#endif
}

click_type BeDecorator::Clicked(BPoint pt, uint32 buttons)
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
	
	if(resizerect.Contains(pt) && dlook==WLOOK_DOCUMENT)
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

void BeDecorator::Resize(BRect rect)
{
	// Here we determine the size of every rectangle that we use
	// internally when we are given the size of the client rectangle.
	
	// Current version simply makes everything fit inside the rect
	// instead of building around it. This will change.
	
#ifdef DEBUG_DECOR
printf("BeDecorator()::Resize()"); rect.PrintToStream();
#endif
	frame=rect;
	tabrect=frame;
	resizerect=frame;
	borderrect=frame;
	closerect=frame;

	closerect.left+=(dlook==WLOOK_FLOATING)?2:4;
	closerect.top+=(dlook==WLOOK_FLOATING)?6:4;
	closerect.right=closerect.left+10;
	closerect.bottom=closerect.top+10;

	borderrect.top+=19;
	
	resizerect.top=resizerect.bottom-18;
	resizerect.left=resizerect.right-18;
	
	tabrect.bottom=tabrect.top+18;
	if(layer->name)
	{
		float titlewidth=closerect.right
			+driver->StringWidth(layer->name->String(),
			layer->name->Length())
			+35;
		tabrect.right=(titlewidth<frame.right -1)?titlewidth:frame.right;
	}
	else
		tabrect.right=tabrect.left+tabrect.Width()/2;

	if(dlook==WLOOK_FLOATING)
		tabrect.top+=4;

	zoomrect=tabrect;
	zoomrect.top+=(dlook==WLOOK_FLOATING)?2:4;
	zoomrect.right-=4;
	zoomrect.bottom-=4;
	zoomrect.left=zoomrect.right-10;
	zoomrect.bottom=zoomrect.top+10;
	
	textoffset=(dlook==WLOOK_FLOATING)?5:7;
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

BPoint BeDecorator::GetMinimumSize(void)
{
	// This is currently unused, but minsize is the minimum size the
	// window may take.
	return minsize;
}

BRegion * BeDecorator::GetFootprint(void)
{
	// This function calculates the decorator's footprint in coordinates
	// relative to the layer. This is most often used to set a WindowBorder
	// object's visible region.
	
	BRegion *reg=new BRegion(borderrect);
	reg->Include(tabrect);
	return reg;
}

void BeDecorator::SetFlags(uint32 wflags)
{
	dflags=wflags;
}

void BeDecorator::UpdateFont(void)
{
	// This will be updated once font capability is implemented
}

void BeDecorator::UpdateTitle(const char *string)
{
	// Designed simply to redraw the title when it has changed on
	// the client side.
	if(string)
	{
		driver->SetDrawingMode(B_OP_OVER);
		rgb_color tmpcol=driver->HighColor();
		driver->SetHighColor(textcol.red,textcol.green,textcol.blue);
		driver->DrawString((char *)string,strlen(string),
			BPoint(closerect.right+textoffset,closerect.bottom-1));
		driver->SetHighColor(tmpcol.red,tmpcol.green,tmpcol.blue);
		driver->SetDrawingMode(B_OP_COPY);
	}

}

void BeDecorator::SetFocus(bool bfocused)
{
	// SetFocus() performs necessary duties for color swapping and
	// other things when a window is deactivated or activated.
	
	focused=bfocused;

	if(focused)
	{
		SetRGBColor(&tab_highcol,255,236,33);
		SetRGBColor(&tab_lowcol,234,181,0);

		SetRGBColor(&button_highcol,255,255,0);
		SetRGBColor(&button_lowcol,255,203,0);
	}
	else
	{
		SetRGBColor(&tab_highcol,235,235,235);
		SetRGBColor(&tab_lowcol,160,160,160);

		SetRGBColor(&button_highcol,229,229,229);
		SetRGBColor(&button_lowcol,153,153,153);
	}

}

void BeDecorator::Draw(BRect update)
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

void BeDecorator::Draw(void)
{
	// Easy way to draw everything - no worries about drawing only certain
	// things

	DrawTab();

	// Draw the top view's client area - just a hack :)
	rgb_color blue={100,100,255,255};
	driver->FillRect(borderrect,blue);

	DrawFrame();
}

void BeDecorator::DrawZoom(BRect r)
{
	// If this has been implemented, then the decorator has a Zoom button
	// which should be drawn based on the state of the member zoomstate
	BRect zr=r;
	zr.left+=zr.Width()/3;
	zr.top+=zr.Height()/3;

	DrawBlendedRect(zr,zoomstate);
	DrawBlendedRect(zr.OffsetToCopy(r.LeftTop()),zoomstate);
}

void BeDecorator::DrawClose(BRect r)
{
	// Just like DrawZoom, but for a close button
	DrawBlendedRect(r,closestate);
}

void BeDecorator::DrawTab(void)
{
	// If a window has a tab, this will draw it and any buttons which are
	// in it.
	if(dlook==WLOOK_NO_BORDER)
		return;
	
	rgb_color tmpcol;
	float rstep,gstep,bstep;

	int steps=tabrect.IntegerHeight();
	rstep=float(tab_highcol.red-tab_lowcol.red)/steps;
	gstep=float(tab_highcol.green-tab_lowcol.green)/steps;
	bstep=float(tab_highcol.blue-tab_lowcol.blue)/steps;

	driver->StrokeRect(tabrect,frame_lowcol);
	driver->BeginLineArray(steps+1);
	for(float i=1;i<=steps; i++)
	{
		SetRGBColor(&tmpcol, uint8(tab_highcol.red-(i*rstep)),
			uint8(tab_highcol.green-(i*gstep)),
			uint8(tab_highcol.blue-(i*bstep)));
		driver->AddLine(BPoint(tabrect.left+1,tabrect.top+i),
			BPoint(tabrect.right-1,tabrect.top+i),tmpcol);
	}
	driver->EndLineArray();
	UpdateTitle(layer->name->String());

	// Draw the buttons if we're supposed to	
	if(!(dflags & NOT_CLOSABLE))
		DrawClose(closerect);
	if(!(dflags & NOT_ZOOMABLE))
		DrawZoom(zoomrect);
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
		startcol=button_lowcol;
		endcol=button_highcol;
	}
	else
	{
		startcol=button_highcol;
		endcol=button_lowcol;
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
		driver->StrokeLine(BPoint(r.left,r.top+i),
			BPoint(r.left+i,r.top),tmpcol);

		SetRGBColor(&tmpcol, uint8(halfcol.red-(i*rstep)),
			uint8(halfcol.green-(i*gstep)),
			uint8(halfcol.blue-(i*bstep)));
		driver->StrokeLine(BPoint(r.left+steps,r.top+i),
			BPoint(r.left+i,r.top+steps),tmpcol);

	}
	driver->StrokeRect(r,frame_lowcol);
}

void BeDecorator::DrawFrame(void)
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
	
	// Draw the resize thumb if we're supposed to
	if(!(dflags & NOT_RESIZABLE))
	{
		r=resizerect;

		int32 w=r.IntegerWidth(),  h=r.IntegerHeight();
		
		// This code is strictly for B_DOCUMENT_WINDOW looks
		if(dlook==WLOOK_DOCUMENT)
		{
			rgb_color tmpcol,halfcol, startcol, endcol;
			float rstep,gstep,bstep,i;
			
			int steps=(w<h)?w:h;
		
			startcol=frame_highercol;
			endcol=frame_lowercol;
		
			halfcol=MakeBlendColor(startcol,endcol,0.5);
		
			rstep=(startcol.red-halfcol.red)/steps;
			gstep=(startcol.green-halfcol.green)/steps;
			bstep=(startcol.blue-halfcol.blue)/steps;

			for(i=0;i<=steps; i++)
			{
				SetRGBColor(&tmpcol, uint8(startcol.red-(i*rstep)),
					uint8(startcol.green-(i*gstep)),
					uint8(startcol.blue-(i*bstep)));
				driver->StrokeLine(BPoint(r.left,r.top+i),
					BPoint(r.left+i,r.top),tmpcol);
		
				SetRGBColor(&tmpcol, uint8(halfcol.red-(i*rstep)),
					uint8(halfcol.green-(i*gstep)),
					uint8(halfcol.blue-(i*bstep)));
				driver->StrokeLine(BPoint(r.left+steps,r.top+i),
					BPoint(r.left+i,r.top+steps),tmpcol);			
			}
			driver->StrokeRect(r,frame_lowercol);
		}
		else
		{
			driver->StrokeLine(BPoint(r.right,r.top),BPoint(r.right-3,r.top),
				frame_lowercol);
			driver->StrokeLine(BPoint(r.left,r.bottom),BPoint(r.left,r.bottom-3),
				frame_lowercol);
		}
	}
}

void BeDecorator::SetLook(uint32 wlook)
{
	dlook=wlook;
}

void BeDecorator::CalculateBorders(void)
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
		{
			bsize.top=18;
			break;
		}
		case WLOOK_MODAL:
		case WLOOK_FLOATING:
		{
			bsize.top=15;
			break;
		}
		default:
		{
			break;
		}
	}
}
