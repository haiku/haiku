/*
 * Copyright 2001-2015 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		DarkWyrm, bpmagic@columbus.rr.com
 *		Ryan Leavengood, leavengood@gmail.com
 *		Philippe Saint-Pierre, stpere@gmail.com
 *		John Scipione, jscipione@gmail.com
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 *		Joseph Groover <looncraz@looncraz.net>
 */


/*!	Decorator made up of tabs */


#include "TabDecorator.h"

#include <algorithm>
#include <cmath>
#include <new>
#include <stdio.h>

#include <Autolock.h>
#include <Debug.h>
#include <GradientLinear.h>
#include <Rect.h>
#include <View.h>

#include <WindowPrivate.h>

#include "BitmapDrawingEngine.h"
#include "DesktopSettings.h"
#include "DrawingEngine.h"
#include "DrawState.h"
#include "FontManager.h"
#include "PatternHandler.h"


//#define DEBUG_DECORATOR
#ifdef DEBUG_DECORATOR
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif


static bool
int_equal(float x, float y)
{
	return abs(x - y) <= 1;
}


static const float kBorderResizeLength = 22.0;
static const float kResizeKnobSize = 18.0;


//	#pragma mark -


// TODO: get rid of DesktopSettings here, and introduce private accessor
//	methods to the Decorator base class
TabDecorator::TabDecorator(DesktopSettings& settings, BRect frame,
							Desktop* desktop)
	:
	Decorator(settings, frame, desktop),
	fOldMovingTab(0, 0, -1, -1)
{
	STRACE(("TabDecorator:\n"));
	STRACE(("\tFrame (%.1f,%.1f,%.1f,%.1f)\n",
		frame.left, frame.top, frame.right, frame.bottom));

	// TODO: If the decorator was created with a frame too small, it should
	// resize itself!
}


TabDecorator::~TabDecorator()
{
	STRACE(("TabDecorator: ~TabDecorator()\n"));
}


// #pragma mark - Public methods


/*!	\brief Updates the decorator in the rectangular area \a updateRect.

	Updates all areas which intersect the frame and tab.

	\param updateRect The rectangular area to update.
*/
void
TabDecorator::Draw(BRect updateRect)
{
	STRACE(("TabDecorator::Draw(BRect "
		"updateRect(l:%.1f, t:%.1f, r:%.1f, b:%.1f))\n",
		updateRect.left, updateRect.top, updateRect.right, updateRect.bottom));

	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(updateRect & fBorderRect);
	_DrawTabs(updateRect & fTitleBarRect);
}


//! Forces a complete decorator update
void
TabDecorator::Draw()
{
	STRACE(("TabDecorator: Draw()"));

	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(fBorderRect);
	_DrawTabs(fTitleBarRect);
}


Decorator::Region
TabDecorator::RegionAt(BPoint where, int32& tab) const
{
	// Let the base class version identify hits of the buttons and the tab.
	Region region = Decorator::RegionAt(where, tab);
	if (region != REGION_NONE)
		return region;

	// check the resize corner
	if (fTopTab->look == B_DOCUMENT_WINDOW_LOOK && fResizeRect.Contains(where))
		return REGION_RIGHT_BOTTOM_CORNER;

	// hit-test the borders
	if (fLeftBorder.Contains(where))
		return REGION_LEFT_BORDER;
	if (fTopBorder.Contains(where))
		return REGION_TOP_BORDER;

	// Part of the bottom and right borders may be a resize-region, so we have
	// to check explicitly, if it has been it.
	if (fRightBorder.Contains(where))
		region = REGION_RIGHT_BORDER;
	else if (fBottomBorder.Contains(where))
		region = REGION_BOTTOM_BORDER;
	else
		return REGION_NONE;

	// check resize area
	if ((fTopTab->flags & B_NOT_RESIZABLE) == 0
		&& (fTopTab->look == B_TITLED_WINDOW_LOOK
			|| fTopTab->look == B_FLOATING_WINDOW_LOOK
			|| fTopTab->look == B_MODAL_WINDOW_LOOK
			|| fTopTab->look == kLeftTitledWindowLook)) {
		BRect resizeRect(BPoint(fBottomBorder.right - kBorderResizeLength,
			fBottomBorder.bottom - kBorderResizeLength),
			fBottomBorder.RightBottom());
		if (resizeRect.Contains(where))
			return REGION_RIGHT_BOTTOM_CORNER;
	}

	return region;
}


