/*
 * Copyright 2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */

#include "Texture.h"

#include <GL/gl.h>


Texture::Texture()
	:
	fId(0)
{
}


Texture::~Texture()
{
	if (glIsTexture(fId)) {
		GLuint ids[1] = {fId};
		glDeleteTextures(1, ids);
	}
}


GLuint
Texture::Id()
{
	return fId;
}


void
Texture::Update(float dt)
{
}
