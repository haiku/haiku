/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

/**	Default and fallback decorator for the app_server - the yellow tabs */


#include "DefaultDecorator.h"

#include "DesktopSettings.h"
#include "DrawingEngine.h"
#include "DrawState.h"
#include "FontManager.h"
#include "PatternHandler.h"
#include "RGBColor.h"

#include <Rect.h>
#include <View.h>

#include <WindowPrivate.h>

#include <stdio.h>

//#define DEBUG_DECORATOR
#ifdef DEBUG_DECORATOR
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
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


//	#pragma mark -


// TODO: get rid of DesktopSettings here, and introduce private accessor
//	methods to the Decorator base class

DefaultDecorator::DefaultDecorator(DesktopSettings& settings, BRect rect,
		window_look look, uint32 flags)
	: Decorator(settings, rect, look, flags),
	fTabOffset(0),
	fTabLocation(0.0),
	fLastClicked(0)
{
	DefaultDecorator::SetLook(settings, look);

	fFrameColors = new RGBColor[6];
	fFrameColors[0].SetColor(152, 152, 152);
	fFrameColors[1].SetColor(255, 255, 255);
	fFrameColors[2].SetColor(216, 216, 216);
	fFrameColors[3].SetColor(136, 136, 136);
	fFrameColors[4].SetColor(152, 152, 152);
	fFrameColors[5].SetColor(96, 96, 96);

	// Set appropriate colors based on the current focus value. In this case, each decorator
	// defaults to not having the focus.
	_SetFocus();

	// Do initial decorator setup
	_DoLayout();

	// ToDo: if the decorator was created with a frame too small, it should resize itself!

	STRACE(("DefaultDecorator:\n"));
	STRACE(("\tFrame (%.1f,%.1f,%.1f,%.1f)\n",
		rect.left, rect.top, rect.right, rect.bottom));
}


DefaultDecorator::~DefaultDecorator()
{
	STRACE(("DefaultDecorator: ~DefaultDecorator()\n"));
	delete [] fFrameColors;
}


void
DefaultDecorator::SetTitle(const char* string, BRegion* updateRegion)
{
	BRect rect = TabRect();

	Decorator::SetTitle(string);
	
	if (updateRegion == NULL)
		return;

	BRect updatedRect = TabRect();
	if (rect.left > updatedRect.left)
		rect.left = updatedRect.left;
	if (rect.right < updatedRect.right)
		rect.right = updatedRect.right;

	rect.bottom++;
		// the border will look differently when the title is adjacent

	updateRegion->Include(rect);
}


void
DefaultDecorator::SetLook(DesktopSettings& settings,
	window_look look, BRegion* updateRegion)
{
	// TODO: we could be much smarter about the update region

	// get previous extent
	if (updateRegion != NULL) {
		BRegion extent;
		GetFootprint(&extent);
		updateRegion->Include(&extent);
	}

	ServerFont font;
	if (look == B_FLOATING_WINDOW_LOOK || look == kLeftTitledWindowLook) {
		settings.GetDefaultPlainFont(font);
		if (look == kLeftTitledWindowLook)
			font.SetRotation(90.0f);
	} else
		settings.GetDefaultBoldFont(font);

	font.SetFlags(B_FORCE_ANTIALIASING);
	font.SetSpacing(B_STRING_SPACING);
	SetFont(&font);

	Decorator::SetLook(settings, look, updateRegion);
	_DoLayout();

	if (updateRegion != NULL) {
		BRegion extent;
		GetFootprint(&extent);
		updateRegion->Include(&extent);
	}
}


void
DefaultDecorator::SetFlags(uint32 flags, BRegion* updateRegion)
{
	// TODO: we could be much smarter about the update region

	// get previous extent
	if (updateRegion != NULL) {
		BRegion extent;
		GetFootprint(&extent);
		updateRegion->Include(&extent);
	}

	Decorator::SetFlags(flags, updateRegion);
	_DoLayout();

	if (updateRegion != NULL) {
		BRegion extent;
		GetFootprint(&extent);
		updateRegion->Include(&extent);
	}
}


void
DefaultDecorator::MoveBy(BPoint pt)
{
	STRACE(("DefaultDecorator: Move By (%.1f, %.1f)\n",pt.x,pt.y));
	// Move all internal rectangles the appropriate amount
	_frame.OffsetBy(pt);
	_closerect.OffsetBy(pt);
	_tabrect.OffsetBy(pt);
	_resizerect.OffsetBy(pt);
	_zoomrect.OffsetBy(pt);
	_borderrect.OffsetBy(pt);
	
	fLeftBorder.OffsetBy(pt);
	fRightBorder.OffsetBy(pt);
	fTopBorder.OffsetBy(pt);
	fBottomBorder.OffsetBy(pt);
}


