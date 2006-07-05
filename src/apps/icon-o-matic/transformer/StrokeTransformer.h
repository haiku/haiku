/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef STROKE_TRANSFORMER_H
#define STROKE_TRANSFORMER_H

#include "agg_conv_stroke.h"

#include "Transformer.h"

typedef agg::conv_stroke<VertexSource>		Stroke;

class StrokeTransformer : public Transformer,
						  public Stroke {
 public:
								StrokeTransformer(
									VertexSource& source);
	virtual						~StrokeTransformer();

    virtual	void				rewind(unsigned path_id);
    virtual	unsigned			vertex(double* x, double* y);

	virtual	void				SetSource(VertexSource& source);

	// IconObject interface
	virtual	PropertyObject*		MakePropertyObject() const;
	virtual	bool				SetToPropertyObject(
									const PropertyObject* object);
};

#endif // STROKE_TRANSFORMER_H
