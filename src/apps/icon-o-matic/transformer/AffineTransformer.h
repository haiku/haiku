/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef AFFINE_TRANSFORMER_H
#define AFFINE_TRANSFORMER_H

#include "agg_conv_transform.h"
#include "agg_trans_affine.h"

#include "Transformer.h"

typedef agg::conv_transform<VertexSource,
							agg::trans_affine>	Affine;

class AffineTransformer : public Transformer,
						  public Affine,
						  public agg::trans_affine {
 public:
								AffineTransformer(
									VertexSource& source);
	virtual						~AffineTransformer();

    virtual	void				rewind(unsigned path_id);
    virtual	unsigned			vertex(double* x, double* y);

	virtual	void				SetSource(VertexSource& source);

	// IconObject interface
	virtual	PropertyObject*		MakePropertyObject() const;
	virtual	bool				SetToPropertyObject(
									const PropertyObject* object);
};

#endif // AFFINE_TRANSFORMER_H