void
DefaultDecorator::ResizeBy(BPoint pt, BRegion* dirty)
{
	STRACE(("DefaultDecorator: Resize By (%.1f, %.1f)\n", pt.x, pt.y));
	// Move all internal rectangles the appropriate amount
	_frame.right += pt.x;
	_frame.bottom += pt.y;

	// handle invalidation of resize rect
	if (dirty && !(fFlags & B_NOT_RESIZABLE)) {
		BRect realResizeRect;
		switch (fLook) {
			case B_DOCUMENT_WINDOW_LOOK:
				realResizeRect = _resizerect;
				// resize rect at old location
				dirty->Include(realResizeRect);
				realResizeRect.OffsetBy(pt);
				// resize rect at new location
				dirty->Include(realResizeRect);
				break;
			case B_TITLED_WINDOW_LOOK:
			case B_FLOATING_WINDOW_LOOK:
			case B_MODAL_WINDOW_LOOK:
			case kLeftTitledWindowLook:
				realResizeRect.Set(fRightBorder.right - 22, fBottomBorder.top,
								   fRightBorder.right - 22, fBottomBorder.bottom - 1);
				// resize rect at old location
				dirty->Include(realResizeRect);
				realResizeRect.OffsetBy(pt);
				// resize rect at new location
				dirty->Include(realResizeRect);

				realResizeRect.Set(fRightBorder.left, fBottomBorder.bottom - 22,
								   fRightBorder.right - 1, fBottomBorder.bottom - 22);
				// resize rect at old location
				dirty->Include(realResizeRect);
				realResizeRect.OffsetBy(pt);
				// resize rect at new location
				dirty->Include(realResizeRect);
				break;
			default:
				break;
		}
	}

	_resizerect.OffsetBy(pt);

	_borderrect.right	+= pt.x;
	_borderrect.bottom	+= pt.y;
	
	fLeftBorder.bottom	+= pt.y;
	fTopBorder.right	+= pt.x;

	fRightBorder.OffsetBy(pt.x, 0.0);
	fRightBorder.bottom	+= pt.y;
	
	fBottomBorder.OffsetBy(0.0, pt.y);
	fBottomBorder.right	+= pt.x;

	if (dirty) {
		if (pt.x > 0.0) {
			BRect t(fRightBorder.left - pt.x, fTopBorder.top,
					fRightBorder.right, fTopBorder.bottom);
			dirty->Include(t);
			t.Set(fRightBorder.left - pt.x, fBottomBorder.top,
				  fRightBorder.right, fBottomBorder.bottom);
			dirty->Include(t);
			dirty->Include(fRightBorder);
		} else if (pt.x < 0.0) {
			dirty->Include(BRect(fRightBorder.left, fTopBorder.top,
								 fRightBorder.right, fBottomBorder.bottom));
		}
		if (pt.y > 0.0) {
			BRect t(fLeftBorder.left, fLeftBorder.bottom - pt.y,
					fLeftBorder.right, fLeftBorder.bottom);
			dirty->Include(t);
			t.Set(fRightBorder.left, fRightBorder.bottom - pt.y,
				  fRightBorder.right, fRightBorder.bottom);
			dirty->Include(t);
			dirty->Include(fBottomBorder);
		} else if (pt.y < 0.0) {
			dirty->Include(fBottomBorder);
		}
	}

	// resize tab and layout tab items
	if (_tabrect.IsValid()) {
		BRect oldTabRect(_tabrect);

		float tabSize;
		float maxLocation;
		if (fLook != kLeftTitledWindowLook) {
			tabSize = fRightBorder.right - fLeftBorder.left;
			maxLocation = tabSize - _tabrect.Width();
		} else {
			tabSize = fBottomBorder.bottom - fTopBorder.top;
			maxLocation = tabSize - _tabrect.Height();
		}

		float tabOffset = floorf(fTabLocation * maxLocation);
		float delta = tabOffset - fTabOffset;
		fTabOffset = (uint32)tabOffset;
		if (fLook != kLeftTitledWindowLook)
			_tabrect.OffsetBy(delta, 0.0);
		else
			_tabrect.OffsetBy(0.0, delta);

		if (tabSize < fMinTabSize)
			tabSize = fMinTabSize;
		if (tabSize > fMaxTabSize)
			tabSize = fMaxTabSize;

		if (fLook != kLeftTitledWindowLook && tabSize != _tabrect.Width()) {
			_tabrect.right = _tabrect.left + tabSize;
		} else if (fLook == kLeftTitledWindowLook && tabSize != _tabrect.Height()) {
			_tabrect.bottom = _tabrect.top + tabSize;
		}

		if (oldTabRect != _tabrect) {
			_LayoutTabItems(_tabrect);

			if (dirty) {
				// NOTE: the tab rect becoming smaller only would
				// handled be the Desktop anyways, so it is sufficient
				// to include it into the dirty region in it's
				// final state
				BRect redraw(_tabrect);
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
	}
}


bool
DefaultDecorator::SetTabLocation(float location, BRegion* updateRegion)
{
	STRACE(("DefaultDecorator: Set Tab Location(%.1f)\n", location));
	if (!_tabrect.IsValid())
		return false;

	if (location < 0)
		location = 0;

	float maxLocation = fRightBorder.right - fLeftBorder.left - _tabrect.Width();
	if (location > maxLocation)
		location = maxLocation;

	float delta = location - fTabOffset;
	if (delta == 0.0)
		return false;

	// redraw old rect (1 pix on the border also must be updated)
	BRect trect(_tabrect);
	trect.bottom++;
	updateRegion->Include(trect);

	_tabrect.OffsetBy(delta, 0);
	fTabOffset = (int32)location;
	_LayoutTabItems(_tabrect);

	fTabLocation = maxLocation > 0.0 ? fTabOffset / maxLocation : 0.0;

	// redraw new rect as well
	trect = _tabrect;
	trect.bottom++;
	updateRegion->Include(trect);
	return true;
}


bool
DefaultDecorator::SetSettings(const BMessage& settings, BRegion* updateRegion)
{
	float tabLocation;
	if (settings.FindFloat("tab location", &tabLocation) == B_OK)
		return SetTabLocation(tabLocation, updateRegion);

	return false;
}


bool
DefaultDecorator::GetSettings(BMessage* settings) const
{
	if (!_tabrect.IsValid())
		return false;

	return settings->AddFloat("tab location", (float)fTabOffset) == B_OK;
}


// #pragma mark -


void
DefaultDecorator::Draw(BRect update)
{
	STRACE(("DefaultDecorator: Draw(%.1f,%.1f,%.1f,%.1f)\n",
		update.left, update.top, update.right, update.bottom));

	// We need to draw a few things: the tab, the resize thumb, the borders,
	// and the buttons

	_DrawFrame(update);
	_DrawTab(update);
}

// Draw
void
DefaultDecorator::Draw()
{
	// Easy way to draw everything - no worries about drawing only certain
	// things

	_DrawFrame(BRect(fTopBorder.LeftTop(), fBottomBorder.RightBottom()));
	_DrawTab(_tabrect);
}

// GetSizeLimits
void
DefaultDecorator::GetSizeLimits(int32* minWidth, int32* minHeight,
								int32* maxWidth, int32* maxHeight) const
{
	if (_tabrect.IsValid())
		*minWidth = (int32)roundf(max_c(*minWidth, fMinTabSize - 2 * fBorderWidth));
	if (_resizerect.IsValid())
		*minHeight = (int32)roundf(max_c(*minHeight, _resizerect.Height() - fBorderWidth));
}

// GetFootprint
void
DefaultDecorator::GetFootprint(BRegion *region)
{
	STRACE(("DefaultDecorator: Get Footprint\n"));
	// This function calculates the decorator's footprint in coordinates
	// relative to the layer. This is most often used to set a WindowLayer
	// object's visible region.
	if (!region)
		return;

	region->MakeEmpty();

	if (fLook == B_NO_BORDER_WINDOW_LOOK)
		return;

	region->Include(fLeftBorder);
	region->Include(fRightBorder);
	region->Include(fTopBorder);
	region->Include(fBottomBorder);
	
	if (fLook == B_BORDERED_WINDOW_LOOK)
		return;

	region->Include(_tabrect);

	if (fLook == B_DOCUMENT_WINDOW_LOOK) {
		// include the rectangular resize knob on the bottom right
		region->Include(BRect(_frame.right - 13.0f, _frame.bottom - 13.0f,
							  _frame.right, _frame.bottom));
	}
}


click_type
DefaultDecorator::Clicked(BPoint pt, int32 buttons, int32 modifiers)
{
#ifdef DEBUG_DECORATOR
	printf("DefaultDecorator: Clicked\n");
	printf("\tPoint: (%.1f,%.1f)\n", pt.x,pt.y);
	printf("\tButtons: %ld, Modifiers: 0x%lx\n", buttons, modifiers);
#endif // DEBUG_DECORATOR

	// TODO: have a real double-click mechanism, ie. take user settings into account
	bigtime_t now = system_time();
	if (buttons != 0) {
		fWasDoubleClick = now - fLastClicked < 200000;
		fLastClicked = now;
	}

	// In checking for hit test stuff, we start with the smallest rectangles the user might
	// be clicking on and gradually work our way out into larger rectangles.
	if (!(fFlags & B_NOT_CLOSABLE) && _closerect.Contains(pt))
		return DEC_CLOSE;

	if (!(fFlags & B_NOT_ZOOMABLE) && _zoomrect.Contains(pt))
		return DEC_ZOOM;

	if (fLook == B_DOCUMENT_WINDOW_LOOK && _resizerect.Contains(pt))
		return DEC_RESIZE;

	bool clicked = false;

	// Clicking in the tab?
	if (_tabrect.Contains(pt)) {
		// tab sliding in any case if either shift key is held down
		// except sliding up-down by moving mouse left-right would look strange
		if ((modifiers & B_SHIFT_KEY) && (fLook != kLeftTitledWindowLook))
			return DEC_SLIDETAB;

		clicked = true;
	} else if (fLeftBorder.Contains(pt) || fRightBorder.Contains(pt)
		|| fTopBorder.Contains(pt) || fBottomBorder.Contains(pt)) {
		// Clicked on border

		// check resize area
		if (!(fFlags & B_NOT_RESIZABLE)
			&& (fLook == B_TITLED_WINDOW_LOOK
				|| fLook == B_FLOATING_WINDOW_LOOK
				|| fLook == B_MODAL_WINDOW_LOOK
				|| fLook == kLeftTitledWindowLook)) {
			BRect temp(BPoint(fBottomBorder.right - 18, fBottomBorder.bottom - 18),
					   fBottomBorder.RightBottom());
			if (temp.Contains(pt))
				return DEC_RESIZE;
		}

		clicked = true;
	}

	if (clicked) {
		// NOTE: On R5, windows are not moved to back if clicked inside the resize area
		// with the second mouse button. So we check this after the check above
		if (buttons == B_SECONDARY_MOUSE_BUTTON)
			return DEC_MOVETOBACK;

		if (fWasDoubleClick)
			return DEC_MINIMIZE;

		return DEC_DRAG;
	}

	// Guess user didn't click anything
	return DEC_NONE;
}


void
DefaultDecorator::_DoLayout()
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
		fTextOffset = (fLook == B_FLOATING_WINDOW_LOOK || fLook == kLeftTitledWindowLook)
			? 10 : 18;

		font_height fontHeight;
		fDrawState.Font().GetHeight(fontHeight);

		if (fLook != kLeftTitledWindowLook) {
			_tabrect.Set(_frame.left - fBorderWidth,
				_frame.top - fBorderWidth - ceilf(fontHeight.ascent + fontHeight.descent + 7.0),
				((_frame.right - _frame.left) < 35.0 ?
					_frame.left + 35.0 : _frame.right) + fBorderWidth,
				_frame.top - fBorderWidth);
		} else {
			_tabrect.Set(_frame.left - fBorderWidth - ceilf(fontHeight.ascent + fontHeight.descent + 5.0),
				_frame.top - fBorderWidth, _frame.left - fBorderWidth,
				_frame.bottom + fBorderWidth);
		}

		// format tab rect for a floating window - make the rect smaller
		if (fLook == B_FLOATING_WINDOW_LOOK) {
			_tabrect.InsetBy(0, 2);
			_tabrect.OffsetBy(0, 2);
		}

		float offset;
		float size;
		_GetButtonSizeAndOffset(_tabrect, &offset, &size);

		// fMinTabSize contains just the room for the buttons
		fMinTabSize = 4.0 + fTextOffset;
		if ((fFlags & B_NOT_CLOSABLE) == 0)
			fMinTabSize += offset + size;
		if ((fFlags & B_NOT_ZOOMABLE) == 0)
			fMinTabSize += offset + size;

		// fMaxTabSize contains fMinWidth + the width required for the title
		fMaxTabSize = _driver ? ceilf(_driver->StringWidth(Title(), strlen(Title()),
			&fDrawState)) : 0.0;
		if (fMaxTabSize > 0.0)
			fMaxTabSize += fTextOffset;
		fMaxTabSize += fMinTabSize;

		float tabSize = fLook != kLeftTitledWindowLook ? _frame.Width() : _frame.Height();
		if (tabSize < fMinTabSize)
			tabSize = fMinTabSize;
		if (tabSize > fMaxTabSize)
			tabSize = fMaxTabSize;

		// layout buttons and truncate text
		if (fLook != kLeftTitledWindowLook)
			_tabrect.right = _tabrect.left + tabSize;
		else
			_tabrect.bottom = _tabrect.top + tabSize;

	} else {
		// no tab
		fMinTabSize = 0.0;
		fMaxTabSize = 0.0;
		_tabrect.Set(0.0, 0.0, -1.0, -1.0);
		_closerect.Set(0.0, 0.0, -1.0, -1.0);
		_zoomrect.Set(0.0, 0.0, -1.0, -1.0);
	}

	// calculate left/top/right/bottom borders
	if (fBorderWidth > 0) {
		// NOTE: no overlapping, the left and right border rects
		// don't include the corners!
		fLeftBorder.Set(_frame.left - fBorderWidth, _frame.top,
			_frame.left - 1, _frame.bottom);

		fRightBorder.Set(_frame.right + 1, _frame.top ,
			_frame.right + fBorderWidth, _frame.bottom);

		fTopBorder.Set(_frame.left - fBorderWidth, _frame.top - fBorderWidth,
			_frame.right + fBorderWidth, _frame.top - 1);

		fBottomBorder.Set(_frame.left - fBorderWidth, _frame.bottom + 1,
			_frame.right + fBorderWidth, _frame.bottom + fBorderWidth);
	} else {
		// no border
		fLeftBorder.Set(0.0, 0.0, -1.0, -1.0);
		fRightBorder.Set(0.0, 0.0, -1.0, -1.0);
		fTopBorder.Set(0.0, 0.0, -1.0, -1.0);
		fBottomBorder.Set(0.0, 0.0, -1.0, -1.0);
	}

	// calculate resize rect
	_resizerect.Set(fBottomBorder.right - 18.0, fBottomBorder.bottom - 18.0,
					fBottomBorder.right, fBottomBorder.bottom);

	if (hasTab) {
		// make sure fTabOffset is within limits and apply it to
		// the _tabrect
		if (fTabOffset < 0)
			fTabOffset = 0;
		if (fTabOffset > (fRightBorder.right - fLeftBorder.left - _tabrect.Width()))
			fTabOffset = (fRightBorder.right - fLeftBorder.left - _tabrect.Width());
		_tabrect.OffsetBy(fTabOffset, 0);

		// finally, layout the buttons and text within the tab rect
		_LayoutTabItems(_tabrect);
	}
}

