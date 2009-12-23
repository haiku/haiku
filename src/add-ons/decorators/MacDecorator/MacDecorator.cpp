#include <Point.h>
#include "DrawingEngine.h"
#include <View.h>
#include "MacDecorator.h"
#include "RGBColor.h"
#include "PatternHandler.h"

//#define DEBUG_DECOR

#ifdef DEBUG_DECOR
#include <stdio.h>
#endif


static inline uint8
blend_color_value(uint8 a, uint8 b, float position)
{
	int16 delta = (int16)b - a;
	int32 value = a + (int32)(position * delta);
	if (value > 255)
		return 255;
	if (value < 0)
		return 0;

	return (uint8)value;
}


/*!
	\brief Function mostly for calculating gradient colors
	\param col Start color
	\param col2 End color
	\param position A floating point number such that 0.0 <= position <= 1.0. 0.0 results in the
		start color and 1.0 results in the end color.
	\return The blended color. If an invalid position was given, {0,0,0,0} is returned.
*/
static rgb_color
make_blend_color(rgb_color colorA, rgb_color colorB, float position)
{
	if (position < 0)
		return colorA;
	if (position > 1)
		return colorB;

	rgb_color blendColor;
	blendColor.red = blend_color_value(colorA.red, colorB.red, position);
	blendColor.green = blend_color_value(colorA.green, colorB.green, position);
	blendColor.blue = blend_color_value(colorA.blue, colorB.blue, position);
	blendColor.alpha = blend_color_value(colorA.alpha, colorB.alpha, position);

	return blendColor;
}



MacDecorator::MacDecorator(DesktopSettings& settings, BRect frame, window_look look, uint32 flags)
 : Decorator(settings, frame, look, flags)
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
	if(fCloseRect.Contains(pt))
	{

#ifdef DEBUG_DECOR
printf("MacDecorator():Clicked() - Close\n");
#endif

		return DEC_CLOSE;
	}

	if(fZoomRect.Contains(pt))
	{

#ifdef DEBUG_DECOR
printf("MacDecorator():Clicked() - Zoom\n");
#endif

		return DEC_ZOOM;
	}
	
	// Clicking in the tab?
	if(fTabRect.Contains(pt))
	{
		// Here's part of our window management stuff
		/* TODO This is missing DEC_MOVETOFRONT
		if(buttons==B_PRIMARY_MOUSE_BUTTON && !IsFocus())
			return DEC_MOVETOFRONT;
		*/
		return DEC_DRAG;
	}

	// We got this far, so user is clicking on the border?
	BRect srect(fFrame);
	srect.top+=19;
	BRect clientrect(srect.InsetByCopy(3,3));
	if(srect.Contains(pt) && !clientrect.Contains(pt))
	{
#ifdef DEBUG_DECOR
printf("MacDecorator():Clicked() - Resize\n");
#endif		
		return DEC_RESIZE;
	}

	// Guess user didn't click anything
#ifdef DEBUG_DECOR
printf("MacDecorator():Clicked()\n");
#endif
	return DEC_NONE;
}

void MacDecorator::_DoLayout(void)
{
#ifdef DEBUG_DECOR
printf("MacDecorator()::_DoLayout()\n");
#endif
	fBorderRect=fFrame;
	fBorderRect.top+=19;
	fTabRect=fFrame;

	fTabRect.bottom=fTabRect.top+19;

	fZoomRect=fTabRect;
	fZoomRect.left=fZoomRect.right-12;
	fZoomRect.bottom=fZoomRect.top+12;
	fZoomRect.OffsetBy(-4,4);
	
	fCloseRect=fZoomRect;
	fMinimizeRect=fZoomRect;

	fCloseRect.OffsetTo(fTabRect.left+4,fTabRect.top+4);
	
	fZoomRect.OffsetBy(0-(fZoomRect.Width()+4),0);
	if(Title() && fDrawingEngine)
	{
		titlepixelwidth=fDrawingEngine->StringWidth(Title(),strlen(Title())/*,&fDrawState*/);
		
		if(titlepixelwidth<(fZoomRect.left-fCloseRect.right-10))
		{
			// start with offset from closerect.right
			textoffset=int(((fZoomRect.left-5)-(fCloseRect.right+5))/2);
			textoffset-=int(titlepixelwidth/2);
			
			// now make it the offset from fTabRect.left
			textoffset+=int(fCloseRect.right+5-fTabRect.left);
		}
		else
			textoffset=int(fCloseRect.right)+5;
	}
	else
	{
		textoffset=0;
		titlepixelwidth=0;
	}
}

