/*
 * Copyright 2001-2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Philippe Saint-Pierre, stpere@gmail.com
 *		Ryan Leavengood <leavengood@gmail.com>
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


/*!	Default and fallback decorator for the app_server - the yellow tabs */


#include "DefaultDecorator.h"

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
#include "ServerBitmap.h"


//#define DEBUG_DECORATOR
#ifdef DEBUG_DECORATOR
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif


DefaultDecorator::Tab::Tab()
	:
	tabOffset(0),
	tabLocation(0.0),
	isHighlighted(false)
{
	closeBitmaps[0] = closeBitmaps[1] = closeBitmaps[2] = closeBitmaps[3]
		= zoomBitmaps[0] = zoomBitmaps[1] = zoomBitmaps[2] = zoomBitmaps[3]
		= NULL;
}


static const float kBorderResizeLength = 22.0;
static const float kResizeKnobSize = 18.0;


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


//	#pragma mark -


const rgb_color DefaultDecorator::kFrameColors[4] = {
	{ 152, 152, 152, 255 },
	{ 240, 240, 240, 255 },
	{ 152, 152, 152, 255 },
	{ 108, 108, 108, 255 }
};

const rgb_color DefaultDecorator::kFocusFrameColors[2] = {
	{ 224, 224, 224, 255 },
	{ 208, 208, 208, 255 }
};

const rgb_color DefaultDecorator::kNonFocusFrameColors[2] = {
	{ 232, 232, 232, 255 },
	{ 232, 232, 232, 255 }
};



// TODO: get rid of DesktopSettings here, and introduce private accessor
//	methods to the Decorator base class
DefaultDecorator::DefaultDecorator(DesktopSettings& settings, BRect rect,
	window_look look, uint32 flags)
	:
	Decorator(settings, rect, look, flags),
	// focus color constants
	kFocusTabColor(settings.UIColor(B_WINDOW_TAB_COLOR)),
	kFocusTabColorLight(tint_color(kFocusTabColor,
 		(B_LIGHTEN_MAX_TINT + B_LIGHTEN_2_TINT) / 2)),
	kFocusTabColorBevel(tint_color(kFocusTabColor, B_LIGHTEN_2_TINT)),
	kFocusTabColorShadow(tint_color(kFocusTabColor,
 		(B_DARKEN_1_TINT + B_NO_TINT) / 2)),
	kFocusTextColor(settings.UIColor(B_WINDOW_TEXT_COLOR)),
	// non-focus color constants
	kNonFocusTabColor(settings.UIColor(B_WINDOW_INACTIVE_TAB_COLOR)),
	kNonFocusTabColorLight(tint_color(kNonFocusTabColor,
 		(B_LIGHTEN_MAX_TINT + B_LIGHTEN_2_TINT) / 2)),
	kNonFocusTabColorBevel(tint_color(kNonFocusTabColor, B_LIGHTEN_2_TINT)),
	kNonFocusTabColorShadow(tint_color(kNonFocusTabColor,
 		(B_DARKEN_1_TINT + B_NO_TINT) / 2)),
	kNonFocusTextColor(settings.UIColor(B_WINDOW_INACTIVE_TEXT_COLOR)),

	fOldMovingTab(0, 0, -1, -1)
{
	_UpdateFont(settings);

	// Do initial decorator setup
	_DoLayout();

	// TODO: If the decorator was created with a frame too small, it should
	// resize itself!

	STRACE(("DefaultDecorator:\n"));
	STRACE(("\tFrame (%.1f,%.1f,%.1f,%.1f)\n",
		rect.left, rect.top, rect.right, rect.bottom));
}


DefaultDecorator::~DefaultDecorator()
{
	STRACE(("DefaultDecorator: ~DefaultDecorator()\n"));
}


float
DefaultDecorator::TabLocation(int32 tab) const
{
	DefaultDecorator::Tab* decoratorTab = _TabAt(tab);
	if (decoratorTab == NULL)
		return 0.;
	return (float)decoratorTab->tabOffset;
}


bool
DefaultDecorator::GetSettings(BMessage* settings) const
{
	if (!fTitleBarRect.IsValid())
		return false;

	if (settings->AddRect("tab frame", fTitleBarRect) != B_OK)
		return false;

	if (settings->AddFloat("border width", fBorderWidth) != B_OK)
		return false;

	// TODO only add the location of the tab of the window who requested the
	// settings
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		DefaultDecorator::Tab* tab = _TabAt(i);
		if (settings->AddFloat("tab location", (float)tab->tabOffset) != B_OK)
			return false;
	}
	return true;
}


// #pragma mark -


void
DefaultDecorator::Draw(BRect update)
{
	STRACE(("DefaultDecorator: Draw(%.1f,%.1f,%.1f,%.1f)\n",
		update.left, update.top, update.right, update.bottom));

	// We need to draw a few things: the tab, the resize knob, the borders,
	// and the buttons
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(update);
	_DrawTabs(update);
}


void
DefaultDecorator::Draw()
{
	// Easy way to draw everything - no worries about drawing only certain
	// things
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(BRect(fTopBorder.LeftTop(), fBottomBorder.RightBottom()));
	_DrawTabs(fTitleBarRect);
}


void
DefaultDecorator::GetSizeLimits(int32* minWidth, int32* minHeight,
	int32* maxWidth, int32* maxHeight) const
{
	float minTabSize = 0;
	if (CountTabs() > 0)
		minTabSize = _TabAt(0)->minTabSize;
	if (fTitleBarRect.IsValid()) {
		*minWidth = (int32)roundf(max_c(*minWidth,
			minTabSize - 2 * fBorderWidth));
	}
	if (fResizeRect.IsValid()) {
		*minHeight = (int32)roundf(max_c(*minHeight,
			fResizeRect.Height() - fBorderWidth));
	}
}


