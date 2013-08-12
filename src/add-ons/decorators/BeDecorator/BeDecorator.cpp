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
#include <Region.h>
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


static const float kBorderResizeLength = 22.0;
static const float kResizeKnobSize = 18.0;


//	#pragma mark - BeDecorAddOn


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


//	#pragma mark - BeDecorator


// TODO: get rid of DesktopSettings here, and introduce private accessor
//	methods to the Decorator base class
BeDecorator::BeDecorator(DesktopSettings& settings, BRect rect)
	:
	TabDecorator(settings, rect)
{
	STRACE(("BeDecorator:\n"));
	STRACE(("\tFrame (%.1f,%.1f,%.1f,%.1f)\n",
		rect.left, rect.top, rect.right, rect.bottom));
}


BeDecorator::~BeDecorator()
{
	STRACE(("BeDecorator: ~BeDecorator()\n"));
	//delete[] fFrameColors;
}


// #pragma mark - Public methods


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


/*!	Returns the frame colors for the specified decorator component.

	The meaning of the color array elements depends on the specified component.
	For some components some array elements are unused.

	\param component The component for which to return the frame colors.
	\param highlight The highlight set for the component.
	\param colors An array of colors to be initialized by the function.
*/
void
BeDecorator::GetComponentColors(Component component, uint8 highlight,
	ComponentColors _colors, Decorator::Tab* _tab)
{
	TabDecorator::Tab* tab = static_cast<TabDecorator::Tab*>(_tab);
	switch (component) {
		case COMPONENT_TAB:
			if (tab && tab->buttonFocus) {
				_colors[COLOR_TAB_FRAME_LIGHT]
					= tint_color(kFocusFrameColor, B_DARKEN_2_TINT);
				_colors[COLOR_TAB_FRAME_DARK]
					= tint_color(kFocusFrameColor, B_DARKEN_3_TINT);
				_colors[COLOR_TAB] = kFocusTabColor;
				_colors[COLOR_TAB_LIGHT] = kFocusTabColorLight;
				_colors[COLOR_TAB_BEVEL] = kFocusTabColorBevel;
				_colors[COLOR_TAB_SHADOW] = kFocusTabColorShadow;
				_colors[COLOR_TAB_TEXT] = kFocusTextColor;
			} else {
				_colors[COLOR_TAB_FRAME_LIGHT]
					= tint_color(kNonFocusFrameColor, B_DARKEN_2_TINT);
				_colors[COLOR_TAB_FRAME_DARK]
					= tint_color(kNonFocusFrameColor, B_DARKEN_3_TINT);
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
		{
			rgb_color base;
			if (tab && tab->buttonFocus)
				base = kFocusFrameColor;
			else
				base = kNonFocusFrameColor;

			//_colors[0].SetColor(152, 152, 152);
			//_colors[1].SetColor(255, 255, 255);
			//_colors[2].SetColor(216, 216, 216);
			//_colors[3].SetColor(136, 136, 136);
			//_colors[4].SetColor(152, 152, 152);
			//_colors[5].SetColor(96, 96, 96);

			_colors[0].red = std::max(0, base.red - 72);
			_colors[0].green = std::max(0, base.green - 72);
			_colors[0].blue = std::max(0, base.blue - 72);
			_colors[0].alpha = 255;

			_colors[1].red = std::min(255, base.red + 64);
			_colors[1].green = std::min(255, base.green  + 64);
			_colors[1].blue = std::min(255, base.blue  + 64);
			_colors[1].alpha = 255;

			_colors[2].red = std::max(0, base.red - 8);
			_colors[2].green = std::max(0, base.green - 8);
			_colors[2].blue = std::max(0, base.blue - 8);
			_colors[2].alpha = 255;

			_colors[3].red = std::max(0, base.red - 88);
			_colors[3].green = std::max(0, base.green - 88);
			_colors[3].blue = std::max(0, base.blue - 88);
			_colors[3].alpha = 255;

			_colors[4].red = std::max(0, base.red - 72);
			_colors[4].green = std::max(0, base.green - 72);
			_colors[4].blue = std::max(0, base.blue - 72);
			_colors[4].alpha = 255;

			_colors[5].red = std::max(0, base.red - 128);
			_colors[5].green = std::max(0, base.green - 128);
			_colors[5].blue = std::max(0, base.blue - 128);
			_colors[5].alpha = 255;

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
}


// #pragma mark - Protected methods


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
				ComponentColors colors;
				_GetComponentColors(COMPONENT_TOP_BORDER, colors, fTopTab);

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
			if (invalid.Intersects(fLeftBorder.InsetByCopy(0, -BorderWidth()))) {
				ComponentColors colors;
				_GetComponentColors(COMPONENT_LEFT_BORDER, colors, fTopTab);

				for (int8 i = 0; i < 5; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.top + i),
						BPoint(r.left + i, r.bottom - i), colors[i]);
				}
			}
			// bottom
			if (invalid.Intersects(fBottomBorder)) {
				ComponentColors colors;
				_GetComponentColors(COMPONENT_BOTTOM_BORDER, colors, fTopTab);

				for (int8 i = 0; i < 5; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.bottom - i),
						BPoint(r.right - i, r.bottom - i),
						colors[(4 - i) == 4 ? 5 : (4 - i)]);
				}
			}
			// right
			if (invalid.Intersects(fRightBorder.InsetByCopy(0, -BorderWidth()))) {
				ComponentColors colors;
				_GetComponentColors(COMPONENT_RIGHT_BORDER, colors, fTopTab);

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
				_GetComponentColors(COMPONENT_TOP_BORDER, colors, fTopTab);

				for (int8 i = 0; i < 3; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.top + i),
						BPoint(r.right - i, r.top + i), colors[i * 2]);
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
			if (invalid.Intersects(fLeftBorder.InsetByCopy(0, -BorderWidth()))) {
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
			if (invalid.Intersects(fBottomBorder)) {
				ComponentColors colors;
				_GetComponentColors(COMPONENT_BOTTOM_BORDER, colors, fTopTab);

				for (int8 i = 0; i < 3; i++) {
					fDrawingEngine->StrokeLine(BPoint(r.left + i, r.bottom - i),
						BPoint(r.right - i, r.bottom - i),
						colors[(2 - i) == 2 ? 5 : (2 - i) * 2]);
				}
			}
			// right
			if (invalid.Intersects(fRightBorder.InsetByCopy(0, -BorderWidth()))) {
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
BeDecorator::_DrawTab(Decorator::Tab* tab, BRect invalid)
{
	STRACE(("_DrawTab(%.1f, %.1f, %.1f, %.1f)\n",
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
	if (tab->look != kLeftTitledWindowLook) {
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
			tabBotton - (tab->look == kLeftTitledWindowLook ? 1 : 0)),
		colors[COLOR_TAB_BEVEL]);
	fDrawingEngine->StrokeLine(BPoint(tabRect.left + 1, tabRect.top + 1),
		BPoint(tabRect.right - (tab->look == kLeftTitledWindowLook ? 0 : 1),
			tabRect.top + 1),
		colors[COLOR_TAB_BEVEL]);

	if (tab->look != kLeftTitledWindowLook) {
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
	if (fTopTab->look != kLeftTitledWindowLook) {
		fDrawingEngine->FillRect(BRect(tabRect.left + 2, tabRect.top + 2,
			tabRect.right - 2, tabRect.bottom), colors[COLOR_TAB]);
	} else {
		fDrawingEngine->FillRect(BRect(tabRect.left + 2, tabRect.top + 2,
			tabRect.right, tabRect.bottom - 2), colors[COLOR_TAB]);
	}

	_DrawTitle(tab, tabRect);

	DrawButtons(tab, invalid);
}


void
BeDecorator::_DrawTitle(Decorator::Tab* _tab, BRect r)
{
	STRACE(("_DrawTitle(%f, %f, %f, %f)\n", r.left, r.top, r.right, r.bottom));

	TabDecorator::Tab* tab = static_cast<TabDecorator::Tab*>(_tab);

	const BRect& tabRect = tab->tabRect;
	const BRect& closeRect = tab->closeRect;
	const BRect& zoomRect = tab->zoomRect;

	ComponentColors colors;
	_GetComponentColors(COMPONENT_TAB, colors, tab);

	fDrawingEngine->SetDrawingMode(B_OP_OVER);
	fDrawingEngine->SetHighColor(ui_color(B_WINDOW_TEXT_COLOR));
	fDrawingEngine->SetLowColor(colors[COLOR_TAB]);
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
BeDecorator::_DrawClose(Decorator::Tab* _tab, bool direct, BRect rect)
{
	STRACE(("_DrawClose(%f,%f,%f,%f)\n", rect.left, rect.top, rect.right,
		rect.bottom));

	TabDecorator::Tab* tab = static_cast<TabDecorator::Tab*>(_tab);

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
BeDecorator::_DrawZoom(Decorator::Tab* _tab, bool direct, BRect rect)
{
	STRACE(("_DrawZoom(%f,%f,%f,%f)\n", rect.left, rect.top, rect.right,
		rect.bottom));

	TabDecorator::Tab* tab = static_cast<TabDecorator::Tab*>(_tab);
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
BeDecorator::_GetButtonSizeAndOffset(const BRect& tabRect, float* _offset,
	float* _size, float* _inset) const
{
	float tabSize = fTopTab->look == kLeftTitledWindowLook ?
		tabRect.Width() : tabRect.Height();

	*_offset = 5.0f;
	*_inset = 0.0f;

	*_size = std::max(0.0f, tabSize - 7.0f);
}


// #pragma mark - Private methods


/*!
	\brief Draws a bevel around a rectangle.
	\param rect The rectangular area to draw in.
	\param down Whether or not the button is pressed down.
	\param light The light color to use.
	\param shadow The shadow color to use.
*/
void
BeDecorator::_DrawBevelRect(DrawingEngine* engine, const BRect rect, bool down,
	rgb_color light, rgb_color shadow)
{
	if (down) {
		BRect inner(rect.InsetByCopy(1.0f, 1.0f));

		engine->StrokeLine(rect.LeftBottom(), rect.LeftTop(), shadow);
		engine->StrokeLine(rect.LeftTop(), rect.RightTop(), shadow);
		engine->StrokeLine(inner.LeftBottom(), inner.LeftTop(), shadow);
		engine->StrokeLine(inner.LeftTop(), inner.RightTop(), shadow);

		engine->StrokeLine(rect.RightTop(), rect.RightBottom(), light);
		engine->StrokeLine(rect.RightBottom(), rect.LeftBottom(), light);
		engine->StrokeLine(inner.RightTop(), inner.RightBottom(), light);
		engine->StrokeLine(inner.RightBottom(), inner.LeftBottom(), light);
	} else {
		BRect r1(rect);
		r1.left += 1.0f;
		r1.top  += 1.0f;

		BRect r2(rect);
		r2.bottom -= 1.0f;
		r2.right  -= 1.0f;

		engine->StrokeRect(r2, shadow);
			// inner dark box
		engine->StrokeRect(rect, shadow);
			// outer dark box
		engine->StrokeRect(r1, light);
			// light box
	}
}


/*!
	\brief Draws a framed rectangle with a gradient.
	\param rect The rectangular area to draw in.
	\param startColor The start color of the gradient.
	\param endColor The end color of the gradient.
*/
void
BeDecorator::_DrawBlendedRect(DrawingEngine* engine, const BRect rect,
	bool down, rgb_color colorA, rgb_color colorB, rgb_color colorC,
	rgb_color colorD)
{
	BRect fillRect(rect.InsetByCopy(1.0f, 1.0f));

	BGradientLinear gradient;
	if (down) {
		gradient.SetStart(fillRect.RightBottom());
		gradient.SetEnd(fillRect.LeftTop());
	} else {
		gradient.SetStart(fillRect.LeftTop());
		gradient.SetEnd(fillRect.RightBottom());
	}

	gradient.AddColor(colorA, 0);
	gradient.AddColor(colorB, 95);
	gradient.AddColor(colorC, 159);
	gradient.AddColor(colorD, 255);

	engine->FillRect(fillRect, gradient);
}


void
BeDecorator::_DrawButtonBitmap(ServerBitmap* bitmap, bool direct, BRect rect)
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


ServerBitmap*
BeDecorator::_GetBitmapForButton(Decorator::Tab* tab, Component item,
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

	const rgb_color buttonColor(colors[COLOR_BUTTON]);

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

	STRACE(("BeDecorator creating bitmap for %s %s at size %ldx%ld\n",
		item == COMPONENT_CLOSE_BUTTON ? "close" : "zoom",
		down ? "down" : "up", width, height));
	switch (item) {
		case COMPONENT_CLOSE_BUTTON:
		{
			rgb_color buttonColorLight1(buttonColor);
			buttonColorLight1.red = std::min(255, buttonColor.red + 32),
			buttonColorLight1.green = std::min(255, buttonColor.green + 32),
			buttonColorLight1.blue = std::min(255, buttonColor.blue + 32);

			rgb_color buttonColorLight2(buttonColor);
			buttonColorLight2.red = std::min(255, buttonColor.red + 64),
			buttonColorLight2.green = std::min(255, buttonColor.green + 64),
			buttonColorLight2.blue = std::min(255, buttonColor.blue + 64);

			rgb_color buttonColorShadow1(buttonColor);
			buttonColorShadow1.red = std::max(0, buttonColor.red - 36),
			buttonColorShadow1.green = std::max(0, buttonColor.green - 36),
			buttonColorShadow1.blue = std::max(0, buttonColor.blue - 36);

			rgb_color buttonColorShadow2(buttonColor);
			buttonColorShadow2.red = std::max(0, buttonColor.red - 72),
			buttonColorShadow2.green = std::max(0, buttonColor.green - 72),
			buttonColorShadow2.blue = std::max(0, buttonColor.blue - 72);

			_DrawBlendedRect(sBitmapDrawingEngine, rect, tab->closePressed,
				buttonColorLight2, buttonColorLight1, buttonColor,
				buttonColorShadow1);

			_DrawBevelRect(sBitmapDrawingEngine, rect, tab->closePressed,
				buttonColorLight2, buttonColorShadow2);

			break;
		}

		case COMPONENT_ZOOM_BUTTON:
		{
			sBitmapDrawingEngine->FillRect(rect, B_TRANSPARENT_COLOR);
				// init the background

			rgb_color buttonColorLight1(buttonColor);
			buttonColorLight1.red = std::min(255, buttonColor.red + 32),
			buttonColorLight1.green = std::min(255, buttonColor.green + 32),
			buttonColorLight1.blue = std::min(255, buttonColor.blue + 32);

			rgb_color buttonColorLight2(buttonColor);
			buttonColorLight2.red = std::min(255, buttonColor.red + 64),
			buttonColorLight2.green = std::min(255, buttonColor.green + 64),
			buttonColorLight2.blue = std::min(255, buttonColor.blue + 64);

			rgb_color buttonColorShadow1(buttonColor);
			buttonColorShadow1.red = std::max(0, buttonColor.red - 22),
			buttonColorShadow1.green = std::max(0, buttonColor.green - 23),
			buttonColorShadow1.blue = std::max(0, buttonColor.blue - 5);

			rgb_color buttonColorShadow2(buttonColor);
			buttonColorShadow2.red = std::max(0, buttonColor.red - 45),
			buttonColorShadow2.green = std::max(0, buttonColor.green - 47),
			buttonColorShadow2.blue = std::max(0, buttonColor.blue - 10);

			// big rect

			BRect zoomRect = rect;
			zoomRect.left += floorf(width * 3.0f / 14.0f);
			zoomRect.top += floorf(height * 3.0f / 14.0f);

			_DrawBlendedRect(sBitmapDrawingEngine, zoomRect, tab->zoomPressed,
				buttonColorLight2, buttonColorLight1, buttonColor,
				buttonColorShadow1);

			_DrawBevelRect(sBitmapDrawingEngine, zoomRect, tab->zoomPressed,
				buttonColorLight2, buttonColorShadow2);

			// small rect

			zoomRect = rect;
			zoomRect.right -= floorf(width * 5.0f / 14.0f);
			zoomRect.bottom -= floorf(height * 5.0f / 14.0f);

			_DrawBlendedRect(sBitmapDrawingEngine, zoomRect, tab->zoomPressed,
				buttonColorLight2, buttonColorLight1, buttonColor,
				buttonColorShadow1);

			_DrawBevelRect(sBitmapDrawingEngine, zoomRect, tab->zoomPressed,
				buttonColorLight2, buttonColorShadow2);

			// Fill in the right top and bottom right corners with buttonColor
			sBitmapDrawingEngine->StrokeLine(zoomRect.RightTop(),
				zoomRect.RightTop(), buttonColor);
			sBitmapDrawingEngine->StrokeLine(zoomRect.LeftBottom(),
				zoomRect.LeftBottom(), buttonColor);

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
BeDecorator::_GetComponentColors(Component component,
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
	return new (std::nothrow)BeDecorAddOn(id, name);
}