// _DrawFrame
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
	switch (fLook) {
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
		case B_MODAL_WINDOW_LOOK:
		{
			// top
			if (invalid.Intersects(fTopBorder)) {
				for (int8 i = 0; i < 5; i++) {
					_driver->StrokeLine(BPoint(r.left + i, r.top + i),
										BPoint(r.right - i, r.top + i),
										fFrameColors[i]);
				}
				if (_tabrect.IsValid()) {
					// grey along the bottom of the tab (overwrites "white" from frame)
					_driver->StrokeLine(BPoint(_tabrect.left + 2, _tabrect.bottom + 1),
										BPoint(_tabrect.right - 2, _tabrect.bottom + 1),
										fFrameColors[2]);
				}
			}
			// left
			if (invalid.Intersects(fLeftBorder.InsetByCopy(0, -fBorderWidth))) {
				for (int8 i = 0; i < 5; i++) {
					_driver->StrokeLine(BPoint(r.left + i, r.top + i),
										BPoint(r.left + i, r.bottom - i),
										fFrameColors[i]);
				}
			}
			// bottom
			if (invalid.Intersects(fBottomBorder)) {
				for (int8 i = 0; i < 5; i++) {
					_driver->StrokeLine(BPoint(r.left + i, r.bottom - i),
										BPoint(r.right - i, r.bottom - i),
										fFrameColors[(4 - i) == 4 ? 5 : (4 - i)]);
				}
			}
			// right
			if (invalid.Intersects(fRightBorder.InsetByCopy(0, -fBorderWidth))) {
				for (int8 i = 0; i < 5; i++) {
					_driver->StrokeLine(BPoint(r.right - i, r.top + i),
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
					_driver->StrokeLine(BPoint(r.left + i, r.top + i),
										BPoint(r.right - i, r.top + i),
										fFrameColors[i * 2]);
				}
				if (_tabrect.IsValid() && fLook != kLeftTitledWindowLook) {
					// grey along the bottom of the tab (overwrites "white" from frame)
					_driver->StrokeLine(BPoint(_tabrect.left + 2, _tabrect.bottom + 1),
										BPoint(_tabrect.right - 2, _tabrect.bottom + 1),
										fFrameColors[2]);
				}
			}
			// left
			if (invalid.Intersects(fLeftBorder.InsetByCopy(0, -fBorderWidth))) {
				for (int8 i = 0; i < 3; i++) {
					_driver->StrokeLine(BPoint(r.left + i, r.top + i),
										BPoint(r.left + i, r.bottom - i),
										fFrameColors[i * 2]);
				}
				if (fLook == kLeftTitledWindowLook && _tabrect.IsValid()) {
					// grey along the right side of the tab (overwrites "white" from frame)
					_driver->StrokeLine(BPoint(_tabrect.right + 1, _tabrect.top + 2),
										BPoint(_tabrect.right + 1, _tabrect.bottom - 2),
										fFrameColors[2]);
				}
			}
			// bottom
			if (invalid.Intersects(fBottomBorder)) {
				for (int8 i = 0; i < 3; i++) {
					_driver->StrokeLine(BPoint(r.left + i, r.bottom - i),
										BPoint(r.right - i, r.bottom - i),
										fFrameColors[(2 - i) == 2 ? 5 : (2 - i) * 2]);
				}
			}
			// right
			if (invalid.Intersects(fRightBorder.InsetByCopy(0, -fBorderWidth))) {
				for (int8 i = 0; i < 3; i++) {
					_driver->StrokeLine(BPoint(r.right - i, r.top + i),
										BPoint(r.right - i, r.bottom - i),
										fFrameColors[(2 - i) == 2 ? 5 : (2 - i) * 2]);
				}
			}
			break;
		}

		case B_BORDERED_WINDOW_LOOK:
			_driver->StrokeRect(r, fFrameColors[5]);
			break;

		default:
			// don't draw a border frame
			break;
	}

	// Draw the resize thumb if we're supposed to
	if (!(fFlags & B_NOT_RESIZABLE)) {
		r = _resizerect;

		switch (fLook) {
			case B_DOCUMENT_WINDOW_LOOK:
			{
				if (!invalid.Intersects(r))
					break;

				float x = r.right - 3;
				float y = r.bottom - 3;

				_driver->FillRect(BRect(x - 13, y - 13, x, y), fFrameColors[2]);
				_driver->StrokeLine(BPoint(x - 15, y - 15), BPoint(x - 15, y - 2),
					fFrameColors[0]);
				_driver->StrokeLine(BPoint(x - 14, y - 14), BPoint(x - 14, y - 1),
					fFrameColors[1]);
				_driver->StrokeLine(BPoint(x - 15, y - 15), BPoint(x - 2, y - 15),
					fFrameColors[0]);
				_driver->StrokeLine(BPoint(x - 14, y - 14), BPoint(x - 1, y - 14),
					fFrameColors[1]);

				if (!IsFocus())
					break;

				for (int8 i = 1; i <= 4; i++) {
					for (int8 j = 1; j <= i; j++) {
						BPoint		pt1(x - (3 * j) + 1, y - (3 * (5 - i)) + 1);
						BPoint		pt2(x - (3 * j) + 2, y - (3 * (5 - i)) + 2);
						_driver->StrokePoint(pt1, fFrameColors[0]);
						_driver->StrokePoint(pt2, fFrameColors[1]);
					}
				}
				break;
			}

			case B_TITLED_WINDOW_LOOK:
			case B_FLOATING_WINDOW_LOOK:
			case B_MODAL_WINDOW_LOOK:
			case kLeftTitledWindowLook:
			{
				if (!invalid.Intersects(BRect(fRightBorder.right - 22,
						fBottomBorder.bottom - 22, fRightBorder.right - 1,
						fBottomBorder.bottom - 1)))
					break;
		
				_driver->StrokeLine(BPoint(fRightBorder.left, fBottomBorder.bottom - 22),
									BPoint(fRightBorder.right - 1, fBottomBorder.bottom - 22),
									fFrameColors[0]);
				_driver->StrokeLine(BPoint(fRightBorder.right - 22, fBottomBorder.top),
									BPoint(fRightBorder.right - 22, fBottomBorder.bottom - 1),
									fFrameColors[0]);
				break;
			}

			default:
				// don't draw resize corner
				break;
		}
	}
}

