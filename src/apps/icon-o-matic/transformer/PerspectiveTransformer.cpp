/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "PerspectiveTransformer.h"

// constructor
PerspectiveTransformer::PerspectiveTransformer(VertexSource& source)
	: Transformer(source),
	  Perspective(source, *this)
{
}

// destructor
PerspectiveTransformer::~PerspectiveTransformer()
{
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

// Name
const char*
PerspectiveTransformer::Name() const
{
	return "Perspective";
}


