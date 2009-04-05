/*
 * Copyright (C) 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "Test.h"

#include <stdio.h>

#include <View.h>

#include "DrawingModeToString.h"


Test::Test()
{
}


Test::~Test()
{
}


void
Test::SetupClipping(BView* view)
{
	BRect bounds = view->Bounds();
	fClippingRegion.Set(bounds);
	BRect grid(bounds.InsetByCopy(40, 40));
	for (float y = grid.top; y < grid.bottom + 5; y += grid.Height() / 2) {
		for (float x = grid.left; x < grid.right + 5;
				x += grid.Width() / 2) {
			BRect r(x, y, x, y);
			r.InsetBy(-30, -30);
			fClippingRegion.Exclude(r);
		}
	}

	view->ConstrainClippingRegion(&fClippingRegion);
}


void
Test::PrintResults(BView* view)
{
	if (fClippingRegion.CountRects() > 0)
		printf("Clipping rects: %ld\n", fClippingRegion.CountRects());

	const char* string;
	if (ToString(view->DrawingMode(), string))
		printf("Drawing mode: %s\n", string);
}


