/*
 * Copyright 2001-2013, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		DarkWyrm, bpmagic@columbus.rr.com
 *		John Scipione, jscipione@gmail.com
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */


//! This one is more like the classic R5 look


#include "BeDecorator.h"

#include <algorithm>
#include <cmath>
#include <new>
#include <stdio.h>

#include <WindowPrivate.h>

#include <Autolock.h>
#include <Debug.h>
#include <GradientLinear.h>
#include <Rect.h>
#include <View.h>

#include "BitmapDrawingEngine.h"
#include "DesktopSettings.h"
#include "DrawingEngine.h"
#include "DrawState.h"
#include "FontManager.h"
#include "PatternHandler.h"
#include "RGBColor.h"
#include "ServerBitmap.h"


//#define DEBUG_DECORATOR
#ifdef DEBUG_DECORATOR
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif


BeDecorator::Tab::Tab()
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


//	#pragma mark -


BeDecorAddOn::BeDecorAddOn(image_id id, const char* name)
	:
	DecorAddOn(id, name)
{

}


Decorator*
BeDecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect)
{
	return new (std::nothrow)BeDecorator(settings, rect);
}


// TODO: get rid of DesktopSettings here, and introduce private accessor
//	methods to the Decorator base class

BeDecorator::BeDecorator(DesktopSettings& settings, BRect rect)
	:
	Decorator(settings, rect),
	fTabOffset(0),
	fWasDoubleClick(false)
{
	fFrameColors = new RGBColor[6];
	fFrameColors[0].SetColor(152, 152, 152);
	fFrameColors[1].SetColor(255, 255, 255);
	fFrameColors[2].SetColor(ui_color(B_MENU_BACKGROUND_COLOR));
	fFrameColors[3].SetColor(136, 136, 136);
	fFrameColors[4].SetColor(152, 152, 152);
	fFrameColors[5].SetColor(96, 96, 96);

	fTabColor.SetColor(ui_color(B_WINDOW_TAB_COLOR));
	fTextColor.SetColor(ui_color(B_WINDOW_TEXT_COLOR));

	fButtonHighColor.SetColor(tint_color(fTabColor.GetColor32(),
		B_LIGHTEN_2_TINT));
	fButtonLowColor.SetColor(tint_color(fTabColor.GetColor32(),
		B_DARKEN_2_TINT));

	//_UpdateFont(settings);

	// TODO: If the decorator was created with a frame too small, it should
	// resize itself!

	STRACE(("BeDecorator:\n"));
	STRACE(("\tFrame (%.1f,%.1f,%.1f,%.1f)\n",
		rect.left, rect.top, rect.right, rect.bottom));
}


BeDecorator::~BeDecorator()
{
	STRACE(("BeDecorator: ~BeDecorator()\n"));
	delete [] fFrameColors;
}


float
BeDecorator::TabLocation(int32 tab) const
{
	BeDecorator::Tab* decoratorTab = _TabAt(tab);
	if (decoratorTab == NULL)
		return 0.0f;

	return (float)decoratorTab->tabOffset;
}


bool
BeDecorator::GetSettings(BMessage* settings) const
{
	if (!fTitleBarRect.IsValid())
		return false;

	if (settings->AddRect("tab frame", fTitleBarRect) != B_OK)
		return false;

	if (settings->AddFloat("border width", fBorderWidth) != B_OK)
		return false;

	// TODO: only add the location of the tab of the window who requested the
	// settings
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		BeDecorator::Tab* tab = _TabAt(i);
		if (settings->AddFloat("tab location", (float)tab->tabOffset) != B_OK)
			return false;
	}

	return true;
}


// #pragma mark -


/*!	\brief Updates the decorator in the rectangular area \a updateRect.

	The default version updates all areas which intersect the frame and tab.

	\param updateRect The rectangular area to update.
*/
void
BeDecorator::Draw(BRect updateRect)
{
	STRACE(("BeDecorator::Draw(BRect updateRect): "));
	updateRect.PrintToStream();

	// We need to draw a few things: the tab, the resize knob, the borders,
	// and the buttons
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(updateRect);
	_DrawTabs(updateRect & fTitleBarRect);
}


