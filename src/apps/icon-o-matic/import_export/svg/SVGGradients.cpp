/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include <stdio.h>
#include <stdlib.h>

#include "GradientTransformable.h"

#include "SVGGradients.h"

namespace agg {
namespace svg {

// constructor
SVGGradient::SVGGradient()
	: BMessage(),
	  fGradient(NULL),
	  fID(""),
	  fGradientUnits(UNSPECIFIED)
{
}

// destructor
SVGGradient::~SVGGradient()
{
}

// SetID
void
SVGGradient::SetID(const char* id)
{
	fID = id;
}

// ID
const char*
SVGGradient::ID() const
{
	return fID.String();
}

// AddStop
void
SVGGradient::AddStop(float offset, rgba8 color)
{
	if (!fGradient)
		fGradient = MakeGradient();

	rgb_color c = { color.r, color.g, color.b, color.a };
	fGradient->AddColor(c, offset);
}

// SetTransformation
void
SVGGradient::SetTransformation(const trans_affine& transform)
{
//printf("SVGGradient::SetTransformation()\n");
	if (!fGradient)
		fGradient = MakeGradient();

	fGradient->multiply(transform);
}

// GetGradient
Gradient*
SVGGradient::GetGradient(BRect objectBounds)
{
	if (fGradient) {
		Gradient* gradient = new Gradient(*fGradient);

		IdentifyGradientUnits();
		if (fGradientUnits == OBJECT_BOUNDING_BOX) {
			gradient->FitToBounds(objectBounds);
		}

		return gradient;
	}
	return NULL;
}

// IdentifyGradientUnits
void
SVGGradient::IdentifyGradientUnits()
{
	if (fGradientUnits == UNSPECIFIED) {
		const char* units;
		if (FindString("gradientUnits", &units) >= B_OK) {
			if (strcmp(units, "objectBoundingBox") == 0)
				fGradientUnits = OBJECT_BOUNDING_BOX;
		}
	}
}

// constructor
SVGLinearGradient::SVGLinearGradient()
	: SVGGradient()
{
}

// destructor
SVGLinearGradient::~SVGLinearGradient()
{
}

// MakeGradient
Gradient*
SVGLinearGradient::MakeGradient() const
{
//printf("SVGLinearGradient::MakeGradient()\n");
//PrintToStream();

	Gradient* gradient = new Gradient(true);
	gradient->SetType(GRADIENT_LINEAR);
	gradient->SetInterpolation(INTERPOLATION_LINEAR);
	// setup the gradient transform 
	BPoint start(-64.0, -64.0);
	BPoint end(64.0, -64.0);
	BString coordinate;
	if (FindString("x1", &coordinate) >= B_OK)
		start.x = atof(coordinate.String());
	if (FindString("y1", &coordinate) >= B_OK)
		start.y = atof(coordinate.String());
	if (FindString("x2", &coordinate) >= B_OK)
		end.x = atof(coordinate.String());
	if (FindString("y2", &coordinate) >= B_OK)
		end.y = atof(coordinate.String());

	// the transformed parallelogram
	double parl[6];
	parl[0] = start.x;
	parl[1] = start.y;
	parl[2] = end.x;
	parl[3] = end.y;
	parl[4] = end.x - (end.y - start.y);
	parl[5] = end.y + (end.x - start.x);

	trans_affine transform(-64.0, -64.0, 64.0, 64.0, parl);
	gradient->multiply(transform);

	return gradient;
}

// constructor
SVGRadialGradient::SVGRadialGradient()
	: SVGGradient()
{
}

// destructor
SVGRadialGradient::~SVGRadialGradient()
{
}

// MakeGradient
Gradient*
SVGRadialGradient::MakeGradient() const
{
//printf("SVGRadialGradient::MakeGradient()\n");
	// TODO: handle userSpaceOnUse/objectBoundingBox
	Gradient* gradient = new Gradient(true);
	gradient->SetType(GRADIENT_CIRCULAR);
	gradient->SetInterpolation(INTERPOLATION_LINEAR);

	double cx = 0.0;
	double cy = 0.0;
	double r = 100.0;

	BString value;
	if (FindString("cx", &value) >= B_OK)
		cx = atof(value.String());
	if (FindString("cy", &value) >= B_OK)
		cy = atof(value.String());
	if (FindString("r", &value) >= B_OK)
		r = atof(value.String());

	// the transformed parallelogram
	double parl[6];
	parl[0] = cx - r;
	parl[1] = cy - r;
	parl[2] = cx + r;
	parl[3] = cy - r;
	parl[4] = cx + r;
	parl[5] = cy + r;

	trans_affine transform(-64.0, -64.0, 64.0, 64.0, parl);
	gradient->multiply(transform);

	return gradient;
}

} // namespace svg
} // namespace agg

