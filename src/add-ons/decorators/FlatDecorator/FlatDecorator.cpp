/*
 * Copyright 2001-2020 Haiku, Inc.
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
 *		Nahuel Tello <nhtello@unarix.com.ar>
 */


/*!	Flat decorator for dark mode theme */


#include "FlatDecorator.h"

#include <algorithm>
#include <cmath>
#include <new>
#include <stdio.h>

#include <Autolock.h>
#include <Debug.h>
#include <GradientLinear.h>
#include <Rect.h>
#include <Region.h>
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


static const float kBorderResizeLength = 22.0;


FlatDecorAddOn::FlatDecorAddOn(image_id id, const char* name)
	:
	DecorAddOn(id, name)
{
}


Decorator*
FlatDecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
{
	return new (std::nothrow)FlatDecorator(settings, rect, desktop);
}



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


// TODO: get rid of DesktopSettings here, and introduce private accessor
//	methods to the Decorator base class
FlatDecorator::FlatDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
	:
	TabDecorator(settings, rect, desktop)
{
	// TODO: If the decorator was created with a frame too small, it should
	// resize itself!

	STRACE(("FlatDecorator:\n"));
	STRACE(("\tFrame (%.1f,%.1f,%.1f,%.1f)\n",
		rect.left, rect.top, rect.right, rect.bottom));
}


FlatDecorator::~FlatDecorator()
{
	STRACE(("FlatDecorator: ~FlatDecorator()\n"));
}


// #pragma mark - Public methods


