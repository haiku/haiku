/*
 * Copyright 2009-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm, bpmagic@columbus.rr.com
 *		Adrien Destugues, pulkomandy@gmail.com
 *		John Scipione, jscipione@gmail.com
 */


/*! Decorator resembling Mac OS 8 and 9 */


#include "MacDecorator.h"

#include <new>
#include <stdio.h>

#include <GradientLinear.h>
#include <Point.h>
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


MacDecorAddOn::MacDecorAddOn(image_id id, const char* name)
	:
	DecorAddOn(id, name)
{
}


Decorator*
MacDecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
{
	return new (std::nothrow)MacDecorator(settings, rect, desktop);
}


MacDecorator::MacDecorator(DesktopSettings& settings, BRect frame,
	Desktop* desktop)
	:
	SATDecorator(settings, frame, desktop),
	fButtonHighColor((rgb_color) { 232, 232, 232, 255 }),
	fButtonLowColor((rgb_color) { 128, 128, 128, 255 }),
	fFrameHighColor((rgb_color) { 255, 255, 255, 255 }),
	fFrameMidColor((rgb_color) { 216, 216, 216, 255 }),
	fFrameLowColor((rgb_color) { 156, 156, 156, 255 }),
	fFrameLowerColor((rgb_color) { 0, 0, 0, 255 }),
	fFocusTextColor(settings.UIColor(B_WINDOW_TEXT_COLOR)),
	fNonFocusTextColor(settings.UIColor(B_WINDOW_INACTIVE_TEXT_COLOR))
{
	STRACE(("MacDecorator()\n"));
	STRACE(("\tFrame (%.1f,%.1f,%.1f,%.1f)\n",
		frame.left, frame.top, frame.right, frame.bottom));
}


MacDecorator::~MacDecorator()
{
	STRACE(("~MacDecorator()\n"));
}


// TODO : Add GetSettings


void
MacDecorator::Draw(BRect updateRect)
{
	STRACE(("MacDecorator: Draw(BRect updateRect): "));
	updateRect.PrintToStream();

	// We need to draw a few things: the tab, the borders,
	// and the buttons
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(updateRect & fBorderRect);
	_DrawTabs(updateRect & fTitleBarRect);
}


void
MacDecorator::Draw()
{
	STRACE("MacDecorator::Draw()\n");
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(fBorderRect);
	_DrawTabs(fTitleBarRect);
}


// TODO : add GetSizeLimits


Decorator::Region
MacDecorator::RegionAt(BPoint where, int32& tab) const
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
			// TODO: Determine the actual border!
	}

	return REGION_NONE;
}


