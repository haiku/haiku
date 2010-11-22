/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef CONTOUR_TRANSFORMER_H
#define CONTOUR_TRANSFORMER_H


#include "IconBuild.h"
#include "Transformer.h"

#include <agg_conv_contour.h>


_BEGIN_ICON_NAMESPACE


typedef agg::conv_contour<VertexSource>		Contour;

class ContourTransformer : public Transformer,
						   public Contour {
 public:
	enum {
		archive_code	= 'cntr',
	};

								ContourTransformer(
									VertexSource& source);
								ContourTransformer(
									VertexSource& source,
									BMessage* archive);

	virtual						~ContourTransformer();

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

_END_ICON_NAMESPACE


#endif	// CONTOUR_TRANSFORMER_H
