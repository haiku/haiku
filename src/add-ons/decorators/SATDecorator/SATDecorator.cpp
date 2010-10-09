/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */

#include "SATDecorator.h"

#include <new>

#include <GradientLinear.h>
#include <WindowPrivate.h>

#include "DrawingEngine.h"


//#define DEBUG_SATDECORATOR
#ifdef DEBUG_SATDECORATOR
#	define STRACE(x) debug_printf x
#else
#	define STRACE(x) ;
#endif


static const float kResizeKnobSize = 18.0;


SATDecorAddOn::SATDecorAddOn(image_id id, const char* name)
	:
	DecorAddOn(id, name)
{
	fDesktopListeners.AddItem(&fStackAndTile);
}


status_t
SATDecorAddOn::InitCheck() const
{
	if (fDesktopListeners.CountItems() != 1)
		return B_ERROR;

	return B_OK;
}


float
SATDecorAddOn::Version()
{
	return 0.1;
}


Decorator*
SATDecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect,
	window_look look, uint32 flags)
{
	return new (std::nothrow)SATDecorator(settings, rect, look, flags);
}


SATDecorator::SATDecorator(DesktopSettings& settings, BRect frame,
	window_look look, uint32 flags)
	:
	DefaultDecorator(settings, frame, look, flags),

	fTabHighlighted(false),
	fBordersHighlighted(false),

	fStackedMode(false),
	fStackedTabLength(0)
{
	fStackedDrawZoom = IsFocus();

	// all colors are state based
	fNonHighlightFrameColors[0] = (rgb_color){ 152, 152, 152, 255 };
	fNonHighlightFrameColors[1] = (rgb_color){ 240, 240, 240, 255 };
	fNonHighlightFrameColors[2] = (rgb_color){ 152, 152, 152, 255 };
	fNonHighlightFrameColors[3] = (rgb_color){ 108, 108, 108, 255 };


	fHighlightFrameColors[0] = (rgb_color){ 152, 0, 0, 255 };
	fHighlightFrameColors[1] = (rgb_color){ 240, 0, 0, 255 };
	fHighlightFrameColors[2] = (rgb_color){ 224, 0, 0, 255 };
	fHighlightFrameColors[3] = (rgb_color){ 208, 0, 0, 255 };
	fHighlightFrameColors[4] = (rgb_color){ 152, 0, 0, 255 };
	fHighlightFrameColors[5] = (rgb_color){ 108, 0, 0, 255 };

	// initial colors
	fFrameColors[0] = fNonHighlightFrameColors[0];
	fFrameColors[1] = fNonHighlightFrameColors[1];
	fFrameColors[4] = fNonHighlightFrameColors[2];
	fFrameColors[5] = fNonHighlightFrameColors[3];

	fHighlightTabColor = (rgb_color){ 255, 0, 0, 255 };
}


void
SATDecorator::HighlightTab(bool active, BRegion* dirty)
{
	if (active)
		fTabColor = fHighlightTabColor;
	else if (IsFocus())
		fTabColor = fFocusTabColor;
	else
		fTabColor = fNonFocusTabColor;
	dirty->Include(fTabRect);
	fTabHighlighted = active;
}


void
SATDecorator::HighlightBorders(bool active, BRegion* dirty)
{
	if (active) {
		fFrameColors[0] = fHighlightFrameColors[0];
		fFrameColors[1] = fHighlightFrameColors[1];
		fFrameColors[2] = fHighlightFrameColors[2];
		fFrameColors[3] = fHighlightFrameColors[3];
		fFrameColors[4] = fHighlightFrameColors[4];
		fFrameColors[5] = fHighlightFrameColors[5];
	} else if (IsFocus()) {
		fFrameColors[0] = fNonHighlightFrameColors[0];
		fFrameColors[1] = fNonHighlightFrameColors[1];
		fFrameColors[2] = fFocusFrameColors[0];
		fFrameColors[3] = fFocusFrameColors[1];
		fFrameColors[4] = fNonHighlightFrameColors[2];
		fFrameColors[5] = fNonHighlightFrameColors[3];
	} else {
		fFrameColors[0] = fNonHighlightFrameColors[0];
		fFrameColors[1] = fNonHighlightFrameColors[1];
		fFrameColors[2] = fNonFocusFrameColors[0];
		fFrameColors[3] = fNonFocusFrameColors[1];
		fFrameColors[4] = fNonHighlightFrameColors[2];
		fFrameColors[5] = fNonHighlightFrameColors[3];
	}
	dirty->Include(fLeftBorder);
	dirty->Include(fRightBorder);
	dirty->Include(fTopBorder);
	dirty->Include(fBottomBorder);
	dirty->Include(fResizeRect);
	fBordersHighlighted = active;
}