bool
TabDecorator::SetRegionHighlight(Region region, uint8 highlight,
	BRegion* dirty, int32 tabIndex)
{
	Decorator::Tab* tab
		= static_cast<Decorator::Tab*>(_TabAt(tabIndex));
	if (tab != NULL) {
		tab->isHighlighted = highlight != 0;
		// Invalidate the bitmap caches for the close/zoom button, when the
		// highlight changes.
		switch (region) {
			case REGION_CLOSE_BUTTON:
				if (highlight != RegionHighlight(region))
					memset(&tab->closeBitmaps, 0, sizeof(tab->closeBitmaps));
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
TabDecorator::UpdateColors(DesktopSettings& settings)
{
	// Desktop is write locked, so be quick about it.
	fFocusFrameColor		= settings.UIColor(B_WINDOW_BORDER_COLOR);
	fFocusTabColor			= settings.UIColor(B_WINDOW_TAB_COLOR);
	fFocusTabColorLight		= tint_color(fFocusTabColor,
								(B_LIGHTEN_MAX_TINT + B_LIGHTEN_2_TINT) / 2);
	fFocusTabColorBevel		= tint_color(fFocusTabColor, B_LIGHTEN_2_TINT);
	fFocusTabColorShadow	= tint_color(fFocusTabColor,
								(B_DARKEN_1_TINT + B_NO_TINT) / 2);
	fFocusTextColor			= settings.UIColor(B_WINDOW_TEXT_COLOR);

	fNonFocusFrameColor		= settings.UIColor(B_WINDOW_INACTIVE_BORDER_COLOR);
	fNonFocusTabColor		= settings.UIColor(B_WINDOW_INACTIVE_TAB_COLOR);
	fNonFocusTabColorLight	= tint_color(fNonFocusTabColor,
								(B_LIGHTEN_MAX_TINT + B_LIGHTEN_2_TINT) / 2);
	fNonFocusTabColorBevel	= tint_color(fNonFocusTabColor, B_LIGHTEN_2_TINT);
	fNonFocusTabColorShadow	= tint_color(fNonFocusTabColor,
								(B_DARKEN_1_TINT + B_NO_TINT) / 2);
	fNonFocusTextColor = settings.UIColor(B_WINDOW_INACTIVE_TEXT_COLOR);
}


void
TabDecorator::_DoLayout()
{
	STRACE(("TabDecorator: Do Layout\n"));
	// Here we determine the size of every rectangle that we use
	// internally when we are given the size of the client rectangle.

	bool hasTab = false;

	switch ((int)fTopTab->look) {
		case B_MODAL_WINDOW_LOOK:
			fBorderWidth = 5;
			break;

		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
			hasTab = true;
			fBorderWidth = 5;
			break;
		case B_FLOATING_WINDOW_LOOK:
		case kLeftTitledWindowLook:
			hasTab = true;
			fBorderWidth = 3;
			break;

		case B_BORDERED_WINDOW_LOOK:
			fBorderWidth = 1;
			break;

		default:
			fBorderWidth = 0;
	}

	// calculate left/top/right/bottom borders
	if (fBorderWidth > 0) {
		// NOTE: no overlapping, the left and right border rects
		// don't include the corners!
		fLeftBorder.Set(fFrame.left - fBorderWidth, fFrame.top,
			fFrame.left - 1, fFrame.bottom);

		fRightBorder.Set(fFrame.right + 1, fFrame.top ,
			fFrame.right + fBorderWidth, fFrame.bottom);

		fTopBorder.Set(fFrame.left - fBorderWidth, fFrame.top - fBorderWidth,
			fFrame.right + fBorderWidth, fFrame.top - 1);

		fBottomBorder.Set(fFrame.left - fBorderWidth, fFrame.bottom + 1,
			fFrame.right + fBorderWidth, fFrame.bottom + fBorderWidth);
	} else {
		// no border
		fLeftBorder.Set(0.0, 0.0, -1.0, -1.0);
		fRightBorder.Set(0.0, 0.0, -1.0, -1.0);
		fTopBorder.Set(0.0, 0.0, -1.0, -1.0);
		fBottomBorder.Set(0.0, 0.0, -1.0, -1.0);
	}

	fBorderRect = BRect(fTopBorder.LeftTop(), fBottomBorder.RightBottom());

	// calculate resize rect
	if (fBorderWidth > 1) {
		fResizeRect.Set(fBottomBorder.right - kResizeKnobSize,
			fBottomBorder.bottom - kResizeKnobSize, fBottomBorder.right,
			fBottomBorder.bottom);
	} else {
		// no border or one pixel border (menus and such)
		fResizeRect.Set(0, 0, -1, -1);
	}

	if (hasTab) {
		_DoTabLayout();
		return;
	} else {
		// no tab
		for (int32 i = 0; i < fTabList.CountItems(); i++) {
			Decorator::Tab* tab = fTabList.ItemAt(i);
			tab->tabRect.Set(0.0, 0.0, -1.0, -1.0);
		}
		fTabsRegion.MakeEmpty();
		fTitleBarRect.Set(0.0, 0.0, -1.0, -1.0);
	}
}


void
TabDecorator::_DoTabLayout()
{
	float tabOffset = 0;
	if (fTabList.CountItems() == 1) {
		float tabSize;
		tabOffset = _SingleTabOffsetAndSize(tabSize);
	}

	float sumTabWidth = 0;
	// calculate our tab rect
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = _TabAt(i);

		BRect& tabRect = tab->tabRect;
		// distance from one item of the tab bar to another.
		// In this case the text and close/zoom rects
		tab->textOffset = _DefaultTextOffset();

		font_height fontHeight;
		fDrawState.Font().GetHeight(fontHeight);

		if (tab->look != kLeftTitledWindowLook) {
			tabRect.Set(fFrame.left - fBorderWidth,
				fFrame.top - fBorderWidth
					- ceilf(fontHeight.ascent + fontHeight.descent + 7.0),
				((fFrame.right - fFrame.left) < 35.0 ?
					fFrame.left + 35.0 : fFrame.right) + fBorderWidth,
				fFrame.top - fBorderWidth);
		} else {
			tabRect.Set(fFrame.left - fBorderWidth
				- ceilf(fontHeight.ascent + fontHeight.descent + 5.0),
					fFrame.top - fBorderWidth, fFrame.left - fBorderWidth,
				fFrame.bottom + fBorderWidth);
		}

		// format tab rect for a floating window - make the rect smaller
		if (tab->look == B_FLOATING_WINDOW_LOOK) {
			tabRect.InsetBy(0, 2);
			tabRect.OffsetBy(0, 2);
		}

		float offset;
		float size;
		float inset;
		_GetButtonSizeAndOffset(tabRect, &offset, &size, &inset);

		// tab->minTabSize contains just the room for the buttons
		tab->minTabSize = inset * 2 + tab->textOffset;
		if ((tab->flags & B_NOT_CLOSABLE) == 0)
			tab->minTabSize += offset + size;
		if ((tab->flags & B_NOT_ZOOMABLE) == 0)
			tab->minTabSize += offset + size;

		// tab->maxTabSize contains tab->minTabSize + the width required for the
		// title
		tab->maxTabSize = fDrawingEngine
			? ceilf(fDrawingEngine->StringWidth(Title(tab), strlen(Title(tab)),
				fDrawState.Font())) : 0.0;
		if (tab->maxTabSize > 0.0)
			tab->maxTabSize += tab->textOffset;
		tab->maxTabSize += tab->minTabSize;

		float tabSize = (tab->look != kLeftTitledWindowLook
			? fFrame.Width() : fFrame.Height()) + fBorderWidth * 2;
		if (tabSize < tab->minTabSize)
			tabSize = tab->minTabSize;
		if (tabSize > tab->maxTabSize)
			tabSize = tab->maxTabSize;

		// layout buttons and truncate text
		if (tab->look != kLeftTitledWindowLook)
			tabRect.right = tabRect.left + tabSize;
		else
			tabRect.bottom = tabRect.top + tabSize;

		// make sure fTabOffset is within limits and apply it to
		// the tabRect
		tab->tabOffset = (uint32)tabOffset;
		if (tab->tabLocation != 0.0 && fTabList.CountItems() == 1
			&& tab->tabOffset > (fRightBorder.right - fLeftBorder.left
				- tabRect.Width())) {
			tab->tabOffset = uint32(fRightBorder.right - fLeftBorder.left
				- tabRect.Width());
		}
		tabRect.OffsetBy(tab->tabOffset, 0);
		tabOffset += tabRect.Width();

		sumTabWidth += tabRect.Width();
	}

	float windowWidth = fFrame.Width() + 2 * fBorderWidth;
	if (CountTabs() > 1 && sumTabWidth > windowWidth)
		_DistributeTabSize(sumTabWidth - windowWidth);

	// finally, layout the buttons and text within the tab rect
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);

		if (i == 0)
			fTitleBarRect = tab->tabRect;
		else
			fTitleBarRect = fTitleBarRect | tab->tabRect;

		_LayoutTabItems(tab, tab->tabRect);
	}

	fTabsRegion = fTitleBarRect;
}