Decorator::Region
DefaultDecorator::RegionAt(BPoint where, int32& tab) const
{
	// Let the base class version identify hits of the buttons and the tab.
	Region region = Decorator::RegionAt(where, tab);
	if (region != REGION_NONE)
		return region;

	// check the resize corner
	if (fLook == B_DOCUMENT_WINDOW_LOOK && fResizeRect.Contains(where))
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
	if ((fFlags & B_NOT_RESIZABLE) == 0
		&& (fLook == B_TITLED_WINDOW_LOOK
			|| fLook == B_FLOATING_WINDOW_LOOK
			|| fLook == B_MODAL_WINDOW_LOOK
			|| fLook == kLeftTitledWindowLook)) {
		BRect resizeRect(BPoint(fBottomBorder.right - kBorderResizeLength,
			fBottomBorder.bottom - kBorderResizeLength),
			fBottomBorder.RightBottom());
		if (resizeRect.Contains(where))
			return REGION_RIGHT_BOTTOM_CORNER;
	}

	return region;
}


bool
DefaultDecorator::SetRegionHighlight(Region region, uint8 highlight,
	BRegion* dirty, int32 tabIndex)
{
	DefaultDecorator::Tab* tab = _TabAt(tabIndex);
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
DefaultDecorator::ExtendDirtyRegion(Region region, BRegion& dirty)
{
	switch (region) {
		case REGION_TAB:
			dirty.Include(fTitleBarRect);
			break;

		case REGION_CLOSE_BUTTON:
			if ((fFlags & B_NOT_CLOSABLE) == 0)
				for (int32 i = 0; i < fTabList.CountItems(); i++)
					dirty.Include(fTabList.ItemAt(i)->closeRect);
			break;

		case REGION_ZOOM_BUTTON:
			if ((fFlags & B_NOT_ZOOMABLE) == 0)
				for (int32 i = 0; i < fTabList.CountItems(); i++)
					dirty.Include(fTabList.ItemAt(i)->zoomRect);
			break;

		case REGION_LEFT_BORDER:
			if (fLeftBorder.IsValid()) {
				// fLeftBorder doesn't include the corners, so we have to add
				// them manually.
				BRect rect(fLeftBorder);
				rect.top = fTopBorder.top;
				rect.bottom = fBottomBorder.bottom;
				dirty.Include(rect);
			}
			break;

		case REGION_RIGHT_BORDER:
			if (fRightBorder.IsValid()) {
				// fRightBorder doesn't include the corners, so we have to add
				// them manually.
				BRect rect(fRightBorder);
				rect.top = fTopBorder.top;
				rect.bottom = fBottomBorder.bottom;
				dirty.Include(rect);
			}
			break;

		case REGION_TOP_BORDER:
			dirty.Include(fTopBorder);
			break;

		case REGION_BOTTOM_BORDER:
			dirty.Include(fBottomBorder);
			break;

		case REGION_RIGHT_BOTTOM_CORNER:
			if ((fFlags & B_NOT_RESIZABLE) == 0)
				dirty.Include(fResizeRect);
			break;

		default:
			break;
	}
}


float
DefaultDecorator::BorderWidth()
{
	return fBorderWidth;
}


float
DefaultDecorator::TabHeight()
{
	if (fTitleBarRect.IsValid())
		return fTitleBarRect.Height();
	return BorderWidth();
}


void
DefaultDecorator::_DoLayout()
{
	STRACE(("DefaultDecorator: Do Layout\n"));
	// Here we determine the size of every rectangle that we use
	// internally when we are given the size of the client rectangle.

	bool hasTab = false;

	switch ((int)Look()) {
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
		fTitleBarRect.Set(0.0, 0.0, -1.0, -1.0);
	}
}


void
DefaultDecorator::_DoTabLayout()
{
	float tabOffset = 0;
	if (fTabList.CountItems() == 1) {
		float tabSize;
		tabOffset = _SingleTabOffsetAndSize(tabSize);
	}

	float sumTabWidth = 0;
	// calculate our tab rect
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		DefaultDecorator::Tab* tab = _TabAt(i);

		BRect& tabRect = tab->tabRect;
		// distance from one item of the tab bar to another.
		// In this case the text and close/zoom rects
		tab->textOffset = _DefaultTextOffset();

		font_height fontHeight;
		fDrawState.Font().GetHeight(fontHeight);

		if (fLook != kLeftTitledWindowLook) {
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
		if (fLook == B_FLOATING_WINDOW_LOOK) {
			tabRect.InsetBy(0, 2);
			tabRect.OffsetBy(0, 2);
		}

		float offset;
		float size;
		float inset;
		_GetButtonSizeAndOffset(tabRect, &offset, &size, &inset);

		// tab->minTabSize contains just the room for the buttons
		tab->minTabSize = inset * 2 + tab->textOffset;
		if ((fFlags & B_NOT_CLOSABLE) == 0)
			tab->minTabSize += offset + size;
		if ((fFlags & B_NOT_ZOOMABLE) == 0)
			tab->minTabSize += offset + size;

		// tab->maxTabSize contains tab->minTabSize + the width required for the
		// title
		tab->maxTabSize = fDrawingEngine
			? ceilf(fDrawingEngine->StringWidth(Title(tab), strlen(Title(tab)),
				fDrawState.Font())) : 0.0;
		if (tab->maxTabSize > 0.0)
			tab->maxTabSize += tab->textOffset;
		tab->maxTabSize += tab->minTabSize;

		float tabSize = (fLook != kLeftTitledWindowLook
			? fFrame.Width() : fFrame.Height()) + fBorderWidth * 2;
		if (tabSize < tab->minTabSize)
			tabSize = tab->minTabSize;
		if (tabSize > tab->maxTabSize)
			tabSize = tab->maxTabSize;

		// layout buttons and truncate text
		if (fLook != kLeftTitledWindowLook)
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


static bool
int_equal(float x, float y)
{
	return abs(x - y) <= 1;
}


void
DefaultDecorator::_DistributeTabSize(float delta)
{
	ASSERT(CountTabs() > 1);

	float maxTabSize = 0;
	float secMaxTabSize = 0;
	int32 nTabsWithMaxSize = 0;
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
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

	float minus = ceil(std::min(maxTabSize - secMaxTabSize, delta));
	delta -= minus;
	minus /= nTabsWithMaxSize;

	Decorator::Tab* prevTab = NULL;
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		if (int_equal(maxTabSize, tab->tabRect.Width()))
			tab->tabRect.right -= minus;

		if (prevTab) {
			tab->tabRect.OffsetBy(prevTab->tabRect.right - tab->tabRect.left,
				0);
		}

		prevTab = tab;
	}

	if (delta > 0) {
		_DistributeTabSize(delta);
		return;
	}

	// done
	prevTab->tabRect.right = floor(fFrame.right + fBorderWidth);

	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		DefaultDecorator::Tab* tab = _TabAt(i);
		tab->tabOffset = uint32(tab->tabRect.left - fLeftBorder.left);
	}
}


Decorator::Tab*
DefaultDecorator::_AllocateNewTab()
{
	Decorator::Tab* tab = new(std::nothrow) DefaultDecorator::Tab;
	if (tab == NULL)
		return NULL;
	// Set appropriate colors based on the current focus value. In this case,
	// each decorator defaults to not having the focus.
	_SetFocus(tab);
	return tab;
}


DefaultDecorator::Tab*
DefaultDecorator::_TabAt(int32 index) const
{
	return static_cast<DefaultDecorator::Tab*>(fTabList.ItemAt(index));
}


void
DefaultDecorator::_DrawFrame(BRect invalid)
{
	STRACE(("_DrawFrame(%f,%f,%f,%f)\n", invalid.left, invalid.top,
		invalid.right, invalid.bottom));

	// NOTE: the DrawingEngine needs to be locked for the entire
	// time for the clipping to stay valid for this decorator

	if (fLook == B_NO_BORDER_WINDOW_LOOK)
		return;

	if (fBorderWidth <= 0)
		return;

	// Draw the border frame
	BRect r = BRect(fTopBorder.LeftTop(), fBottomBorder.RightBottom());
	switch ((int)fLook) {
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
		case B_MODAL_WINDOW_LOOK:
		{
			// top
			if (invalid.Intersects(fTopBorder)) {
				ComponentColors colors;
				_GetComponentColors(COMPONENT_TOP_BORDER, colors);

				for (int8 i = 0; i < 5; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.top + i),
						BPoint(r.right - i, r.top + i), colors[i]);
				}
				if (fTitleBarRect.IsValid()) {
					// grey along the bottom of the tab
					// (overwrites "white" from frame)
					fDrawingEngine->StrokeLine(
						BPoint(fTitleBarRect.left + 2,
							fTitleBarRect.bottom + 1),
						BPoint(fTitleBarRect.right - 2,
							fTitleBarRect.bottom + 1),
						colors[2]);
				}
			}
			// left
			if (invalid.Intersects(fLeftBorder.InsetByCopy(0, -fBorderWidth))) {
				ComponentColors colors;
				_GetComponentColors(COMPONENT_LEFT_BORDER, colors);

				for (int8 i = 0; i < 5; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.top + i),
						BPoint(r.left + i, r.bottom - i), colors[i]);
				}
			}
			// bottom
			if (invalid.Intersects(fBottomBorder)) {
				ComponentColors colors;
				_GetComponentColors(COMPONENT_BOTTOM_BORDER, colors);

				for (int8 i = 0; i < 5; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.bottom - i),
						BPoint(r.right - i, r.bottom - i),
						colors[(4 - i) == 4 ? 5 : (4 - i)]);
				}
			}
			// right
			if (invalid.Intersects(fRightBorder.InsetByCopy(0, -fBorderWidth))) {
				ComponentColors colors;
				_GetComponentColors(COMPONENT_RIGHT_BORDER, colors);

				for (int8 i = 0; i < 5; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.right - i, r.top + i),
						BPoint(r.right - i, r.bottom - i),
						colors[(4 - i) == 4 ? 5 : (4 - i)]);
				}
			}
			break;
		}

		case B_FLOATING_WINDOW_LOOK:
		case kLeftTitledWindowLook:
		{
			// top
			if (invalid.Intersects(fTopBorder)) {
				ComponentColors colors;
				_GetComponentColors(COMPONENT_TOP_BORDER, colors);

				for (int8 i = 0; i < 3; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.top + i),
						BPoint(r.right - i, r.top + i), colors[i * 2]);
				}
				if (fTitleBarRect.IsValid() && fLook != kLeftTitledWindowLook) {
					// grey along the bottom of the tab
					// (overwrites "white" from frame)
					fDrawingEngine->StrokeLine(
						BPoint(fTitleBarRect.left + 2,
							fTitleBarRect.bottom + 1),
						BPoint(fTitleBarRect.right - 2,
							fTitleBarRect.bottom + 1), colors[2]);
				}
			}
			// left
			if (invalid.Intersects(fLeftBorder.InsetByCopy(0, -fBorderWidth))) {
				ComponentColors colors;
				_GetComponentColors(COMPONENT_LEFT_BORDER, colors);

				for (int8 i = 0; i < 3; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.top + i),
						BPoint(r.left + i, r.bottom - i), colors[i * 2]);
				}
				if (fLook == kLeftTitledWindowLook && fTitleBarRect.IsValid()) {
					// grey along the right side of the tab
					// (overwrites "white" from frame)
					fDrawingEngine->StrokeLine(
						BPoint(fTitleBarRect.right + 1,
							fTitleBarRect.top + 2),
						BPoint(fTitleBarRect.right + 1,
							fTitleBarRect.bottom - 2), colors[2]);
				}
			}
			// bottom
			if (invalid.Intersects(fBottomBorder)) {
				ComponentColors colors;
				_GetComponentColors(COMPONENT_BOTTOM_BORDER, colors);

				for (int8 i = 0; i < 3; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.bottom - i),
						BPoint(r.right - i, r.bottom - i),
						colors[(2 - i) == 2 ? 5 : (2 - i) * 2]);
				}
			}
			// right
			if (invalid.Intersects(fRightBorder.InsetByCopy(0, -fBorderWidth))) {
				ComponentColors colors;
				_GetComponentColors(COMPONENT_RIGHT_BORDER, colors);

				for (int8 i = 0; i < 3; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.right - i, r.top + i),
						BPoint(r.right - i, r.bottom - i),
						colors[(2 - i) == 2 ? 5 : (2 - i) * 2]);
				}
			}
			break;
		}

		case B_BORDERED_WINDOW_LOOK:
		{
			// TODO: Draw the borders individually!
			ComponentColors colors;
			_GetComponentColors(COMPONENT_LEFT_BORDER, colors);

			fDrawingEngine->StrokeRect(r, colors[5]);
			break;
		}

		default:
			// don't draw a border frame
			break;
	}

	// Draw the resize knob if we're supposed to
	if (!(fFlags & B_NOT_RESIZABLE)) {
		r = fResizeRect;

		ComponentColors colors;
		_GetComponentColors(COMPONENT_RESIZE_CORNER, colors);

		switch ((int)fLook) {
			case B_DOCUMENT_WINDOW_LOOK:
			{
				if (!invalid.Intersects(r))
					break;

				float x = r.right - 3;
				float y = r.bottom - 3;

				BRect bg(x - 13, y - 13, x, y);

				BGradientLinear gradient;
				gradient.SetStart(bg.LeftTop());
				gradient.SetEnd(bg.RightBottom());
				gradient.AddColor(colors[1], 0);
				gradient.AddColor(colors[2], 255);

				fDrawingEngine->FillRect(bg, gradient);

				fDrawingEngine->StrokeLine(BPoint(x - 15, y - 15),
					BPoint(x - 15, y - 2), colors[0]);
				fDrawingEngine->StrokeLine(BPoint(x - 14, y - 14),
					BPoint(x - 14, y - 1), colors[1]);
				fDrawingEngine->StrokeLine(BPoint(x - 15, y - 15),
					BPoint(x - 2, y - 15), colors[0]);
				fDrawingEngine->StrokeLine(BPoint(x - 14, y - 14),
					BPoint(x - 1, y - 14), colors[1]);

				if (fTopTab && !IsFocus(fTopTab))
					break;

				static const rgb_color kWhite
					= (rgb_color){ 255, 255, 255, 255 };
				for (int8 i = 1; i <= 4; i++) {
					for (int8 j = 1; j <= i; j++) {
						BPoint pt1(x - (3 * j) + 1, y - (3 * (5 - i)) + 1);
						BPoint pt2(x - (3 * j) + 2, y - (3 * (5 - i)) + 2);
						fDrawingEngine->StrokePoint(pt1, colors[0]);
						fDrawingEngine->StrokePoint(pt2, kWhite);
					}
				}
				break;
			}

			case B_TITLED_WINDOW_LOOK:
			case B_FLOATING_WINDOW_LOOK:
			case B_MODAL_WINDOW_LOOK:
			case kLeftTitledWindowLook:
			{
				if (!invalid.Intersects(BRect(fRightBorder.right - kBorderResizeLength,
					fBottomBorder.bottom - kBorderResizeLength, fRightBorder.right - 1,
					fBottomBorder.bottom - 1)))
					break;

				fDrawingEngine->StrokeLine(
					BPoint(fRightBorder.left, fBottomBorder.bottom - kBorderResizeLength),
					BPoint(fRightBorder.right - 1, fBottomBorder.bottom - kBorderResizeLength),
					colors[0]);
				fDrawingEngine->StrokeLine(
					BPoint(fRightBorder.right - kBorderResizeLength, fBottomBorder.top),
					BPoint(fRightBorder.right - kBorderResizeLength, fBottomBorder.bottom - 1),
					colors[0]);
				break;
			}

			default:
				// don't draw resize corner
				break;
		}
	}
}