void MacDecorator::MoveBy(BPoint pt)
{
	// Move all internal rectangles the appropriate amount
	fFrame.OffsetBy(pt);
	fCloseRect.OffsetBy(pt);
	fTabRect.OffsetBy(pt);
	fBorderRect.OffsetBy(pt);
	fZoomRect.OffsetBy(pt);
	fMinimizeRect.OffsetBy(pt);
}


void
MacDecorator::ResizeBy(BPoint pt, BRegion* dirty)
{
	// Move all internal rectangles the appropriate amount
	fFrame.right += pt.x;
	fFrame.bottom += pt.y;

	fTabRect.right += pt.x;
	fBorderRect.right += pt.x;
	fBorderRect.bottom += pt.y;
	fZoomRect.OffsetBy(pt.x,0);
	fMinimizeRect.OffsetBy(pt.x,0);

	// handle invalidation of resize rect
	if (dirty && !(fFlags & B_NOT_RESIZABLE)) {
		BRect realResizeRect;
		switch (fLook) {
			case B_DOCUMENT_WINDOW_LOOK:
				realResizeRect = fResizeRect;
				// resize rect at old location
				dirty->Include(realResizeRect);
				realResizeRect.OffsetBy(pt);
				// resize rect at new location
				dirty->Include(realResizeRect);
				break;
			case B_TITLED_WINDOW_LOOK:
			case B_FLOATING_WINDOW_LOOK:
			case B_MODAL_WINDOW_LOOK:
				// resize rect at old location
				dirty->Include(fBorderRect);
				fBorderRect.OffsetBy(pt);
				// resize rect at new location
				dirty->Include(fBorderRect);
				break;
			default:
				break;
		}
	}

	// TODO probably some other layouting stuff here
}


void MacDecorator::_DrawTitle(BRect r)
{
	if(IsFocus())
		fDrawState.SetHighColor(textcol);
	else
		fDrawState.SetHighColor(inactive_textcol);
		
	fDrawState.SetLowColor(frame_midcol);

	fTruncatedTitle = Title();
	fDrawState.Font().TruncateString(&fTruncatedTitle, B_TRUNCATE_END,
		(fZoomRect.left - 5) - (fCloseRect.right + 5));
	fTruncatedTitleLength = fTruncatedTitle.Length();

	fDrawingEngine->DrawString(fTruncatedTitle,fTruncatedTitleLength,
		BPoint(fTabRect.left+textoffset,fCloseRect.bottom-1)/*,&fDrawState*/);
}

void MacDecorator::GetFootprint(BRegion *region)
{
	// This function calculates the decorator's footprint in coordinates
	// relative to the layer. This is most often used to set a WinBorder
	// object's visible region.
	if(!region)
		return;
	
	region->Set(fBorderRect);
	region->Include(fTabRect);
}

void MacDecorator::Draw(BRect update)
{
#ifdef DEBUG_DECOR
printf("MacDecorator::Draw(): "); update.PrintToStream();
#endif
/*
	// Draw the top view's client area - just a hack :)
	fDrawingEngine->FillRect(fBorderRect,_colors->document_background);

	if(fBorderRect.Intersects(update))
		fDrawingEngine->FillRect(fBorderRect,_colors->document_background);
*/	
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
//	fDrawState.SetHighColor(blue);

//	fDrawingEngine->FillRect(fBorderRect,_colors->document_background);
//	fDrawingEngine->FillRect(fBorderRect,_colors->document_background);
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
	fDrawState.SetHighColor(RGBColor(136,136,136));
	fDrawingEngine->StrokeLine(pt,pt2,fDrawState.HighColor());
	
	pt2=r.LeftBottom();
	pt2.y--;
	fDrawingEngine->StrokeLine(pt,pt2,fDrawState.HighColor());
	
	pt=r.RightBottom();
	pt2=r.RightTop();
	pt2.y++;
	fDrawState.SetHighColor(RGBColor(255,255,255));
	fDrawingEngine->StrokeLine(pt,pt2,fDrawState.HighColor());
	
	pt2=r.LeftBottom();
	pt2.x++;
	fDrawingEngine->StrokeLine(pt,pt2,fDrawState.HighColor());

	rect.InsetBy(1,1);
	fDrawState.SetHighColor(RGBColor(0,0,0));
	fDrawingEngine->StrokeRect(rect,fDrawState.HighColor());
	
	rect.InsetBy(1,1);
	DrawBlendedRect(rect,down);
	rect.InsetBy(1,1);
	DrawBlendedRect(rect,!down);
	
	rect.top+=2;
	rect.left--;
	rect.right++;
	
	fDrawState.SetHighColor(RGBColor(0,0,0));
	fDrawingEngine->StrokeLine(rect.LeftTop(),rect.RightTop(),fDrawState.HighColor());
}