void
TabDecorator::_DistributeTabSize(float delta)
{
	int32 tabCount = fTabList.CountItems();
	ASSERT(tabCount > 1);

	float maxTabSize = 0;
	float secMaxTabSize = 0;
	int32 nTabsWithMaxSize = 0;
	for (int32 i = 0; i < tabCount; i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		if (tab == NULL)
			continue;

		float tabWidth = tab->tabRect.Width();
		if (int_equal(maxTabSize, tabWidth)) {
			nTabsWithMaxSize++;
			continue;
		}
		if (maxTabSize < tabWidth) {
			secMaxTabSize = maxTabSize;
			maxTabSize = tabWidth;
			nTabsWithMaxSize = 1;
		} else if (secMaxTabSize <= tabWidth)
			secMaxTabSize = tabWidth;
	}

	float minus = ceilf(std::min(maxTabSize - secMaxTabSize, delta));
	if (minus < 1.0)
		return;
	delta -= minus;
	minus /= nTabsWithMaxSize;

	Decorator::Tab* previousTab = NULL;
	for (int32 i = 0; i < tabCount; i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		if (tab == NULL)
			continue;

		if (int_equal(maxTabSize, tab->tabRect.Width()))
			tab->tabRect.right -= minus;

		if (previousTab != NULL) {
			float offsetX = previousTab->tabRect.right - tab->tabRect.left;
			tab->tabRect.OffsetBy(offsetX, 0);
		}

		previousTab = tab;
	}

	if (delta > 0) {
		_DistributeTabSize(delta);
		return;
	}

	// done
	if (previousTab != NULL)
		previousTab->tabRect.right = floorf(fFrame.right + fBorderWidth);

	for (int32 i = 0; i < tabCount; i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		if (tab == NULL)
			continue;

		tab->tabOffset = uint32(tab->tabRect.left - fLeftBorder.left);
	}
}