void
BeDecorator::Draw()
{
	// Easy way to draw everything - no worries about drawing only certain
	// things
	fDrawingEngine->SetDrawState(&fDrawState);

	_DrawFrame(BRect(fTopBorder.LeftTop(), fBottomBorder.RightBottom()));
	_DrawTabs(fTitleBarRect);
}


void
BeDecorator::GetSizeLimits(int32* minWidth, int32* minHeight,
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
BeDecorator::RegionAt(BPoint where, int32& tab) const
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


void
BeDecorator::_DoLayout()
{
	STRACE(("BeDecorator: Do Layout\n"));
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
BeDecorator::_DoTabLayout()
{
	float tabOffset = 0;
	if (fTabList.CountItems() == 1) {
		float tabSize;
		tabOffset = _SingleTabOffsetAndSize(tabSize);
	}

	float sumTabWidth = 0;
	// calculate our tab rect
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		BeDecorator::Tab* tab = _TabAt(i);

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


static bool
int_equal(float x, float y)
{
	return abs(x - y) <= 1;
}


void
BeDecorator::_DistributeTabSize(float delta)
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
		BeDecorator::Tab* tab = _TabAt(i);
		tab->tabOffset = uint32(tab->tabRect.left - fLeftBorder.left);
	}
}


Decorator::Tab*
BeDecorator::_AllocateNewTab()
{
	Decorator::Tab* tab = new(std::nothrow) BeDecorator::Tab;
	if (tab == NULL)
		return NULL;
	// Set appropriate colors based on the current focus value. In this case,
	// each decorator defaults to not having the focus.
	_SetFocus(tab);
	return tab;
}


BeDecorator::Tab*
BeDecorator::_TabAt(int32 index) const
{
	return static_cast<BeDecorator::Tab*>(fTabList.ItemAt(index));
}


void
BeDecorator::_DrawFrame(BRect invalid)
{
	STRACE(("_DrawFrame(%f,%f,%f,%f)\n", invalid.left, invalid.top,
		invalid.right, invalid.bottom));

	// NOTE: the DrawingEngine needs to be locked for the entire
	// time for the clipping to stay valid for this decorator

	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK)
		return;

	if (fBorderWidth <= 0)
		return;

	// Draw the border frame
	BRect r = BRect(fTopBorder.LeftTop(), fBottomBorder.RightBottom());
	switch ((int)fTopTab->look) {
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
		case B_MODAL_WINDOW_LOOK:
		{
			// top
			if (invalid.Intersects(fTopBorder)) {
				for (int8 i = 0; i < 5; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.top + i),
						BPoint(r.right - i, r.top + i), fFrameColors[i]);
				}
				if (fTitleBarRect.IsValid()) {
					// grey along the bottom of the tab
					// (overwrites "white" from frame)
					fDrawingEngine->StrokeLine(
						BPoint(fTitleBarRect.left + 2,
							fTitleBarRect.bottom + 1),
						BPoint(fTitleBarRect.right - 2,
							fTitleBarRect.bottom + 1),
						fFrameColors[2]);
				}
			}
			// left
			if (invalid.Intersects(fLeftBorder.InsetByCopy(0, -fBorderWidth))) {
				for (int8 i = 0; i < 5; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.top + i),
						BPoint(r.left + i, r.bottom - i), fFrameColors[i]);
				}
			}
			// bottom
			if (invalid.Intersects(fBottomBorder)) {
				for (int8 i = 0; i < 5; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.bottom - i),
						BPoint(r.right - i, r.bottom - i),
						fFrameColors[(4 - i) == 4 ? 5 : (4 - i)]);
				}
			}
			// right
			if (invalid.Intersects(fRightBorder.InsetByCopy(0, -fBorderWidth))) {
				for (int8 i = 0; i < 5; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.right - i, r.top + i),
						BPoint(r.right - i, r.bottom - i),
						fFrameColors[(4 - i) == 4 ? 5 : (4 - i)]);
				}
			}
			break;
		}

		case B_FLOATING_WINDOW_LOOK:
		case kLeftTitledWindowLook:
		{
			// top
			if (invalid.Intersects(fTopBorder)) {
				for (int8 i = 0; i < 3; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.top + i),
						BPoint(r.right - i, r.top + i), fFrameColors[i * 2]);
				}
				if (fTitleBarRect.IsValid() && fTopTab->look != kLeftTitledWindowLook) {
					// grey along the bottom of the tab
					// (overwrites "white" from frame)
					fDrawingEngine->StrokeLine(
						BPoint(fTitleBarRect.left + 2,
							fTitleBarRect.bottom + 1),
						BPoint(fTitleBarRect.right - 2,
							fTitleBarRect.bottom + 1), fFrameColors[2]);
				}
			}
			// left
			if (invalid.Intersects(fLeftBorder.InsetByCopy(0, -fBorderWidth))) {
				for (int8 i = 0; i < 3; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.top + i),
						BPoint(r.left + i, r.bottom - i), fFrameColors[i * 2]);
				}
				if (fTopTab->look == kLeftTitledWindowLook
					&& fTitleBarRect.IsValid()) {
					// grey along the right side of the tab
					// (overwrites "white" from frame)
					fDrawingEngine->StrokeLine(
						BPoint(fTitleBarRect.right + 1,
							fTitleBarRect.top + 2),
						BPoint(fTitleBarRect.right + 1,
							fTitleBarRect.bottom - 2), fFrameColors[2]);
				}
			}
			// bottom
			if (invalid.Intersects(fBottomBorder)) {
				for (int8 i = 0; i < 3; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.bottom - i),
						BPoint(r.right - i, r.bottom - i),
						fFrameColors[(2 - i) == 2 ? 5 : (2 - i) * 2]);
				}
			}
			// right
			if (invalid.Intersects(fRightBorder.InsetByCopy(0, -fBorderWidth))) {
				for (int8 i = 0; i < 3; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.right - i, r.top + i),
						BPoint(r.right - i, r.bottom - i),
						fFrameColors[(2 - i) == 2 ? 5 : (2 - i) * 2]);
				}
			}
			break;
		}

		case B_BORDERED_WINDOW_LOOK:
		{
			// TODO: Draw the borders individually!
			fDrawingEngine->StrokeRect(r, fFrameColors[5]);
			break;
		}

		default:
			// don't draw a border frame
			break;
	}

	// Draw the resize knob if we're supposed to
	if (!(fTopTab->flags & B_NOT_RESIZABLE)) {
		r = fResizeRect;

		switch ((int)fTopTab->look) {
			case B_DOCUMENT_WINDOW_LOOK:
			{
				if (!invalid.Intersects(r))
					break;

				float x = r.right - 3;
				float y = r.bottom - 3;

				fDrawingEngine->FillRect(BRect(x - 13, y - 13, x, y),
					fFrameColors[2]);

				fDrawingEngine->StrokeLine(BPoint(x - 15, y - 15),
					BPoint(x - 15, y - 2), fFrameColors[0]);
				fDrawingEngine->StrokeLine(BPoint(x - 14, y - 14),
					BPoint(x - 14, y - 1), fFrameColors[1]);
				fDrawingEngine->StrokeLine(BPoint(x - 15, y - 15),
					BPoint(x - 2, y - 15), fFrameColors[0]);
				fDrawingEngine->StrokeLine(BPoint(x - 14, y - 14),
					BPoint(x - 1, y - 14), fFrameColors[1]);

				if (!IsFocus(fTopTab))
					break;

				for (int8 i = 1; i <= 4; i++) {
					for (int8 j = 1; j <= i; j++) {
						BPoint pt1(x - (3 * j) + 1, y - (3 * (5 - i)) + 1);
						BPoint pt2(x - (3 * j) + 2, y - (3 * (5 - i)) + 2);
						fDrawingEngine->StrokePoint(pt1, fFrameColors[0]);
						fDrawingEngine->StrokePoint(pt2, fFrameColors[1]);
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
					fFrameColors[0]);
				fDrawingEngine->StrokeLine(
					BPoint(fRightBorder.right - kBorderResizeLength, fBottomBorder.top),
					BPoint(fRightBorder.right - kBorderResizeLength, fBottomBorder.bottom - 1),
					fFrameColors[0]);
				break;
			}

			default:
				// don't draw resize corner
				break;
		}
	}
}


