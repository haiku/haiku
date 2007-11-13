/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef POLYGON_QUEUE_H
#define POLYGON_QUEUE_H

#include <SupportDefs.h>

class Polygon;

class PolygonQueue {
 public:
								PolygonQueue(Polygon* start, int32 depth);
	virtual						~PolygonQueue();

			Polygon*			Head() const;
			Polygon*			Tail() const;

			void				Step();

 private:
			Polygon**			fPolygons;
			int32				fDepth;
};

#endif // POLYGON_QUEUE_H
