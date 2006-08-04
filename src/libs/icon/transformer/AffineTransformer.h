/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef AFFINE_TRANSFORMER_H
#define AFFINE_TRANSFORMER_H

#include <agg_conv_transform.h>
#include <agg_trans_affine.h>

#include "Transformer.h"

typedef agg::conv_transform<VertexSource,
							agg::trans_affine>	Affine;

class AffineTransformer : public Transformer,
						  public Affine,
						  public agg::trans_affine {
 public:
	enum {
		archive_code	= 'affn',
	};

								AffineTransformer(
									VertexSource& source);
#ifdef ICON_O_MATIC
								AffineTransformer(
									VertexSource& source,
									BMessage* archive);
#endif
	virtual						~AffineTransformer();

	virtual	Transformer*		Clone(VertexSource& source) const;

	virtual	void				rewind(unsigned path_id);
    virtual	unsigned			vertex(double* x, double* y);

	virtual	void				SetSource(VertexSource& source);

	virtual	double				ApproximationScale() const;

#ifdef ICON_O_MATIC
	// IconObject interface
	virtual	status_t			Archive(BMessage* into,
										bool deep = true) const;

	virtual	PropertyObject*		MakePropertyObject() const;
	virtual	bool				SetToPropertyObject(
									const PropertyObject* object);
#endif
};

#endif // AFFINE_TRANSFORMER_H
