/*
 * Copyright 2006, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "GradientControl.h"

#include <stdio.h>

#include <AppDefs.h>
#include <Bitmap.h>
#include <ControlLook.h>
#include <Message.h>
#include <Window.h>

#include "ui_defines.h"
#include "support_ui.h"

#include "GradientTransformable.h"


GradientControl::GradientControl(BMessage* message, BHandler* target)
	: BView(BRect(0, 0, 100, 19), "gradient control", B_FOLLOW_NONE,
			B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE),
	  fGradient(new ::Gradient()),
	  fGradientBitmap(NULL),
	  fDraggingStepIndex(-1),
	  fCurrentStepIndex(-1),
	  fDropOffset(-1.0),
	  fDropIndex(-1),
	  fEnabled(true),
	  fMessage(message),
	  fTarget(target)
{
	FrameResized(Bounds().Width(), Bounds().Height());
	SetViewColor(B_TRANSPARENT_32_BIT);
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


GradientControl::~GradientControl()
{
	delete fGradient;
	delete fGradientBitmap;
	delete fMessage;
}


#if LIB_LAYOUT

minimax
GradientControl::layoutprefs()
{
	mpm.mini.x = 256 + 4;
	mpm.maxi.x = mpm.mini.x + 10000;
	mpm.mini.y = 20;
	mpm.maxi.y = mpm.mini.y + 10;

	mpm.weight = 2.0;

	return mpm;
}


BRect
GradientControl::layout(BRect frame)
{
	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(), frame.Height());
	return Frame();
}

#endif // LIB_LAYOUT


void
GradientControl::WindowActivated(bool active)
{
	if (IsFocus())
		Invalidate();
}


void
GradientControl::MakeFocus(bool focus)
{
	if (focus != IsFocus()) {
		_UpdateCurrentColor();
		Invalidate();
		if (fTarget) {
			if (BLooper* looper = fTarget->Looper())
				looper->PostMessage(MSG_GRADIENT_CONTROL_FOCUS_CHANGED, fTarget);
		}
	}
	BView::MakeFocus(focus);
}


void
GradientControl::MouseDown(BPoint where)
{
	if (!fEnabled)
		return;

	if (!IsFocus()) {
		MakeFocus(true);
	}

	fDraggingStepIndex = _StepIndexFor(where);

	if (fDraggingStepIndex >= 0)
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);

	// handle double click
	int32 clicks;
	if (Window()->CurrentMessage()->FindInt32("clicks", &clicks) >= B_OK && clicks >= 2) {
		if (fDraggingStepIndex < 0) {
			// create a new offset at the click location that uses
			// the interpolated color
			float offset = _OffsetFor(where);
			// create a clean gradient
			uint32 width = fGradientBitmap->Bounds().IntegerWidth();
			uint8* temp = new uint8[width * 4];
			fGradient->MakeGradient((uint32*)temp, width);
			// get the color at the offset
			rgb_color color;
			uint8* bits = temp;
			bits += 4 * (uint32)((width - 1) * offset);
			color.red = bits[0];
			color.green = bits[1];
			color.blue = bits[2];
			color.alpha = bits[3];
			fCurrentStepIndex = fGradient->AddColor(color, offset);
			fDraggingStepIndex = -1;
			_UpdateColors();
			Invalidate();
			_UpdateCurrentColor();
			delete[] temp;
		}
	}

	if (fCurrentStepIndex != fDraggingStepIndex && fDraggingStepIndex >= 0) {
		// start dragging this stop
		fCurrentStepIndex = fDraggingStepIndex;
		Invalidate();
		_UpdateCurrentColor();
	}
}


void
GradientControl::MouseUp(BPoint where)
{
	fDraggingStepIndex = -1;
}


void
GradientControl::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	if (!fEnabled)
		return;

	float offset = _OffsetFor(where);

	if (fDraggingStepIndex >= 0) {
		BGradient::ColorStop* step = fGradient->ColorAt(fDraggingStepIndex);
		if (step) {
			if (fGradient->SetOffset(fDraggingStepIndex, offset)) {
				_UpdateColors();
				Invalidate();
			}
		}
	}
	int32 dropIndex = -1;
	float dropOffset = -1.0;
	if (dragMessage && (transit == B_INSIDE_VIEW || transit == B_ENTERED_VIEW)) {
		rgb_color dragColor;
		if (restore_color_from_message(dragMessage, dragColor, 0) >= B_OK) {
			dropIndex = _StepIndexFor(where);
			// fall back to inserting a color step if no direct hit on an existing step
			if (dropIndex < 0)
				dropOffset = offset;
		}
	}
	if (fDropOffset != dropOffset || fDropIndex != dropIndex) {
		fDropOffset = dropOffset;
		fDropIndex = dropIndex;
		Invalidate();
	}
}


void
GradientControl::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_PASTE:
			if (fEnabled) {
				rgb_color color;
				if (restore_color_from_message(message, color, 0) >= B_OK) {
					bool update = false;
					if (fDropIndex >= 0) {
						if (BGradient::ColorStop* step
								= fGradient->ColorAt(fDropIndex)) {
							color.alpha = step->color.alpha;
						}
						fGradient->SetColor(fDropIndex, color);
						fCurrentStepIndex = fDropIndex;
						fDropIndex = -1;
						update = true;
					} else if (fDropOffset >= 0.0) {
						fCurrentStepIndex = fGradient->AddColor(color,
							fDropOffset);
						fDropOffset = -1.0;
						update = true;
					}
					if (update) {
						_UpdateColors();
						if (!IsFocus())
							MakeFocus(true);
						else
							Invalidate();
						_UpdateCurrentColor();
					}
				}
			}
			break;
		default:
			BView::MessageReceived(message);
	}
}


void
GradientControl::KeyDown(const char* bytes, int32 numBytes)
{
	bool handled = false;
	bool update = false;
	if (fEnabled) {
		if (numBytes > 0) {
			handled = true;
			int32 count = fGradient->CountColors();
			switch (bytes[0]) {
				case B_DELETE:
					// remove step
					update = fGradient->RemoveColor(fCurrentStepIndex);
					if (update) {
						fCurrentStepIndex = max_c(0, fCurrentStepIndex - 1);
						_UpdateCurrentColor();
					}
					break;

				case B_HOME:
				case B_END:
				case B_LEFT_ARROW:
				case B_RIGHT_ARROW: {
					if (BGradient::ColorStop* step
							= fGradient->ColorAt(fCurrentStepIndex)) {
						BRect r = _GradientBitmapRect();
						float x = r.left + r.Width() * step->offset;
						switch (bytes[0]) {
							case B_LEFT_ARROW:
								// move step to the left
								x = max_c(r.left, x - 1.0);
								break;
							case B_RIGHT_ARROW:
								// move step to the right
								x = min_c(r.right, x + 1.0);
								break;
							case B_HOME:
								// move step to the start
								x = r.left;
								break;
							case B_END:
								// move step to the start
								x = r.right;
								break;
						}
						update = fGradient->SetOffset(fCurrentStepIndex,
							(x - r.left) / r.Width());
					}
					break;
				}

				case B_UP_ARROW:
					// previous step
					fCurrentStepIndex--;
					if (fCurrentStepIndex < 0) {
						fCurrentStepIndex = count - 1;
					}
					_UpdateCurrentColor();
					break;
				case B_DOWN_ARROW:
					// next step
					fCurrentStepIndex++;
					if (fCurrentStepIndex >= count) {
						fCurrentStepIndex = 0;
					}
					_UpdateCurrentColor();
					break;

				default:
					handled = false;
					break;
			}
		}
	}
	if (!handled)
		BView::KeyDown(bytes, numBytes);
	else {
		if (update)
			_UpdateColors();
		Invalidate();
	}
}


void
GradientControl::Draw(BRect updateRect)
{
	BRect b = _GradientBitmapRect();
	b.InsetBy(-2.0, -2.0);
	// background
	// left of gradient rect
	BRect lb(updateRect.left, updateRect.top, b.left - 1, b.bottom);
	if (lb.IsValid())
		FillRect(lb, B_SOLID_LOW);
	// right of gradient rect
	BRect rb(b.right + 1, updateRect.top, updateRect.right, b.bottom);
	if (rb.IsValid())
		FillRect(rb, B_SOLID_LOW);
	// bottom of gradient rect
	BRect bb(updateRect.left, b.bottom + 1, updateRect.right, updateRect.bottom);
	if (bb.IsValid())
		FillRect(bb, B_SOLID_LOW);

	bool isFocus = IsFocus() && Window()->IsActive();

	rgb_color bg = LowColor();
	rgb_color black;
	rgb_color shadow;
	rgb_color darkShadow;
	rgb_color light;

	if (fEnabled) {
		shadow = tint_color(bg, B_DARKEN_1_TINT);
		darkShadow = tint_color(bg, B_DARKEN_3_TINT);
		light = tint_color(bg, B_LIGHTEN_MAX_TINT);
		black = tint_color(bg, B_DARKEN_MAX_TINT);
	} else {
		shadow = bg;
		darkShadow = tint_color(bg, B_DARKEN_1_TINT);
		light = tint_color(bg, B_LIGHTEN_2_TINT);
		black = tint_color(bg, B_DARKEN_2_TINT);
	}

	rgb_color focus = isFocus ? ui_color(B_KEYBOARD_NAVIGATION_COLOR) : black;

	uint32 flags = 0;

	if (be_control_look != NULL) {
		if (!fEnabled)
			flags |= BControlLook::B_DISABLED;
		if (isFocus)
			flags |= BControlLook::B_FOCUSED;
		be_control_look->DrawTextControlBorder(this, b, updateRect, bg, flags);
	} else {	
		stroke_frame(this, b, shadow, shadow, light, light);
		b.InsetBy(1.0, 1.0);
		if (isFocus)
			stroke_frame(this, b, focus, focus, focus, focus);
		else
			stroke_frame(this, b, darkShadow, darkShadow, bg, bg);
		b.InsetBy(1.0, 1.0);
	}

//	DrawBitmapAsync(fGradientBitmap, b.LeftTop());
//	Sync();
	DrawBitmap(fGradientBitmap, b.LeftTop());

	// show drop offset
	if (fDropOffset >= 0.0) {
		SetHighColor(255, 0, 0, 255);
		float x = b.left + b.Width() * fDropOffset;
		StrokeLine(BPoint(x, b.top), BPoint(x, b.bottom));
	}

	BPoint markerPos;
	markerPos.y = b.bottom + 4.0;
	BPoint leftBottom(-6.0, 6.0);
	BPoint rightBottom(6.0, 6.0);
	for (int32 i = 0; BGradient::ColorStop* step = fGradient->ColorAt(i);
			i++) {
		markerPos.x = b.left + (b.Width() * step->offset);

		if (i == fCurrentStepIndex) {
			SetLowColor(focus);
			if (isFocus) {
				StrokeLine(markerPos + leftBottom + BPoint(1.0, 4.0),
					markerPos + rightBottom + BPoint(-1.0, 4.0), B_SOLID_LOW);
			}
		} else {
			SetLowColor(black);
		}

		// override in case this is the drop index step
		if (i == fDropIndex)
			SetLowColor(255, 0, 0, 255);

		if (be_control_look != NULL) {
			// TODO: Drop indication!
			BRect rect(markerPos.x + leftBottom.x, markerPos.y,
				markerPos.x + rightBottom.x, markerPos.y + rightBottom.y);
			be_control_look->DrawSliderTriangle(this, rect, updateRect, bg,
				step->color, flags, B_HORIZONTAL);
		} else {
			StrokeTriangle(markerPos, markerPos + leftBottom,
				markerPos + rightBottom, B_SOLID_LOW);
			if (fEnabled) {
				SetHighColor(step->color);
			} else {
				rgb_color c = step->color;
				c.red = (uint8)(((uint32)bg.red + (uint32)c.red) / 2);
				c.green = (uint8)(((uint32)bg.green + (uint32)c.green) / 2);
				c.blue = (uint8)(((uint32)bg.blue + (uint32)c.blue) / 2);
				SetHighColor(c);
			}
			FillTriangle(markerPos + BPoint(0.0, 1.0),
						 markerPos + leftBottom + BPoint(1.0, 0.0),
						 markerPos + rightBottom + BPoint(-1.0, 0.0));
			StrokeLine(markerPos + leftBottom + BPoint(0.0, 1.0),
					   markerPos + rightBottom + BPoint(0.0, 1.0), B_SOLID_LOW);
		}
	}
}


void
GradientControl::FrameResized(float width, float height)
{
	BRect r = _GradientBitmapRect();
	_AllocBitmap(r.IntegerWidth() + 1, r.IntegerHeight() + 1);
	_UpdateColors();
	Invalidate();

}


void
GradientControl::GetPreferredSize(float* width, float* height)
{
	if (width != NULL)
		*width = 100;

	if (height != NULL)
		*height = 19;
}


void
GradientControl::SetGradient(const ::Gradient* gradient)
{
	if (!gradient)
		return;

	*fGradient = *gradient;
	_UpdateColors();

	fDropOffset = -1.0;
	fDropIndex = -1;
	fDraggingStepIndex = -1;
	if (fCurrentStepIndex > gradient->CountColors() - 1)
		fCurrentStepIndex = gradient->CountColors() - 1;

	Invalidate();
}


void
GradientControl::SetCurrentStop(const rgb_color& color)
{
	if (fEnabled && fCurrentStepIndex >= 0) {
		fGradient->SetColor(fCurrentStepIndex, color);
		_UpdateColors();
		Invalidate();
	}
}


bool
GradientControl::GetCurrentStop(rgb_color* color) const
{
	BGradient::ColorStop* stop
		= fGradient->ColorAt(fCurrentStepIndex);
	if (stop && color) {
		*color = stop->color;
		return true;
	}
	return false;
}


void
GradientControl::SetEnabled(bool enabled)
{
	if (enabled == fEnabled)
		return;

	fEnabled = enabled;

	if (!fEnabled)
		fDropIndex = -1;

	_UpdateColors();
	Invalidate();
}


inline void
blend_colors(uint8* d, uint8 alpha, uint8 c1, uint8 c2, uint8 c3)
{
	if (alpha > 0) {
		if (alpha == 255) {
			d[0] = c1;
			d[1] = c2;
			d[2] = c3;
		} else {
			d[0] += (uint8)(((c1 - d[0]) * alpha) >> 8);
			d[1] += (uint8)(((c2 - d[1]) * alpha) >> 8);
			d[2] += (uint8)(((c3 - d[2]) * alpha) >> 8);
		}
	}
}


void
GradientControl::_UpdateColors()
{
	if (!fGradientBitmap || !fGradientBitmap->IsValid())
		return;

	// fill in top row by gradient
	uint8* topRow = (uint8*)fGradientBitmap->Bits();
	uint32 width = fGradientBitmap->Bounds().IntegerWidth() + 1;
	fGradient->MakeGradient((uint32*)topRow, width);
	// flip colors, since they are the wrong endianess
	// make colors the disabled look
	// TODO: apply gamma lut
	uint8* p = topRow;
	if (!fEnabled) {
		rgb_color bg = LowColor();
		for (uint32 x = 0; x < width; x++) {
			uint8 p0 = p[0];
			p[0] = (uint8)(((uint32)bg.blue + (uint32)p[2]) / 2);
			p[1] = (uint8)(((uint32)bg.green + (uint32)p[1]) / 2);
			p[2] = (uint8)(((uint32)bg.red + (uint32)p0) / 2);
			p += 4;
		}
	} else {
		for (uint32 x = 0; x < width; x++) {
			uint8 p0 = p[0];
			p[0] = p[2];
			p[2] = p0;
			p += 4;
		}
	}
	// copy top row to rest of bitmap
	uint32 height = fGradientBitmap->Bounds().IntegerHeight() + 1;
	uint32 bpr = fGradientBitmap->BytesPerRow();
	uint8* dstRow = topRow + bpr;
	for (uint32 i = 1; i < height; i++) {
		memcpy(dstRow, topRow, bpr);
		dstRow += bpr;
	}
	// post process bitmap to underlay it with a pattern
	// in order to make gradient steps with alpha more visible!
	uint8* row = topRow;
	for (uint32 i = 0; i < height; i++) {
		uint8* p = row;
		for (uint32 x = 0; x < width; x++) {
			uint8 alpha = p[3];
			if (alpha < 255) {
				p[3] = 255;
				alpha = 255 - alpha;
				if (x % 8 >= 4) {
					if (i % 8 >= 4) {
						blend_colors(p, alpha,
									 kAlphaLow.blue,
									 kAlphaLow.green,
									 kAlphaLow.red);
					} else {
						blend_colors(p, alpha,
									 kAlphaHigh.blue,
									 kAlphaHigh.green,
									 kAlphaHigh.red);
					}
				} else {
					if (i % 8 >= 4) {
						blend_colors(p, alpha,
									 kAlphaHigh.blue,
									 kAlphaHigh.green,
									 kAlphaHigh.red);
					} else {
						blend_colors(p, alpha,
									 kAlphaLow.blue,
									 kAlphaLow.green,
									 kAlphaLow.red);
					}
				}
			}
			p += 4;
		}
		row += bpr;
	}
}


void
GradientControl::_AllocBitmap(int32 width, int32 height)
{
	if (width < 2 || height < 2)
		return;

	delete fGradientBitmap;
	fGradientBitmap = new BBitmap(BRect(0, 0, width - 1, height - 1), 0, B_RGB32);
}


BRect
GradientControl::_GradientBitmapRect() const
{
	BRect r = Bounds();
	r.left += 6.0;
	r.top += 2.0;
	r.right -= 6.0;
	r.bottom -= 14.0;
	return r;
}


int32
GradientControl::_StepIndexFor(BPoint where) const
{
	int32 index = -1;
	BRect r = _GradientBitmapRect();
	BRect markerFrame(Bounds());
	for (int32 i = 0; BGradient::ColorStop* step
			= fGradient->ColorAt(i); i++) {
		markerFrame.left = markerFrame.right = r.left
			+ (r.Width() * step->offset);
		markerFrame.InsetBy(-6.0, 0.0);
		if (markerFrame.Contains(where)) {
			index = i;
			break;
		}
	}
	return index;
}


float
GradientControl::_OffsetFor(BPoint where) const
{
	BRect r = _GradientBitmapRect();
	float offset = (where.x - r.left) / r.Width();
	offset = max_c(0.0, offset);
	offset = min_c(1.0, offset);
	return offset;
}


void
GradientControl::_UpdateCurrentColor() const
{
	if (!fMessage || !fTarget || !fTarget->Looper())
		return;
	// set the CanvasView current color
	if (BGradient::ColorStop* step = fGradient->ColorAt(fCurrentStepIndex)) {
		BMessage message(*fMessage);
		store_color_in_message(&message, step->color);
		fTarget->Looper()->PostMessage(&message, fTarget);
	}
}
