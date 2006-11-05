/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef LAYER_H
#define LAYER_H

#include <Rect.h>
#include <String.h>

// property flags
enum {
	FLAG_INVISIBLE		= 0x01,
};

// blending modes (as of WonderBrush 2.0)
enum {
	MODE_NORMAL					= 0,
	MODE_MULTIPLY				= 1,
	MODE_INVERSE_MULTIPLY		= 2,
	MODE_LUMINANCE				= 3,
	MODE_MULTIPLY_ALPHA			= 4,
	MODE_MULTIPLY_INVERSE_ALPHA	= 5,

	MODE_REPLACE_RED			= 6,
	MODE_REPLACE_GREEN			= 7,
	MODE_REPLACE_BLUE			= 8,

	MODE_DARKEN					= 9,
	MODE_LIGHTEN				= 10,

	MODE_HARD_LIGHT				= 11,
	MODE_SOFT_LIGHT				= 12,
};

class BBitmap;
class BMessage;

class Layer {
 public:
								Layer();
								~Layer();

	// active area of layer
	inline	BRect				ActiveBounds() const
									{ return fBounds; }

	// composing
			status_t			Compose(const BBitmap* into,
										BRect area);

	// loading
			status_t			Unarchive(const BMessage* archive);

 protected:
			BBitmap*			fBitmap;
			BRect				fBounds;

			float				fAlpha;
			uint32				fMode;
			uint32				fFlags;
};

#endif // LAYER_H
