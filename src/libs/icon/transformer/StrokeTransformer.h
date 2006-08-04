/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef STROKE_TRANSFORMER_H
#define STROKE_TRANSFORMER_H

#include <agg_conv_stroke.h>

#include "Transformer.h"

typedef agg::conv_stroke<VertexSource>		Stroke;

class StrokeTransformer : public Transformer,
						  public Stroke {
 public:
	enum {
		archive_code	= 'strk',
	};

								StrokeTransformer(
									VertexSource& source);
#ifdef ICON_O_MATIC
								StrokeTransformer(
									VertexSource& source,
									BMessage* archive);
#endif
	virtual						~StrokeTransformer();

	// Transformer interface
	virtual	Transformer*		Clone(VertexSource& source) const;

	virtual	void				rewind(unsigned path_id);
	virtual	unsigned			vertex(double* x, double* y);

	virtual	void				SetSource(VertexSource& source);

	virtual	bool				WantsOpenPaths() const;
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

#endif // STROKE_TRANSFORMER_H
