/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef POLYGON_H
#define POLYGON_H

#include <List.h>
#include <Point.h>
#include <Rect.h>

struct point_vector {
	BPoint	point;
	BPoint	vector;
};

class Polygon {
 public:
								Polygon(BRect bounds, BList points);
								Polygon(BRect bounds, int32 vertices);
	virtual						~Polygon();

			Polygon*			Step() const;

			uint32				CountPoints() const;
			BPoint				PointAt(int32 index) const;

 private:
			BList				fPoints;
			BRect				fBounds;
};

#endif // ABOUT_VPOLYGON_HIEW_H