void MacDecorator::_DrawClose(BRect r)
{
	bool down=GetClose();
	
	// Just like DrawZoom, but for a close button
	BRect rect(r);

	BPoint pt(r.LeftTop()),pt2(r.RightTop());
	
	pt2.x--;
	fDrawState.SetHighColor(RGBColor(136,136,136));
	fDrawingEngine->StrokeLine(pt,pt2,fDrawState.HighColor());
	
	pt2=r.LeftBottom();
	pt2.y--;
	fDrawingEngine->StrokeLine(pt,pt2,fDrawState.HighColor());
	
	pt=r.RightBottom();
	pt2=r.RightTop();
	pt2.y++;
	fDrawState.SetHighColor(RGBColor(255,255,255));
	fDrawingEngine->StrokeLine(pt,pt2,fDrawState.HighColor());
	
	pt2=r.LeftBottom();
	pt2.x++;
	fDrawingEngine->StrokeLine(pt,pt2,fDrawState.HighColor());

	rect.InsetBy(1,1);
	fDrawState.SetHighColor(RGBColor(0,0,0));
	fDrawingEngine->StrokeRect(rect,fDrawState.HighColor());
	
	rect.InsetBy(1,1);
	DrawBlendedRect(rect,down);
	rect.InsetBy(1,1);
	DrawBlendedRect(rect,!down);
	
//	rect.top+=4;
//	rect.left++;
//	rect.right--;
	
//	fDrawState.SetHighColor(RGBColor(0,0,0));
//	fDrawingEngine->StrokeLine(rect.LeftTop(),rect.RightTop(),&fDrawState,pat_solidhigh);
}

void MacDecorator::_DrawMinimize(BRect r)
{
	bool down=GetClose();
	
	// Just like DrawZoom, but for a close button
	BRect rect(r);

	BPoint pt(r.LeftTop()),pt2(r.RightTop());
	
	pt2.x--;
	fDrawState.SetHighColor(RGBColor(136,136,136));
	fDrawingEngine->StrokeLine(pt,pt2,fDrawState.HighColor());
	
	pt2=r.LeftBottom();
	pt2.y--;
	fDrawingEngine->StrokeLine(pt,pt2,fDrawState.HighColor());
	
	pt=r.RightBottom();
	pt2=r.RightTop();
	pt2.y++;
	fDrawState.SetHighColor(RGBColor(255,255,255));
	fDrawingEngine->StrokeLine(pt,pt2,fDrawState.HighColor());
	
	pt2=r.LeftBottom();
	pt2.x++;
	fDrawingEngine->StrokeLine(pt,pt2,fDrawState.HighColor());

	rect.InsetBy(1,1);
	fDrawState.SetHighColor(RGBColor(0,0,0));
	fDrawingEngine->StrokeRect(rect,fDrawState.HighColor());
	
	rect.InsetBy(1,1);
	DrawBlendedRect(rect,down);
	rect.InsetBy(1,1);
	DrawBlendedRect(rect,!down);
	
	rect.top+=4;
	rect.bottom-=4;
	rect.InsetBy(-2,0);
	
	fDrawState.SetHighColor(RGBColor(0,0,0));
	fDrawingEngine->StrokeRect(rect,fDrawState.HighColor());
}

