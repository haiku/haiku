/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Simple BShape to agg::path_storage converter, implemented as BShapeIterator.
 *
 */


#include "ShapeConverter.h"

// constructor
ShapeConverter::ShapeConverter()
	: BShapeIterator(),
	  Transformable(),
	  fPath(NULL)
{
}

// constructor
ShapeConverter::ShapeConverter(agg::path_storage* path)
	: BShapeIterator(),
	  Transformable(),
	  fPath(path)
{
}

// SetPath
void
ShapeConverter::SetPath(agg::path_storage* path)
{
	fPath = path;
}

// IterateMoveTo
status_t
ShapeConverter::IterateMoveTo(BPoint* point)
{
	double x = point->x;
	double y = point->y;
	Transform(&x, &y);

	fPath->move_to(x, y);

	return B_OK;
}

// IterateLineTo
status_t
ShapeConverter::IterateLineTo(int32 lineCount, BPoint* linePts)
{
	while (lineCount--) {

		double x = linePts->x;
		double y = linePts->y;
		Transform(&x, &y);

		fPath->line_to(x, y);

		linePts++;
	}
	return B_OK;
}

// IterateBezierTo
status_t
ShapeConverter::IterateBezierTo(int32 bezierCount, BPoint* bezierPts)
{
	bezierCount /= 3;
	while (bezierCount--) {

		double x1 = bezierPts[0].x;
		double y1 = bezierPts[0].y;

		double x2 = bezierPts[1].x;
		double y2 = bezierPts[1].y;

		double x3 = bezierPts[2].x;
		double y3 = bezierPts[2].y;

		Transform(&x1, &y1);
		Transform(&x2, &y2);
		Transform(&x3, &y3);

		fPath->curve4(x1, y1, x2, y2, x3, y3);

		bezierPts += 3;
	}
	return B_OK;
}

// IterateClose
status_t
ShapeConverter::IterateClose()
{
	fPath->close_polygon();
	return B_OK;
}
