/*
 * Copyright 2009-2010, Haiku.
 * Distributed under the terms of the MIT License.
 */


/*! Decorator looking like Windows 95 */


#include "WinDecorator.h"

#include <Point.h>
#include <View.h>

#include "DesktopSettings.h"
#include "DrawingEngine.h"
#include "PatternHandler.h"
#include "RGBColor.h"


//#define DEBUG_DECORATOR
#ifdef DEBUG_DECORATOR
#	include <stdio.h>
#	define STRACE(x) printf x ;
#else
#	define STRACE(x) ;
#endif


WinDecorator::WinDecorator(DesktopSettings& settings, BRect rect,
		window_look look, uint32 flags)
	:
	Decorator(settings, rect, look, flags)
{
	taboffset=0;

	frame_highcol = (rgb_color){ 255, 255, 255, 255 };
	frame_midcol = (rgb_color){ 216, 216, 216, 255 };
	frame_lowcol = (rgb_color){ 110, 110, 110, 255 };
	frame_lowercol = (rgb_color){ 0, 0, 0, 255 };

	fFocusTabColor = settings.UIColor(B_WINDOW_TAB_COLOR);
	fFocusTextColor = settings.UIColor(B_WINDOW_TEXT_COLOR);
	fNonFocusTabColor = settings.UIColor(B_WINDOW_INACTIVE_TAB_COLOR);
	fNonFocusTextColor = settings.UIColor(B_WINDOW_INACTIVE_TEXT_COLOR);

	_UpdateFont(settings);
	_DoLayout();
	
	textoffset=5;

	STRACE(("WinDecorator()\n"));
}


WinDecorator::~WinDecorator()
{
	STRACE(("~WinDecorator()\n"));
}


void
WinDecorator::SetTitle(const char* string, BRegion* updateRegion)
{
	// TODO: we could be much smarter about the update region

	BRect rect = TabRect();

	Decorator::SetTitle(string);

	if (updateRegion == NULL)
		return;

	BRect updatedRect = TabRect();
	if (rect.left > updatedRect.left)
		rect.left = updatedRect.left;
	if (rect.right < updatedRect.right)
		rect.right = updatedRect.right;

	updateRegion->Include(rect);
}


void
WinDecorator::FontsChanged(DesktopSettings& settings, BRegion* updateRegion)
{
	// get previous extent
	if (updateRegion != NULL) {
		BRegion extent;
		GetFootprint(&extent);
		updateRegion->Include(&extent);
	}

	_UpdateFont(settings);
	_DoLayout();

	if (updateRegion != NULL) {
		BRegion extent;
		GetFootprint(&extent);
		updateRegion->Include(&extent);
	}
}


void
WinDecorator::MoveBy(BPoint pt)
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
WinDecorator::ResizeBy(BPoint offset, BRegion* dirty)
{
	// Move all internal rectangles the appropriate amount
	fFrame.right += offset.x;
	fFrame.bottom += offset.y;

	fTabRect.right += offset.x;
	fBorderRect.right += offset.x;
	fBorderRect.bottom += offset.y;
	// fZoomRect.OffsetBy(offset.x,0);
	// fMinimizeRect.OffsetBy(offset.x,0);
	if (dirty) {
		dirty->Include(fTabRect);
		dirty->Include(fBorderRect);
	}


	// TODO probably some other layouting stuff here
	_DoLayout();
}


void
WinDecorator::GetFootprint(BRegion* region)
{
	// This function calculates the decorator's footprint in coordinates
	// relative to the view. This is most often used to set a Window
	// object's visible region.
	if (!region)
		return;

	region->MakeEmpty();

	if (fLook == B_NO_BORDER_WINDOW_LOOK)
		return;
	
	region->Set(fBorderRect);
	region->Include(fTabRect);
	region->Exclude(fFrame);
}


click_type
WinDecorator::Clicked(BPoint pt, int32 buttons, int32 modifiers)
{
	if (fCloseRect.Contains(pt)) {
		STRACE(("WinDecorator():Clicked() - Close\n"));
		return CLICK_CLOSE;
	}

	if (fZoomRect.Contains(pt)) {
		STRACE(("WinDecorator():Clicked() - Zoom\n"));
		return CLICK_ZOOM;
	}
	
	// Clicking in the tab?
	if (fTabRect.Contains(pt)) {
		// Here's part of our window management stuff
		/* TODO This is missing CLICK_MOVETOFRONT
		if(buttons==B_PRIMARY_MOUSE_BUTTON && !IsFocus())
			return CLICK_MOVETOFRONT;
		*/
		return CLICK_DRAG;
	}

	// We got this far, so user is clicking on the border?
	if (fBorderRect.Contains(pt) && !fFrame.Contains(pt)) {
		STRACE(("WinDecorator():Clicked() - Resize\n"));
		return CLICK_RESIZE;
	}

	// Guess user didn't click anything
	STRACE(("WinDecorator():Clicked()\n"));
	return CLICK_NONE;
}


