/*
 * Copyright 2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
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

static const rgb_color kHighlightFrameColors[6] = {
	{ 52, 52, 52, 255 },
	{ 140, 140, 140, 255 },
	{ 124, 124, 124, 255 },
	{ 108, 108, 108, 255 },
	{ 52, 52, 52, 255 },
	{ 8, 8, 8, 255 }
};

static const rgb_color kTabColor = {255, 203, 0, 255};
static const rgb_color kHighlightTabColor = tint_color(kTabColor,
	B_DARKEN_2_TINT);
static const rgb_color kHighlightTabColorLight = tint_color(kHighlightTabColor,
	(B_LIGHTEN_MAX_TINT + B_LIGHTEN_2_TINT) / 2);
static const rgb_color kHighlightTabColorBevel = tint_color(kHighlightTabColor,
	B_LIGHTEN_2_TINT);
static const rgb_color kHighlightTabColorShadow = tint_color(kHighlightTabColor,
	(B_DARKEN_1_TINT + B_NO_TINT) / 2);


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

	fStackedMode(false),
	fStackedTabLength(0)
{
	fStackedDrawZoom = IsFocus();
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


void
SATDecorator::DrawButtons(const BRect& invalid)
{
	// Draw the buttons if we're supposed to
	if (!(fFlags & B_NOT_CLOSABLE) && invalid.Intersects(fCloseRect))
		_DrawClose(fCloseRect);

	if (fStackedMode) {
		// TODO: This should be solved differently. We don't just want to not
		// draw the button, we actually want it removed. So rather add extra
		// flags to remove the individual buttons to DefaultDecorator.
		if (fStackedDrawZoom && invalid.Intersects(fZoomRect))
			_DrawZoom(fZoomRect);
	} else if (!(fFlags & B_NOT_ZOOMABLE) && invalid.Intersects(fZoomRect))
		_DrawZoom(fZoomRect);
}


void
SATDecorator::GetComponentColors(Component component, uint8 highlight,
	ComponentColors _colors)
{
	// we handle only our own highlights
	if (highlight != HIGHLIGHT_STACK_AND_TILE) {
		DefaultDecorator::GetComponentColors(component, highlight, _colors);
		return;
	}

	switch (component) {
		case COMPONENT_TAB:
			_colors[COLOR_TAB_FRAME_LIGHT] = kFrameColors[0];
			_colors[COLOR_TAB_FRAME_DARK] = kFrameColors[3];
			_colors[COLOR_TAB] = kHighlightTabColor;
			_colors[COLOR_TAB_LIGHT] = kHighlightTabColorLight;
			_colors[COLOR_TAB_BEVEL] = kHighlightTabColorBevel;
			_colors[COLOR_TAB_SHADOW] = kHighlightTabColorShadow;
			_colors[COLOR_TAB_TEXT] = kFocusTextColor;
			break;

		case COMPONENT_CLOSE_BUTTON:
		case COMPONENT_ZOOM_BUTTON:
			_colors[COLOR_BUTTON] = kHighlightTabColor;
			_colors[COLOR_BUTTON_LIGHT] = kHighlightTabColorLight;
			break;

		case COMPONENT_LEFT_BORDER:
		case COMPONENT_RIGHT_BORDER:
		case COMPONENT_TOP_BORDER:
		case COMPONENT_BOTTOM_BORDER:
		case COMPONENT_RESIZE_CORNER:
		default:
			_colors[0] = kHighlightFrameColors[0];
			_colors[1] = kHighlightFrameColors[1];
			_colors[2] = kHighlightFrameColors[2];
			_colors[3] = kHighlightFrameColors[3];
			_colors[4] = kHighlightFrameColors[4];
			_colors[5] = kHighlightFrameColors[5];
			break;
	}
}


extern "C" DecorAddOn* (instantiate_decor_addon)(image_id id, const char* name)
{
	return new (std::nothrow)SATDecorAddOn(id, name);
}