// _DrawTab
void
DefaultDecorator::_DrawTab(BRect invalid)
{
	STRACE(("_DrawTab(%.1f,%.1f,%.1f,%.1f)\n",
			invalid.left, invalid.top, invalid.right, invalid.bottom));
	// If a window has a tab, this will draw it and any buttons which are
	// in it.
	if (!_tabrect.IsValid() || !invalid.Intersects(_tabrect))
		return;

	// TODO: cache these
	RGBColor tabColorLight = RGBColor(tint_color(fTabColor.GetColor32(),
												 (B_LIGHTEN_2_TINT + B_LIGHTEN_MAX_TINT) / 2));
	RGBColor tabColorShadow = RGBColor(tint_color(fTabColor.GetColor32(),
												  B_DARKEN_2_TINT));

	// outer frame
	_driver->StrokeLine(_tabrect.LeftTop(), _tabrect.LeftBottom(), fFrameColors[0]);
	_driver->StrokeLine(_tabrect.LeftTop(), _tabrect.RightTop(), fFrameColors[0]);
	if (fLook != kLeftTitledWindowLook)
		_driver->StrokeLine(_tabrect.RightTop(),_tabrect.RightBottom(), fFrameColors[5]);
	else
		_driver->StrokeLine(_tabrect.LeftBottom(),_tabrect.RightBottom(), fFrameColors[5]);

	// bevel
	_driver->StrokeLine(BPoint(_tabrect.left + 1, _tabrect.top + 1),
		BPoint(_tabrect.left + 1, _tabrect.bottom - (fLook == kLeftTitledWindowLook ? 1 : 0)),
		tabColorLight);
	_driver->StrokeLine(BPoint(_tabrect.left + 1, _tabrect.top + 1),
		BPoint(_tabrect.right - (fLook == kLeftTitledWindowLook ? 0 : 1), _tabrect.top + 1),
		tabColorLight);

	if (fLook != kLeftTitledWindowLook) {
		_driver->StrokeLine(BPoint(_tabrect.right - 1, _tabrect.top + 2),
			BPoint(_tabrect.right - 1, _tabrect.bottom), tabColorShadow);
	} else {
		_driver->StrokeLine(BPoint(_tabrect.left + 2, _tabrect.bottom - 1),
			BPoint(_tabrect.right, _tabrect.bottom - 1), tabColorShadow);
	}

	// fill
	if (fLook != kLeftTitledWindowLook) {
		_driver->FillRect(BRect(_tabrect.left + 2, _tabrect.top + 2,
			_tabrect.right - 2, _tabrect.bottom), fTabColor);
	} else {
		_driver->FillRect(BRect(_tabrect.left + 2, _tabrect.top + 2,
			_tabrect.right, _tabrect.bottom - 2), fTabColor);
	}

	_DrawTitle(_tabrect);

	// Draw the buttons if we're supposed to	
	if (!(fFlags & B_NOT_CLOSABLE) && invalid.Intersects(_closerect))
		_DrawClose(_closerect);
	if (!(fFlags & B_NOT_ZOOMABLE) && invalid.Intersects(_zoomrect))
		_DrawZoom(_zoomrect);
}

