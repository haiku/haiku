/*
 * Copyright 2009-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm, bpmagic@columbus.rr.com
 *		Adrien Destugues, pulkomandy@gmail.com
 *		John Scipione, jscipione@gmail.com
 */


/*! Decorator resembling Windows 95 */


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
WinDecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
{
	return new (std::nothrow)WinDecorator(settings, rect, desktop);
}


WinDecorator::WinDecorator(DesktopSettings& settings, BRect frame,
	Desktop* desktop)
	:
	SATDecorator(settings, frame, desktop),
	// common colors to both focus and non focus state
	fFrameHighColor((rgb_color){ 255, 255, 255, 255 }),
	fFrameMidColor((rgb_color){ 216, 216, 216, 255 }),
	fFrameLowColor((rgb_color){ 110, 110, 110, 255 }),
	fFrameLowerColor((rgb_color){ 0, 0, 0, 255 }),

	// state based colors
	fFocusTabColor(settings.UIColor(B_WINDOW_TAB_COLOR)),
	fFocusTextColor(settings.UIColor(B_WINDOW_TEXT_COLOR)),
	fNonFocusTabColor(settings.UIColor(B_WINDOW_INACTIVE_TAB_COLOR)),
	fNonFocusTextColor(settings.UIColor(B_WINDOW_INACTIVE_TEXT_COLOR))
{
	STRACE(("WinDecorator:\n"));
	STRACE(("\tFrame (%.1f,%.1f,%.1f,%.1f)\n",
		frame.left, frame.top, frame.right, frame.bottom));
}


WinDecorator::~WinDecorator()
{
	STRACE(("~WinDecorator()\n"));
}


/*!	\brief Updates the decorator in the rectangular area \a updateRect.

	Updates all areas which intersect the frame and tab.

	\param updateRect The rectangular area to update.
*/
void
WinDecorator::Draw(BRect updateRect)
{
	STRACE(("WinDecorator::Draw(BRect updateRect): "));
	updateRect.PrintToStream();

	// We need to draw a few things: the tab, the resize knob, the borders,
	// and the buttons
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(updateRect & fBorderRect);
	_DrawTabs(updateRect & fTitleBarRect);
}


//! Forces a complete decorator update
void
WinDecorator::Draw()
{
	STRACE(("WinDecorator: Draw()"));

	// Easy way to draw everything - no worries about drawing only certain
	// things
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(fBorderRect);
	_DrawTabs(fTitleBarRect);
}


// TODO : add GetSizeLimits


Decorator::Region
WinDecorator::RegionAt(BPoint where, int32& tabIndex) const
{
	tabIndex = -1;

	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		if (tab->minimizeRect.Contains(where)) {
			tabIndex = i;
			return REGION_MINIMIZE_BUTTON;
		}
	}
	// Let the base class version identify hits of the buttons and the tab.
	Region region = Decorator::RegionAt(where, tabIndex);
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


bool
WinDecorator::SetRegionHighlight(Region region, uint8 highlight,
	BRegion* dirty, int32 tabIndex)
{
	Decorator::Tab* tab
		= static_cast<Decorator::Tab*>(_TabAt(tabIndex));
	if (tab != NULL) {
		tab->isHighlighted = highlight != 0;
		// Invalidate the bitmap caches for the close/minimize/zoom button
		// when the highlight changes.
		switch (region) {
			case REGION_CLOSE_BUTTON:
				if (highlight != RegionHighlight(region))
					memset(&tab->closeBitmaps, 0, sizeof(tab->closeBitmaps));
				break;
			case REGION_MINIMIZE_BUTTON:
				if (highlight != RegionHighlight(region)) {
					memset(&tab->minimizeBitmaps, 0,
						sizeof(tab->minimizeBitmaps));
				}
				break;
			case REGION_ZOOM_BUTTON:
				if (highlight != RegionHighlight(region))
					memset(&tab->zoomBitmaps, 0, sizeof(tab->zoomBitmaps));
				break;
			default:
				break;
		}
	}

	return Decorator::SetRegionHighlight(region, highlight, dirty, tabIndex);
}