bool
MacDecorator::SetRegionHighlight(Region region, uint8 highlight,
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
MacDecorator::_DoLayout()
{
	STRACE(("MacDecorator: Do Layout\n"));

	// Here we determine the size of every rectangle that we use
	// internally when we are given the size of the client rectangle.

	const int32 kDefaultBorderWidth = 6;

	bool hasTab = false;

	if (fTopTab) {
		switch (fTopTab->look) {
			case B_MODAL_WINDOW_LOOK:
				fBorderWidth = kDefaultBorderWidth;
				break;

			case B_TITLED_WINDOW_LOOK:
			case B_DOCUMENT_WINDOW_LOOK:
				hasTab = true;
				fBorderWidth = kDefaultBorderWidth;
				break;

			case B_FLOATING_WINDOW_LOOK:
				hasTab = true;
				fBorderWidth = 3;
				break;

			case B_BORDERED_WINDOW_LOOK:
				fBorderWidth = 1;
				break;

			default:
				fBorderWidth = 0;
		}
	} else
		fBorderWidth = 0;

	fBorderRect = fFrame.InsetByCopy(-fBorderWidth, -fBorderWidth);

	// calculate our tab rect
	if (hasTab) {
		fBorderRect.top += 3;

		font_height fontHeight;
		fDrawState.Font().GetHeight(fontHeight);

		// TODO the tab is drawn in a fixed height for now
		fTitleBarRect.Set(fFrame.left - fBorderWidth,
			fFrame.top - 22,
			((fFrame.right - fFrame.left) < 32.0 ?
				fFrame.left + 32.0 : fFrame.right) + fBorderWidth,
			fFrame.top - 3);

		for (int32 i = 0; i < fTabList.CountItems(); i++) {
			Decorator::Tab* tab = fTabList.ItemAt(i);

			tab->tabRect = fTitleBarRect;
				// TODO actually handle multiple tabs

			tab->zoomRect = fTitleBarRect;
			tab->zoomRect.left = tab->zoomRect.right - 12;
			tab->zoomRect.bottom = tab->zoomRect.top + 12;
			tab->zoomRect.OffsetBy(-4, 4);

			tab->closeRect = tab->zoomRect;
			tab->minimizeRect = tab->zoomRect;

			tab->closeRect.OffsetTo(fTitleBarRect.left + 4,
				fTitleBarRect.top + 4);

			tab->zoomRect.OffsetBy(0 - (tab->zoomRect.Width() + 4), 0);
			if (Title(tab) != NULL && fDrawingEngine != NULL) {
				tab->truncatedTitle = Title(tab);
				fDrawingEngine->SetFont(fDrawState.Font());
				tab->truncatedTitleLength
					= (int32)fDrawingEngine->StringWidth(Title(tab),
						strlen(Title(tab)));

				if (tab->truncatedTitleLength < (tab->zoomRect.left
						- tab->closeRect.right - 10)) {
					// start with offset from closerect.right
					tab->textOffset = int(((tab->zoomRect.left - 5)
						- (tab->closeRect.right + 5)) / 2);
					tab->textOffset -= int(tab->truncatedTitleLength / 2);

					// now make it the offset from fTabRect.left
					tab->textOffset += int(tab->closeRect.right + 5
						- fTitleBarRect.left);
				} else
					tab->textOffset = int(tab->closeRect.right) + 5;
			} else
				tab->textOffset = 0;
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
MacDecorator::_DrawFrame(BRect invalid)
{
	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK)
		return;

	if (fBorderWidth <= 0)
		return;

	BRect r = fBorderRect;
	switch (fTopTab->look) {
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
		case B_MODAL_WINDOW_LOOK:
		{
			if (IsFocus(fTopTab)) {
				BPoint offset = r.LeftTop();
				BPoint pt2 = r.LeftBottom();

				// Draw the left side of the frame
				fDrawingEngine->StrokeLine(offset, pt2, fFrameLowerColor);
				offset.x++;
				pt2.x++;
				pt2.y--;

				fDrawingEngine->StrokeLine(offset, pt2, fFrameHighColor);
				offset.x++;
				pt2.x++;
				pt2.y--;

				fDrawingEngine->StrokeLine(offset, pt2, fFrameMidColor);
				offset.x++;
				pt2.x++;
				fDrawingEngine->StrokeLine(offset, pt2, fFrameMidColor);
				offset.x++;
				pt2.x++;
				pt2.y--;

				fDrawingEngine->StrokeLine(offset, pt2, fFrameLowColor);
				offset.x++;
				offset.y += 2;
				BPoint topleftpt = offset;
				pt2.x++;
				pt2.y--;

				fDrawingEngine->StrokeLine(offset, pt2, fFrameLowerColor);

				offset = r.RightTop();
				pt2 = r.RightBottom();

				// Draw the right side of the frame
				fDrawingEngine->StrokeLine(offset, pt2, fFrameLowerColor);
				offset.x--;
				pt2.x--;

				fDrawingEngine->StrokeLine(offset, pt2, fFrameLowColor);
				offset.x--;
				pt2.x--;

				fDrawingEngine->StrokeLine(offset, pt2, fFrameMidColor);
				offset.x--;
				pt2.x--;
				fDrawingEngine->StrokeLine(offset, pt2, fFrameMidColor);
				offset.x--;
				pt2.x--;

				fDrawingEngine->StrokeLine(offset, pt2, fFrameHighColor);
				offset.x--;
				offset.y += 2;
				BPoint toprightpt = offset;
				pt2.x--;

				fDrawingEngine->StrokeLine(offset, pt2, fFrameLowerColor);

				// Draw the top side of the frame that is not in the tab
				if (fTopTab->look == B_MODAL_WINDOW_LOOK) {
					offset = r.LeftTop();
					pt2 = r.RightTop();

					fDrawingEngine->StrokeLine(offset, pt2, fFrameLowerColor);
					offset.x++;
					offset.y++;
					pt2.x--;
					pt2.y++;

					fDrawingEngine->StrokeLine(offset, pt2, fFrameHighColor);
					offset.x++;
					offset.y++;
					pt2.x--;
					pt2.y++;

					fDrawingEngine->StrokeLine(offset, pt2, fFrameMidColor);
					offset.x++;
					offset.y++;
					pt2.x--;
					pt2.y++;

					fDrawingEngine->StrokeLine(offset, pt2, fFrameMidColor);
					offset.x++;
					offset.y++;
					pt2.x--;
					pt2.y++;

					fDrawingEngine->StrokeLine(offset, pt2, fFrameLowColor);
					offset.x++;
					offset.y++;
					pt2.x--;
					pt2.y++;

					fDrawingEngine->StrokeLine(offset, pt2, fFrameLowerColor);
				} else {
					// Some odd stuff here where the title bar is melded into the
					// window border so that the sides are drawn into the title
					// so we draw this bottom up 
					offset = topleftpt;
					pt2 = toprightpt;

					fDrawingEngine->StrokeLine(offset, pt2, fFrameLowerColor);
					offset.y--;
					offset.x++;
					pt2.y--;

					fDrawingEngine->StrokeLine(offset, pt2, fFrameLowColor);
				}

				// Draw the bottom side of the frame
				offset = r.LeftBottom();
				pt2 = r.RightBottom();

				fDrawingEngine->StrokeLine(offset, pt2, fFrameLowerColor);
				offset.x++;
				offset.y--;
				pt2.x--;
				pt2.y--;

				fDrawingEngine->StrokeLine(offset, pt2, fFrameLowColor);
				offset.x++;
				offset.y--;
				pt2.x--;
				pt2.y--;

				fDrawingEngine->StrokeLine(offset, pt2, fFrameMidColor);
				offset.x++;
				offset.y--;
				pt2.x--;
				pt2.y--;

				fDrawingEngine->StrokeLine(offset, pt2, fFrameMidColor);
				offset.x++;
				offset.y--;
				pt2.x--;
				pt2.y--;

				fDrawingEngine->StrokeLine(offset, pt2, fFrameHighColor);
				offset.x += 2;
				offset.y--;
				pt2.x--;
				pt2.y--;

				fDrawingEngine->StrokeLine(offset, pt2, fFrameLowerColor);
				offset.y--;
				pt2.x--;
				pt2.y--;
			} else {
				r.top -= 3;
				RGBColor inactive(82, 82, 82);

				fDrawingEngine->StrokeLine(r.LeftTop(), r.LeftBottom(),
					inactive);
				fDrawingEngine->StrokeLine(r.RightTop(), r.RightBottom(),
					inactive);
				fDrawingEngine->StrokeLine(r.LeftBottom(), r.RightBottom(),
					inactive);

				for (int i = 0; i < 4; i++) {
					r.InsetBy(1, 1);
					fDrawingEngine->StrokeLine(r.LeftTop(), r.LeftBottom(),
						fFrameMidColor);
					fDrawingEngine->StrokeLine(r.RightTop(), r.RightBottom(),
						fFrameMidColor);
					fDrawingEngine->StrokeLine(r.LeftBottom(), r.RightBottom(),
						fFrameMidColor);
					fDrawingEngine->StrokeLine(r.LeftTop(), r.RightTop(),
						fFrameMidColor);
				}

				r.InsetBy(1, 1);
				fDrawingEngine->StrokeLine(r.LeftTop(), r.LeftBottom(),
					inactive);
				fDrawingEngine->StrokeLine(r.RightTop(), r.RightBottom(),
					inactive);
				fDrawingEngine->StrokeLine(r.LeftBottom(), r.RightBottom(),
					inactive);
				fDrawingEngine->StrokeLine(r.LeftTop(), r.RightTop(),
					inactive);
			}
			break;
		}
		case B_BORDERED_WINDOW_LOOK:
			fDrawingEngine->StrokeRect(r, fFrameMidColor);
			break;

		default:
			// don't draw a border frame
		break;
	}
}


void
MacDecorator::_DrawTab(Decorator::Tab* tab, BRect invalid)
{
	// If a window has a tab, this will draw it and any buttons which are
	// in it.
	if (!tab->tabRect.IsValid() || !invalid.Intersects(tab->tabRect))
		return;

	BRect rect(tab->tabRect);
	fDrawingEngine->SetHighColor(RGBColor(fFrameMidColor));
	fDrawingEngine->FillRect(rect, fFrameMidColor);

	if (IsFocus(tab)) {
		fDrawingEngine->StrokeLine(rect.LeftTop(), rect.RightTop(),
			fFrameLowerColor);
		fDrawingEngine->StrokeLine(rect.LeftTop(), rect.LeftBottom(),
			fFrameLowerColor);
		fDrawingEngine->StrokeLine(rect.RightBottom(), rect.RightTop(),
			fFrameLowerColor);

		rect.InsetBy(1, 1);
		rect.bottom++;

		fDrawingEngine->StrokeLine(rect.LeftTop(), rect.RightTop(),
			fFrameHighColor);
		fDrawingEngine->StrokeLine(rect.LeftTop(), rect.LeftBottom(),
			fFrameHighColor);
		fDrawingEngine->StrokeLine(rect.RightBottom(), rect.RightTop(),
			fFrameLowColor);

		// Draw the neat little lines on either side of the title if there's
		// room
		float left;
		if ((tab->flags & B_NOT_CLOSABLE) == 0)
			left = tab->closeRect.right;
		else
			left = tab->tabRect.left;

		float right;
		if ((tab->flags & B_NOT_ZOOMABLE) == 0)
			right = tab->zoomRect.left;
		else if ((tab->flags & B_NOT_MINIMIZABLE) == 0)
			right = tab->minimizeRect.left;
		else
			right = tab->tabRect.right;

		if (tab->tabRect.left + tab->textOffset > left + 5) {
			RGBColor dark(115, 115, 115);

			// Left side

			BPoint offset(left + 5, tab->closeRect.top);
			BPoint pt2(tab->tabRect.left + tab->textOffset - 5,
				tab->closeRect.top);

			fDrawState.SetHighColor(RGBColor(fFrameHighColor));
			for (int32 i = 0; i < 6; i++) {
				fDrawingEngine->StrokeLine(offset, pt2,
					fDrawState.HighColor());
				offset.y += 2;
				pt2.y += 2;
			}

			offset.Set(left + 6, tab->closeRect.top + 1);
			pt2.Set(tab->tabRect.left + tab->textOffset - 4,
				tab->closeRect.top + 1);

			fDrawState.SetHighColor(dark);
			for (int32 i = 0; i < 6; i++) {
				fDrawingEngine->StrokeLine(offset, pt2,
					fDrawState.HighColor());
				offset.y += 2;
				pt2.y += 2;
			}

			// Right side

			offset.Set(tab->tabRect.left + tab->textOffset
				+ tab->truncatedTitleLength + 3, tab->zoomRect.top);
			pt2.Set(right - 8, tab->zoomRect.top);

			if (offset.x < pt2.x) {
				fDrawState.SetHighColor(RGBColor(fFrameHighColor));
				for (int32 i = 0; i < 6; i++) {
					fDrawingEngine->StrokeLine(offset, pt2,
						fDrawState.HighColor());
					offset.y += 2;
					pt2.y += 2;
				}

				offset.Set(tab->tabRect.left + tab->textOffset
					+ tab->truncatedTitleLength + 4, tab->zoomRect.top + 1);
				pt2.Set(right - 7, tab->zoomRect.top + 1);

				fDrawState.SetHighColor(dark);
				for(int32 i = 0; i < 6; i++) {
					fDrawingEngine->StrokeLine(offset, pt2,
						fDrawState.HighColor());
					offset.y += 2;
					pt2.y += 2;
				}
			}
		}

		_DrawButtons(tab, rect);
	} else {
		RGBColor inactive(82, 82, 82);
		// Not focused - Just draw a plain light grey area with the title
		// in the middle
		fDrawingEngine->StrokeLine(rect.LeftTop(), rect.RightTop(),
			inactive);
		fDrawingEngine->StrokeLine(rect.LeftTop(), rect.LeftBottom(),
			inactive);
		fDrawingEngine->StrokeLine(rect.RightBottom(), rect.RightTop(),
			inactive);
	}

	_DrawTitle(tab, tab->tabRect);
}


void
MacDecorator::_DrawButtons(Decorator::Tab* tab, const BRect& invalid)
{
	if ((tab->flags & B_NOT_CLOSABLE) == 0
		&& invalid.Intersects(tab->closeRect)) {
		_DrawClose(tab, false, tab->closeRect);
	}
	if ((tab->flags & B_NOT_MINIMIZABLE) == 0
		&& invalid.Intersects(tab->minimizeRect)) {
		_DrawMinimize(tab, false, tab->minimizeRect);
	}
	if ((tab->flags & B_NOT_ZOOMABLE) == 0
		&& invalid.Intersects(tab->zoomRect)) {
		_DrawZoom(tab, false, tab->zoomRect);
	}
}


void
MacDecorator::_DrawTitle(Decorator::Tab* tab, BRect rect)
{
	fDrawingEngine->SetHighColor(IsFocus(tab)
		? fFocusTextColor : fNonFocusTextColor);

	fDrawingEngine->SetLowColor(fFrameMidColor);

	tab->truncatedTitle = Title(tab);
	fDrawState.Font().TruncateString(&tab->truncatedTitle, B_TRUNCATE_END,
		(tab->zoomRect.left - 5) - (tab->closeRect.right + 5));
	fDrawingEngine->SetFont(fDrawState.Font());

	fDrawingEngine->DrawString(tab->truncatedTitle, tab->truncatedTitle.Length(),
		BPoint(fTitleBarRect.left + tab->textOffset,
			tab->closeRect.bottom - 1));
}


void
MacDecorator::_DrawClose(Decorator::Tab* tab, bool direct, BRect r)
{
	_DrawButton(tab, direct, r, tab->closePressed);
}


void
MacDecorator::_DrawZoom(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawButton(tab, direct, rect, tab->zoomPressed);

	rect.top++;
	rect.left++;
	rect.bottom = rect.top + 6;
	rect.right = rect.left + 6;

	fDrawState.SetHighColor(RGBColor(33, 33, 33));
	fDrawingEngine->StrokeRect(rect, fDrawState.HighColor());
}


void
MacDecorator::_DrawMinimize(Decorator::Tab* tab, bool direct, BRect rect)
{
	_DrawButton(tab, direct, rect, tab->minimizePressed);

	rect.InsetBy(1, 5);

	fDrawState.SetHighColor(RGBColor(33, 33, 33));
	fDrawingEngine->StrokeRect(rect, fDrawState.HighColor());
}


void
MacDecorator::_SetTitle(Tab* tab, const char* string, BRegion* updateRegion)
{
	// TODO: we could be much smarter about the update region
	// TODO may this change the other tabs too ? (to make space for a longer
	// title ?)

	BRect rect = TabRect(tab);

	_DoLayout();

	if (updateRegion == NULL)
		return;

	rect = rect | TabRect(tab);

	rect.bottom++;
		// the border will look differently when the title is adjacent

	updateRegion->Include(rect);
}


// TODO : _SetFocus


void
MacDecorator::_MoveBy(BPoint offset)
{
	// Move all internal rectangles the appropriate amount
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		tab->zoomRect.OffsetBy(offset);
		tab->minimizeRect.OffsetBy(offset);
		tab->closeRect.OffsetBy(offset);
		tab->tabRect.OffsetBy(offset);
	}

	fFrame.OffsetBy(offset);
	fTitleBarRect.OffsetBy(offset);
	fResizeRect.OffsetBy(offset);
	fBorderRect.OffsetBy(offset);
}


void
MacDecorator::_ResizeBy(BPoint offset, BRegion* dirty)
{
	// Move all internal rectangles the appropriate amount
	fFrame.right += offset.x;
	fFrame.bottom += offset.y;

	fTitleBarRect.right += offset.x;
	fBorderRect.right += offset.x;
	fBorderRect.bottom += offset.y;
	// fZoomRect.OffsetBy(offset.x, 0);
	// fMinimizeRect.OffsetBy(offset.x, 0);
	if (dirty) {
		dirty->Include(fTitleBarRect);
		dirty->Include(fBorderRect);
	}

	// TODO probably some other layouting stuff here
	_DoLayout();
}


// TODO : _SetSettings


Decorator::Tab*
MacDecorator::_AllocateNewTab()
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
MacDecorator::_AddTab(DesktopSettings& settings, int32 index,
	BRegion* updateRegion)
{
	_UpdateFont(settings);

	_DoLayout();
	if (updateRegion != NULL)
		updateRegion->Include(fTitleBarRect);
	return true;
}


bool
MacDecorator::_RemoveTab(int32 index, BRegion* updateRegion)
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
MacDecorator::_MoveTab(int32 from, int32 to, bool isMoving,
	BRegion* updateRegion)
{
	return false;

#if 0
	MacDecorator::Tab* toTab = _TabAt(to);
	if (toTab == NULL)
		return false;

	if (from < to) {
		fOldMovingTab.OffsetBy(toTab->tabRect.Width(), 0);
		toTab->tabRect.OffsetBy(-fOldMovingTab.Width(), 0);
	} else {
		fOldMovingTab.OffsetBy(-toTab->tabRect.Width(), 0);
		toTab->tabRect.OffsetBy(fOldMovingTab.Width(), 0);
	}

	toTab->tabOffset = uint32(toTab->tabRect.left - fLeftBorder.left);
	_LayoutTabItems(toTab, toTab->tabRect);

	_CalculateTabsRegion();

	if (updateRegion != NULL)
		updateRegion->Include(fTitleBarRect);
	return true;
#endif
}


void
MacDecorator::_GetFootprint(BRegion* region)
{
	// This function calculates the decorator's footprint in coordinates
	// relative to the view. This is most often used to set a Window
	// object's visible region.
	if (!region)
		return;

	region->MakeEmpty();

	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK)
		return;

	region->Set(fBorderRect);
	region->Exclude(fFrame);

	if (fTopTab->look == B_BORDERED_WINDOW_LOOK)
		return;
	region->Include(fTitleBarRect);
}


void
MacDecorator::_UpdateFont(DesktopSettings& settings)
{
	ServerFont font;
	if (fTopTab && fTopTab->look == B_FLOATING_WINDOW_LOOK)
		settings.GetDefaultPlainFont(font);
	else
		settings.GetDefaultBoldFont(font);

	font.SetFlags(B_FORCE_ANTIALIASING);
	font.SetSpacing(B_STRING_SPACING);
	fDrawState.SetFont(font);
}


// #pragma mark - Private methods


// Draw a mac-style button
void
MacDecorator::_DrawButton(Decorator::Tab* tab, bool direct, BRect r,
	bool down)
{
	BRect rect(r);

	BPoint offset(r.LeftTop()), pt2(r.RightTop());

	// Topleft dark grey border
	pt2.x--;
	fDrawingEngine->SetHighColor(RGBColor(136, 136, 136));
	fDrawingEngine->StrokeLine(offset, pt2);

	pt2 = r.LeftBottom();
	pt2.y--;
	fDrawingEngine->StrokeLine(offset, pt2);

	// Bottomright white border
	offset = r.RightBottom();
	pt2 = r.RightTop();
	pt2.y++;
	fDrawingEngine->SetHighColor(RGBColor(255, 255, 255));
	fDrawingEngine->StrokeLine(offset, pt2);

	pt2 = r.LeftBottom();
	pt2.x++;
	fDrawingEngine->StrokeLine(offset, pt2);

	// Black outline
	rect.InsetBy(1, 1);
	fDrawingEngine->SetHighColor(RGBColor(33, 33, 33));
	fDrawingEngine->StrokeRect(rect);

	// Double-shaded button
	rect.InsetBy(1, 1);
	fDrawingEngine->SetHighColor(RGBColor(140, 140, 140));
	fDrawingEngine->StrokeLine(rect.RightBottom(), rect.RightTop());
	fDrawingEngine->StrokeLine(rect.RightBottom(), rect.LeftBottom());
	fDrawingEngine->SetHighColor(RGBColor(206, 206, 206));
	fDrawingEngine->StrokeLine(rect.LeftBottom(), rect.LeftTop());
	fDrawingEngine->StrokeLine(rect.LeftTop(), rect.RightTop());
	fDrawingEngine->SetHighColor(RGBColor(255, 255, 255));
	fDrawingEngine->StrokeLine(rect.LeftTop(), rect.LeftTop());

	rect.InsetBy(1, 1);
	_DrawBlendedRect(fDrawingEngine, rect, !down);
}


/*!	\brief Draws a rectangle with a gradient.
  \param down The rectangle should be drawn recessed or not
*/
void
MacDecorator::_DrawBlendedRect(DrawingEngine* engine, BRect rect,
	bool down/*, bool focus*/)
{
	// figure out which colors to use
	rgb_color startColor, endColor;
	if (down) {
		startColor = fButtonLowColor;
		endColor = fFrameHighColor;
	} else {
		startColor = fButtonHighColor;
		endColor = fFrameLowerColor;
	}

	// fill
	BGradientLinear gradient;
	gradient.SetStart(rect.LeftTop());
	gradient.SetEnd(rect.RightBottom());
	gradient.AddColor(startColor, 0);
	gradient.AddColor(endColor, 255);

	engine->FillRect(rect, gradient);
}


// #pragma mark - DecorAddOn


extern "C" DecorAddOn*
instantiate_decor_addon(image_id id, const char* name)
{
	return new (std::nothrow)MacDecorAddOn(id, name);
}
