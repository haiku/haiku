/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef WONDERBRUSH_IMAGE_H
#define WONDERBRUSH_IMAGE_H

#include <Message.h>

class BBitmap;
class BPositionIO;
class Canvas;

class WonderBrushImage {
 public:
								WonderBrushImage();
	virtual						~WonderBrushImage();

			status_t			SetTo(BPositionIO* stream);
			BBitmap*			Bitmap() const;

 private:
			BMessage			fArchive;
};

#endif // WONDERBRUSH_IMAGE_H