void
DefaultDecorator::_DrawTab(Decorator::Tab* tab, BRect invalid)
{
	STRACE(("_DrawTab(%.1f,%.1f,%.1f,%.1f)\n",
		invalid.left, invalid.top, invalid.right, invalid.bottom));
	const BRect& tabRect = tab->tabRect;
	// If a window has a tab, this will draw it and any buttons which are
	// in it.
	if (!tabRect.IsValid() || !invalid.Intersects(tabRect))
		return;

	ComponentColors colors;
	_GetComponentColors(COMPONENT_TAB, colors, tab);

	// outer frame
	fDrawingEngine->StrokeLine(tabRect.LeftTop(), tabRect.LeftBottom(),
		colors[COLOR_TAB_FRAME_LIGHT]);
	fDrawingEngine->StrokeLine(tabRect.LeftTop(), tabRect.RightTop(),
		colors[COLOR_TAB_FRAME_LIGHT]);
	if (fLook != kLeftTitledWindowLook) {
		fDrawingEngine->StrokeLine(tabRect.RightTop(), tabRect.RightBottom(),
			colors[COLOR_TAB_FRAME_DARK]);
	} else {
		fDrawingEngine->StrokeLine(tabRect.LeftBottom(),
			tabRect.RightBottom(), colors[COLOR_TAB_FRAME_DARK]);
	}

	float tabBotton = tabRect.bottom;
	if (fTopTab != tab)
		tabBotton -= 1;

	// bevel
	fDrawingEngine->StrokeLine(BPoint(tabRect.left + 1, tabRect.top + 1),
		BPoint(tabRect.left + 1,
			tabBotton - (fLook == kLeftTitledWindowLook ? 1 : 0)),
		colors[COLOR_TAB_BEVEL]);
	fDrawingEngine->StrokeLine(BPoint(tabRect.left + 1, tabRect.top + 1),
		BPoint(tabRect.right - (fLook == kLeftTitledWindowLook ? 0 : 1),
			tabRect.top + 1),
		colors[COLOR_TAB_BEVEL]);

	if (fLook != kLeftTitledWindowLook) {
		fDrawingEngine->StrokeLine(BPoint(tabRect.right - 1, tabRect.top + 2),
			BPoint(tabRect.right - 1, tabBotton),
			colors[COLOR_TAB_SHADOW]);
	} else {
		fDrawingEngine->StrokeLine(
			BPoint(tabRect.left + 2, tabRect.bottom - 1),
			BPoint(tabRect.right, tabRect.bottom - 1),
			colors[COLOR_TAB_SHADOW]);
	}

	// fill
	BGradientLinear gradient;
	gradient.SetStart(tabRect.LeftTop());
	gradient.AddColor(colors[COLOR_TAB_LIGHT], 0);
	gradient.AddColor(colors[COLOR_TAB], 255);

	if (fLook != kLeftTitledWindowLook) {
		gradient.SetEnd(tabRect.LeftBottom());
		fDrawingEngine->FillRect(BRect(tabRect.left + 2, tabRect.top + 2,
			tabRect.right - 2, tabBotton), gradient);
	} else {
		gradient.SetEnd(tabRect.RightTop());
		fDrawingEngine->FillRect(BRect(tabRect.left + 2, tabRect.top + 2,
			tabRect.right, tabRect.bottom - 2), gradient);
	}

	_DrawTitle(tab, tabRect);

	DrawButtons(tab, invalid);
}