// _DrawClose
void
DefaultDecorator::_DrawClose(BRect r)
{
	STRACE(("_DrawClose(%f,%f,%f,%f)\n", r.left, r.top, r.right, r.bottom));
	// Just like DrawZoom, but for a close button
	_DrawBlendedRect(r, GetClose());
}

// _DrawTitle
void
DefaultDecorator::_DrawTitle(BRect r)
{
	STRACE(("_DrawTitle(%f,%f,%f,%f)\n", r.left, r.top, r.right, r.bottom));

	fDrawState.SetHighColor(fTextColor);
	fDrawState.SetLowColor(fTabColor);

	// figure out position of text
	font_height fontHeight;
	fDrawState.Font().GetHeight(fontHeight);

	BPoint titlePos;
	if (fLook != kLeftTitledWindowLook) {
		titlePos.x = _closerect.IsValid() ? _closerect.right + fTextOffset
			: _tabrect.left + fTextOffset;
		titlePos.y = floorf(((_tabrect.top + 2.0) + _tabrect.bottom + fontHeight.ascent
			+ fontHeight.descent) / 2.0 - fontHeight.descent + 0.5);
	} else {
		titlePos.x = floorf(((_tabrect.left + 2.0) + _tabrect.right + fontHeight.ascent
			+ fontHeight.descent) / 2.0 - fontHeight.descent + 0.5);
		titlePos.y = _zoomrect.IsValid() ? _zoomrect.top - fTextOffset
			: _tabrect.bottom - fTextOffset;
	}

	_driver->DrawString(fTruncatedTitle.String(), fTruncatedTitleLength, titlePos, &fDrawState);
}

