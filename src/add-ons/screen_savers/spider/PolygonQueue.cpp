/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#include "PolygonQueue.h"

#include <stdio.h>
#include <string.h>

#include "Polygon.h"

// constructor
PolygonQueue::PolygonQueue(Polygon* start, int32 depth)
	: fPolygons(new Polygon*[depth]),
	  fDepth(depth)
{
	for (int32 i = 0; i < fDepth; i++)
		fPolygons[i] = NULL;
	fPolygons[fDepth - 1] = start;
}

// destructor
PolygonQueue::~PolygonQueue()
{
	for (int32 i = 0; i < fDepth; i++)
		delete fPolygons[i];
	delete[] fPolygons;
}

// Head
Polygon*
PolygonQueue::Head() const
{
	return fPolygons[fDepth - 1];
}

// Tail
Polygon*
PolygonQueue::Tail() const
{
	return fPolygons[0];
}

// Step
void
PolygonQueue::Step()
{
	if (Polygon* p = Head()) {
		Polygon *np = p->Step();
		// drop tail
		delete Tail();
		// shift
		for (int32 i = 0; i < fDepth - 1; i++)
			fPolygons[i] = fPolygons[i + 1];
		// and put new head at top
		fPolygons[fDepth - 1] = np;
	}
}
