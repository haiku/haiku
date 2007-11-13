/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#include "Polygon.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// constructor
Polygon::Polygon(BRect bounds, int32 vertices)
	: fPoints(vertices),
	  fBounds(bounds)
{
	float min = bounds.Width() / 64000.0;
	float max = bounds.Width() / 320.0;
	for (int32 i = 0; i < vertices; i++) {
		point_vector* pv = new point_vector;
		pv->point.x = bounds.left + fmod(lrand48(), bounds.Width());
		pv->point.y = bounds.top + fmod(lrand48(), bounds.Height());
		pv->vector.x = min + fmod(lrand48(), max - min);
		pv->vector.y = min + fmod(lrand48(), max - min);
		fPoints.AddItem((void*)pv);
	}
}

// constructor
Polygon::Polygon(BRect bounds, BList points)
	: fPoints(points.CountItems()),
	  fBounds(bounds)
{
	fPoints = points;
}

// destructor
Polygon::~Polygon()
{
	while (point_vector* pv = (point_vector*)fPoints.RemoveItem(0L))
		delete pv;
}

// Step
Polygon*
Polygon::Step() const
{
	BList points(CountPoints());
	for (int32 i = 0; point_vector *pv = (point_vector*)fPoints.ItemAt(i); i++) {
		point_vector* npv = new point_vector;
		BPoint p = pv->point + pv->vector;
		if (p.x < fBounds.left || p.x > fBounds.right)
			npv->vector.x = -pv->vector.x;
		else
			npv->vector.x = pv->vector.x;
		if (p.y < fBounds.top || p.y > fBounds.bottom)
			npv->vector.y = -pv->vector.y;
		else
			npv->vector.y = pv->vector.y;
		npv->point = pv->point + npv->vector;
		points.AddItem((void*)npv);
	}
	return new Polygon(fBounds, points);
}

// CountPoints
uint32
Polygon::CountPoints() const
{
	return fPoints.CountItems();
}

// PointAt
BPoint
Polygon::PointAt(int32 index) const
{
	BPoint p;
	if (point_vector* pv = (point_vector*)fPoints.ItemAt(index))
		p = pv->point;
	return p;
}
