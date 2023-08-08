/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef TRANSFORMER_FACTORY_H
#define TRANSFORMER_FACTORY_H


#include <String.h>

#include "IconBuild.h"


class BMessage;


_BEGIN_ICON_NAMESPACE


class Shape;
class Transformer;
class VertexSource;

enum {
	AFFINE_TRANSFORMER,
	PERSPECTIVE_TRANSFORMER,
	CONTOUR_TRANSFORMER,
	STROKE_TRANSFORMER,
};


class TransformerFactory {
 public:

	static	Transformer*		TransformerFor(uint32 type,
											   VertexSource& source,
											   Shape* shape);

	static	Transformer*		TransformerFor(BMessage* archive,
											   VertexSource& source,
											   Shape* shape);
};


_END_ICON_NAMESPACE


#endif	// TRANSFORMER_FACTORY_H
