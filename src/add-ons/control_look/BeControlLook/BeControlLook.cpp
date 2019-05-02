/*
 * Copyright 2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		DarkWyrm, bpmagic@columbus.rr.com
 *		John Scipione, jscipione@gmail.com
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */


/*! BControlLook resembling BeOS R5 */


#include "BeControlLook.h"

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


//#define DEBUG_CONTROL_LOOK
#ifdef DEBUG_CONTROL_LOOK
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif


BeControlLook::BeControlLook(image_id id)
	: HaikuControlLook()
{
}


BeControlLook::~BeControlLook()
{
}


void
BeControlLook::DrawMenuBarBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	// the surface edges

	// colors
	float topTint;
	float bottomTint;

	if ((flags & B_ACTIVATED) != 0) {
		rgb_color bevelColor1 = tint_color(base, 1.40);
		rgb_color bevelColor2 = tint_color(base, 1.25);

		topTint = 1.25;
		bottomTint = 1.20;

		_DrawFrame(view, rect,
			bevelColor1, bevelColor1,
			bevelColor2, bevelColor2,
			borders & B_TOP_BORDER);
	} else {
		rgb_color cornerColor = tint_color(base, 0.9);
		rgb_color bevelColorTop = tint_color(base, 0.5);
		rgb_color bevelColorLeft = tint_color(base, 0.7);
		rgb_color bevelColorRightBottom = tint_color(base, 1.08);

		topTint = 0.69;
		bottomTint = 1.03;

		_DrawFrame(view, rect,
			bevelColorLeft, bevelColorTop,
			bevelColorRightBottom, bevelColorRightBottom,
			cornerColor, cornerColor,
			borders);
	}

	// draw surface top
	//_FillGradient(view, rect, base, topTint, bottomTint);
	view->SetLowColor(base);
	view->FillRect(rect, B_SOLID_LOW);
}


void
BeControlLook::DrawMenuItemBackground(BView* view, BRect& rect,
	const BRect& updateRect, const rgb_color& base, uint32 flags,
	uint32 borders)
{
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	// surface edges
	float topTint;
	float bottomTint;
	rgb_color selectedColor = base;

	if ((flags & B_ACTIVATED) != 0) {
		topTint = 0.9;
		bottomTint = 1.05;
	} else if ((flags & B_DISABLED) != 0) {
		topTint = 0.80;
		bottomTint = 1.07;
	} else {
		topTint = 0.6;
		bottomTint = 1.12;
	}

	rgb_color bevelLightColor = tint_color(selectedColor, topTint);
	rgb_color bevelShadowColor = tint_color(selectedColor, bottomTint);

	// draw surface edges
	_DrawFrame(view, rect,
		bevelLightColor, bevelLightColor,
		bevelShadowColor, bevelShadowColor,
		borders);

	// draw surface top
	view->SetLowColor(selectedColor);
	//_FillGradient(view, rect, selectedColor, topTint, bottomTint);
	view->FillRect(rect, B_SOLID_LOW);
}


extern "C" BControlLook* (instantiate_control_look)(image_id id)
{
	return new (std::nothrow)BeControlLook(id);
}

