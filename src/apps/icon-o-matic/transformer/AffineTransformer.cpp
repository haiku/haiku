/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "AffineTransformer.h"

// constructor
AffineTransformer::AffineTransformer(VertexSource& source)
	: Transformer(source),
	  Affine(source, *this)
{
}

// destructor
AffineTransformer::~AffineTransformer()
{
}

// rewind
void
AffineTransformer::rewind(unsigned path_id)
{
	Affine::rewind(path_id);
}

// vertex
unsigned
AffineTransformer::vertex(double* x, double* y)
{
	return Affine::vertex(x, y);
}

// SetSource
void
AffineTransformer::SetSource(VertexSource& source)
{
	Transformer::SetSource(source);
	Affine::attach(source);
}

// Name
const char*
AffineTransformer::Name() const
{
	return "Transformation";
}