void
WinDecorator::_DoLayout()
{
	STRACE(("WinDecorator()::_DoLayout()\n"));

	bool hasTab = false;
	
	fBorderRect=fFrame;
	fTabRect=fFrame;

	switch (Look()) {
		case B_MODAL_WINDOW_LOOK:
			fBorderRect.InsetBy(-4, -4);
			break;

		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
			hasTab = true;
			fBorderRect.InsetBy(-4, -4);
			break;
		case B_FLOATING_WINDOW_LOOK:
			hasTab = true;
			break;

		case B_BORDERED_WINDOW_LOOK:
			fBorderRect.InsetBy(-1, -1);
			break;

		default:
			break;
	}

	if (hasTab) {
		fBorderRect.top -= 19;

		fTabRect.top -= 19;
		fTabRect.bottom=fTabRect.top+19;

		fZoomRect=fTabRect;
		fZoomRect.top+=3;
		fZoomRect.right-=3;
		fZoomRect.bottom-=3;
		fZoomRect.left=fZoomRect.right-15;

		fCloseRect=fZoomRect;
		fZoomRect.OffsetBy(0-fZoomRect.Width()-3,0);

		fMinimizeRect=fZoomRect;
		fMinimizeRect.OffsetBy(0-fZoomRect.Width()-1,0);
	}
}


void
WinDecorator::_DrawTitle(BRect r)
{
	fDrawingEngine->SetHighColor(textcol);
	fDrawingEngine->SetLowColor(IsFocus()?fFocusTabColor:fNonFocusTabColor);

	fTruncatedTitle = Title();
	fDrawState.Font().TruncateString(&fTruncatedTitle, B_TRUNCATE_END,
		(fTabRect.left + textoffset) - (fZoomRect.left - 5));
	fTruncatedTitleLength = fTruncatedTitle.Length();
	fDrawingEngine->SetFont(fDrawState.Font());

	fDrawingEngine->DrawString(fTruncatedTitle,fTruncatedTitleLength,
		BPoint(fTabRect.left+textoffset,fCloseRect.bottom-1));
}


void
WinDecorator::_SetFocus(void)
{
	// SetFocus() performs necessary duties for color swapping and
	// other things when a window is deactivated or activated.

	if (IsFocus()) {
//		tab_highcol.SetColor(100,100,255);
//		tab_lowcol.SetColor(40,0,255);
		tab_highcol=fFocusTabColor;
		textcol=fFocusTextColor;
	} else {
//		tab_highcol.SetColor(220,220,220);
//		tab_lowcol.SetColor(128,128,128);
		tab_highcol=fNonFocusTabColor;
		textcol=fNonFocusTextColor;
	}
}


void
WinDecorator::Draw(BRect update)
{
	STRACE(("WinDecorator::Draw(): ")); update.PrintToStream();

	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(update);
	_DrawTab(update);
}


void
WinDecorator::Draw(void)
{
	STRACE(("WinDecorator::Draw()\n"));

	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(fBorderRect);
	_DrawTab(fTabRect);
}


void
WinDecorator::_DrawZoom(BRect r)
{
	DrawBeveledRect(r,GetZoom());
	
	// Draw the Zoom box

	BRect rect(r);
	rect.InsetBy(2,2);
	rect.InsetBy(1,0);
	rect.bottom--;
	rect.right--;
	
	if (GetZoom())
		rect.OffsetBy(1,1);

	fDrawingEngine->SetHighColor(RGBColor(0,0,0));
	fDrawingEngine->StrokeRect(rect);
	rect.InsetBy(1,1);
	fDrawingEngine->StrokeLine(rect.LeftTop(),rect.RightTop());
	
}

void
WinDecorator::_DrawClose(BRect r)
{
	// Just like DrawZoom, but for a close button
	DrawBeveledRect(r,GetClose());
	
	// Draw the X

	BRect rect(r);
	rect.InsetBy(4,4);
	rect.right--;
	rect.top--;
	
	if (GetClose())
		rect.OffsetBy(1,1);

	fDrawingEngine->SetHighColor(RGBColor(0,0,0));
	fDrawingEngine->StrokeLine(rect.LeftTop(),rect.RightBottom());
	fDrawingEngine->StrokeLine(rect.RightTop(),rect.LeftBottom());
	rect.OffsetBy(1,0);
	fDrawingEngine->StrokeLine(rect.LeftTop(),rect.RightBottom());
	fDrawingEngine->StrokeLine(rect.RightTop(),rect.LeftBottom());
}


void
WinDecorator::_DrawMinimize(BRect r)
{
	// Just like DrawZoom, but for a Minimize button
	DrawBeveledRect(r,GetMinimize());

	fDrawingEngine->SetHighColor(textcol);
	BRect rect(r.left+5,r.bottom-4,r.right-5,r.bottom-3);
	if(GetMinimize())
		rect.OffsetBy(1,1);
	
	fDrawingEngine->SetHighColor(RGBColor(0,0,0));
	fDrawingEngine->StrokeRect(rect);
}


