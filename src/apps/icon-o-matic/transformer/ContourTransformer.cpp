/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "ContourTransformer.h"

// constructor
ContourTransformer::ContourTransformer(VertexSource& source)
	: Transformer(source),
	  Contour(source)
{
}

// destructor
ContourTransformer::~ContourTransformer()
{
}

// rewind
void
ContourTransformer::rewind(unsigned path_id)
{
	Contour::rewind(path_id);
}

// vertex
unsigned
ContourTransformer::vertex(double* x, double* y)
{
	return Contour::vertex(x, y);
}

// SetSource
void
ContourTransformer::SetSource(VertexSource& source)
{
	Transformer::SetSource(source);
	Contour::attach(source);
}

