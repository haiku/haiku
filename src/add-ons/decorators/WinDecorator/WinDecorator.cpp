/*
 * Copyright 2009-2010, Haiku.
 * Distributed under the terms of the MIT License.
 */


/*! Decorator looking like Windows 95 */


#include "WinDecorator.h"

#include <new>
#include <stdio.h>

#include <Point.h>
#include <Rect.h>
#include <View.h>

#include "DesktopSettings.h"
#include "DrawingEngine.h"
#include "PatternHandler.h"
#include "RGBColor.h"


//#define DEBUG_DECORATOR
#ifdef DEBUG_DECORATOR
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif


WinDecorAddOn::WinDecorAddOn(image_id id, const char* name)
	:
	DecorAddOn(id, name)
{
}


Decorator*
WinDecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect)
{
	return new (std::nothrow)WinDecorator(settings, rect);
}


WinDecorator::WinDecorator(DesktopSettings& settings, BRect rect)
	:
	Decorator(settings, rect)
{
	_UpdateFont(settings);

	// common colors to both focus and non focus state
	frame_highcol = (rgb_color){ 255, 255, 255, 255 };
	frame_midcol = (rgb_color){ 216, 216, 216, 255 };
	frame_lowcol = (rgb_color){ 110, 110, 110, 255 };
	frame_lowercol = (rgb_color){ 0, 0, 0, 255 };

	// state based colors
	fFocusTabColor = settings.UIColor(B_WINDOW_TAB_COLOR);
	fFocusTextColor = settings.UIColor(B_WINDOW_TEXT_COLOR);
	fNonFocusTabColor = settings.UIColor(B_WINDOW_INACTIVE_TAB_COLOR);
	fNonFocusTextColor = settings.UIColor(B_WINDOW_INACTIVE_TEXT_COLOR);

	fTabList.AddItem(_AllocateNewTab());

	// Set appropriate colors based on the current focus value. In this case,
	// each decorator defaults to not having the focus.
	_SetFocus(_TabAt(0));

	// Do initial decorator setup
	_DoLayout();

	textoffset = 5;

	STRACE(("WinDecorator()\n"));
}


WinDecorator::~WinDecorator()
{
	STRACE(("~WinDecorator()\n"));
}


// TODO : Add GetSettings


/*!	\brief Updates the decorator in the rectangular area \a updateRect.

	The default version updates all areas which intersect the frame and tab.

	\param updateRect The rectangular area to update.
*/
void
WinDecorator::Draw(BRect updateRect)
{
	STRACE(("WinDecorator::Draw(BRect updateRect): "));
	updateRect.PrintToStream();

	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(updateRect & fFrame);
	_DrawTabs(updateRect & fTitleBarRect);
}


void
WinDecorator::Draw()
{
	STRACE(("WinDecorator::Draw()\n"));
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(fFrame);
	_DrawTabs(fTitleBarRect);
}


// TODO : add GetSizeLimits


Decorator::Region
WinDecorator::RegionAt(BPoint where, int32& tab) const
{
	// Let the base class version identify hits of the buttons and the tab.
	Region region = Decorator::RegionAt(where, tab);
	if (region != REGION_NONE)
		return region;

	// check the resize corner
	if (fTopTab->look == B_DOCUMENT_WINDOW_LOOK && fResizeRect.Contains(where))
		return REGION_RIGHT_BOTTOM_CORNER;

	// hit-test the borders
	if (!(fTopTab->flags & B_NOT_RESIZABLE)
		&& (fTopTab->look == B_TITLED_WINDOW_LOOK
			|| fTopTab->look == B_FLOATING_WINDOW_LOOK
			|| fTopTab->look == B_MODAL_WINDOW_LOOK)
		&& fBorderRect.Contains(where) && !fFrame.Contains(where)) {
		return REGION_BOTTOM_BORDER;
			// TODO Determine the actual border!
	}

	return REGION_NONE;
}


void
WinDecorator::_DoLayout()
{
	STRACE(("WinDecorator()::_DoLayout()\n"));

	bool hasTab = false;

	fBorderRect = fFrame;
	switch ((int)fTopTab->look) {
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

		for (int32 i = 0; i < fTabList.CountItems(); i++) {
			Decorator::Tab* tab = fTabList.ItemAt(i);

			tab->tabRect.top -= 19;
			tab->tabRect.bottom = tab->tabRect.top + 19;

			tab->zoomRect = tab->tabRect;
			tab->zoomRect.top += 3;
			tab->zoomRect.right -= 3;
			tab->zoomRect.bottom -= 3;
			tab->zoomRect.left = tab->zoomRect.right - 15;

			tab->closeRect = tab->zoomRect;
			tab->zoomRect.OffsetBy(0 - tab->zoomRect.Width() - 3, 0);

			tab->minimizeRect = tab->zoomRect;
			tab->minimizeRect.OffsetBy(0 - tab->zoomRect.Width() - 1, 0);
		}
	} else {
		for (int32 i = 0; i < fTabList.CountItems(); i++) {
			Decorator::Tab* tab = fTabList.ItemAt(i);
			tab->tabRect.Set(0.0, 0.0, -1.0, -1.0);
			tab->closeRect.Set(0.0, 0.0, -1.0, -1.0);
			tab->zoomRect.Set(0.0, 0.0, -1.0, -1.0);
			tab->minimizeRect.Set(0.0, 0.0, -1.0, -1.0);
		}
	}
}