/*!	Returns the frame colors for the specified decorator component.

	The meaning of the color array elements depends on the specified component.
	For some components some array elements are unused.

	\param component The component for which to return the frame colors.
	\param highlight The highlight set for the component.
	\param colors An array of colors to be initialized by the function.
*/
void
FlatDecorator::GetComponentColors(Component component, uint8 highlight,
	ComponentColors _colors, Decorator::Tab* _tab)
{
	Decorator::Tab* tab = static_cast<Decorator::Tab*>(_tab);
	switch (component) {
		case COMPONENT_TAB:
			if (highlight != 0) {
				_colors[COLOR_TAB_FRAME_LIGHT]
					= tint_color(fFocusTabColor, 1.0);
				_colors[COLOR_TAB_FRAME_DARK]
					= tint_color(fFocusTabColor, 1.2);
				_colors[COLOR_TAB] = tint_color(fFocusTabColor, 0.95);
				_colors[COLOR_TAB_LIGHT] = fFocusTabColorLight; 
				_colors[COLOR_TAB_BEVEL] = fFocusTabColorBevel;
				_colors[COLOR_TAB_SHADOW] = fFocusTabColorShadow;
				_colors[COLOR_TAB_TEXT] = tint_color(fFocusTextColor, 0.5);
			} 
			else if (tab && tab->buttonFocus) {
				_colors[COLOR_TAB_FRAME_LIGHT]
					= tint_color(fFocusTabColor, 1.0);
				_colors[COLOR_TAB_FRAME_DARK]
					= tint_color(fFocusTabColor, 1.2);
				_colors[COLOR_TAB] = fFocusTabColor;
				_colors[COLOR_TAB_LIGHT] = fFocusTabColorLight;
				_colors[COLOR_TAB_BEVEL] = fFocusTabColorBevel;
				_colors[COLOR_TAB_SHADOW] = fFocusTabColorShadow;
				_colors[COLOR_TAB_TEXT] = fFocusTextColor;
			} else {
				_colors[COLOR_TAB_FRAME_LIGHT]
					= tint_color(fNonFocusTabColor, 1.0);
				_colors[COLOR_TAB_FRAME_DARK]
					= tint_color(fNonFocusTabColor, 1.2);
				_colors[COLOR_TAB] = fNonFocusTabColor;
				_colors[COLOR_TAB_LIGHT] = fNonFocusTabColorLight;
				_colors[COLOR_TAB_BEVEL] = fNonFocusTabColorBevel;
				_colors[COLOR_TAB_SHADOW] = fNonFocusTabColorShadow;
				_colors[COLOR_TAB_TEXT] = fNonFocusTextColor;
			}
			break;

		case COMPONENT_CLOSE_BUTTON:
		case COMPONENT_ZOOM_BUTTON:
			if (tab && tab->buttonFocus) {
				_colors[COLOR_BUTTON] = fFocusTabColor;
				_colors[COLOR_BUTTON_LIGHT] = fFocusTabColorLight;
			} else {
				_colors[COLOR_BUTTON] = fNonFocusTabColor;
				_colors[COLOR_BUTTON_LIGHT] = fNonFocusTabColorLight;
			}
			break;

		case COMPONENT_TOP_BORDER:
			if (tab && tab->buttonFocus) {
				_colors[0] = tint_color(fFocusTabColor, 1.2); // borde exterior
				_colors[1] = tint_color(fFocusTabColor, 1.0); // borde top
				_colors[2] = tint_color(fFocusTabColor, 1.0); // borde top
				_colors[3] = tint_color(fFocusTabColor, 1.0); // borde top
				_colors[4] = tint_color(fFocusFrameColor, 1.1); // borde interior
				_colors[5] = tint_color(fFocusFrameColor, 1.1); // borde menu 1
			} else {
				_colors[0] = tint_color(fNonFocusTabColor, 1.2); // borde exterior
				_colors[1] = tint_color(fNonFocusTabColor, B_NO_TINT);
				_colors[2] = tint_color(fNonFocusTabColor, B_NO_TINT);
				_colors[3] = tint_color(fNonFocusTabColor, B_NO_TINT);
				_colors[4] = tint_color(fNonFocusFrameColor, 1.1); // borde interior
				_colors[5] = tint_color(fNonFocusFrameColor, 1.1); // borde menu 1
			}
			break;
		case COMPONENT_RESIZE_CORNER:
			if (tab && tab->buttonFocus) {
				_colors[0] = tint_color(fFocusFrameColor, 1.25); // borde exterior
				_colors[1] = tint_color(fFocusFrameColor, 1.0); // borde top
				_colors[2] = tint_color(fFocusFrameColor, 1.0); // borde top
				_colors[3] = tint_color(fFocusTabColor, 1.0); // borde top
				_colors[4] = tint_color(fFocusFrameColor, 1.1); // borde interior
				_colors[5] = tint_color(fFocusFrameColor, 1.1); // borde menu 1
			} else {
				_colors[0] = tint_color(fNonFocusFrameColor, 1.25); // borde exterior
				_colors[1] = tint_color(fNonFocusFrameColor, B_NO_TINT);
				_colors[2] = tint_color(fNonFocusFrameColor, B_NO_TINT);
				_colors[3] = tint_color(fNonFocusFrameColor, B_NO_TINT);
				_colors[4] = tint_color(fNonFocusFrameColor, 1.1); // borde interior
				_colors[5] = tint_color(fNonFocusFrameColor, 1.1); // borde menu 1
			}
			break;
		case COMPONENT_LEFT_BORDER:
		case COMPONENT_RIGHT_BORDER:
			if (tab && tab->buttonFocus) {
				_colors[0] = tint_color(fFocusFrameColor, 1.25); // borde exterior
				_colors[1] = tint_color(fFocusFrameColor, B_NO_TINT);
				_colors[2] = tint_color(fFocusFrameColor, B_NO_TINT);
				_colors[3] = tint_color(fFocusFrameColor, B_NO_TINT);
				_colors[4] = tint_color(fFocusFrameColor, 1.05); // borde interior
				_colors[5] = tint_color(fFocusFrameColor, 1.1); // borde menu 1
				_colors[6] = tint_color(fFocusTabColor, 1.2); // border tab to be part
			} else {
				_colors[0] = tint_color(fNonFocusFrameColor, 1.25); // borde exterior
				_colors[1] = tint_color(fNonFocusFrameColor, B_NO_TINT);
				_colors[2] = tint_color(fNonFocusFrameColor, B_NO_TINT);
				_colors[3] = tint_color(fNonFocusFrameColor, B_NO_TINT);
				_colors[4] = tint_color(fNonFocusFrameColor, 1.05); // borde interior
				_colors[5] = tint_color(fNonFocusFrameColor, 1.0); // borde menu 1
				_colors[6] = tint_color(fNonFocusTabColor, 1.2); // border tab to be part
			}
			break;
		case COMPONENT_BOTTOM_BORDER:
		default:
			if (tab && tab->buttonFocus) {
				_colors[0] = tint_color(fFocusFrameColor, 1.25); // borde exterior
				_colors[1] = tint_color(fFocusFrameColor, B_NO_TINT);
				_colors[2] = tint_color(fFocusFrameColor, B_NO_TINT);
				_colors[3] = tint_color(fFocusFrameColor, B_NO_TINT);
				_colors[4] = tint_color(fFocusFrameColor, 1.1); // borde interior
				_colors[5] = tint_color(fFocusFrameColor, 1.1); // borde menu 1
			} else {
				_colors[0] = tint_color(fNonFocusFrameColor, 1.25); // borde exterior
				_colors[1] = tint_color(fNonFocusFrameColor, B_NO_TINT);
				_colors[2] = tint_color(fNonFocusFrameColor, B_NO_TINT);
				_colors[3] = tint_color(fNonFocusFrameColor, B_NO_TINT);
				_colors[4] = tint_color(fNonFocusFrameColor, 1.1); // borde interior
				_colors[5] = tint_color(fNonFocusFrameColor, 1.1); // borde menu 1
			}

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
FlatDecorator::UpdateColors(DesktopSettings& settings)
{
	TabDecorator::UpdateColors(settings);
}


// #pragma mark - Protected methods


void
FlatDecorator::_DrawFrame(BRect rect)
{
	STRACE(("_DrawFrame(%f,%f,%f,%f)\n", rect.left, rect.top,
		rect.right, rect.bottom));

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
			// left
			if (rect.Intersects(fLeftBorder.InsetByCopy(0, -fBorderWidth))) {
				ComponentColors colors;
				_GetComponentColors(COMPONENT_LEFT_BORDER, colors, fTopTab);

				for (int8 i = 0; i < 5; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.top + i),
						BPoint(r.left + i, r.bottom - i), colors[i]);
				}
				// redraw line to be part of tab title
				fDrawingEngine->StrokeLine(BPoint(r.left, r.top),
					BPoint(r.left, r.top + 4), colors[6]);
				
			}
			// bottom
			if (rect.Intersects(fBottomBorder)) {
				ComponentColors colors;
				_GetComponentColors(COMPONENT_BOTTOM_BORDER, colors, fTopTab);

				for (int8 i = 0; i < 5; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.bottom - i),
						BPoint(r.right - i, r.bottom - i),
						colors[i]);
				}
			}
			// right
			if (rect.Intersects(fRightBorder.InsetByCopy(0, -fBorderWidth))) {
				ComponentColors colors;
				_GetComponentColors(COMPONENT_RIGHT_BORDER, colors, fTopTab);

				for (int8 i = 0; i < 5; i++) {
						fDrawingEngine->StrokeLine(BPoint(r.right - i, r.top + i),
							BPoint(r.right - i, r.bottom - i),
							colors[i]);
				}
				// redraw line to be part of tab title
				fDrawingEngine->StrokeLine(BPoint(r.right, r.top),
					BPoint(r.right, r.top + 4),
					colors[6]);
			}
			// top
			if (rect.Intersects(fTopBorder)) {
				ComponentColors colors;
				_GetComponentColors(COMPONENT_TOP_BORDER, colors, fTopTab);

				for (int8 i = 0; i < 5; i++) {
					if (i<4)
					{
						fDrawingEngine->StrokeLine(BPoint(r.left + 1, r.top + i),
							BPoint(r.right - 1, r.top + i), tint_color(colors[i], (i*0.01+1)));
					}
					else
					{
						fDrawingEngine->StrokeLine(BPoint(r.left + 1, r.top + i),
							BPoint(r.right - 1, r.top + i), tint_color(colors[3], 1.1));
					}
				}
			}
			break;
		}

		case B_FLOATING_WINDOW_LOOK:
		case kLeftTitledWindowLook:
		{
			// top
			if (rect.Intersects(fTopBorder)) {
				ComponentColors colors;
				_GetComponentColors(COMPONENT_TOP_BORDER, colors, fTopTab);

				for (int8 i = 0; i < 3; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.top + i),
						BPoint(r.right - i, r.top + i), tint_color(colors[1], 0.95));
				}
				if (fTitleBarRect.IsValid() && fTopTab->look != kLeftTitledWindowLook) {
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
			if (rect.Intersects(fLeftBorder.InsetByCopy(0, -fBorderWidth))) {
				ComponentColors colors;
				_GetComponentColors(COMPONENT_LEFT_BORDER, colors, fTopTab);

				for (int8 i = 0; i < 3; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.top + i),
						BPoint(r.left + i, r.bottom - i), colors[i * 2]);
				}
				if (fTopTab->look == kLeftTitledWindowLook
					&& fTitleBarRect.IsValid()) {
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
			if (rect.Intersects(fBottomBorder)) {
				ComponentColors colors;
				_GetComponentColors(COMPONENT_BOTTOM_BORDER, colors, fTopTab);

				for (int8 i = 0; i < 3; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.bottom - i),
						BPoint(r.right - i, r.bottom - i),
						colors[(2 - i) == 2 ? 5 : (2 - i) * 2]);
				}
			}
			// right
			if (rect.Intersects(fRightBorder.InsetByCopy(0, -fBorderWidth))) {
				ComponentColors colors;
				_GetComponentColors(COMPONENT_RIGHT_BORDER, colors, fTopTab);

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
			_GetComponentColors(COMPONENT_LEFT_BORDER, colors, fTopTab);

			fDrawingEngine->StrokeRect(r, colors[5]);
			break;
		}

		default:
			// don't draw a border frame
			break;
	}

	// Draw the resize knob if we're supposed to
	if (!(fTopTab->flags & B_NOT_RESIZABLE)) {
		r = fResizeRect;

		ComponentColors colors;
		_GetComponentColors(COMPONENT_RESIZE_CORNER, colors, fTopTab);

		switch ((int)fTopTab->look) {
			case B_DOCUMENT_WINDOW_LOOK:
			{
				if (!rect.Intersects(r))
					break;

				float x = r.right - 3;
				float y = r.bottom - 3;

				BRect bg(x - 15, y - 15, x, y);

				BGradientLinear gradient;
				gradient.SetStart(bg.LeftTop());
				gradient.SetEnd(bg.RightBottom());
				gradient.AddColor(tint_color(colors[1], 1.05), 0);
				gradient.AddColor(tint_color(colors[1], 1.0), 255);

				fDrawingEngine->FillRect(bg, gradient);

				fDrawingEngine->StrokeLine(BPoint(x - 15, y - 15),
					BPoint(x - 15, y - 1), colors[4]);
				fDrawingEngine->StrokeLine(BPoint(x - 15, y - 15),
					BPoint(x - 1, y - 15), colors[4]);

				if (fTopTab && !IsFocus(fTopTab))
					break;

				for (int8 i = 1; i <= 4; i++) {
					for (int8 j = 1; j <= i; j++) {
						BPoint pt1(x - (3 * j) + 1, y - (3 * (5 - i)) + 1);
						BPoint pt2(x - (3 * j) + 2, y - (3 * (5 - i)) + 2);
						fDrawingEngine->StrokePoint(pt1, tint_color(colors[1], 1.5));
						fDrawingEngine->StrokePoint(pt2, tint_color(colors[1], 0.75));
					}
				}
				break;
			}

			case B_TITLED_WINDOW_LOOK:
			case B_FLOATING_WINDOW_LOOK:
			case B_MODAL_WINDOW_LOOK:
			case kLeftTitledWindowLook:
			{
				if (!rect.Intersects(BRect(fRightBorder.right - kBorderResizeLength,
					fBottomBorder.bottom - kBorderResizeLength, fRightBorder.right - 1,
					fBottomBorder.bottom - 1)))
					break;

				fDrawingEngine->StrokeLine(
					BPoint(fRightBorder.left, fBottomBorder.bottom - kBorderResizeLength),
					BPoint(fRightBorder.right - 1, fBottomBorder.bottom - kBorderResizeLength),
					tint_color(colors[1], 1.2));
				fDrawingEngine->StrokeLine(
					BPoint(fRightBorder.right - kBorderResizeLength, fBottomBorder.top),
					BPoint(fRightBorder.right - kBorderResizeLength, fBottomBorder.bottom - 1),
					tint_color(colors[1], 1.2));

				// Try to draw line in yellow to the resize place
				for (int8 i = 1; i < 4; i++) {
					fDrawingEngine->StrokeLine(
						BPoint(fRightBorder.left+i, fBottomBorder.bottom - kBorderResizeLength + 1),
						BPoint(fRightBorder.left+i, fBottomBorder.bottom - 1),
						tint_color(colors[3], (i * 0.06) + 1));
				}
				int rez[] = {4,3,2,1};
				for (int8 i = 1; i < 4; i++) {
					fDrawingEngine->StrokeLine(
						BPoint(fRightBorder.right - kBorderResizeLength + 1, fBottomBorder.bottom - i),
						BPoint(fRightBorder.right - i, fBottomBorder.bottom - i ),
						tint_color(colors[3], (rez[i] * 0.06) + 1));
				}

				break;
			}

			default:
				// don't draw resize corner
				break;
		}
	}
}


