/*
 * Copyright 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef STYLE_TRANSFORMER_H
#define STYLE_TRANSFORMER_H


#include "IconBuild.h"


class BPoint;

_BEGIN_ICON_NAMESPACE


/*! Warps a shape's style.
	Implements the same interface as any other class of the agg:trans_ series.
	This class can be used wherever AGG needs a transformer (for example,
	agg::span_interpolator_linear<StyleTransformer> is valid).
*/
class StyleTransformer {
public:
								StyleTransformer() {}
	virtual						~StyleTransformer();

	/*! Transform a single point of the shape's style.
	    This function should be fast since it will be called many times.

		\note This function is lowercase so that it satisfies the role of an agg
			transformer
	*/
	virtual void				transform(double* x, double* y) const = 0;

	/*! Alias of \c transform.
		Use of this in our code is preffered because it follows the normal
		capitalization scheme.
	*/
			void				Transform(double* x, double* y) const
									{ transform(x, y); }
			void				Transform(BPoint* point) const;
			BPoint				Transform(const BPoint& point) const;

	virtual void				Invert() = 0;

	/*! Is the transformation a linear transformation?
	    This allows using linear interpolation instead of calling \c transform
		for each point. */
	virtual bool				IsLinear()
									{ return false; }
};


_END_ICON_NAMESPACE


#endif	// STYLE_TRANSFORMER_H