Decorator::Tab*
WinDecorator::_AllocateNewTab()
{
	Decorator::Tab* tab = new(std::nothrow) WinDecorator::Tab;
	if (tab == NULL)
		return NULL;
	// Set appropriate colors based on the current focus value. In this case,
	// each decorator defaults to not having the focus.
	_SetFocus(tab);
	return tab;
}


WinDecorator::Tab*
WinDecorator::_TabAt(int32 index) const
{
	// TODO stub
	return static_cast<WinDecorator::Tab*>(fTabList.ItemAt(index));
}


void
WinDecorator::_DrawFrame(BRect rect)
{
	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK)
		return;

	if (fBorderRect == fFrame)
		return;

	BRect r = fBorderRect;

	fDrawingEngine->SetHighColor(frame_lowercol);
	fDrawingEngine->StrokeRect(r);

	if (fTopTab->look == B_BORDERED_WINDOW_LOOK)
		return;

	BPoint pt;

	pt = r.RightTop();
	pt.x--;
	fDrawingEngine->StrokeLine(r.LeftTop(), pt, frame_midcol);
	pt = r.LeftBottom();
	pt.y--;
	fDrawingEngine->StrokeLine(r.LeftTop(), pt, frame_midcol);

	fDrawingEngine->StrokeLine(r.RightTop(), r.RightBottom(), frame_lowercol);
	fDrawingEngine->StrokeLine(r.LeftBottom(), r.RightBottom(), frame_lowercol);

	r.InsetBy(1,1);
	pt = r.RightTop();
	pt.x--;
	fDrawingEngine->StrokeLine(r.LeftTop(),pt,frame_highcol);
	pt = r.LeftBottom();
	pt.y--;
	fDrawingEngine->StrokeLine(r.LeftTop(),pt,frame_highcol);

	fDrawingEngine->StrokeLine(r.RightTop(), r.RightBottom(), frame_lowcol);
	fDrawingEngine->StrokeLine(r.LeftBottom(), r.RightBottom(), frame_lowcol);

	r.InsetBy(1, 1);
	fDrawingEngine->StrokeRect(r, frame_midcol);
	r.InsetBy(1, 1);
	fDrawingEngine->StrokeRect(r, frame_midcol);
}


void
WinDecorator::_DrawTab(Decorator::Tab* tab, BRect rect)
{
	const BRect& tabRect = tab->tabRect;

	// If a window has a tab, this will draw it and any buttons in it.
	if (!tabRect.IsValid() || !rect.Intersects(tabRect)
		|| fTopTab->look == B_NO_BORDER_WINDOW_LOOK) {
		return;
	}

	fDrawingEngine->FillRect(tabRect, tab_highcol);

	_DrawTitle(tab, tabRect);

	// Draw the buttons if we're supposed to
	// TODO : we should still draw the buttons if they are disabled, but grey them out
	DrawButtons(tab, rect);
}


void
WinDecorator::_DrawTitle(Decorator::Tab* tab, BRect rect)
{
	const BRect& tabRect = tab->tabRect;
	const BRect& closeRect = tab->closeRect;
	const BRect& zoomRect = tab->zoomRect;

	//fDrawingEngine->SetDrawingMode(B_OP_OVER);
	fDrawingEngine->SetHighColor(textcol);
	fDrawingEngine->SetLowColor(IsFocus(_TabAt(0))
		? fFocusTabColor : fNonFocusTabColor);

	fTruncatedTitle = Title(_TabAt(0));
	fDrawState.Font().TruncateString(&fTruncatedTitle, B_TRUNCATE_END,
		((zoomRect.IsValid() ? zoomRect.left :
			closeRect.IsValid() ? closeRect.left : tabRect.right) - 5)
		- (tabRect.left + textoffset));
	fTruncatedTitleLength = fTruncatedTitle.Length();
	fDrawingEngine->SetFont(fDrawState.Font());

	fDrawingEngine->DrawString(fTruncatedTitle, fTruncatedTitleLength,
		BPoint(tabRect.left + textoffset,closeRect.bottom - 1));

	//fDrawingEngine->SetDrawingMode(B_OP_COPY);
}


