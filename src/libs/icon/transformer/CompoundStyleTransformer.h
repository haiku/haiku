/*
 * Copyright 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Zardshard
 */
#ifndef COMPOUND_STYLE_TRANSFORMER_H
#define COMPOUND_STYLE_TRANSFORMER_H


#include <SupportDefs.h>

#include "IconBuild.h"
#include "StyleTransformer.h"


_BEGIN_ICON_NAMESPACE

class VertexSource;
class StyleTransformer;


/*! Allows turning an array of StyleTransformers into a single StyleTransformer.

	\note This class is not meant to be exposed to the GUI or saved in a file. It
		is currently only used for rendering.
*/
class CompoundStyleTransformer : public StyleTransformer {
public:
								CompoundStyleTransformer(
									StyleTransformer** transformers,
									int32 count);
	virtual						~CompoundStyleTransformer();

	// StyleTransformer interface
	virtual	void				transform(double* x, double* y) const;
	virtual	void				Invert();

	virtual bool				IsLinear();

private:
			StyleTransformer**	fTransformers;
			int32				fCount;
};


_END_ICON_NAMESPACE


#endif	// COMPOUND_STYLE_TRANSFORMER_H
