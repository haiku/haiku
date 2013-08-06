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
#include "ServerBitmap.h"


//#define DEBUG_DECORATOR
#ifdef DEBUG_DECORATOR
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif


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


/*!	\brief Function for calculating gradient colors
	\param colorA Start color
	\param colorB End color
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
	TabDecorator(settings, rect)
{
	//fFrameColors = new RGBColor[6];
	//fFrameColors[0].SetColor(152, 152, 152);
	//fFrameColors[1].SetColor(255, 255, 255);
	//fFrameColors[2].SetColor(216, 216, 216);
	//fFrameColors[3].SetColor(136, 136, 136);
	//fFrameColors[4].SetColor(152, 152, 152);
	//fFrameColors[5].SetColor(96, 96, 96);

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
BeDecorator::_DrawClose(Decorator::Tab* tab, bool direct, BRect rect)
{
	STRACE(("_DrawClose(%f,%f,%f,%f)\n", rect.left, rect.top, rect.right,
		rect.bottom));
	// Just like DrawZoom, but for a close button
	_DrawBlendedRect(tab, rect, tab->closePressed);
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
BeDecorator::_DrawZoom(Decorator::Tab* tab, bool direct, BRect rect)
{
	STRACE(("_DrawZoom(%f,%f,%f,%f)\n", rect.left, rect.top, rect.right,
		rect.bottom));
	// If this has been implemented, then the decorator has a Zoom button
	// which should be drawn based on the state of the member zoomstate

	BRect zoomRect(rect);
	zoomRect.left += 3.0;
	zoomRect.top += 3.0;
	_DrawBlendedRect(tab, zoomRect, tab->zoomPressed);

	zoomRect = rect;
	zoomRect.right -= 5.0;
	zoomRect.bottom -= 5.0;
	_DrawBlendedRect(tab, zoomRect, tab->zoomPressed);
}


/*!
	\brief Draws a framed rectangle with a gradient.
	\param tab The tab to draw on.
	\param rect The rectangular area to draw in.
	\param down Whether or not the button is drawn in down state.
*/
void
BeDecorator::_DrawBlendedRect(Decorator::Tab* tab, BRect rect, bool down)
{
	BRect r1 = rect;
	BRect r2 = rect;

	r1.left += 1.0;
	r1.top  += 1.0;

	r2.bottom -= 1.0;
	r2.right  -= 1.0;

	ComponentColors colors;
	_GetComponentColors(COMPONENT_TAB, colors, tab);

	const rgb_color tabColor(colors[COLOR_TAB]);

	rgb_color tabColorLight(tabColor);
	tabColorLight.red = std::min(255, tabColor.red + 64),
	tabColorLight.green = std::min(255, tabColor.green + 64),
	tabColorLight.blue = std::min(255, tabColor.blue + 64);

	rgb_color tabColorShadow(tabColor);
	tabColorShadow.red = std::max(0, tabColor.red - 72),
	tabColorShadow.green = std::max(0, tabColor.green - 72),
	tabColorShadow.blue = std::max(0, tabColor.blue - 72);

	int32 w = rect.IntegerWidth();
	int32 h = rect.IntegerHeight();
	int32 steps = w < h ? w : h;

	rgb_color startColor;
	rgb_color endColor;

	if (down) {
		startColor = tabColorShadow;
		endColor = tabColorLight;
	} else {
		startColor = tabColorLight;
		endColor = tabColorShadow;
	}

	rgb_color halfColor(make_blend_color(startColor, endColor, 0.5));

	float rstep = float(startColor.red - halfColor.red) / steps;
	float gstep = float(startColor.green - halfColor.green) / steps;
	float bstep = float(startColor.blue - halfColor.blue) / steps;

	for (int32 i = 0; i < steps; i += steps / 3) {
		RGBColor tempRGBCol;

		tempRGBCol.SetColor(uint8(startColor.red - (i * rstep)),
							uint8(startColor.green - (i * gstep)),
							uint8(startColor.blue - (i * bstep)));

		for (int32 j = i; j < steps && j < i + steps / 3; j++) {
			fDrawingEngine->StrokeLine(BPoint(rect.left, rect.top + j),
				BPoint(rect.left + j, rect.top), tempRGBCol);
		}

		tempRGBCol.SetColor(uint8(halfColor.red - (i * rstep)),
							uint8(halfColor.green - (i * gstep)),
							uint8(halfColor.blue - (i * bstep)));

		for (int32 j = i; j < steps && j < i + steps / 3; j++) {
			fDrawingEngine->StrokeLine(BPoint(rect.left + steps, rect.top + j),
				BPoint(rect.left + j, rect.top + steps), tempRGBCol);
		}
	}

	// draw bevel effect on box
	if (down) {
		fDrawingEngine->StrokeRect(r2,   RGBColor(tabColorLight));
			// inner light box
		fDrawingEngine->StrokeRect(rect, RGBColor(tabColorLight));
			// outer light box
		fDrawingEngine->StrokeRect(r1,   RGBColor(tabColorShadow));
			// dark box
	} else {
		fDrawingEngine->StrokeRect(r2,   RGBColor(tabColorShadow));
			// inner dark box
		fDrawingEngine->StrokeRect(rect, RGBColor(tabColorShadow));
			// outer dark box
		fDrawingEngine->StrokeRect(r1,   RGBColor(tabColorLight));
			// light box
	}
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