void
TabDecorator::_SetTitle(Decorator::Tab* tab, const char* string,
	BRegion* updateRegion)
{
	// TODO: we could be much smarter about the update region

	BRect rect = TabRect((int32) 0) | TabRect(CountTabs() - 1);
		// Get a rect of all the tabs

	_DoLayout();

	if (updateRegion == NULL)
		return;

	rect = rect | TabRect(CountTabs() - 1);
		// Update the rect to guarantee it updates all the tabs

	rect.bottom++;
		// the border will look differently when the title is adjacent

	updateRegion->Include(rect);
}


void
TabDecorator::_MoveBy(BPoint offset)
{
	STRACE(("TabDecorator: Move By (%.1f, %.1f)\n", offset.x, offset.y));

	// Move all internal rectangles the appropriate amount
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		tab->zoomRect.OffsetBy(offset);
		tab->closeRect.OffsetBy(offset);
		tab->tabRect.OffsetBy(offset);
	}

	fFrame.OffsetBy(offset);
	fTitleBarRect.OffsetBy(offset);
	fTabsRegion.OffsetBy(offset);
	fResizeRect.OffsetBy(offset);
	fBorderRect.OffsetBy(offset);

	fLeftBorder.OffsetBy(offset);
	fRightBorder.OffsetBy(offset);
	fTopBorder.OffsetBy(offset);
	fBottomBorder.OffsetBy(offset);
}


