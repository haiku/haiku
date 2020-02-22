/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexandre Deckner <alex@zappotek.com>
 */

#include "BitmapTexture.h"

#include <Bitmap.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>


BitmapTexture::BitmapTexture(BBitmap* bitmap)
	:
	Texture()
{
	_Load(bitmap);
}


BitmapTexture::~BitmapTexture()
{
}


void
BitmapTexture::_Load(BBitmap* bitmap) {
	if (bitmap == NULL)
		return;

	glGenTextures(1, &fId);
	glBindTexture(GL_TEXTURE_2D, fId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
		(int) bitmap->Bounds().Width() + 1,
		(int) bitmap->Bounds().Height() + 1,
		0, GL_BGRA, GL_UNSIGNED_BYTE,
		bitmap->Bits());

	printf("BitmapTexture::_Load, loaded texture %u "
		"(%" B_PRIi32 ", %" B_PRIi32 ", %" B_PRIi32 "bits)\n",
		fId, (int32) bitmap->Bounds().Width(),
		(int32) bitmap->Bounds().Height(),
		8 * bitmap->BytesPerRow() / (int)bitmap->Bounds().Width());

	delete bitmap;
}
