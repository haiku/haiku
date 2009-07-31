/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */
#ifndef _BITMAP_TEXTURE_H
#define _BITMAP_TEXTURE_H

#include "Texture.h"

class BBitmap;

class BitmapTexture : public Texture {
public:
				BitmapTexture(BBitmap* bitmap);
	virtual		~BitmapTexture();

protected:
	void		_Load(BBitmap* bitmap);
};

#endif /* _BITMAP_TEXTURE_H */