void
TabDecorator::_ResizeBy(BPoint offset, BRegion* dirty)
{
	STRACE(("TabDecorator: Resize By (%.1f, %.1f)\n", offset.x, offset.y));

	// Move all internal rectangles the appropriate amount
	fFrame.right += offset.x;
	fFrame.bottom += offset.y;

	// Handle invalidation of resize rect
	if (dirty != NULL && !(fTopTab->flags & B_NOT_RESIZABLE)) {
		BRect realResizeRect;
		switch ((int)fTopTab->look) {
			case B_DOCUMENT_WINDOW_LOOK:
				realResizeRect = fResizeRect;
				// Resize rect at old location
				dirty->Include(realResizeRect);
				realResizeRect.OffsetBy(offset);
				// Resize rect at new location
				dirty->Include(realResizeRect);
				break;

			case B_TITLED_WINDOW_LOOK:
			case B_FLOATING_WINDOW_LOOK:
			case B_MODAL_WINDOW_LOOK:
			case kLeftTitledWindowLook:
				// The bottom border resize line
				realResizeRect.Set(fRightBorder.right - kBorderResizeLength,
					fBottomBorder.top,
					fRightBorder.right - kBorderResizeLength,
					fBottomBorder.bottom - 1);
				// Old location
				dirty->Include(realResizeRect);
				realResizeRect.OffsetBy(offset);
				// New location
				dirty->Include(realResizeRect);

				// The right border resize line
				realResizeRect.Set(fRightBorder.left,
					fBottomBorder.bottom - kBorderResizeLength,
					fRightBorder.right - 1,
					fBottomBorder.bottom - kBorderResizeLength);
				// Old location
				dirty->Include(realResizeRect);
				realResizeRect.OffsetBy(offset);
				// New location
				dirty->Include(realResizeRect);
				break;

			default:
				break;
		}
	}

	fResizeRect.OffsetBy(offset);

	fBorderRect.right += offset.x;
	fBorderRect.bottom += offset.y;

	fLeftBorder.bottom += offset.y;
	fTopBorder.right += offset.x;

	fRightBorder.OffsetBy(offset.x, 0.0);
	fRightBorder.bottom	+= offset.y;

	fBottomBorder.OffsetBy(0.0, offset.y);
	fBottomBorder.right	+= offset.x;

	if (dirty) {
		if (offset.x > 0.0) {
			BRect t(fRightBorder.left - offset.x, fTopBorder.top,
				fRightBorder.right, fTopBorder.bottom);
			dirty->Include(t);
			t.Set(fRightBorder.left - offset.x, fBottomBorder.top,
				fRightBorder.right, fBottomBorder.bottom);
			dirty->Include(t);
			dirty->Include(fRightBorder);
		} else if (offset.x < 0.0) {
			dirty->Include(BRect(fRightBorder.left, fTopBorder.top,
				fRightBorder.right, fBottomBorder.bottom));
		}
		if (offset.y > 0.0) {
			BRect t(fLeftBorder.left, fLeftBorder.bottom - offset.y,
				fLeftBorder.right, fLeftBorder.bottom);
			dirty->Include(t);
			t.Set(fRightBorder.left, fRightBorder.bottom - offset.y,
				fRightBorder.right, fRightBorder.bottom);
			dirty->Include(t);
			dirty->Include(fBottomBorder);
		} else if (offset.y < 0.0) {
			dirty->Include(fBottomBorder);
		}
	}

	// resize tab and layout tab items
	if (fTitleBarRect.IsValid()) {
		if (fTabList.CountItems() > 1) {
			_DoTabLayout();
			if (dirty != NULL)
				dirty->Include(fTitleBarRect);
			return;
		}

		Decorator::Tab* tab = _TabAt(0);
		BRect& tabRect = tab->tabRect;
		BRect oldTabRect(tabRect);

		float tabSize;
		float tabOffset = _SingleTabOffsetAndSize(tabSize);

		float delta = tabOffset - tab->tabOffset;
		tab->tabOffset = (uint32)tabOffset;
		if (fTopTab->look != kLeftTitledWindowLook)
			tabRect.OffsetBy(delta, 0.0);
		else
			tabRect.OffsetBy(0.0, delta);

		if (tabSize < tab->minTabSize)
			tabSize = tab->minTabSize;
		if (tabSize > tab->maxTabSize)
			tabSize = tab->maxTabSize;

		if (fTopTab->look != kLeftTitledWindowLook
			&& tabSize != tabRect.Width()) {
			tabRect.right = tabRect.left + tabSize;
		} else if (fTopTab->look == kLeftTitledWindowLook
			&& tabSize != tabRect.Height()) {
			tabRect.bottom = tabRect.top + tabSize;
		}

		if (oldTabRect != tabRect) {
			_LayoutTabItems(tab, tabRect);

			if (dirty) {
				// NOTE: the tab rect becoming smaller only would
				// handled be the Desktop anyways, so it is sufficient
				// to include it into the dirty region in it's
				// final state
				BRect redraw(tabRect);
				if (delta != 0.0) {
					redraw = redraw | oldTabRect;
					if (fTopTab->look != kLeftTitledWindowLook)
						redraw.bottom++;
					else
						redraw.right++;
				}
				dirty->Include(redraw);
			}
		}
		fTitleBarRect = tabRect;
		fTabsRegion = fTitleBarRect;
	}
}


