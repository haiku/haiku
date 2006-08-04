/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef PERSPECTIVE_TRANSFORMER_H
#define PERSPECTIVE_TRANSFORMER_H

#include <agg_conv_transform.h>
#include <agg_trans_perspective.h>

#include "Transformer.h"

typedef agg::conv_transform<VertexSource,
							agg::trans_perspective>	Perspective;

class PerspectiveTransformer : public Transformer,
							   public Perspective,
							   public agg::trans_perspective {
 public:
	enum {
		archive_code	= 'prsp',
	};

								PerspectiveTransformer(
									VertexSource& source);
#ifdef ICON_O_MATIC
								PerspectiveTransformer(
									VertexSource& source,
									BMessage* archive);
#endif
	virtual						~PerspectiveTransformer();

	// Transformer interface
	virtual	Transformer*		Clone(VertexSource& source) const;

	virtual	void				rewind(unsigned path_id);
	virtual	unsigned			vertex(double* x, double* y);

	virtual	void				SetSource(VertexSource& source);

	virtual	double				ApproximationScale() const;

#ifdef ICON_O_MATIC
	// IconObject interface
	virtual	status_t			Archive(BMessage* into,
										bool deep = true) const;

#endif
};

#endif // PERSPECTIVE_TRANSFORMER_H
