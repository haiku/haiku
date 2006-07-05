/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef PERSPECTIVE_TRANSFORMER_H
#define PERSPECTIVE_TRANSFORMER_H

#include "agg_conv_transform.h"
#include "agg_trans_perspective.h"

#include "Transformer.h"

typedef agg::conv_transform<VertexSource,
							agg::trans_perspective>	Perspective;

class PerspectiveTransformer : public Transformer,
							   public Perspective,
							   public agg::trans_perspective {
 public:
								PerspectiveTransformer(
									VertexSource& source);
	virtual						~PerspectiveTransformer();

    virtual	void				rewind(unsigned path_id);
    virtual	unsigned			vertex(double* x, double* y);

	virtual	void				SetSource(VertexSource& source);
	virtual	void				SetLast();
};

#endif // PERSPECTIVE_TRANSFORMER_H