void
WinDecorator::_DrawClose(Decorator::Tab* tab, bool direct, BRect rect)
{
	STRACE(("_DrawClose(%f, %f, %f, %f)\n", rect.left, rect.top, rect.right,
		rect.bottom));

	// Just like DrawZoom, but for a close button
	_DrawBeveledRect(rect, true);

	// Draw the X

	BRect closeBox(rect);
	closeBox.InsetBy(4, 4);
	closeBox.right--;
	closeBox.top--;

	if (true)
		closeBox.OffsetBy(1, 1);

	fDrawingEngine->SetHighColor(RGBColor(0, 0, 0));
	fDrawingEngine->StrokeLine(closeBox.LeftTop(), closeBox.RightBottom());
	fDrawingEngine->StrokeLine(closeBox.RightTop(), closeBox.LeftBottom());
	closeBox.OffsetBy(1, 0);
	fDrawingEngine->StrokeLine(closeBox.LeftTop(), closeBox.RightBottom());
	fDrawingEngine->StrokeLine(closeBox.RightTop(), closeBox.LeftBottom());
}


void
WinDecorator::_DrawZoom(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawBeveledRect(rect, true);

	// Draw the Zoom box

	BRect zoomBox(rect);
	zoomBox.InsetBy(2, 2);
	zoomBox.InsetBy(1, 0);
	zoomBox.bottom--;
	zoomBox.right--;

	if (true)
		zoomBox.OffsetBy(1, 1);

	fDrawingEngine->SetHighColor(RGBColor(0,0,0));
	fDrawingEngine->StrokeRect(zoomBox);
	zoomBox.InsetBy(1, 1);
	fDrawingEngine->StrokeLine(zoomBox.LeftTop(), zoomBox.RightTop());
}


void
WinDecorator::_DrawMinimize(Decorator::Tab* tab, bool direct, BRect rect)
{
	// Just like DrawZoom, but for a Minimize button
	_DrawBeveledRect(rect, true);

	fDrawingEngine->SetHighColor(textcol);
	BRect minimizeBox(rect.left + 5, rect.bottom - 4, rect.right - 5,
		rect.bottom - 3);
	if (true)
		minimizeBox.OffsetBy(1, 1);

	fDrawingEngine->SetHighColor(RGBColor(0, 0, 0));
	fDrawingEngine->StrokeRect(minimizeBox);
}


void
WinDecorator::_SetTitle(Decorator::Tab* tab, const char* string,
	BRegion* updateRegion)
{
	// TODO we could be much smarter about the update region

	BRect rect = TabRect(tab);

	if (updateRegion == NULL)
		return;

	BRect updatedRect = TabRect(tab);
	if (rect.left > updatedRect.left)
		rect.left = updatedRect.left;
	if (rect.right < updatedRect.right)
		rect.right = updatedRect.right;

	updateRegion->Include(rect);
}


void
WinDecorator::_FontsChanged(DesktopSettings& settings,
	BRegion* updateRegion)
{
	// get previous extent
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());

	_UpdateFont(settings);
	_DoLayout();

	_InvalidateFootprint();
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());
}


void
WinDecorator::_SetLook(Decorator::Tab* tab, DesktopSettings& settings,
	window_look look, BRegion* updateRegion)
{
	tab->look = look;

	// TODO we could be much smarter about the update region

	// get previous extent
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());

	fTopTab->look = look;

	_UpdateFont(settings);
	_DoLayout();

	_InvalidateFootprint();
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());
}


void
WinDecorator::_SetFlags(Decorator::Tab* tab, uint32 flags, BRegion* updateRegion)
{
	tab->flags = flags;

	// TODO we could be much smarter about the update region

	// get previous extent
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());

	_DoLayout();

	_InvalidateFootprint();
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());
}


void
WinDecorator::_SetFocus(Decorator::Tab* tab)
{
	// TODO stub

	// SetFocus() performs necessary duties for color swapping and
	// other things when a window is deactivated or activated.

	if (IsFocus(_TabAt(0))) {
//		tab_highcol.SetColor(100, 100, 255);
//		tab_lowcol.SetColor(40, 0, 255);
		tab_highcol = fFocusTabColor;
		textcol = fFocusTextColor;
	} else {
//		tab_highcol.SetColor(220, 220, 220);
//		tab_lowcol.SetColor(128, 128, 128);
		tab_highcol = fNonFocusTabColor;
		textcol = fNonFocusTextColor;
	}
}


