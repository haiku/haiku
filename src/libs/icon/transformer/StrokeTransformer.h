/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef STROKE_TRANSFORMER_H
#define STROKE_TRANSFORMER_H


#include "IconBuild.h"
#include "Transformer.h"

#include <agg_conv_stroke.h>


_BEGIN_ICON_NAMESPACE


typedef agg::conv_stroke<VertexSource> Stroke;

class StrokeTransformer : public Transformer,
						  public Stroke {
 public:
	enum {
		archive_code	= 'strk',
	};

								StrokeTransformer(
									VertexSource& source);
								StrokeTransformer(
									VertexSource& source,
									BMessage* archive);

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


_END_ICON_NAMESPACE


#endif	// STROKE_TRANSFORMER_H
