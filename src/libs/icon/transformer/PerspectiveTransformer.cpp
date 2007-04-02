/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "PerspectiveTransformer.h"

#ifdef ICON_O_MATIC
#	include <Message.h>
#endif

#include <new>

using namespace BPrivate::Icon;
using std::nothrow;


// constructor
PerspectiveTransformer::PerspectiveTransformer(VertexSource& source)
	: Transformer(source, "Perspective"),
	  Perspective(source, *this)
{
}

#ifdef ICON_O_MATIC
// constructor
PerspectiveTransformer::PerspectiveTransformer(VertexSource& source,
											   BMessage* archive)
	: Transformer(source, archive),
	  Perspective(source, *this)
{
	// TODO: upgrade AGG to be able to use load_from() etc
}
#endif

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

// #pragma mark -

#ifdef ICON_O_MATIC

// Archive
status_t
PerspectiveTransformer::Archive(BMessage* into, bool deep) const
{
	status_t ret = Transformer::Archive(into, deep);

	if (ret == B_OK)
		into->what = archive_code;

	// TODO: upgrade AGG to be able to use store_to()

	return ret;
}

#endif // ICON_O_MATIC