void
WinDecorator::_MoveBy(BPoint offset)
{
	// Move all tab rectangles over
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);

		tab->zoomRect.OffsetBy(offset);
		tab->closeRect.OffsetBy(offset);
		tab->minimizeRect.OffsetBy(offset);
		tab->tabRect.OffsetBy(offset);
	}

	// Move all internal rectangles over
	fFrame.OffsetBy(offset);
	fBorderRect.OffsetBy(offset);
}


void
WinDecorator::_ResizeBy(BPoint offset, BRegion* dirty)
{
	// Move all internal rectangles the appropriate amount
	fFrame.right += offset.x;
	fFrame.bottom += offset.y;
	fBorderRect.right += offset.x;
	fBorderRect.bottom += offset.y;

	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);

		tab->tabRect.right += offset.x;
		if (dirty != NULL)
			dirty->Include(tab->tabRect);
		//tab->zoomRect.right += offset.x;
		//tab->closeRect.right += offset.x;
		//tab->minimizeRect.right += offset.x;
	}
	if (dirty != NULL)
		dirty->Include(fBorderRect);

	// TODO probably some other layouting stuff here
	_DoLayout();
}


// TODO : _SetSettings


bool
WinDecorator::_AddTab(DesktopSettings& settings, int32 index,
	BRegion* updateRegion)
{
	// TODO stub
	return false;
}


bool
WinDecorator::_RemoveTab(int32 index, BRegion* updateRegion)
{
	// TODO stub
	return false;
}


bool
WinDecorator::_MoveTab(int32 from, int32 to, bool isMoving,
	BRegion* updateRegion)
{
	// TODO stub
	return false;
}


void
WinDecorator::_GetFootprint(BRegion* region)
{
	// This function calculates the decorator's footprint in coordinates
	// relative to the view. This is most often used to set a Window
	// object's visible region.
	if (region == NULL)
		return;

	region->MakeEmpty();

	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK)
		return;

	region->Set(fBorderRect);
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		region->Include(tab->tabRect);
	}
	region->Exclude(fFrame);
}


void
WinDecorator::DrawButtons(Decorator::Tab* tab, const BRect& invalid)
{
	if (!(tab->flags & B_NOT_MINIMIZABLE) && invalid.Intersects(tab->minimizeRect))
		_DrawMinimize(tab, false, tab->minimizeRect);
	if (!(tab->flags & B_NOT_ZOOMABLE) && invalid.Intersects(tab->zoomRect))
		_DrawZoom(tab, false, tab->zoomRect);
	if (!(tab->flags & B_NOT_CLOSABLE) && invalid.Intersects(tab->closeRect))
		_DrawClose(tab, false, tab->closeRect);
}


void
WinDecorator::_UpdateFont(DesktopSettings& settings)
{
	ServerFont font;
	if (fTopTab->look == B_FLOATING_WINDOW_LOOK)
		settings.GetDefaultPlainFont(font);
	else
		settings.GetDefaultBoldFont(font);

	font.SetFlags(B_FORCE_ANTIALIASING);
	font.SetSpacing(B_STRING_SPACING);
	fDrawState.SetFont(font);
}


void
WinDecorator::_DrawBeveledRect(BRect r, bool down)
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
	BPoint point;

	// Top highlight
	fDrawingEngine->SetHighColor(higher);
	fDrawingEngine->StrokeLine(rect.LeftTop(), rect.RightTop());

	// Left highlight
	fDrawingEngine->StrokeLine(rect.LeftTop(), rect.LeftBottom());

	// Right shading
	point = rect.RightTop();
	point.y++;
	fDrawingEngine->StrokeLine(point, rect.RightBottom(), lower);

	// Bottom shading
	point = rect.LeftBottom();
	point.x++;
	fDrawingEngine->StrokeLine(point, rect.RightBottom(), lower);

	rect.InsetBy(1,1);

	// Top inside highlight
	fDrawingEngine->StrokeLine(rect.LeftTop(), rect.RightTop());

	// Left inside highlight
	fDrawingEngine->StrokeLine(rect.LeftTop(), rect.LeftBottom());

	// Right inside shading
	point = rect.RightTop();
	point.y++;
	fDrawingEngine->StrokeLine(point, rect.RightBottom(), lower);

	// Bottom inside shading
	point = rect.LeftBottom();
	point.x++;
	fDrawingEngine->StrokeLine(point, rect.RightBottom(), lower);

	rect.InsetBy(1,1);

	fDrawingEngine->FillRect(rect,mid);
}


// #pragma mark -


extern "C" DecorAddOn*
instantiate_decor_addon(image_id id, const char* name)
{
	return new (std::nothrow)WinDecorAddOn(id, name);
}