void
BeDecorator::_DrawTab(Decorator::Tab* tab, BRect invalid)
{
	STRACE(("_DrawTab(%.1f, %.1f, %.1f, %.1f)\n",
			invalid.left, invalid.top, invalid.right, invalid.bottom));
	const BRect& tabRect = tab->tabRect;
	// If a window has a tab, this will draw it and any buttons which are
	// in it.
	if (!tabRect.IsValid() || !invalid.Intersects(tabRect))
		return;

	if (fBorderWidth <= 0)
		return;

	// TODO: cache these
	RGBColor tabColorLight = RGBColor(tint_color(fTabColor.GetColor32(),
		B_LIGHTEN_2_TINT));
	RGBColor tabColorShadow = RGBColor(tint_color(fTabColor.GetColor32(),
		B_DARKEN_2_TINT));

	// outer frame
	fDrawingEngine->StrokeLine(tabRect.LeftTop(), tabRect.LeftBottom(),
		fFrameColors[0]);
	fDrawingEngine->StrokeLine(tabRect.LeftTop(), tabRect.RightTop(),
		fFrameColors[0]);
	if (fTopTab->look != kLeftTitledWindowLook) {
		fDrawingEngine->StrokeLine(tabRect.RightTop(), tabRect.RightBottom(),
			fFrameColors[5]);
	} else {
		fDrawingEngine->StrokeLine(tabRect.LeftBottom(),
			tabRect.RightBottom(), fFrameColors[5]);
	}

	// bevel
	fDrawingEngine->StrokeLine(BPoint(tabRect.left + 1, tabRect.top + 1),
		BPoint(tabRect.left + 1,
			tabRect.bottom - (fTopTab->look == kLeftTitledWindowLook ? 1 : 0)),
		tabColorLight);
	fDrawingEngine->StrokeLine(BPoint(tabRect.left + 1, tabRect.top + 1),
		BPoint(tabRect.right - (fTopTab->look == kLeftTitledWindowLook ? 0 : 1),
			tabRect.top + 1),
		tabColorLight);

	if (fTopTab->look != kLeftTitledWindowLook) {
		fDrawingEngine->StrokeLine(BPoint(tabRect.right - 1, tabRect.top + 2),
			BPoint(tabRect.right - 1, tabRect.bottom), tabColorShadow);
	} else {
		fDrawingEngine->StrokeLine(
			BPoint(tabRect.left + 2, tabRect.bottom - 1),
			BPoint(tabRect.right, tabRect.bottom - 1), tabColorShadow);
	}

	// fill
	if (fTopTab->look != kLeftTitledWindowLook) {
		fDrawingEngine->FillRect(BRect(tabRect.left + 2, tabRect.top + 2,
			tabRect.right - 2, tabRect.bottom), fTabColor);
	} else {
		fDrawingEngine->FillRect(BRect(tabRect.left + 2, tabRect.top + 2,
			tabRect.right, tabRect.bottom - 2), fTabColor);
	}

	_DrawTitle(tab, tabRect);

	DrawButtons(tab, invalid);
}