// _DrawZoom
void
DefaultDecorator::_DrawZoom(BRect r)
{
	STRACE(("_DrawZoom(%f,%f,%f,%f)\n", r.left, r.top, r.right, r.bottom));
	// If this has been implemented, then the decorator has a Zoom button
	// which should be drawn based on the state of the member zoomstate

	BRect zr(r);
	zr.left += 3.0;
	zr.top += 3.0;
	_DrawBlendedRect(zr, GetZoom());

	zr = r;
	zr.right -= 5.0;
	zr.bottom -= 5.0;
	_DrawBlendedRect(zr, GetZoom());
}

// _SetFocus
void
DefaultDecorator::_SetFocus()
{
	// SetFocus() performs necessary duties for color swapping and
	// other things when a window is deactivated or activated.
	
	if (IsFocus() || ((fLook == B_FLOATING_WINDOW_LOOK || fLook == kLeftTitledWindowLook)
			&& (fFlags & B_AVOID_FOCUS) != 0)) {
		fTabColor = UIColor(B_WINDOW_TAB_COLOR);
		fTextColor = UIColor(B_WINDOW_TEXT_COLOR);
		fButtonHighColor.SetColor(tint_color(fTabColor.GetColor32(), B_LIGHTEN_2_TINT));
		fButtonLowColor.SetColor(tint_color(fTabColor.GetColor32(), B_DARKEN_1_TINT));

//		fFrameColors[0].SetColor(152, 152, 152);
//		fFrameColors[1].SetColor(255, 255, 255);
		fFrameColors[2].SetColor(216, 216, 216);
		fFrameColors[3].SetColor(136, 136, 136);
//		fFrameColors[4].SetColor(152, 152, 152);
//		fFrameColors[5].SetColor(96, 96, 96);
	} else {
		fTabColor = UIColor(B_WINDOW_INACTIVE_TAB_COLOR);
		fTextColor = UIColor(B_WINDOW_INACTIVE_TEXT_COLOR);
		fButtonHighColor.SetColor(tint_color(fTabColor.GetColor32(), B_LIGHTEN_2_TINT));
		fButtonLowColor.SetColor(tint_color(fTabColor.GetColor32(), B_DARKEN_1_TINT));

//		fFrameColors[0].SetColor(152, 152, 152);
//		fFrameColors[1].SetColor(255, 255, 255);
		fFrameColors[2].SetColor(232, 232, 232);
		fFrameColors[3].SetColor(148, 148, 148);
//		fFrameColors[4].SetColor(152, 152, 152);
//		fFrameColors[5].SetColor(96, 96, 96);
	}
}