/*!	\brief Actually draws the tab

	This function is called when the tab itself needs drawn. Other items,
	like the window title or buttons, should not be drawn here.

	\param tab The \a tab to update.
	\param rect The area of the \a tab to update.
*/
void
FlatDecorator::_DrawTab(Decorator::Tab* tab, BRect invalid)
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

	if (tab && tab->buttonFocus) {
		// outer frame
		fDrawingEngine->StrokeLine(tabRect.LeftTop(), tabRect.LeftBottom(),
			colors[COLOR_TAB_FRAME_DARK]);
		fDrawingEngine->StrokeLine(tabRect.LeftTop(), tabRect.RightTop(),
			colors[COLOR_TAB_FRAME_DARK]);
		if (tab->look != kLeftTitledWindowLook) {
			fDrawingEngine->StrokeLine(tabRect.RightTop(), tabRect.RightBottom(),
				colors[COLOR_TAB_FRAME_DARK]);
		} else {
			fDrawingEngine->StrokeLine(tabRect.LeftBottom(),
				tabRect.RightBottom(), fFocusFrameColor);
		}
	} else {
		// outer frame
		fDrawingEngine->StrokeLine(tabRect.LeftTop(), tabRect.LeftBottom(),
			colors[COLOR_TAB_FRAME_DARK]);
		fDrawingEngine->StrokeLine(tabRect.LeftTop(), tabRect.RightTop(),
			colors[COLOR_TAB_FRAME_DARK]);
		if (tab->look != kLeftTitledWindowLook) {
			fDrawingEngine->StrokeLine(tabRect.RightTop(), tabRect.RightBottom(),
				colors[COLOR_TAB_FRAME_DARK]);
		} else {
			fDrawingEngine->StrokeLine(tabRect.LeftBottom(),
				tabRect.RightBottom(), fFocusFrameColor);
		}
	}

	float tabBotton = tabRect.bottom;
	if (fTopTab != tab)
		tabBotton -= 1;

	// bevel
	fDrawingEngine->StrokeLine(BPoint(tabRect.left + 1, tabRect.top + 1),
		BPoint(tabRect.left + 1,
			tabBotton - (tab->look == kLeftTitledWindowLook ? 1 : 0)),
		colors[COLOR_TAB]);
	fDrawingEngine->StrokeLine(BPoint(tabRect.left + 1, tabRect.top + 1),
		BPoint(tabRect.right - (tab->look == kLeftTitledWindowLook ? 0 : 1),
			tabRect.top + 1),
		tint_color(colors[COLOR_TAB], 0.9));

	if (tab->look != kLeftTitledWindowLook) {
		fDrawingEngine->StrokeLine(BPoint(tabRect.right - 1, tabRect.top + 2),
			BPoint(tabRect.right - 1, tabBotton),
			colors[COLOR_TAB]);
	} else {
		fDrawingEngine->StrokeLine(
			BPoint(tabRect.left + 2, tabRect.bottom - 1),
			BPoint(tabRect.right, tabRect.bottom - 1),
			colors[COLOR_TAB]);
	}

	// fill
	BGradientLinear gradient;
	gradient.SetStart(tabRect.LeftTop());
	if (tab && tab->buttonFocus) {
		gradient.AddColor(tint_color(colors[COLOR_TAB], 0.6), 0);
		gradient.AddColor(tint_color(colors[COLOR_TAB], 1.0), 200);
	} else {
		gradient.AddColor(tint_color(colors[COLOR_TAB], 0.9), 0);
		gradient.AddColor(tint_color(colors[COLOR_TAB], 1.0), 150);
	}

	if (tab->look != kLeftTitledWindowLook) {
		gradient.SetEnd(tabRect.LeftBottom());
		fDrawingEngine->FillRect(BRect(tabRect.left + 2, tabRect.top + 2,
			tabRect.right - 2, tabBotton), gradient);
	} else {
		gradient.SetEnd(tabRect.RightTop());
		fDrawingEngine->FillRect(BRect(tabRect.left + 2, tabRect.top + 2,
			tabRect.right, tabRect.bottom - 2), gradient);
	}

	_DrawTitle(tab, tabRect);

	_DrawButtons(tab, invalid);
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
FlatDecorator::_DrawTitle(Decorator::Tab* _tab, BRect rect)
{
	STRACE(("_DrawTitle(%f,%f,%f,%f)\n", rect.left, rect.top, rect.right,
		rect.bottom));

	Decorator::Tab* tab = static_cast<Decorator::Tab*>(_tab);

	const BRect& tabRect = tab->tabRect;
	const BRect& closeRect = tab->closeRect;
	const BRect& zoomRect = tab->zoomRect;

	ComponentColors colors;
	_GetComponentColors(COMPONENT_TAB, colors, tab);

	fDrawingEngine->SetDrawingMode(B_OP_OVER);
	fDrawingEngine->SetHighColor(colors[COLOR_TAB_TEXT]);
	fDrawingEngine->SetFont(fDrawState.Font());

	// figure out position of text
	font_height fontHeight;
	fDrawState.Font().GetHeight(fontHeight);

	BPoint titlePos;
	if (tab->look != kLeftTitledWindowLook) {
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


/*!	\brief Actually draws the close button

	Unless a subclass has a particularly large button, it is probably
	unnecessary to check the update rectangle.

	\param _tab The \a tab to update.
	\param direct Draw without double buffering.
	\param rect The area of the button to update.
*/
void
FlatDecorator::_DrawClose(Decorator::Tab* _tab, bool direct, BRect rect)
{
	STRACE(("_DrawClose(%f,%f,%f,%f)\n", rect.left, rect.top, rect.right,
		rect.bottom));

	Decorator::Tab* tab = static_cast<Decorator::Tab*>(_tab);

	int32 index = (tab->buttonFocus ? 0 : 1) + (tab->closePressed ? 0 : 2);
	ServerBitmap* bitmap = tab->closeBitmaps[index];
	if (bitmap == NULL) {
		bitmap = _GetBitmapForButton(tab, COMPONENT_CLOSE_BUTTON,
			tab->closePressed, rect.IntegerWidth(), rect.IntegerHeight());
		tab->closeBitmaps[index] = bitmap;
	}

	_DrawButtonBitmap(bitmap, direct, rect);
}


/*!	\brief Actually draws the zoom button

	Unless a subclass has a particularly large button, it is probably
	unnecessary to check the update rectangle.

	\param _tab The \a tab to update.
	\param direct Draw without double buffering.
	\param rect The area of the button to update.
*/
void
FlatDecorator::_DrawZoom(Decorator::Tab* _tab, bool direct, BRect rect)
{
	STRACE(("_DrawZoom(%f,%f,%f,%f)\n", rect.left, rect.top, rect.right,
		rect.bottom));

	if (rect.IntegerWidth() < 1)
		return;

	Decorator::Tab* tab = static_cast<Decorator::Tab*>(_tab);
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
FlatDecorator::_DrawMinimize(Decorator::Tab* tab, bool direct, BRect rect)
{
	// This decorator doesn't have this button
}


// #pragma mark - Private methods


void
FlatDecorator::_DrawButtonBitmap(ServerBitmap* bitmap, bool direct,
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
	\param engine The drawing engine to use.
	\param rect The rectangular area to draw in.
	\param down The rectangle should be drawn recessed or not.
	\param colors A button color array of the colors to be used.
*/
void
FlatDecorator::_DrawBlendedRect(DrawingEngine* engine, const BRect rect,
	bool down, const ComponentColors& colors)
{
	engine->FillRect(rect, B_TRANSPARENT_COLOR);

	// figure out which colors to use
	rgb_color startColor, endColor;
	if (down) {
		startColor = tint_color(colors[COLOR_BUTTON], 1.0);
		endColor = tint_color(colors[COLOR_BUTTON], 0.9);
	} else {
		startColor = tint_color(colors[COLOR_BUTTON], 0.95);
		endColor = tint_color(colors[COLOR_BUTTON], 0.5);
	}

	// fill
	BRect fillRect(rect.InsetByCopy(1.0f, 1.0f));

	BGradientLinear gradient;
	gradient.SetStart(fillRect.LeftBottom());
	gradient.SetEnd(fillRect.LeftTop());
	gradient.AddColor(startColor, 0);
	gradient.AddColor(endColor, 250);

	engine->FillRect(fillRect, gradient);

	// outline
	engine->StrokeRect(rect, tint_color(colors[COLOR_BUTTON], 1.25));

}


ServerBitmap*
FlatDecorator::_GetBitmapForButton(Decorator::Tab* tab, Component item,
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

	STRACE(("FlatDecorator creating bitmap for %s %s at size %ldx%ld\n",
		item == COMPONENT_CLOSE_BUTTON ? "close" : "zoom",
		down ? "down" : "up", width, height));
	switch (item) {
		case COMPONENT_CLOSE_BUTTON:
			if (tab && tab->buttonFocus)
				_DrawBlendedRect(sBitmapDrawingEngine, rect, down, colors);
			else
				_DrawBlendedRect(sBitmapDrawingEngine, rect, true, colors);
			break;

		case COMPONENT_ZOOM_BUTTON:
		{
			sBitmapDrawingEngine->FillRect(rect, B_TRANSPARENT_COLOR);
				// init the background

			float inset = floorf(width / 4.0);
			BRect zoomRect(rect);
			zoomRect.left += inset;
			zoomRect.top += inset;
			if (tab && tab->buttonFocus)
				_DrawBlendedRect(sBitmapDrawingEngine, zoomRect, down, colors);
			else
				_DrawBlendedRect(sBitmapDrawingEngine, zoomRect, true, colors);

			inset = floorf(width / 2.1);
			zoomRect = rect;
			zoomRect.right -= inset;
			zoomRect.bottom -= inset;
			if (tab && tab->buttonFocus)
				_DrawBlendedRect(sBitmapDrawingEngine, zoomRect, down, colors);
			else
				_DrawBlendedRect(sBitmapDrawingEngine, zoomRect, true, colors);
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
FlatDecorator::_GetComponentColors(Component component,
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


extern "C" DecorAddOn* (instantiate_decor_addon)(image_id id, const char* name)
{
	return new (std::nothrow)FlatDecorAddOn(id, name);
}