void
BeDecorator::_DrawTitle(Decorator::Tab* _tab, BRect r)
{
	STRACE(("_DrawTitle(%f, %f, %f, %f)\n", r.left, r.top, r.right, r.bottom));

	BeDecorator::Tab* tab = static_cast<BeDecorator::Tab*>(_tab);

	const BRect& tabRect = tab->tabRect;
	const BRect& closeRect = tab->closeRect;
	const BRect& zoomRect = tab->zoomRect;

	fDrawingEngine->SetDrawingMode(B_OP_OVER);
	fDrawingEngine->SetHighColor(fTextColor);
	fDrawingEngine->SetLowColor(fTabColor);
	fDrawingEngine->SetFont(fDrawState.Font());

	// figure out position of text
	font_height fontHeight;
	fDrawState.Font().GetHeight(fontHeight);

	BPoint titlePos;
	if (fTopTab->look != kLeftTitledWindowLook) {
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

	fDrawingEngine->SetFont(fDrawState.Font());

	fDrawingEngine->DrawString(tab->truncatedTitle.String(), tab->truncatedTitleLength,
		titlePos);

	fDrawingEngine->SetDrawingMode(B_OP_COPY);
}


void
BeDecorator::_DrawClose(Decorator::Tab* tab, bool direct, BRect rect)
{
	STRACE(("_DrawClose(%f,%f,%f,%f)\n", rect.left, rect.top, rect.right,
		rect.bottom));
	// Just like DrawZoom, but for a close button
	_DrawBlendedRect(rect, true);
}


void
BeDecorator::_DrawZoom(Decorator::Tab* tab, bool direct, BRect rect)
{
	STRACE(("_DrawZoom(%f,%f,%f,%f)\n", rect.left, rect.top, rect.right,
		rect.bottom));
	// If this has been implemented, then the decorator has a Zoom button
	// which should be drawn based on the state of the member zoomstate

	BRect zr(rect);
	zr.left += 3.0;
	zr.top += 3.0;
	_DrawBlendedRect(zr, true);

	zr = rect;
	zr.right -= 5.0;
	zr.bottom -= 5.0;
	_DrawBlendedRect(zr, true);
}


void
BeDecorator::_SetTitle(Decorator::Tab* tab, const char* string,
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
BeDecorator::_FontsChanged(DesktopSettings& settings,
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
BeDecorator::_SetLook(Decorator::Tab* tab, DesktopSettings& settings,
	window_look look, BRegion* updateRegion)
{
	// TODO: we could be much smarter about the update region

	// get previous extent
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());

	tab->look = look;

	_UpdateFont(settings);
	_InvalidateBitmaps();
	_DoLayout();

	_InvalidateFootprint();
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());
}


void
BeDecorator::_SetFlags(Decorator::Tab* tab, uint32 flags,
	BRegion* updateRegion)
{
	// TODO: we could be much smarter about the update region

	// get previous extent
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());

	tab->flags = flags;
	_DoLayout();

	_InvalidateFootprint();
	if (updateRegion != NULL)
		updateRegion->Include(&GetFootprint());
}


