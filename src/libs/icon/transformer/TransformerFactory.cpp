/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "TransformerFactory.h"

#include "AffineTransformer.h"
#include "ContourTransformer.h"
#include "PerspectiveTransformer.h"
#include "Shape.h"
#include "StrokeTransformer.h"

#ifdef ICON_O_MATIC
#include <Catalog.h>
#include <Message.h>
#endif


_USING_ICON_NAMESPACE


// TransformerFor
Transformer*
TransformerFactory::TransformerFor(uint32 type, VertexSource& source, Shape* shape)
{
	switch (type) {
		case AFFINE_TRANSFORMER:
			return new AffineTransformer(source);
		case PERSPECTIVE_TRANSFORMER:
			return new PerspectiveTransformer(source, shape);
		case CONTOUR_TRANSFORMER:
			return new ContourTransformer(source);
		case STROKE_TRANSFORMER:
			return new StrokeTransformer(source);
	}

	return NULL;
}

// TransformerFor
Transformer*
TransformerFactory::TransformerFor(BMessage* message, VertexSource& source, Shape* shape)
{
	switch (message->what) {
		case AffineTransformer::archive_code:
			return new AffineTransformer(source, message);
		case PerspectiveTransformer::archive_code:
			return new PerspectiveTransformer(source, shape, message);
		case ContourTransformer::archive_code:
			return new ContourTransformer(source, message);
		case StrokeTransformer::archive_code:
			return new StrokeTransformer(source, message);
	}

	return NULL;
}

