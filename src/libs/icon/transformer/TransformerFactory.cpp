/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "TransformerFactory.h"

#ifdef ICON_O_MATIC
# include <Message.h>
#endif

#include "AffineTransformer.h"
#include "ContourTransformer.h"
#include "PerspectiveTransformer.h"
#include "StrokeTransformer.h"

// TransformerFor
Transformer*
TransformerFactory::TransformerFor(uint32 type, VertexSource& source)
{
	switch (type) {
		case 0:
			return new AffineTransformer(source);
		case 1:
			return new PerspectiveTransformer(source);
		case 2:
			return new ContourTransformer(source);
		case 3:
			return new StrokeTransformer(source);
	}

	return NULL;
}

#ifdef ICON_O_MATIC
// TransformerFor
Transformer*
TransformerFactory::TransformerFor(BMessage* message,
								   VertexSource& source)
{
	switch (message->what) {
		case AffineTransformer::archive_code:
			return new AffineTransformer(source, message);
		case PerspectiveTransformer::archive_code:
			return new PerspectiveTransformer(source, message);
		case ContourTransformer::archive_code:
			return new ContourTransformer(source, message);
		case StrokeTransformer::archive_code:
			return new StrokeTransformer(source, message);
	}

	return NULL;
}

// NextType
bool
TransformerFactory::NextType(int32* cookie, uint32* type, BString* name)
{
	*type = *cookie;
	*cookie = *cookie + 1;

	switch (*type) {
		case 0:
			*name = "Transformation";
			return true;
		case 1:
			*name = "Perspective";
			return true;
		case 2:
			*name = "Contour";
			return true;
		case 3:
			*name = "Stroke";
			return true;
	}

	return false;
}
#endif // ICON_O_MATIC