// _SetColors
void
DefaultDecorator::_SetColors()
{
	_SetFocus();
}


/*!
	\brief Draws a framed rectangle with a gradient.
	\param down The rectangle should be drawn recessed or not
*/
void
DefaultDecorator::_DrawBlendedRect(BRect r, bool down)
{
	// Actually just draws a blended square
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
		
		_driver->StrokeLine(BPoint(r.left, r.top + i),
							BPoint(r.left + i, r.top), temprgbcol);

		temprgbcol.SetColor(uint8(halfColor.red - (i * rstep)),
							uint8(halfColor.green - (i * gstep)),
							uint8(halfColor.blue - (i * bstep)));

		_driver->StrokeLine(BPoint(r.left + steps, r.top + i),
							BPoint(r.left + i, r.top + steps), temprgbcol);
	}
	_driver->StrokeRect(r, fFrameColors[3]);
}

// _GetButtonSizeAndOffset
void
DefaultDecorator::_GetButtonSizeAndOffset(const BRect& tabRect, float* _offset,
	float* _size) const
{
	*_offset = fLook == B_FLOATING_WINDOW_LOOK || fLook == kLeftTitledWindowLook ? 4.0 : 5.0;

	// "+ 2" so that the rects are centered within the solid area
	// (without the 2 pixels for the top border)
	if (fLook != kLeftTitledWindowLook)
		*_size = tabRect.Height() - 2.0 * *_offset + 2.0f;
	else
		*_size = tabRect.Width() - 2.0 * *_offset + 2.0f;
}

