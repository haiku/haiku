/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "PerspectiveTransformer.h"

#include <new>

using std::nothrow;

// constructor
PerspectiveTransformer::PerspectiveTransformer(VertexSource& source)
	: Transformer(source, "Perspective"),
	  Perspective(source, *this)
{
}

// destructor
PerspectiveTransformer::~PerspectiveTransformer()
{
}

// Clone
Transformer*
PerspectiveTransformer::Clone(VertexSource& source) const
{
	PerspectiveTransformer* clone
		= new (nothrow) PerspectiveTransformer(source);
	if (clone) {
// TODO: upgrade AGG
//		clone->multiply(*this);
	}
	return clone;
}

// rewind
void
PerspectiveTransformer::rewind(unsigned path_id)
{
	Perspective::rewind(path_id);
}

// vertex
unsigned
PerspectiveTransformer::vertex(double* x, double* y)
{
	return Perspective::vertex(x, y);
}

// SetSource
void
PerspectiveTransformer::SetSource(VertexSource& source)
{
	Transformer::SetSource(source);
	Perspective::attach(source);
}

// ApproximationScale
double
PerspectiveTransformer::ApproximationScale() const
{
	// TODO: upgrade AGG
	return fSource.ApproximationScale();// * scale();
}