void
BeDecorator::_SetFocus(Decorator::Tab* _tab)
{
	BeDecorator::Tab* tab = static_cast<BeDecorator::Tab*>(_tab);
	tab->buttonFocus = IsFocus(tab)
		|| ((tab->look == B_FLOATING_WINDOW_LOOK
			|| tab->look == kLeftTitledWindowLook)
			&& (tab->flags & B_AVOID_FOCUS) != 0);
	if (CountTabs() > 1)
		_LayoutTabItems(tab, tab->tabRect);
}


void
BeDecorator::_MoveBy(BPoint offset)
{
	STRACE(("BeDecorator: Move By (%.1f, %.1f)\n", offset.x, offset.y));
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
BeDecorator::_ResizeBy(BPoint offset, BRegion* dirty)
{
	STRACE(("BeDecorator: Resize By (%.1f, %.1f)\n", offset.x, offset.y));
	// Move all internal rectangles the appropriate amount
	fFrame.right += offset.x;
	fFrame.bottom += offset.y;

	// Handle invalidation of resize rect
	if (dirty && !(fTopTab->flags & B_NOT_RESIZABLE)) {
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
					fBottomBorder.top, fRightBorder.right - kBorderResizeLength,
					fBottomBorder.bottom - 1);
				// Old location
				dirty->Include(realResizeRect);
				realResizeRect.OffsetBy(offset);
				// New location
				dirty->Include(realResizeRect);

				// The right border resize line
				realResizeRect.Set(fRightBorder.left,
					fBottomBorder.bottom - kBorderResizeLength, fRightBorder.right - 1,
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

		BeDecorator::Tab* tab = _TabAt(0);
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


bool
BeDecorator::_SetTabLocation(Decorator::Tab* _tab, float location,
	bool isShifting, BRegion* updateRegion)
{
	STRACE(("BeDecorator: Set Tab Location(%.1f)\n", location));
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

	BeDecorator::Tab* tab = static_cast<BeDecorator::Tab*>(_tab);
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
BeDecorator::_SetSettings(const BMessage& settings, BRegion* updateRegion)
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
BeDecorator::_AddTab(DesktopSettings& settings, int32 index,
	BRegion* updateRegion)
{
	_UpdateFont(settings);

	_DoLayout();
	if (updateRegion != NULL)
		updateRegion->Include(fTitleBarRect);
	return true;
}


bool
BeDecorator::_RemoveTab(int32 index, BRegion* updateRegion)
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
BeDecorator::_MoveTab(int32 from, int32 to, bool isMoving,
	BRegion* updateRegion)
{
	BeDecorator::Tab* toTab = _TabAt(to);
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
BeDecorator::_GetFootprint(BRegion *region)
{
	STRACE(("BeDecorator: Get Footprint\n"));
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
BeDecorator::DrawButtons(Decorator::Tab* tab, const BRect& invalid)
{
	// Draw the buttons if we're supposed to
	if (!(tab->flags & B_NOT_CLOSABLE) && invalid.Intersects(tab->closeRect))
		_DrawClose(tab, false, tab->closeRect);
	if (!(tab->flags & B_NOT_ZOOMABLE) && invalid.Intersects(tab->zoomRect))
		_DrawZoom(tab, false, tab->zoomRect);
}


void
BeDecorator::_UpdateFont(DesktopSettings& settings)
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


/*!
	\brief Draws a framed rectangle with a gradient.
	\param down The rectangle should be drawn recessed or not
*/
void
BeDecorator::_DrawBlendedRect(BRect r, bool down)
{
	BRect r1 = r;
	BRect r2 = r;

	r1.left += 1.0;
	r1.top  += 1.0;

	r2.bottom -= 1.0;
	r2.right  -= 1.0;

	// TODO: replace these with cached versions? does R5 use different colours?
	RGBColor tabColorLight = RGBColor(tint_color(fTabColor.GetColor32(),
		B_LIGHTEN_2_TINT));
	RGBColor tabColorShadow = RGBColor(tint_color(fTabColor.GetColor32(),
		B_DARKEN_2_TINT));

	int32 w = r.IntegerWidth();
	int32 h = r.IntegerHeight();

	RGBColor temprgbcol;
	rgb_color halfColor, startColor, endColor;
	float rstep, gstep, bstep;

	int steps = w < h ? w : h;

	if (down) {
		startColor = fButtonLowColor.GetColor32();
		endColor = fButtonHighColor.GetColor32();
	} else {
		startColor = fButtonHighColor.GetColor32();
		endColor = fButtonLowColor.GetColor32();
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


void
BeDecorator::_GetButtonSizeAndOffset(const BRect& tabRect, float* _offset,
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
BeDecorator::_LayoutTabItems(Decorator::Tab* _tab, const BRect& tabRect)
{
	BeDecorator::Tab* tab = static_cast<BeDecorator::Tab*>(_tab);

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


void
BeDecorator::_InvalidateBitmaps()
{
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		BeDecorator::Tab* tab = _TabAt(i);
		for (int32 index = 0; index < 4; index++) {
			tab->closeBitmaps[index] = NULL;
			tab->zoomBitmaps[index] = NULL;
		}
	}
}


float
BeDecorator::_DefaultTextOffset() const
{
	return (fTopTab->look == B_FLOATING_WINDOW_LOOK
		|| fTopTab->look == kLeftTitledWindowLook) ? 10 : 18;
}


float
BeDecorator::_SingleTabOffsetAndSize(float& tabSize)
{
	float maxLocation;
	if (fTopTab->look != kLeftTitledWindowLook) {
		tabSize = fRightBorder.right - fLeftBorder.left;
	} else {
		tabSize = fBottomBorder.bottom - fTopBorder.top;
	}
	BeDecorator::Tab* tab = _TabAt(0);
	maxLocation = tabSize - tab->maxTabSize;
	if (maxLocation < 0)
		maxLocation = 0;

	return floorf(tab->tabLocation * maxLocation);
}


void
BeDecorator::_CalculateTabsRegion()
{
	fTabsRegion.MakeEmpty();
	for (int32 i = 0; i < fTabList.CountItems(); i++)
		fTabsRegion.Include(fTabList.ItemAt(i)->tabRect);
}


extern "C" DecorAddOn* (instantiate_decor_addon)(image_id id, const char* name)
{
	return new (std::nothrow)BeDecorAddOn(id, name);
}