void
DefaultDecorator::_DrawClose(Decorator::Tab* _tab, bool direct, BRect rect)
{
	STRACE(("_DrawClose(%f,%f,%f,%f)\n", rect.left, rect.top, rect.right,
		rect.bottom));

	DefaultDecorator::Tab* tab = static_cast<DefaultDecorator::Tab*>(_tab);

	int32 index = (tab->buttonFocus ? 0 : 1) + (tab->closePressed ? 0 : 2);
	ServerBitmap* bitmap = tab->closeBitmaps[index];
	if (bitmap == NULL) {
		bitmap = _GetBitmapForButton(tab, COMPONENT_CLOSE_BUTTON,
			tab->closePressed, rect.IntegerWidth(), rect.IntegerHeight());
		tab->closeBitmaps[index] = bitmap;
	}

	_DrawButtonBitmap(bitmap, direct, rect);
}


void
DefaultDecorator::_DrawTitle(Decorator::Tab* _tab, BRect r)
{
	DefaultDecorator::Tab* tab = static_cast<DefaultDecorator::Tab*>(_tab);

	const BRect& tabRect = tab->tabRect;
	const BRect& closeRect = tab->closeRect;
	const BRect& zoomRect = tab->zoomRect;

	STRACE(("_DrawTitle(%f,%f,%f,%f)\n", r.left, r.top, r.right, r.bottom));

	ComponentColors colors;
	_GetComponentColors(COMPONENT_TAB, colors, tab);

	fDrawingEngine->SetDrawingMode(B_OP_OVER);
	fDrawingEngine->SetHighColor(colors[COLOR_TAB_TEXT]);
	fDrawingEngine->SetFont(fDrawState.Font());

	// figure out position of text
	font_height fontHeight;
	fDrawState.Font().GetHeight(fontHeight);

	BPoint titlePos;
	if (fLook != kLeftTitledWindowLook) {
		titlePos.x = closeRect.IsValid() ? closeRect.right + tab->textOffset
			: tabRect.left + tab->textOffset;
		titlePos.y = floorf(((tabRect.top + 2.0) + tabRect.bottom
			+ fontHeight.ascent + fontHeight.descent) / 2.0
			- fontHeight.descent + 0.5);
	} else {
		titlePos.x = floorf(((tabRect.left + 2.0) + tabRect.right
			+ fontHeight.ascent + fontHeight.descent) / 2.0
			- fontHeight.descent + 0.5);
		titlePos.y = zoomRect.IsValid() ? zoomRect.top - tab->textOffset
			: tabRect.bottom - tab->textOffset;
	}

	fDrawingEngine->DrawString(tab->truncatedTitle, tab->truncatedTitleLength,
		titlePos);

	fDrawingEngine->SetDrawingMode(B_OP_COPY);
}