void
TabDecorator::_SetFocus(Decorator::Tab* tab)
{
	Decorator::Tab* decoratorTab = static_cast<Decorator::Tab*>(tab);

	decoratorTab->buttonFocus = IsFocus(tab)
		|| ((decoratorTab->look == B_FLOATING_WINDOW_LOOK
			|| decoratorTab->look == kLeftTitledWindowLook)
			&& (decoratorTab->flags & B_AVOID_FOCUS) != 0);
	if (CountTabs() > 1)
		_LayoutTabItems(decoratorTab, decoratorTab->tabRect);
}


bool
TabDecorator::_SetTabLocation(Decorator::Tab* _tab, float location,
	bool isShifting, BRegion* updateRegion)
{
	STRACE(("TabDecorator: Set Tab Location(%.1f)\n", location));

	if (CountTabs() > 1) {
		if (isShifting == false) {
			_DoTabLayout();
			if (updateRegion != NULL)
				updateRegion->Include(fTitleBarRect);

			fOldMovingTab = BRect(0, 0, -1, -1);
			return true;
		} else {
			if (fOldMovingTab.IsValid() == false)
				fOldMovingTab = _tab->tabRect;
		}
	}

	Decorator::Tab* tab = static_cast<Decorator::Tab*>(_tab);
	BRect& tabRect = tab->tabRect;
	if (tabRect.IsValid() == false)
		return false;

	if (location < 0)
		location = 0;

	float maxLocation
		= fRightBorder.right - fLeftBorder.left - tabRect.Width();
	if (CountTabs() > 1)
		maxLocation = fTitleBarRect.right - fLeftBorder.left - tabRect.Width();

	if (location > maxLocation)
		location = maxLocation;

	float delta = floor(location - tab->tabOffset);
	if (delta == 0.0)
		return false;

	// redraw old rect (1 pixel on the border must also be updated)
	BRect rect(tabRect);
	rect.bottom++;
	if (updateRegion != NULL)
		updateRegion->Include(rect);

	tabRect.OffsetBy(delta, 0);
	tab->tabOffset = (int32)location;
	_LayoutTabItems(_tab, tabRect);
	tab->tabLocation = maxLocation > 0.0 ? tab->tabOffset / maxLocation : 0.0;

	if (fTabList.CountItems() == 1)
		fTitleBarRect = tabRect;

	_CalculateTabsRegion();

	// redraw new rect as well
	rect = tabRect;
	rect.bottom++;
	if (updateRegion != NULL)
		updateRegion->Include(rect);

	return true;
}


bool
TabDecorator::_SetSettings(const BMessage& settings, BRegion* updateRegion)
{
	float tabLocation;
	bool modified = false;
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		if (settings.FindFloat("tab location", i, &tabLocation) != B_OK)
			return false;
		modified |= SetTabLocation(i, tabLocation, updateRegion);
	}
	return modified;
}