void MacDecorator::_DrawTab(BRect r)
{
	// If a window has a tab, this will draw it and any buttons which are
	// in it.
	if(fLook==B_NO_BORDER_WINDOW_LOOK)
		return;
	
//	fDrawState.SetHighColor(frame_lowcol);
//	fDrawingEngine->StrokeRect(fTabRect,fDrawState.HighColor());

//	UpdateTitle(layer->name->String());
	BRect rect(fTabRect);
	fDrawState.SetHighColor(RGBColor(frame_midcol));
	fDrawingEngine->FillRect(rect,frame_midcol);
	
	
	if(IsFocus())
	{
		fDrawingEngine->StrokeLine(rect.LeftTop(),rect.RightTop(),frame_lowercol);
		fDrawingEngine->StrokeLine(rect.LeftTop(),rect.LeftBottom(),frame_lowercol);
		fDrawingEngine->StrokeLine(rect.RightBottom(),rect.RightTop(),frame_lowercol);
	
		rect.InsetBy(1,1);
		rect.bottom++;
		
		fDrawingEngine->StrokeLine(rect.LeftTop(),rect.RightTop(),frame_highcol);
		fDrawingEngine->StrokeLine(rect.LeftTop(),rect.LeftBottom(),frame_highcol);
		fDrawingEngine->StrokeLine(rect.RightBottom(),rect.RightTop(),frame_lowcol);
		
		// Draw the neat little lines on either side of the title if there's room
		if((fTabRect.left+textoffset)>(fCloseRect.right+5))
		{
			// Left side

			BPoint pt(fCloseRect.right+5,fCloseRect.top),
				pt2(fTabRect.left+textoffset-5,fCloseRect.top);
			fDrawState.SetHighColor(RGBColor(frame_highcol));
			for(int32 i=0;i<6;i++)
			{
				fDrawingEngine->StrokeLine(pt,pt2,fDrawState.HighColor());
				pt.y+=2;
				pt2.y+=2;
			}
			
			pt.Set(fCloseRect.right+6,fCloseRect.top+1),
				pt2.Set(fTabRect.left+textoffset-4,fCloseRect.top+1);
			fDrawState.SetHighColor(RGBColor(frame_lowcol));
			for(int32 i=0;i<6;i++)
			{
				fDrawingEngine->StrokeLine(pt,pt2,fDrawState.HighColor());
				pt.y+=2;
				pt2.y+=2;
			}
			
			// Right side
			
			pt.Set(fTabRect.left+textoffset+titlepixelwidth+6,fZoomRect.top),
				pt2.Set(fZoomRect.left-6,fZoomRect.top);
			if(pt.x<pt2.x)
			{
				fDrawState.SetHighColor(RGBColor(frame_highcol));
				for(int32 i=0;i<6;i++)
				{
					fDrawingEngine->StrokeLine(pt,pt2,fDrawState.HighColor());
					pt.y+=2;
					pt2.y+=2;
				}
				pt.Set(fTabRect.left+textoffset+titlepixelwidth+7,fZoomRect.top+1),
					pt2.Set(fZoomRect.left-5,fZoomRect.top+1);
				fDrawState.SetHighColor(frame_lowcol);
				for(int32 i=0;i<6;i++)
				{
					fDrawingEngine->StrokeLine(pt,pt2,fDrawState.HighColor());
					pt.y+=2;
					pt2.y+=2;
				}
			}
		}
		
		// Draw the buttons if we're supposed to	
		if(!(fFlags & B_NOT_CLOSABLE))
			_DrawClose(fCloseRect);
		if(!(fFlags & B_NOT_ZOOMABLE))
			_DrawZoom(fZoomRect);
	}
	
}

void MacDecorator::DrawBlendedRect(BRect r, bool down)
{
	BRect r1 = r;
	BRect r2 = r;

	r1.left += 1.0;
	r1.top  += 1.0;

	r2.bottom -= 1.0;
	r2.right  -= 1.0;

	// TODO: replace these with cached versions? does R5 use different colours?
	RGBColor tabColorLight = RGBColor(tint_color(tab_col.GetColor32(),B_LIGHTEN_2_TINT));
	RGBColor tabColorShadow = RGBColor(tint_color(tab_col.GetColor32(),B_DARKEN_2_TINT));

	int32 w = r.IntegerWidth();
	int32 h = r.IntegerHeight();

	RGBColor temprgbcol;
	rgb_color halfColor, startColor, endColor;
	float rstep, gstep, bstep;

	int steps = w < h ? w : h;

	if (down) {
		startColor = button_lowcol.GetColor32();
		endColor = button_highcol.GetColor32();
	} else {
		startColor = button_highcol.GetColor32();
		endColor = button_lowcol.GetColor32();
	}

	halfColor = make_blend_color(startColor, endColor, 0.5);

	rstep = float(startColor.red - halfColor.red) / steps;
	gstep = float(startColor.green - halfColor.green) / steps;
	bstep = float(startColor.blue - halfColor.blue) / steps;

	for (int32 i = 0; i <= steps; i++) {
		temprgbcol.SetColor(uint8(startColor.red - (i * rstep)),
							uint8(startColor.green - (i * gstep)),
							uint8(startColor.blue - (i * bstep)));

		fDrawingEngine->StrokeLine(BPoint(r.left, r.top + i),
							BPoint(r.left + i, r.top), temprgbcol);

		temprgbcol.SetColor(uint8(halfColor.red - (i * rstep)),
							uint8(halfColor.green - (i * gstep)),
							uint8(halfColor.blue - (i * bstep)));

		fDrawingEngine->StrokeLine(BPoint(r.left + steps, r.top + i),
							BPoint(r.left + i, r.top + steps), temprgbcol);
	}

	// draw bevelling effect on box
	fDrawingEngine->StrokeRect(r2, tabColorShadow); // inner dark box
	fDrawingEngine->StrokeRect(r,  tabColorShadow); // outer dark box
	fDrawingEngine->StrokeRect(r1, tabColorLight);  // light box
}