void
SATDecorator::SetStackedMode(bool stacked, BRegion* dirty)
{
	fStackedMode = stacked;

	dirty->Include(fTabRect);
	_DoLayout();
	_InvalidateFootprint();
	dirty->Include(fTabRect);
}


void
SATDecorator::SetStackedTabLength(float length, BRegion* dirty)
{
	fStackedTabLength = length;

	dirty->Include(fTabRect);
	_DoLayout();
	_InvalidateFootprint();
	dirty->Include(fTabRect);
}


void
SATDecorator::_DoLayout()
{
	STRACE(("DefaultDecorator: Do Layout\n"));
	// Here we determine the size of every rectangle that we use
	// internally when we are given the size of the client rectangle.

	bool hasTab = false;

	switch (Look()) {
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

	// calculate our tab rect
	if (hasTab) {
		// distance from one item of the tab bar to another.
		// In this case the text and close/zoom rects
		fTextOffset = (fLook == B_FLOATING_WINDOW_LOOK
			|| fLook == kLeftTitledWindowLook) ? 10 : 18;

		font_height fontHeight;
		fDrawState.Font().GetHeight(fontHeight);

		if (fLook != kLeftTitledWindowLook) {
			fTabRect.Set(fFrame.left - fBorderWidth,
				fFrame.top - fBorderWidth
					- ceilf(fontHeight.ascent + fontHeight.descent + 7.0),
				((fFrame.right - fFrame.left) < 35.0 ?
					fFrame.left + 35.0 : fFrame.right) + fBorderWidth,
				fFrame.top - fBorderWidth);
		} else {
			fTabRect.Set(fFrame.left - fBorderWidth
				- ceilf(fontHeight.ascent + fontHeight.descent + 5.0),
					fFrame.top - fBorderWidth, fFrame.left - fBorderWidth,
				fFrame.bottom + fBorderWidth);
		}

		// format tab rect for a floating window - make the rect smaller
		if (fLook == B_FLOATING_WINDOW_LOOK) {
			fTabRect.InsetBy(0, 2);
			fTabRect.OffsetBy(0, 2);
		}

		if (fStackedMode)
			fTabRect.right = fTabRect.left + fStackedTabLength;

		float offset;
		float size;
		float inset;
		_GetButtonSizeAndOffset(fTabRect, &offset, &size, &inset);

		// fMinTabSize contains just the room for the buttons
		fMinTabSize = inset * 2 + fTextOffset;
		if ((fFlags & B_NOT_CLOSABLE) == 0)
			fMinTabSize += offset + size;
		if ((fFlags & B_NOT_ZOOMABLE) == 0)
			fMinTabSize += offset + size;

		// fMaxTabSize contains fMinWidth + the width required for the title
		fMaxTabSize = fDrawingEngine
			? ceilf(fDrawingEngine->StringWidth(Title(), strlen(Title()),
				fDrawState.Font())) : 0.0;
		if (fMaxTabSize > 0.0)
			fMaxTabSize += fTextOffset;
		fMaxTabSize += fMinTabSize;

		float tabSize = (fLook != kLeftTitledWindowLook
			? fFrame.Width() : fFrame.Height()) + fBorderWidth * 2;

		if (fStackedMode) {
			tabSize = fStackedTabLength;
			fMaxTabSize = tabSize;
		}
		else {
			if (tabSize < fMinTabSize)
				tabSize = fMinTabSize;
			if (tabSize > fMaxTabSize)
				tabSize = fMaxTabSize;
		}
		// layout buttons and truncate text
		if (fLook != kLeftTitledWindowLook)
			fTabRect.right = fTabRect.left + tabSize;
		else
			fTabRect.bottom = fTabRect.top + tabSize;
	} else {
		// no tab
		fMinTabSize = 0.0;
		fMaxTabSize = 0.0;
		fTabRect.Set(0.0, 0.0, -1.0, -1.0);
		fCloseRect.Set(0.0, 0.0, -1.0, -1.0);
		fZoomRect.Set(0.0, 0.0, -1.0, -1.0);
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
		// make sure fTabOffset is within limits and apply it to
		// the fTabRect
		if (fTabOffset < 0)
			fTabOffset = 0;
		if (fTabLocation != 0.0
			&& fTabOffset > (fRightBorder.right - fLeftBorder.left
				- fTabRect.Width()))
			fTabOffset = uint32(fRightBorder.right - fLeftBorder.left
				- fTabRect.Width());
		fTabRect.OffsetBy(fTabOffset, 0);

		// finally, layout the buttons and text within the tab rect
		_LayoutTabItems(fTabRect);
	}
}


void
SATDecorator::_LayoutTabItems(const BRect& tabRect)
{
	float offset;
	float size;
	float inset;
	_GetButtonSizeAndOffset(tabRect, &offset, &size, &inset);

	// calulate close rect based on the tab rectangle
	if (fLook != kLeftTitledWindowLook) {
		fCloseRect.Set(tabRect.left + offset, tabRect.top + offset,
			tabRect.left + offset + size, tabRect.top + offset + size);

		fZoomRect.Set(tabRect.right - offset - size, tabRect.top + offset,
			tabRect.right - offset, tabRect.top + offset + size);

		// hidden buttons have no width
		if ((Flags() & B_NOT_CLOSABLE) != 0)
			fCloseRect.right = fCloseRect.left - offset;
		if ((Flags() & B_NOT_ZOOMABLE) != 0)
			fZoomRect.left = fZoomRect.right + offset;
	} else {
		fCloseRect.Set(tabRect.left + offset, tabRect.top + offset,
			tabRect.left + offset + size, tabRect.top + offset + size);

		fZoomRect.Set(tabRect.left + offset, tabRect.bottom - offset - size,
			tabRect.left + size + offset, tabRect.bottom - offset);

		// hidden buttons have no height
		if ((Flags() & B_NOT_CLOSABLE) != 0)
			fCloseRect.bottom = fCloseRect.top - offset;
		if ((Flags() & B_NOT_ZOOMABLE) != 0)
			fZoomRect.top = fZoomRect.bottom + offset;
	}

	// calculate room for title
	// TODO: the +2 is there because the title often appeared
	//	truncated for no apparent reason - OTOH the title does
	//	also not appear perfectly in the middle
	if (fLook != kLeftTitledWindowLook)
		size = (fZoomRect.left - fCloseRect.right) - fTextOffset * 2 + inset;
	else
		size = (fZoomRect.top - fCloseRect.bottom) - fTextOffset * 2 + inset;

	if (fStackedMode && !fStackedDrawZoom) {
		fZoomRect.Set(0, 0, 0, 0);
		size = (fTabRect.right - fCloseRect.right) - fTextOffset * 2 + inset;
	}
	uint8 truncateMode = B_TRUNCATE_MIDDLE;

	if (fStackedMode) {
		if (fStackedTabLength < 100)
			truncateMode = B_TRUNCATE_END;
		float titleWidth = fDrawState.Font().StringWidth(Title(),
			BString(Title()).Length());
		if (size < titleWidth) {
			float oldTextOffset = fTextOffset;
			fTextOffset -= (titleWidth - size) / 2;
			const float kMinTextOffset = 5.;
			if (fTextOffset < kMinTextOffset)
				fTextOffset = kMinTextOffset;
			size += oldTextOffset * 2;
			size -= fTextOffset * 2;
		}
	}

	fTruncatedTitle = Title();
	fDrawState.Font().TruncateString(&fTruncatedTitle, truncateMode, size);
	fTruncatedTitleLength = fTruncatedTitle.Length();
}


void
SATDecorator::_DrawTab(BRect invalid)
{
	STRACE(("_DrawTab(%.1f,%.1f,%.1f,%.1f)\n",
			invalid.left, invalid.top, invalid.right, invalid.bottom));
	// If a window has a tab, this will draw it and any buttons which are
	// in it.
	if (!fTabRect.IsValid() || !invalid.Intersects(fTabRect))
		return;

	// outer frame
	fDrawingEngine->StrokeLine(fTabRect.LeftTop(), fTabRect.LeftBottom(),
		fFrameColors[0]);
	fDrawingEngine->StrokeLine(fTabRect.LeftTop(), fTabRect.RightTop(),
		fFrameColors[0]);
	if (fLook != kLeftTitledWindowLook) {
		fDrawingEngine->StrokeLine(fTabRect.RightTop(), fTabRect.RightBottom(),
			fFrameColors[5]);
	} else {
		fDrawingEngine->StrokeLine(fTabRect.LeftBottom(),
			fTabRect.RightBottom(), fFrameColors[5]);
	}

	// bevel
	fDrawingEngine->StrokeLine(BPoint(fTabRect.left + 1, fTabRect.top + 1),
		BPoint(fTabRect.left + 1,
			fTabRect.bottom - (fLook == kLeftTitledWindowLook ? 1 : 0)),
		fTabColorBevel);
	fDrawingEngine->StrokeLine(BPoint(fTabRect.left + 1, fTabRect.top + 1),
		BPoint(fTabRect.right - (fLook == kLeftTitledWindowLook ? 0 : 1),
			fTabRect.top + 1),
		fTabColorBevel);

	if (fLook != kLeftTitledWindowLook) {
		fDrawingEngine->StrokeLine(BPoint(fTabRect.right - 1, fTabRect.top + 2),
			BPoint(fTabRect.right - 1, fTabRect.bottom), fTabColorShadow);
	} else {
		fDrawingEngine->StrokeLine(
			BPoint(fTabRect.left + 2, fTabRect.bottom - 1),
			BPoint(fTabRect.right, fTabRect.bottom - 1), fTabColorShadow);
	}

	// fill
	BGradientLinear gradient;
	gradient.SetStart(fTabRect.LeftTop());
	gradient.AddColor(fTabColorLight, 0);
	gradient.AddColor(fTabColor, 255);

	if (fLook != kLeftTitledWindowLook) {
		gradient.SetEnd(fTabRect.LeftBottom());
		fDrawingEngine->FillRect(BRect(fTabRect.left + 2, fTabRect.top + 2,
			fTabRect.right - 2, fTabRect.bottom), gradient);
	} else {
		gradient.SetEnd(fTabRect.RightTop());
		fDrawingEngine->FillRect(BRect(fTabRect.left + 2, fTabRect.top + 2,
			fTabRect.right, fTabRect.bottom - 2), gradient);
	}

	_DrawTitle(fTabRect);

	// Draw the buttons if we're supposed to
	if (!(fFlags & B_NOT_CLOSABLE) && invalid.Intersects(fCloseRect))
		_DrawClose(fCloseRect);
	
	if (fStackedMode) {
		if (fStackedDrawZoom && invalid.Intersects(fZoomRect))
			_DrawZoom(fZoomRect);
	}
	else if (!(fFlags & B_NOT_ZOOMABLE) && invalid.Intersects(fZoomRect))
		_DrawZoom(fZoomRect);
}


bool
SATDecorator::_SetTabLocation(float location, BRegion* updateRegion)
{
	STRACE(("DefaultDecorator: Set Tab Location(%.1f)\n", location));
	if (!fTabRect.IsValid())
		return false;

	if (location < 0)
		location = 0;

	float maxLocation = 0.;
	if (fStackedMode)
		maxLocation = fRightBorder.right - fLeftBorder.left - fStackedTabLength;
	else
		maxLocation = fRightBorder.right - fLeftBorder.left - fTabRect.Width();
	if (location > maxLocation)
		location = maxLocation;

	float delta = location - fTabOffset;
	if (delta == 0.0)
		return false;

	// redraw old rect (1 pix on the border also must be updated)
	BRect trect(fTabRect);
	trect.bottom++;
	updateRegion->Include(trect);

	fTabRect.OffsetBy(delta, 0);
	fTabOffset = (int32)location;
	_LayoutTabItems(fTabRect);

	fTabLocation = maxLocation > 0.0 ? fTabOffset / maxLocation : 0.0;

	// redraw new rect as well
	trect = fTabRect;
	trect.bottom++;
	updateRegion->Include(trect);
	return true;
}


void
SATDecorator::_SetFocus()
{
	DefaultDecorator::_SetFocus();

	if (!fStackedMode)
		return;

	if (IsFocus())
		fStackedDrawZoom = true;
	else
		fStackedDrawZoom = false;

	_DoLayout();

}


extern "C" DecorAddOn* (instantiate_decor_addon)(image_id id, const char* name)
{
	return new (std::nothrow)SATDecorAddOn(id, name);
}

