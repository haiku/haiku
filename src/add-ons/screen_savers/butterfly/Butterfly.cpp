/*
 * Copyright 2010-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Geoffry Song, goffrie@gmail.com
 *		Ryan Leavengood, leavengood@gmail.com
 */


#include "Butterfly.h"

#include <math.h>
#include <stdlib.h>

#include <Catalog.h>
#include <OS.h>
#include <View.h>

#include <BuildScreenSaverDefaultSettingsView.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Screensaver Butterfly"


extern "C" BScreenSaver*
instantiate_screen_saver(BMessage* archive, image_id imageId)
{
	return new Butterfly(archive, imageId);
}


// #pragma mark -


Butterfly::Butterfly(BMessage* archive, image_id imageId)
	:
	BScreenSaver(archive, imageId)
{
}


void
Butterfly::StartConfig(BView* view)
{
	BPrivate::BuildScreenSaverDefaultSettingsView(view, "Butterfly",
		B_TRANSLATE("by Geoffry Song"));
}


status_t
Butterfly::StartSaver(BView* view, bool preview)
{
	view->SetLowColor(0, 0, 0);
	view->FillRect(view->Bounds(), B_SOLID_LOW);
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
	view->SetLineMode(B_ROUND_CAP, B_ROUND_JOIN);
	if (!preview)
		view->SetPenSize(2.0);

	SetTickSize(20000);

	// Set fBase to a random radian value scaled by 1000. The scaling produces
	// more interesting results.
	srand48(system_time());
	fBase = drand48() * 2 * M_PI * 1000;

	// calculate transformation
	BRect bounds = view->Bounds();
	fScale = MIN(bounds.Width(), bounds.Height()) * 0.1f;
	fTrans.Set(bounds.Width() * 0.5f, bounds.Height() * 0.5f);
	fBounds = bounds;

	fLast[0] = _Iterate();
	fLast[1] = _Iterate();
	fLast[2] = _Iterate();

	return B_OK;
}


void
Butterfly::Draw(BView* view, int32 frame)
{
	if (frame == 1024) {
		// calculate bounding box ( (-5.92,-5.92) to (5.92, 5.92) )
		fBounds.Set(-5.92f * fScale + fTrans.x, -5.92f * fScale + fTrans.y,
			5.92f * fScale + fTrans.x, 5.92f * fScale + fTrans.y);
	}
	if ((frame & 3) == 0) {
		// fade out
		view->SetHighColor(0, 0, 0, 4);
		view->FillRect(fBounds);
	}
	// create a color from a hue of (fBase * 15) degrees
	view->SetHighColor(_HueToColor(fBase * 15));
	BPoint p = _Iterate();

	// cubic Hermite interpolation from fLast[1] to fLast[2]
	// calculate tangents for a Catmull-Rom spline
	//(these values need to be halved)
	BPoint m1 = fLast[2] - fLast[0]; // tangent for fLast[1]
	BPoint m2 = p - fLast[1]; // tangent for fLast[2]

	// draw Bezier from fLast[1] to fLast[2] with control points
	// fLast[1] + m1/3, fLast[2] - m2/3
	m1.x /= 6;
	m1.y /= 6;
	m2.x /= 6;
	m2.y /= 6;
	BPoint control[4] = { fLast[1], fLast[1] + m1, fLast[2] - m2, fLast[2] };
	view->StrokeBezier(control);

	fLast[0] = fLast[1];
	fLast[1] = fLast[2];
	fLast[2] = p;
}


//! Convert from a hue in degrees to a fully saturated color
inline rgb_color
Butterfly::_HueToColor(float hue)
{
	// convert from [0..360) to [0..1530)
	int h = static_cast<int>(fmodf(hue, 360) * 4.25f);
	int x = 255 - abs(h % 510 - 255);

	rgb_color result = {0, 0, 0, 255};
	if (h < 255) {
		result.red = 255;
		result.green = x;
	} else if (h < 510) {
		result.red = x;
		result.green = 255;
	} else if (h < 765) {
		result.green = 255;
		result.blue = x;
	} else if (h < 1020) {
		result.green = x;
		result.blue = 255;
	} else if (h < 1275) {
		result.red = x;
		result.blue = 255;
	} else {
		result.red = 255;
		result.blue = x;
	}
	return result;
}


inline BPoint
Butterfly::_Iterate()
{
	float r = powf(M_E, cosf(fBase)) - 2 * cosf(4 * fBase)
		- powf(sinf(fBase / 12), 5);
	// rotate and move it a bit
	BPoint p(sinf(fBase * 1.01f) * r + cosf(fBase * 1.02f) * 0.2f,
		cosf(fBase * 1.01f) * r + sinf(fBase * 1.02f) * 0.2f);
	// transform to view coordinates
	p.x *= fScale;
	p.y *= fScale;
	p += fTrans;
	// move on
	fBase += 0.05f;
	return p;
}
