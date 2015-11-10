/*
 * Copyright (c) 2001-2015, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Adrien Destugues <pulkomandy@pulkomandy.tk>
 *		Julian Harnath <julian.harnath@rwth-aachen.de>
 */

#ifndef SIMPLE_TRANSFORM_H
#define SIMPLE_TRANSFORM_H

#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientDiamond.h>
#include <GradientConic.h>
#include <Point.h>
#include <Region.h>

#include "IntPoint.h"
#include "IntRect.h"


class SimpleTransform {
public:
	SimpleTransform()
		:
		fScale(1.0)
	{
	}

	void AddOffset(float x, float y)
	{
		fOffset.x += x;
		fOffset.y += y;
	}

	void SetScale(float scale)
	{
		fScale = scale;
	}

	void Apply(BPoint* point) const
	{
		_Apply(point->x, point->y);
	}

	void Apply(IntPoint* point) const
	{
		_Apply(point->x, point->y);
	}

	void Apply(BRect* rect) const
	{
		if (fScale == 1.0) {
			rect->OffsetBy(fOffset.x, fOffset.y);
		} else {
			_Apply(rect->left, rect->top);
			_Apply(rect->right, rect->bottom);
		}
	}

	void Apply(IntRect* rect) const
	{
		if (fScale == 1.0) {
			rect->OffsetBy(fOffset.x, fOffset.y);
		} else {
			_Apply(rect->left, rect->top);
			_Apply(rect->right, rect->bottom);
		}
	}

	void Apply(BRegion* region) const
	{
		if (fScale == 1.0) {
			region->OffsetBy(fOffset.x, fOffset.y);
		} else {
			// TODO: optimize some more
			BRegion converted;
			int32 count = region->CountRects();
			for (int32 i = 0; i < count; i++) {
				BRect r = region->RectAt(i);
				BPoint lt(r.LeftTop());
				BPoint rb(r.RightBottom());
				// offset to bottom right corner of pixel before transformation
				rb.x++;
				rb.y++;
				// apply transformation
				_Apply(lt.x, lt.y);
				_Apply(rb.x, rb.y);
				// reset bottom right to pixel "index"
				rb.x--;
				rb.y--;
				// add rect to converted region
				// NOTE/TODO: the rect would not have to go
				// through the whole intersection test process,
				// it is guaranteed not to overlap with any rect
				// already contained in the region
				converted.Include(BRect(lt, rb));
			}
			*region = converted;
		}
	}

	void Apply(BGradient* gradient) const
	{
		switch (gradient->GetType()) {
			case BGradient::TYPE_LINEAR:
			{
				BGradientLinear* linear = (BGradientLinear*) gradient;
				BPoint start = linear->Start();
				BPoint end = linear->End();
				Apply(&start);
				Apply(&end);
				linear->SetStart(start);
				linear->SetEnd(end);
				break;
			}

			case BGradient::TYPE_RADIAL:
			{
				BGradientRadial* radial = (BGradientRadial*) gradient;
				BPoint center = radial->Center();
				Apply(&center);
				radial->SetCenter(center);
				break;
			}

			case BGradient::TYPE_RADIAL_FOCUS:
			{
				BGradientRadialFocus* radialFocus =
					(BGradientRadialFocus*)gradient;
				BPoint center = radialFocus->Center();
				BPoint focal = radialFocus->Focal();
				Apply(&center);
				Apply(&focal);
				radialFocus->SetCenter(center);
				radialFocus->SetFocal(focal);
				break;
			}

			case BGradient::TYPE_DIAMOND:
			{
				BGradientDiamond* diamond = (BGradientDiamond*) gradient;
				BPoint center = diamond->Center();
				Apply(&center);
				diamond->SetCenter(center);
				break;
			}

			case BGradient::TYPE_CONIC:
			{
				BGradientConic* conic = (BGradientConic*) gradient;
				BPoint center = conic->Center();
				Apply(&center);
				conic->SetCenter(center);
				break;
			}

			case BGradient::TYPE_NONE:
			{
				break;
			}
		}

		// Make sure the gradient is fully padded so that out of bounds access
		// get the correct colors
		gradient->SortColorStopsByOffset();

		BGradient::ColorStop* end = gradient->ColorStopAtFast(
			gradient->CountColorStops() - 1);

		if (end->offset != 255)
			gradient->AddColor(end->color, 255);

		BGradient::ColorStop* start = gradient->ColorStopAtFast(0);

		if (start->offset != 0)
			gradient->AddColor(start->color, 0);

		gradient->SortColorStopsByOffset();
	}

	void Apply(BPoint* destination, const BPoint* source, int32 count) const
	{
		// TODO: optimize this, it should be smarter
		while (count--) {
			*destination = *source;
			Apply(destination);
			source++;
			destination++;
		}
	}

	void Apply(BRect* destination, const BRect* source, int32 count) const
	{
		// TODO: optimize this, it should be smarter
		while (count--) {
			*destination = *source;
			Apply(destination);
			source++;
			destination++;
		}
	}

	void Apply(BRegion* destination, const BRegion* source, int32 count) const
	{
		// TODO: optimize this, it should be smarter
		while (count--) {
			*destination = *source;
			Apply(destination);
			source++;
			destination++;
		}
	}

private:
	void _Apply(int32& x, int32& y) const
	{
		x *= (int32)fScale;
		y *= (int32)fScale;
		x += (int32)fOffset.x;
		y += (int32)fOffset.y;
	}

	void _Apply(float& x, float& y) const
	{
		x *= fScale;
		y *= fScale;
		x += fOffset.x;
		y += fOffset.y;
	}

private:
	BPoint	fOffset;
	float	fScale;
};


#endif // SIMPLE_TRANSFORM_H
