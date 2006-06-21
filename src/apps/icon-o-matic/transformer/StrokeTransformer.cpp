/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "StrokeTransformer.h"

// constructor
StrokeTransformer::StrokeTransformer(VertexSource& source)
	: Transformer(source),
	  Stroke(source)
{
}

// destructor
StrokeTransformer::~StrokeTransformer()
{
}

// rewind
void
StrokeTransformer::rewind(unsigned path_id)
{
	Stroke::rewind(path_id);
}

// vertex
unsigned
StrokeTransformer::vertex(double* x, double* y)
{
	return Stroke::vertex(x, y);
}

// SetSource
void
StrokeTransformer::SetSource(VertexSource& source)
{
	Transformer::SetSource(source);
	Stroke::attach(source);
}