void
DefaultDecorator::_DrawZoom(Decorator::Tab* _tab, bool direct, BRect rect)
{
	STRACE(("_DrawZoom(%f,%f,%f,%f)\n", rect.left, rect.top, rect.right,
		rect.bottom));
	if (rect.IntegerWidth() < 1)
		return;
	DefaultDecorator::Tab* tab = static_cast<DefaultDecorator::Tab*>(_tab);

	int32 index = (tab->buttonFocus ? 0 : 1) + (tab->zoomPressed ? 0 : 2);
	ServerBitmap* bitmap = tab->zoomBitmaps[index];
	if (bitmap == NULL) {
		bitmap = _GetBitmapForButton(tab, COMPONENT_ZOOM_BUTTON,
			tab->zoomPressed, rect.IntegerWidth(), rect.IntegerHeight());
		tab->zoomBitmaps[index] = bitmap;
	}

	_DrawButtonBitmap(bitmap, direct, rect);
}


void
DefaultDecorator::_SetTitle(Decorator::Tab* tab, const char* string,
	BRegion* updateRegion)
{
	// TODO: we could be much smarter about the update region

	BRect rect = TabRect(tab);

	_DoLayout();

	if (updateRegion == NULL)
		return;

	rect = rect | TabRect(tab);

	rect.bottom++;
		// the border will look differently when the title is adjacent

	updateRegion->Include(rect);
}


void
DefaultDecorator::_FontsChanged(DesktopSettings& settings,
	BRegion* updateRegion)
{
	// get previous extent
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());

	_UpdateFont(settings);
	_InvalidateBitmaps();
	_DoLayout();

	_InvalidateFootprint();
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());
}


void
DefaultDecorator::_SetLook(DesktopSettings& settings, window_look look,
	BRegion* updateRegion)
{
	// TODO: we could be much smarter about the update region

	// get previous extent
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());

	fLook = look;

	_UpdateFont(settings);
	_InvalidateBitmaps();
	_DoLayout();

	_InvalidateFootprint();
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());
}


void
DefaultDecorator::_SetFlags(uint32 flags, BRegion* updateRegion)
{
	// TODO: we could be much smarter about the update region

	// get previous extent
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());

	fFlags = flags;
	_DoLayout();

	_InvalidateFootprint();
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());
}


void
DefaultDecorator::_SetFocus(Decorator::Tab* _tab)
{
	DefaultDecorator::Tab* tab = static_cast<DefaultDecorator::Tab*>(_tab);
	tab->buttonFocus = IsFocus(tab)
		|| ((fLook == B_FLOATING_WINDOW_LOOK || fLook == kLeftTitledWindowLook)
			&& (fFlags & B_AVOID_FOCUS) != 0);
	if (CountTabs() > 1)
		_LayoutTabItems(tab, tab->tabRect);
}