bool
TabDecorator::_AddTab(DesktopSettings& settings, int32 index,
	BRegion* updateRegion)
{
	_UpdateFont(settings);

	_DoLayout();
	if (updateRegion != NULL)
		updateRegion->Include(fTitleBarRect);
	return true;
}


bool
TabDecorator::_RemoveTab(int32 index, BRegion* updateRegion)
{
	BRect oldRect = TabRect(index) | TabRect(CountTabs() - 1);
		// Get a rect of all the tabs to the right - they will all be moved
	_DoLayout();
	if (updateRegion != NULL) {
		updateRegion->Include(oldRect);
		updateRegion->Include(fTitleBarRect);
	}
	return true;
}


bool
TabDecorator::_MoveTab(int32 from, int32 to, bool isMoving,
	BRegion* updateRegion)
{
	Decorator::Tab* toTab = _TabAt(to);
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
}


void
TabDecorator::_GetFootprint(BRegion *region)
{
	STRACE(("TabDecorator: GetFootprint\n"));

	// This function calculates the decorator's footprint in coordinates
	// relative to the view. This is most often used to set a Window
	// object's visible region.
	if (region == NULL)
		return;

	region->MakeEmpty();

	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK)
		return;

	region->Include(fTopBorder);
	region->Include(fLeftBorder);
	region->Include(fRightBorder);
	region->Include(fBottomBorder);

	if (fTopTab->look == B_BORDERED_WINDOW_LOOK)
		return;

	region->Include(&fTabsRegion);

	if (fTopTab->look == B_DOCUMENT_WINDOW_LOOK) {
		// include the rectangular resize knob on the bottom right
		float knobSize = kResizeKnobSize - fBorderWidth;
		region->Include(BRect(fFrame.right - knobSize, fFrame.bottom - knobSize,
			fFrame.right, fFrame.bottom));
	}
}


void
TabDecorator::_DrawButtons(Decorator::Tab* tab, const BRect& invalid)
{
	STRACE(("TabDecorator: _DrawButtons\n"));

	// Draw the buttons if we're supposed to
	if (!(tab->flags & B_NOT_CLOSABLE) && invalid.Intersects(tab->closeRect))
		_DrawClose(tab, false, tab->closeRect);
	if (!(tab->flags & B_NOT_ZOOMABLE) && invalid.Intersects(tab->zoomRect))
		_DrawZoom(tab, false, tab->zoomRect);
}


void
TabDecorator::_UpdateFont(DesktopSettings& settings)
{
	ServerFont font;
	if (fTopTab->look == B_FLOATING_WINDOW_LOOK
		|| fTopTab->look == kLeftTitledWindowLook) {
		settings.GetDefaultPlainFont(font);
		if (fTopTab->look == kLeftTitledWindowLook)
			font.SetRotation(90.0f);
	} else
		settings.GetDefaultBoldFont(font);

	font.SetFlags(B_FORCE_ANTIALIASING);
	font.SetSpacing(B_STRING_SPACING);
	fDrawState.SetFont(font);
}


void
TabDecorator::_GetButtonSizeAndOffset(const BRect& tabRect, float* _offset,
	float* _size, float* _inset) const
{
	float tabSize = fTopTab->look == kLeftTitledWindowLook ?
		tabRect.Width() : tabRect.Height();

	bool smallTab = fTopTab->look == B_FLOATING_WINDOW_LOOK
		|| fTopTab->look == kLeftTitledWindowLook;

	*_offset = smallTab ? floorf(fDrawState.Font().Size() / 2.6)
		: floorf(fDrawState.Font().Size() / 2.3);
	*_inset = smallTab ? floorf(fDrawState.Font().Size() / 5.0)
		: floorf(fDrawState.Font().Size() / 6.0);

	// "+ 2" so that the rects are centered within the solid area
	// (without the 2 pixels for the top border)
	*_size = tabSize - 2 * *_offset + *_inset;
}