void
WinDecorator::_DoLayout()
{
	STRACE(("WinDecorator()::_DoLayout()\n"));

	bool hasTab = false;

	fBorderRect = fFrame;
	switch ((int)fTopTab->look) {
		case B_MODAL_WINDOW_LOOK:
			fBorderWidth = 4;
			break;

		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
			hasTab = true;
			fBorderWidth = 4;
			break;
		case B_FLOATING_WINDOW_LOOK:
			fBorderWidth = 0;
			hasTab = true;
			break;

		case B_BORDERED_WINDOW_LOOK:
			fBorderWidth = 1;
			break;

		default:
			fBorderWidth = 0;
			break;
	}

	fBorderRect.InsetBy(-fBorderWidth, -fBorderWidth);

	if (hasTab) {
		font_height fontHeight;
		fDrawState.Font().GetHeight(fontHeight);

		float tabSize = ceilf(fontHeight.ascent + fontHeight.descent + 4.0);

		if (tabSize < 20)
			tabSize = 20;
		fBorderRect.top -= tabSize;

		fTitleBarRect.Set(fFrame.left - 1,
			fFrame.top - tabSize,
			((fFrame.right - fFrame.left) < 32.0 ?
				fFrame.left + 32.0 : fFrame.right) + 1,
			fFrame.top - 1);

		for (int32 i = 0; i < fTabList.CountItems(); i++) {
			Decorator::Tab* tab = fTabList.ItemAt(i);

			tab->tabRect = fTitleBarRect;

			const float buttonsInset = 3;

			tab->zoomRect = tab->tabRect;
			tab->zoomRect.top += buttonsInset;
			tab->zoomRect.right -= buttonsInset;
			tab->zoomRect.bottom -= buttonsInset;
			tab->zoomRect.left = tab->zoomRect.right - tabSize + buttonsInset;

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


void
WinDecorator::_DrawFrame(BRect rect)
{
	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK)
		return;

	if (fBorderRect == fFrame)
		return;

	BRect r = fBorderRect;

	fDrawingEngine->SetHighColor(fFrameLowerColor);
	fDrawingEngine->StrokeRect(r);

	if (fTopTab->look == B_BORDERED_WINDOW_LOOK)
		return;

	BPoint pt;

	pt = r.RightTop();
	pt.x--;
	fDrawingEngine->StrokeLine(r.LeftTop(), pt, fFrameMidColor);
	pt = r.LeftBottom();
	pt.y--;
	fDrawingEngine->StrokeLine(r.LeftTop(), pt, fFrameMidColor);

	fDrawingEngine->StrokeLine(r.RightTop(), r.RightBottom(), fFrameLowerColor);
	fDrawingEngine->StrokeLine(r.LeftBottom(), r.RightBottom(), fFrameLowerColor);

	r.InsetBy(1, 1);
	pt = r.RightTop();
	pt.x--;
	fDrawingEngine->StrokeLine(r.LeftTop(),pt,fFrameHighColor);
	pt = r.LeftBottom();
	pt.y--;
	fDrawingEngine->StrokeLine(r.LeftTop(),pt,fFrameHighColor);

	fDrawingEngine->StrokeLine(r.RightTop(), r.RightBottom(), fFrameLowColor);
	fDrawingEngine->StrokeLine(r.LeftBottom(), r.RightBottom(), fFrameLowColor);

	r.InsetBy(1, 1);
	fDrawingEngine->StrokeRect(r, fFrameMidColor);
	r.InsetBy(1, 1);
	fDrawingEngine->StrokeRect(r, fFrameMidColor);
}


/*!	\brief Actually draws the tab

	This function is called when the tab itself needs drawn. Other items,
	like the window title or buttons, should not be drawn here.

	\param tab The \a tab to update.
	\param rect The area of the \a tab to update.
*/
void
WinDecorator::_DrawTab(Decorator::Tab* tab, BRect rect)
{
	const BRect& tabRect = tab->tabRect;

	// If a window has a tab, this will draw it and any buttons in it.
	if (!tabRect.IsValid() || !rect.Intersects(tabRect)
		|| fTopTab->look == B_NO_BORDER_WINDOW_LOOK) {
		return;
	}

	fDrawingEngine->FillRect(tabRect & rect, fTabColor);

	_DrawTitle(tab, tabRect);

	// Draw the buttons if we're supposed to
	// TODO : we should still draw the buttons if they are disabled, but grey them out
	_DrawButtons(tab, rect);
}


/*!	\brief Actually draws the title

	The main tasks for this function are to ensure that the decorator draws
	the title only in its own area and drawing the title itself.
	Using B_OP_COPY for drawing the title is recommended because of the marked
	performance hit of the other drawing modes, but it is not a requirement.

	\param tab The \a tab to update.
	\param rect area of the title to update.
*/
void
WinDecorator::_DrawTitle(Decorator::Tab* tab, BRect rect)
{
	const BRect& tabRect = tab->tabRect;
	const BRect& minimizeRect = tab->minimizeRect;
	const BRect& zoomRect = tab->zoomRect;
	const BRect& closeRect = tab->closeRect;

	fDrawingEngine->SetHighColor(fTextColor);
	fDrawingEngine->SetLowColor(IsFocus(tab)
		? fFocusTabColor : fNonFocusTabColor);

	tab->truncatedTitle = Title(tab);
	fDrawState.Font().TruncateString(&tab->truncatedTitle, B_TRUNCATE_END,
		((minimizeRect.IsValid() ? minimizeRect.left :
			zoomRect.IsValid() ? zoomRect.left :
			closeRect.IsValid() ? closeRect.left : tabRect.right) - 5)
		- (tabRect.left + 5));
	tab->truncatedTitleLength = tab->truncatedTitle.Length();
	fDrawingEngine->SetFont(fDrawState.Font());

	font_height fontHeight;
	fDrawState.Font().GetHeight(fontHeight);

	BPoint titlePos;
	titlePos.x = tabRect.left + 5;
	titlePos.y = floorf(((tabRect.top + 2.0) + tabRect.bottom
			+ fontHeight.ascent + fontHeight.descent) / 2.0
			- fontHeight.descent + 0.5);

	fDrawingEngine->DrawString(tab->truncatedTitle, tab->truncatedTitleLength,
		titlePos);
}


void
WinDecorator::_DrawButtons(Decorator::Tab* tab, const BRect& invalid)
{
	if ((tab->flags & B_NOT_MINIMIZABLE) == 0
		&& invalid.Intersects(tab->minimizeRect)) {
		_DrawMinimize(tab, false, tab->minimizeRect);
	}
	if ((tab->flags & B_NOT_ZOOMABLE) == 0
		&& invalid.Intersects(tab->zoomRect)) {
		_DrawZoom(tab, false, tab->zoomRect);
	}
	if ((tab->flags & B_NOT_CLOSABLE) == 0
		&& invalid.Intersects(tab->closeRect)) {
		_DrawClose(tab, false, tab->closeRect);
	}
}


/*!
	\brief Actually draws the minimize button

	Unless a subclass has a particularly large button, it is probably
	unnecessary to check the update rectangle.

	\param tab The \a tab to update.
	\param direct Draw without double buffering.
	\param rect The area of the button to update.
*/
void
WinDecorator::_DrawMinimize(Decorator::Tab* tab, bool direct, BRect rect)
{
	// Just like DrawZoom, but for a Minimize button
	_DrawBeveledRect(rect, tab->minimizePressed);

	fDrawingEngine->SetHighColor(fTextColor);
	BRect minimizeBox(rect.left + 5, rect.bottom - 4, rect.right - 5,
		rect.bottom - 3);
	if (true)
		minimizeBox.OffsetBy(1, 1);

	fDrawingEngine->SetHighColor(RGBColor(0, 0, 0));
	fDrawingEngine->StrokeRect(minimizeBox);
}


/*!	\brief Actually draws the zoom button

	Unless a subclass has a particularly large button, it is probably
	unnecessary to check the update rectangle.

	\param tab The \a tab to update.
	\param direct Draw without double buffering.
	\param rect The area of the button to update.
*/
void
WinDecorator::_DrawZoom(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawBeveledRect(rect, tab->zoomPressed);

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


/*!	\brief Actually draws the close button

	Unless a subclass has a particularly large button, it is probably
	unnecessary to check the update rectangle.

	\param tab The \a tab to update.
	\param direct Draw without double buffering.
	\param rect The area of the button to update.
*/
void
WinDecorator::_DrawClose(Decorator::Tab* tab, bool direct, BRect rect)
{
	STRACE(("_DrawClose(%f, %f, %f, %f)\n", rect.left, rect.top, rect.right,
		rect.bottom));

	// Just like DrawZoom, but for a close button
	_DrawBeveledRect(rect, tab->closePressed);

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
WinDecorator::_SetFocus(Decorator::Tab* tab)
{
	// TODO stub

	// SetFocus() performs necessary duties for color swapping and
	// other things when a window is deactivated or activated.

	if (IsFocus(tab)) {
		fTabColor = fFocusTabColor;
		fTextColor = fFocusTextColor;
	} else {
		fTabColor = fNonFocusTabColor;
		fTextColor = fNonFocusTextColor;
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
	fTitleBarRect.OffsetBy(offset);
	fResizeRect.OffsetBy(offset);
	fBorderRect.OffsetBy(offset);
}


void
WinDecorator::_ResizeBy(BPoint offset, BRegion* dirty)
{
	// Move all internal rectangles the appropriate amount
	fFrame.right += offset.x;
	fFrame.bottom += offset.y;
	fTitleBarRect.right += offset.x;
	fTitleBarRect.bottom += offset.y;
	fResizeRect.OffsetBy(offset);
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


Decorator::Tab*
WinDecorator::_AllocateNewTab()
{
	Decorator::Tab* tab = new(std::nothrow) Decorator::Tab;
	if (tab == NULL)
		return NULL;

	// Set appropriate colors based on the current focus value. In this case,
	// each decorator defaults to not having the focus.
	_SetFocus(tab);
	return tab;
}


bool
WinDecorator::_AddTab(DesktopSettings& settings, int32 index,
	BRegion* updateRegion)
{
	_UpdateFont(settings);

	_DoLayout();
	if (updateRegion != NULL)
		updateRegion->Include(fTitleBarRect);

	return true;
}


bool
WinDecorator::_RemoveTab(int32 index, BRegion* updateRegion)
{
	BRect oldTitle = fTitleBarRect;
	_DoLayout();
	if (updateRegion != NULL) {
		updateRegion->Include(oldTitle);
		updateRegion->Include(fTitleBarRect);
	}
	return true;
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
	fDrawingEngine->SetHighColor(high);
	// Top inside highlight
	fDrawingEngine->StrokeLine(rect.LeftTop(), rect.RightTop());

	// Left inside highlight
	fDrawingEngine->StrokeLine(rect.LeftTop(), rect.LeftBottom());

	// Right inside shading
	point = rect.RightTop();
	point.y++;
	fDrawingEngine->StrokeLine(point, rect.RightBottom(), low);

	// Bottom inside shading
	point = rect.LeftBottom();
	point.x++;
	fDrawingEngine->StrokeLine(point, rect.RightBottom(), low);

	rect.InsetBy(1,1);

	fDrawingEngine->FillRect(rect, mid);
}


// #pragma mark - DecorAddOn


extern "C" DecorAddOn*
instantiate_decor_addon(image_id id, const char* name)
{
	return new (std::nothrow)WinDecorAddOn(id, name);
}
