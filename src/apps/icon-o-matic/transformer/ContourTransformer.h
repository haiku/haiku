/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef CONTOUR_TRANSFORMER_H
#define CONTOUR_TRANSFORMER_H

#include "agg_conv_contour.h"

#include "Transformer.h"

typedef agg::conv_contour<VertexSource>		Contour;

class ContourTransformer : public Transformer,
						   public Contour {
 public:
								ContourTransformer(
									VertexSource& source);
	virtual						~ContourTransformer();

    virtual	void				rewind(unsigned path_id);
    virtual	unsigned			vertex(double* x, double* y);

	virtual	void				SetSource(VertexSource& source);
	virtual	void				SetLast();
};

#endif // CONTOUR_TRANSFORMER_H
