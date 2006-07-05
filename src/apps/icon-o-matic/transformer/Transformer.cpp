/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Transformer.h"

// constructor
VertexSource::VertexSource()
{
}

// destructor
VertexSource::~VertexSource()
{
}

// Finish
void
VertexSource::SetLast()
{
}

// #pragma mark -

// constructor
Transformer::Transformer(VertexSource& source, const char* name)
	: IconObject(name),
	  fSource(source)
{
}

// destructor
Transformer::~Transformer()
{
}

// rewind
void
Transformer::rewind(unsigned path_id)
{
	fSource.rewind(path_id);
}

// vertex
unsigned
Transformer::vertex(double* x, double* y)
{
	return fSource.vertex(x, y);
}

// SetSource
void
Transformer::SetSource(VertexSource& source)
{
	fSource = source;
}


