/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */
#ifndef _MESH_H
#define _MESH_H

#include <Referenceable.h>
#include <SupportDefs.h>

#include "Vector3.h"


struct Vertex {
	Vector3 p;
	float u;
	float v;
};


struct Face {
	Vertex v[4];
	uint16 vertexCount;
};


class Mesh : public BReferenceable {
public:
	virtual			~Mesh();

	virtual Face&	GetFace(uint32 index) const = 0;
	virtual uint32	FaceCount() const = 0;
};


#endif /* _MESH_H */
