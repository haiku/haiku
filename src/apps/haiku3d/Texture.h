/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */
#ifndef _TEXTURE_H
#define _TEXTURE_H

#include <Referenceable.h>

#include <GL/gl.h>


class Texture : public BReferenceable {
public:
					Texture();
	virtual			~Texture();

	GLuint			Id();

	virtual	void	Update(float dt);

protected:
	GLuint			fId;
};

#endif /* _TEXTURE_H */
