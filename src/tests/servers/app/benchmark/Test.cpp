/*
 * Copyright (C) 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "Test.h"

#include <Region.h>
#include <View.h>


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
	BRegion clipping(bounds);
	BRect grid(bounds.InsetByCopy(10, 10));
	for (float y = grid.top; y < grid.bottom - 5; y += grid.Height() / 20) {
		for (float x = grid.left; x < grid.right - 5;
				x += grid.Width() / 20) {
			BRect r(x, y, x, y);
			r.InsetBy(-5, -5);
			clipping.Exclude(r);
		}
	}

	view->ConstrainClippingRegion(&clipping);
}
