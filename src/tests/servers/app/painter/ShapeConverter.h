/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Simple BShape to agg::path_storage converter, implemented as BShapeIterator.
 *
 */


#ifndef SHAPE_CONVERTER_H
#define SHAPE_CONVERTER_H

#include <Shape.h>

#include <agg_path_storage.h>

#include "Transformable.h"

class BPoint;

// TODO: test if this actually works

class ShapeConverter : public BShapeIterator,
					   public Transformable {
 public:
								ShapeConverter();
								ShapeConverter(agg::path_storage* path);
	virtual						~ShapeConverter() {};

			void				SetPath(agg::path_storage* path);

	virtual	status_t			IterateMoveTo(BPoint* point);
	virtual	status_t			IterateLineTo(int32 lineCount, BPoint* linePts);
	virtual	status_t			IterateBezierTo(int32 bezierCount, BPoint* bezierPts);
	virtual	status_t			IterateClose();

 private:
	agg::path_storage*			fPath;
};

#endif // SHAPE_CONVERTER_H