/*
void MacDecorator::_SetColors(void)
{
	_SetFocus();
}
*/
void MacDecorator::_DrawFrame(BRect rect)
{
	if(fLook==B_NO_BORDER_WINDOW_LOOK)
		return;

	BRect r=fBorderRect;
	BPoint pt,pt2,topleftpt,toprightpt;
	
	pt=r.LeftTop();
	pt2=r.LeftBottom();

	// Draw the left side of the frame
	fDrawingEngine->StrokeLine(pt,pt2,frame_lowercol);
	pt.x++;
	pt2.x++;
	pt2.y--;
	
	fDrawingEngine->StrokeLine(pt,pt2,frame_highcol);
	pt.x++;
	pt2.x++;
	pt2.y--;
	
	fDrawingEngine->StrokeLine(pt,pt2,frame_midcol);
	pt.x++;
	pt2.x++;
	fDrawingEngine->StrokeLine(pt,pt2,frame_midcol);
	pt.x++;
	pt2.x++;
	pt2.y--;

	fDrawingEngine->StrokeLine(pt,pt2,frame_lowcol);
	pt.x++;
	pt.y+=2;
	topleftpt=pt;
	pt2.x++;
	pt2.y--;

	fDrawingEngine->StrokeLine(pt,pt2,frame_lowercol);


	pt=r.RightTop();
	pt2=r.RightBottom();
	
	// Draw the right side of the frame
	fDrawingEngine->StrokeLine(pt,pt2,frame_lowercol);
	pt.x--;
	pt2.x--;

	fDrawingEngine->StrokeLine(pt,pt2,frame_lowcol);
	pt.x--;
	pt2.x--;

	fDrawingEngine->StrokeLine(pt,pt2,frame_midcol);
	pt.x--;
	pt2.x--;
	fDrawingEngine->StrokeLine(pt,pt2,frame_midcol);
	pt.x--;
	pt2.x--;

	fDrawingEngine->StrokeLine(pt,pt2,frame_highcol);
	pt.x--;
	pt.y+=2;
	toprightpt=pt;
	pt2.x--;
	
	fDrawingEngine->StrokeLine(pt,pt2,frame_lowercol);

	// Draw the top side of the frame that is not in the tab
	fDrawingEngine->StrokeLine(topleftpt,toprightpt,frame_lowercol);
	topleftpt.y--;
	toprightpt.x++;
	toprightpt.y--;

	fDrawingEngine->StrokeLine(topleftpt,toprightpt,frame_lowcol);
	
	pt=r.RightTop();
	pt2=r.RightBottom();


	pt=r.LeftBottom();
	pt2=r.RightBottom();
	
	// Draw the bottom side of the frame
	fDrawingEngine->StrokeLine(pt,pt2,frame_lowercol);
	pt.x++;
	pt.y--;
	pt2.x--;
	pt2.y--;

	fDrawingEngine->StrokeLine(pt,pt2,frame_lowcol);
	pt.x++;
	pt.y--;
	pt2.x--;
	pt2.y--;

	fDrawingEngine->StrokeLine(pt,pt2,frame_midcol);
	pt.x++;
	pt.y--;
	pt2.x--;
	pt2.y--;

	fDrawingEngine->StrokeLine(pt,pt2,frame_midcol);
	pt.x++;
	pt.y--;
	pt2.x--;
	pt2.y--;

	fDrawingEngine->StrokeLine(pt,pt2,frame_highcol);
	pt.x+=2;
	pt.y--;
	pt2.x--;
	pt2.y--;
	
	fDrawingEngine->StrokeLine(pt,pt2,frame_lowercol);
	pt.y--;
	pt2.x--;
	pt2.y--;
}

extern "C" float get_decorator_version(void)
{
	return 1.00;
}


extern "C" Decorator *(instantiate_decorator)(DesktopSettings &desktopSetting, BRect rec,
										window_look loo, uint32 flag)
{
	return (new MacDecorator(desktopSetting, rec, loo, flag));
}
