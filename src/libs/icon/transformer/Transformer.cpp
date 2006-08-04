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

// #pragma mark -

// constructor
Transformer::Transformer(VertexSource& source, const char* name)
#ifdef ICON_O_MATIC
	: IconObject(name),
#else
	:
#endif
	  fSource(source)
{
}

#ifdef ICON_O_MATIC
// constructor
Transformer::Transformer(VertexSource& source,
						 BMessage* archive)
	: IconObject(archive),
	  fSource(source)
{
}
#endif // ICON_O_MATIC

// destructor
Transformer::~Transformer()
{
}

// #pragma mark -

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

// WantsOpenPaths
bool
Transformer::WantsOpenPaths() const
{
	return fSource.WantsOpenPaths();
}

// ApproximationScale
double
Transformer::ApproximationScale() const
{
	return fSource.ApproximationScale();
}