void
WinDecorator::_DrawTab(BRect r)
{
	// If a window has a tab, this will draw it and any buttons which are
	// in it.
	if(fLook==B_NO_BORDER_WINDOW_LOOK)
		return;


	fDrawingEngine->FillRect(fTabRect,tab_highcol);

	_DrawTitle(r);

	// Draw the buttons if we're supposed to	
	if(!(fFlags & B_NOT_CLOSABLE))
		_DrawClose(fCloseRect);
	if(!(fFlags & B_NOT_ZOOMABLE))
		_DrawZoom(fZoomRect);
}


void
WinDecorator::DrawBeveledRect(BRect r, bool down)
{
	RGBColor higher;
	RGBColor high;
	RGBColor mid;
	RGBColor low;
	RGBColor lower;
	
	if (down) {
		lower.SetColor(255,255,255);
		low.SetColor(216,216,216);
		mid.SetColor(192,192,192);
		high.SetColor(128,128,128);
		higher.SetColor(0,0,0);
	} else {
		higher.SetColor(255,255,255);
		high.SetColor(216,216,216);
		mid.SetColor(192,192,192);
		low.SetColor(128,128,128);
		lower.SetColor(0,0,0);
	}

	BRect rect(r);
	BPoint pt;

	// Top highlight
	fDrawingEngine->SetHighColor(higher);
	fDrawingEngine->StrokeLine(rect.LeftTop(),rect.RightTop());

	// Left highlight
	fDrawingEngine->StrokeLine(rect.LeftTop(),rect.LeftBottom());

	// Right shading
	pt=rect.RightTop();
	pt.y++;
	fDrawingEngine->StrokeLine(pt,rect.RightBottom(),lower);
	
	// Bottom shading
	pt=rect.LeftBottom();
	pt.x++;
	fDrawingEngine->StrokeLine(pt,rect.RightBottom(),lower);

	rect.InsetBy(1,1);

	// Top inside highlight
	fDrawingEngine->StrokeLine(rect.LeftTop(),rect.RightTop());

	// Left inside highlight
	fDrawingEngine->StrokeLine(rect.LeftTop(),rect.LeftBottom());

	// Right inside shading
	pt=rect.RightTop();
	pt.y++;
	fDrawingEngine->StrokeLine(pt,rect.RightBottom(),lower);
	
	// Bottom inside shading
	pt=rect.LeftBottom();
	pt.x++;
	fDrawingEngine->StrokeLine(pt,rect.RightBottom(),lower);
	
	rect.InsetBy(1,1);

	fDrawingEngine->FillRect(rect,mid);
}


void
WinDecorator::_SetColors()
{
	_SetFocus();
}


void
WinDecorator::_UpdateFont(DesktopSettings& settings)
{
	ServerFont font;
	if (fLook == B_FLOATING_WINDOW_LOOK)
		settings.GetDefaultPlainFont(font);
	else
		settings.GetDefaultBoldFont(font);

	font.SetFlags(B_FORCE_ANTIALIASING);
	font.SetSpacing(B_STRING_SPACING);
	fDrawState.SetFont(font);
}


void
WinDecorator::_DrawFrame(BRect rect)
{
	if (fLook == B_NO_BORDER_WINDOW_LOOK)
		return;

	if (fBorderRect == fFrame)
		return;

	BRect r = fBorderRect;
	
	fDrawingEngine->SetHighColor(frame_lowercol);
	fDrawingEngine->StrokeRect(r);

	if (fLook == B_BORDERED_WINDOW_LOOK)
		return;
	
	BPoint pt;

	pt=r.RightTop();
	pt.x--;
	fDrawingEngine->StrokeLine(r.LeftTop(),pt,frame_midcol);
	pt=r.LeftBottom();
	pt.y--;
	fDrawingEngine->StrokeLine(r.LeftTop(),pt,frame_midcol);

	fDrawingEngine->StrokeLine(r.RightTop(),r.RightBottom(),frame_lowercol);
	fDrawingEngine->StrokeLine(r.LeftBottom(),r.RightBottom(),frame_lowercol);
	
	r.InsetBy(1,1);
	pt=r.RightTop();
	pt.x--;
	fDrawingEngine->StrokeLine(r.LeftTop(),pt,frame_highcol);
	pt=r.LeftBottom();
	pt.y--;
	fDrawingEngine->StrokeLine(r.LeftTop(),pt,frame_highcol);

	fDrawingEngine->StrokeLine(r.RightTop(),r.RightBottom(),frame_lowcol);
	fDrawingEngine->StrokeLine(r.LeftBottom(),r.RightBottom(),frame_lowcol);
	
	r.InsetBy(1,1);
	fDrawingEngine->StrokeRect(r,frame_midcol);
	r.InsetBy(1,1);
	fDrawingEngine->StrokeRect(r,frame_midcol);
}


extern "C" float
get_decorator_version(void)
{
	return 1.00;
}


extern "C" Decorator *
instantiate_decorator(DesktopSettings &desktopSetting, BRect rect,
	window_look wlook, int32 wflags)
{
	return new WinDecorator(desktopSetting, rect, wlook, wflags);
}