// _LayoutTabItems
void
DefaultDecorator::_LayoutTabItems(const BRect& tabRect)
{
	float offset;
	float size;
	_GetButtonSizeAndOffset(tabRect, &offset, &size);

	// calulate close rect based on the tab rectangle
	if (fLook != kLeftTitledWindowLook) {
		_closerect.Set(tabRect.left + offset, tabRect.top + offset,
			tabRect.left + offset + size, tabRect.top + offset + size);

		_zoomrect.Set(tabRect.right - offset - size, tabRect.top + offset,
			tabRect.right - offset, tabRect.top + offset + size);

		// hidden buttons have no width
		if ((Flags() & B_NOT_CLOSABLE) != 0)
			_closerect.right = _closerect.left - offset;
		if ((Flags() & B_NOT_ZOOMABLE) != 0)
			_zoomrect.left = _zoomrect.right + offset;
	} else {
		_closerect.Set(tabRect.left + offset, tabRect.top + offset,
			tabRect.left + offset + size, tabRect.top + offset + size);

		_zoomrect.Set(tabRect.left + offset, tabRect.bottom - offset - size,
			tabRect.left + size + offset, tabRect.bottom - offset);

		// hidden buttons have no height
		if ((Flags() & B_NOT_CLOSABLE) != 0)
			_closerect.bottom = _closerect.top - offset;
		if ((Flags() & B_NOT_ZOOMABLE) != 0)
			_zoomrect.top = _zoomrect.bottom + offset;
	}

	// calculate room for title
	// ToDo: the +2 is there because the title often appeared
	//	truncated for no apparent reason - OTOH the title does
	//	also not appear perfectly in the middle
	if (fLook != kLeftTitledWindowLook)
		size = (_zoomrect.left - _closerect.right) - fTextOffset * 2 + 2;
	else
		size = (_zoomrect.top - _closerect.bottom) - fTextOffset * 2 + 2;

	fTruncatedTitle = Title();
	fDrawState.Font().TruncateString(&fTruncatedTitle, B_TRUNCATE_END, size);
	fTruncatedTitleLength = fTruncatedTitle.Length();
}