void
TabDecorator::_LayoutTabItems(Decorator::Tab* _tab, const BRect& tabRect)
{
	Decorator::Tab* tab = static_cast<Decorator::Tab*>(_tab);

	float offset;
	float size;
	float inset;
	_GetButtonSizeAndOffset(tabRect, &offset, &size, &inset);

	// default textOffset
	tab->textOffset = _DefaultTextOffset();

	BRect& closeRect = tab->closeRect;
	BRect& zoomRect = tab->zoomRect;

	// calulate close rect based on the tab rectangle
	if (tab->look != kLeftTitledWindowLook) {
		closeRect.Set(tabRect.left + offset, tabRect.top + offset,
			tabRect.left + offset + size, tabRect.top + offset + size);

		zoomRect.Set(tabRect.right - offset - size, tabRect.top + offset,
			tabRect.right - offset, tabRect.top + offset + size);

		// hidden buttons have no width
		if ((tab->flags & B_NOT_CLOSABLE) != 0)
			closeRect.right = closeRect.left - offset;
		if ((tab->flags & B_NOT_ZOOMABLE) != 0)
			zoomRect.left = zoomRect.right + offset;
	} else {
		closeRect.Set(tabRect.left + offset, tabRect.top + offset,
			tabRect.left + offset + size, tabRect.top + offset + size);

		zoomRect.Set(tabRect.left + offset, tabRect.bottom - offset - size,
			tabRect.left + size + offset, tabRect.bottom - offset);

		// hidden buttons have no height
		if ((tab->flags & B_NOT_CLOSABLE) != 0)
			closeRect.bottom = closeRect.top - offset;
		if ((tab->flags & B_NOT_ZOOMABLE) != 0)
			zoomRect.top = zoomRect.bottom + offset;
	}

	// calculate room for title
	// TODO: the +2 is there because the title often appeared
	//	truncated for no apparent reason - OTOH the title does
	//	also not appear perfectly in the middle
	if (tab->look != kLeftTitledWindowLook)
		size = (zoomRect.left - closeRect.right) - tab->textOffset * 2 + inset;
	else
		size = (zoomRect.top - closeRect.bottom) - tab->textOffset * 2 + inset;

	bool stackMode = fTabList.CountItems() > 1;
	if (stackMode && IsFocus(tab) == false) {
		zoomRect.Set(0, 0, 0, 0);
		size = (tab->tabRect.right - closeRect.right) - tab->textOffset * 2
			+ inset;
	}
	uint8 truncateMode = B_TRUNCATE_MIDDLE;
	if (stackMode) {
		if (tab->tabRect.Width() < 100)
			truncateMode = B_TRUNCATE_END;
		float titleWidth = fDrawState.Font().StringWidth(Title(tab),
			BString(Title(tab)).Length());
		if (size < titleWidth) {
			float oldTextOffset = tab->textOffset;
			tab->textOffset -= (titleWidth - size) / 2;
			const float kMinTextOffset = 5.;
			if (tab->textOffset < kMinTextOffset)
				tab->textOffset = kMinTextOffset;
			size += oldTextOffset * 2;
			size -= tab->textOffset * 2;
		}
	}
	tab->truncatedTitle = Title(tab);
	fDrawState.Font().TruncateString(&tab->truncatedTitle, truncateMode, size);
	tab->truncatedTitleLength = tab->truncatedTitle.Length();
}


float
TabDecorator::_DefaultTextOffset() const
{
	return (fTopTab->look == B_FLOATING_WINDOW_LOOK
		|| fTopTab->look == kLeftTitledWindowLook) ? 10 : 18;
}


float
TabDecorator::_SingleTabOffsetAndSize(float& tabSize)
{
	float maxLocation;
	if (fTopTab->look != kLeftTitledWindowLook) {
		tabSize = fRightBorder.right - fLeftBorder.left;
	} else {
		tabSize = fBottomBorder.bottom - fTopBorder.top;
	}
	Decorator::Tab* tab = _TabAt(0);
	maxLocation = tabSize - tab->maxTabSize;
	if (maxLocation < 0)
		maxLocation = 0;

	return floorf(tab->tabLocation * maxLocation);
}


void
TabDecorator::_CalculateTabsRegion()
{
	fTabsRegion.MakeEmpty();
	for (int32 i = 0; i < fTabList.CountItems(); i++)
		fTabsRegion.Include(fTabList.ItemAt(i)->tabRect);
}