void
DefaultDecorator::_MoveBy(BPoint offset)
{
	STRACE(("DefaultDecorator: Move By (%.1f, %.1f)\n", offset.x, offset.y));
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
DefaultDecorator::_ResizeBy(BPoint offset, BRegion* dirty)
{
	STRACE(("DefaultDecorator: Resize By (%.1f, %.1f)\n", offset.x, offset.y));
	// Move all internal rectangles the appropriate amount
	fFrame.right += offset.x;
	fFrame.bottom += offset.y;

	// Handle invalidation of resize rect
	if (dirty && !(fFlags & B_NOT_RESIZABLE)) {
		BRect realResizeRect;
		switch ((int)fLook) {
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
				realResizeRect.Set(fRightBorder.right - kBorderResizeLength, fBottomBorder.top,
					fRightBorder.right - kBorderResizeLength, fBottomBorder.bottom - 1);
				// Old location
				dirty->Include(realResizeRect);
				realResizeRect.OffsetBy(offset);
				// New location
				dirty->Include(realResizeRect);

				// The right border resize line
				realResizeRect.Set(fRightBorder.left, fBottomBorder.bottom - kBorderResizeLength,
					fRightBorder.right - 1, fBottomBorder.bottom - kBorderResizeLength);
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
			dirty->Include(fTitleBarRect);
			return;
		}

		DefaultDecorator::Tab* tab = _TabAt(0);
		BRect& tabRect = tab->tabRect;
		BRect oldTabRect(tabRect);

		float tabSize;
		float tabOffset = _SingleTabOffsetAndSize(tabSize);

		float delta = tabOffset - tab->tabOffset;
		tab->tabOffset = (uint32)tabOffset;
		if (fLook != kLeftTitledWindowLook)
			tabRect.OffsetBy(delta, 0.0);
		else
			tabRect.OffsetBy(0.0, delta);

		if (tabSize < tab->minTabSize)
			tabSize = tab->minTabSize;
		if (tabSize > tab->maxTabSize)
			tabSize = tab->maxTabSize;

		if (fLook != kLeftTitledWindowLook && tabSize != tabRect.Width()) {
			tabRect.right = tabRect.left + tabSize;
		} else if (fLook == kLeftTitledWindowLook
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
					if (fLook != kLeftTitledWindowLook)
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


bool
DefaultDecorator::_SetTabLocation(Decorator::Tab* _tab, float location,
	bool isShifting, BRegion* updateRegion)
{
	STRACE(("DefaultDecorator: Set Tab Location(%.1f)\n", location));
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
	
	DefaultDecorator::Tab* tab = static_cast<DefaultDecorator::Tab*>(_tab);
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
DefaultDecorator::_SetSettings(const BMessage& settings, BRegion* updateRegion)
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
DefaultDecorator::_AddTab(int32 index, BRegion* updateRegion)
{
	_DoLayout();
	if (updateRegion != NULL)
		updateRegion->Include(fTitleBarRect);
	return true;
}


bool
DefaultDecorator::_RemoveTab(int32 index, BRegion* updateRegion)
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
DefaultDecorator::_MoveTab(int32 from, int32 to, bool isMoving,
	BRegion* updateRegion)
{
	DefaultDecorator::Tab* toTab = _TabAt(to);
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
DefaultDecorator::_GetFootprint(BRegion *region)
{
	STRACE(("DefaultDecorator: Get Footprint\n"));
	// This function calculates the decorator's footprint in coordinates
	// relative to the view. This is most often used to set a Window
	// object's visible region.
	if (!region)
		return;

	region->MakeEmpty();

	if (fLook == B_NO_BORDER_WINDOW_LOOK)
		return;

	region->Include(fTopBorder);
	region->Include(fLeftBorder);
	region->Include(fRightBorder);
	region->Include(fBottomBorder);

	if (fLook == B_BORDERED_WINDOW_LOOK)
		return;

	region->Include(&fTabsRegion);

	if (fLook == B_DOCUMENT_WINDOW_LOOK) {
		// include the rectangular resize knob on the bottom right
		float knobSize = kResizeKnobSize - fBorderWidth;
		region->Include(BRect(fFrame.right - knobSize, fFrame.bottom - knobSize,
			fFrame.right, fFrame.bottom));
	}
}


void
DefaultDecorator::DrawButtons(Decorator::Tab* tab, const BRect& invalid)
{
	// Draw the buttons if we're supposed to
	if (!(fFlags & B_NOT_CLOSABLE) && invalid.Intersects(tab->closeRect))
		_DrawClose(tab, false, tab->closeRect);
	if (!(fFlags & B_NOT_ZOOMABLE) && invalid.Intersects(tab->zoomRect))
		_DrawZoom(tab, false, tab->zoomRect);
}


/*!	Returns the frame colors for the specified decorator component.

	The meaning of the color array elements depends on the specified component.
	For some components some array elements are unused.

	\param component The component for which to return the frame colors.
	\param highlight The highlight set for the component.
	\param colors An array of colors to be initialized by the function.
*/
void
DefaultDecorator::GetComponentColors(Component component, uint8 highlight,
	ComponentColors _colors, Decorator::Tab* _tab)
{
	DefaultDecorator::Tab* tab = static_cast<DefaultDecorator::Tab*>(_tab);
	switch (component) {
		case COMPONENT_TAB:
			_colors[COLOR_TAB_FRAME_LIGHT] = kFrameColors[0];
			_colors[COLOR_TAB_FRAME_DARK] = kFrameColors[3];
			if (tab && tab->buttonFocus) {
				_colors[COLOR_TAB] = kFocusTabColor;
				_colors[COLOR_TAB_LIGHT] = kFocusTabColorLight;
				_colors[COLOR_TAB_BEVEL] = kFocusTabColorBevel;
				_colors[COLOR_TAB_SHADOW] = kFocusTabColorShadow;
				_colors[COLOR_TAB_TEXT] = kFocusTextColor;
			} else {
				_colors[COLOR_TAB] = kNonFocusTabColor;
				_colors[COLOR_TAB_LIGHT] = kNonFocusTabColorLight;
				_colors[COLOR_TAB_BEVEL] = kNonFocusTabColorBevel;
				_colors[COLOR_TAB_SHADOW] = kNonFocusTabColorShadow;
				_colors[COLOR_TAB_TEXT] = kNonFocusTextColor;
			}
			break;

		case COMPONENT_CLOSE_BUTTON:
		case COMPONENT_ZOOM_BUTTON:
			if (tab && tab->buttonFocus) {
				_colors[COLOR_BUTTON] = kFocusTabColor;
				_colors[COLOR_BUTTON_LIGHT] = kFocusTabColorLight;
			} else {
				_colors[COLOR_BUTTON] = kNonFocusTabColor;
				_colors[COLOR_BUTTON_LIGHT] = kNonFocusTabColorLight;
			}
			break;

		case COMPONENT_LEFT_BORDER:
		case COMPONENT_RIGHT_BORDER:
		case COMPONENT_TOP_BORDER:
		case COMPONENT_BOTTOM_BORDER:
		case COMPONENT_RESIZE_CORNER:
		default:
			_colors[0] = kFrameColors[0];
			_colors[1] = kFrameColors[1];
			if (tab && tab->buttonFocus) {
				_colors[2] = kFocusFrameColors[0];
				_colors[3] = kFocusFrameColors[1];
			} else {
				_colors[2] = kNonFocusFrameColors[0];
				_colors[3] = kNonFocusFrameColors[1];
			}
			_colors[4] = kFrameColors[2];
			_colors[5] = kFrameColors[3];

			// for the resize-border highlight dye everything bluish.
			if (highlight == HIGHLIGHT_RESIZE_BORDER) {
				for (int32 i = 0; i < 6; i++) {
					_colors[i].red = std::max((int)_colors[i].red - 80, 0);
					_colors[i].green = std::max((int)_colors[i].green - 80, 0);
					_colors[i].blue = 255;
				}
			}
			break;
	}
}


void
DefaultDecorator::_UpdateFont(DesktopSettings& settings)
{
	ServerFont font;
	if (fLook == B_FLOATING_WINDOW_LOOK || fLook == kLeftTitledWindowLook) {
		settings.GetDefaultPlainFont(font);
		if (fLook == kLeftTitledWindowLook)
			font.SetRotation(90.0f);
	} else
		settings.GetDefaultBoldFont(font);

	font.SetFlags(B_FORCE_ANTIALIASING);
	font.SetSpacing(B_STRING_SPACING);
	fDrawState.SetFont(font);
}


void
DefaultDecorator::_DrawButtonBitmap(ServerBitmap* bitmap, bool direct,
	BRect rect)
{
	if (bitmap == NULL)
		return;

	bool copyToFrontEnabled = fDrawingEngine->CopyToFrontEnabled();
	fDrawingEngine->SetCopyToFrontEnabled(direct);
	drawing_mode oldMode;
	fDrawingEngine->SetDrawingMode(B_OP_OVER, oldMode);
	fDrawingEngine->DrawBitmap(bitmap, rect.OffsetToCopy(0, 0), rect);
	fDrawingEngine->SetDrawingMode(oldMode);
	fDrawingEngine->SetCopyToFrontEnabled(copyToFrontEnabled);
}


/*!	\brief Draws a framed rectangle with a gradient.
	\param down The rectangle should be drawn recessed or not.
	\param colors A button color array with the colors to be used.
*/
void
DefaultDecorator::_DrawBlendedRect(DrawingEngine* engine, BRect rect,
	bool down, const ComponentColors& colors)
{
	// figure out which colors to use
	rgb_color startColor, endColor;
	if (down) {
		startColor = tint_color(colors[COLOR_BUTTON], B_DARKEN_1_TINT);
		endColor = colors[COLOR_BUTTON_LIGHT];
	} else {
		startColor = tint_color(colors[COLOR_BUTTON], B_LIGHTEN_MAX_TINT);
		endColor = colors[COLOR_BUTTON];
	}

	// fill
	rect.InsetBy(1, 1);
	BGradientLinear gradient;
	gradient.SetStart(rect.LeftTop());
	gradient.SetEnd(rect.RightBottom());
	gradient.AddColor(startColor, 0);
	gradient.AddColor(endColor, 255);

	engine->FillRect(rect, gradient);

	// outline
	rect.InsetBy(-1, -1);
	engine->StrokeRect(rect, tint_color(colors[COLOR_BUTTON], B_DARKEN_2_TINT));
}


void
DefaultDecorator::_GetButtonSizeAndOffset(const BRect& tabRect, float* _offset,
	float* _size, float* _inset) const
{
	float tabSize = fLook == kLeftTitledWindowLook ?
		tabRect.Width() : tabRect.Height();

	bool smallTab = fLook == B_FLOATING_WINDOW_LOOK
		|| fLook == kLeftTitledWindowLook;

	*_offset = smallTab ? floorf(fDrawState.Font().Size() / 2.6)
		: floorf(fDrawState.Font().Size() / 2.3);
	*_inset = smallTab ? floorf(fDrawState.Font().Size() / 5.0)
		: floorf(fDrawState.Font().Size() / 6.0);

	// "+ 2" so that the rects are centered within the solid area
	// (without the 2 pixels for the top border)
	*_size = tabSize - 2 * *_offset + *_inset;
}


void
DefaultDecorator::_LayoutTabItems(Decorator::Tab* _tab, const BRect& tabRect)
{
	DefaultDecorator::Tab* tab = static_cast<DefaultDecorator::Tab*>(_tab);

	float offset;
	float size;
	float inset;
	_GetButtonSizeAndOffset(tabRect, &offset, &size, &inset);

	// default textOffset
	tab->textOffset = _DefaultTextOffset();

	BRect& closeRect = tab->closeRect;
	BRect& zoomRect = tab->zoomRect;

	// calulate close rect based on the tab rectangle
	if (fLook != kLeftTitledWindowLook) {
		closeRect.Set(tabRect.left + offset, tabRect.top + offset,
			tabRect.left + offset + size, tabRect.top + offset + size);

		zoomRect.Set(tabRect.right - offset - size, tabRect.top + offset,
			tabRect.right - offset, tabRect.top + offset + size);

		// hidden buttons have no width
		if ((Flags() & B_NOT_CLOSABLE) != 0)
			closeRect.right = closeRect.left - offset;
		if ((Flags() & B_NOT_ZOOMABLE) != 0)
			zoomRect.left = zoomRect.right + offset;
	} else {
		closeRect.Set(tabRect.left + offset, tabRect.top + offset,
			tabRect.left + offset + size, tabRect.top + offset + size);

		zoomRect.Set(tabRect.left + offset, tabRect.bottom - offset - size,
			tabRect.left + size + offset, tabRect.bottom - offset);

		// hidden buttons have no height
		if ((Flags() & B_NOT_CLOSABLE) != 0)
			closeRect.bottom = closeRect.top - offset;
		if ((Flags() & B_NOT_ZOOMABLE) != 0)
			zoomRect.top = zoomRect.bottom + offset;
	}

	// calculate room for title
	// TODO: the +2 is there because the title often appeared
	//	truncated for no apparent reason - OTOH the title does
	//	also not appear perfectly in the middle
	if (fLook != kLeftTitledWindowLook)
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


void
DefaultDecorator::_InvalidateBitmaps()
{
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		DefaultDecorator::Tab* tab = _TabAt(i);
		for (int32 index = 0; index < 4; index++) {
			tab->closeBitmaps[index] = NULL;
			tab->zoomBitmaps[index] = NULL;
		}
	}
}


ServerBitmap*
DefaultDecorator::_GetBitmapForButton(Decorator::Tab* tab, Component item,
	bool down, int32 width, int32 height)
{
	// TODO: the list of shared bitmaps is never freed
	struct decorator_bitmap {
		Component			item;
		bool				down;
		int32				width;
		int32				height;
		rgb_color			baseColor;
		rgb_color			lightColor;
		UtilityBitmap*		bitmap;
		decorator_bitmap*	next;
	};

	static BLocker sBitmapListLock("decorator lock", true);
	static decorator_bitmap* sBitmapList = NULL;

	ComponentColors colors;
	_GetComponentColors(item, colors, tab);

	BAutolock locker(sBitmapListLock);

	// search our list for a matching bitmap
	// TODO: use a hash map instead?
	decorator_bitmap* current = sBitmapList;
	while (current) {
		if (current->item == item && current->down == down
			&& current->width == width && current->height == height
			&& current->baseColor == colors[COLOR_BUTTON]
			&& current->lightColor == colors[COLOR_BUTTON_LIGHT]) {
			return current->bitmap;
		}

		current = current->next;
	}

	static BitmapDrawingEngine* sBitmapDrawingEngine = NULL;

	// didn't find any bitmap, create a new one
	if (sBitmapDrawingEngine == NULL)
		sBitmapDrawingEngine = new(std::nothrow) BitmapDrawingEngine();
	if (sBitmapDrawingEngine == NULL
		|| sBitmapDrawingEngine->SetSize(width, height) != B_OK)
		return NULL;

	BRect rect(0, 0, width - 1, height - 1);

	STRACE(("DefaultDecorator creating bitmap for %s %s at size %ldx%ld\n",
		item == COMPONENT_CLOSE_BUTTON ? "close" : "zoom",
		down ? "down" : "up", width, height));
	switch (item) {
		case COMPONENT_CLOSE_BUTTON:
			_DrawBlendedRect(sBitmapDrawingEngine, rect, down, colors);
			break;

		case COMPONENT_ZOOM_BUTTON:
		{
			// init the background
			sBitmapDrawingEngine->FillRect(rect, B_TRANSPARENT_COLOR);

			float inset = floorf(width / 4.0);
			BRect zoomRect(rect);
			zoomRect.left += inset;
			zoomRect.top += inset;
			_DrawBlendedRect(sBitmapDrawingEngine, zoomRect, down, colors);

			inset = floorf(width / 2.1);
			zoomRect = rect;
			zoomRect.right -= inset;
			zoomRect.bottom -= inset;
			_DrawBlendedRect(sBitmapDrawingEngine, zoomRect, down, colors);
			break;
		}

		default:
			break;
	}

	UtilityBitmap* bitmap = sBitmapDrawingEngine->ExportToBitmap(width, height,
		B_RGB32);
	if (bitmap == NULL)
		return NULL;

	// bitmap ready, put it into the list
	decorator_bitmap* entry = new(std::nothrow) decorator_bitmap;
	if (entry == NULL) {
		delete bitmap;
		return NULL;
	}

	entry->item = item;
	entry->down = down;
	entry->width = width;
	entry->height = height;
	entry->bitmap = bitmap;
	entry->baseColor = colors[COLOR_BUTTON];
	entry->lightColor = colors[COLOR_BUTTON_LIGHT];
	entry->next = sBitmapList;
	sBitmapList = entry;
	return bitmap;
}


void
DefaultDecorator::_GetComponentColors(Component component,
	ComponentColors _colors, Decorator::Tab* tab)
{
	// get the highlight for our component
	Region region = REGION_NONE;
	switch (component) {
		case COMPONENT_TAB:
			region = REGION_TAB;
			break;
		case COMPONENT_CLOSE_BUTTON:
			region = REGION_CLOSE_BUTTON;
			break;
		case COMPONENT_ZOOM_BUTTON:
			region = REGION_ZOOM_BUTTON;
			break;
		case COMPONENT_LEFT_BORDER:
			region = REGION_LEFT_BORDER;
			break;
		case COMPONENT_RIGHT_BORDER:
			region = REGION_RIGHT_BORDER;
			break;
		case COMPONENT_TOP_BORDER:
			region = REGION_TOP_BORDER;
			break;
		case COMPONENT_BOTTOM_BORDER:
			region = REGION_BOTTOM_BORDER;
			break;
		case COMPONENT_RESIZE_CORNER:
			region = REGION_RIGHT_BOTTOM_CORNER;
			break;
	}

	return GetComponentColors(component, RegionHighlight(region), _colors, tab);
}


float
DefaultDecorator::_DefaultTextOffset() const
{
	return (fLook == B_FLOATING_WINDOW_LOOK
		|| fLook == kLeftTitledWindowLook) ? 10 : 18;
}


float
DefaultDecorator::_SingleTabOffsetAndSize(float& tabSize)
{
	float maxLocation;
	if (fLook != kLeftTitledWindowLook) {
		tabSize = fRightBorder.right - fLeftBorder.left;
	} else {
		tabSize = fBottomBorder.bottom - fTopBorder.top;
	}
	DefaultDecorator::Tab* tab = _TabAt(0);
	maxLocation = tabSize - tab->maxTabSize;
	if (maxLocation < 0)
		maxLocation = 0;

	return floorf(tab->tabLocation * maxLocation);
}


void
DefaultDecorator::_CalculateTabsRegion()
{
	fTabsRegion.MakeEmpty();
	for (int32 i = 0; i < fTabList.CountItems(); i++)
		fTabsRegion.Include(fTabList.ItemAt(i)->tabRect);
}
