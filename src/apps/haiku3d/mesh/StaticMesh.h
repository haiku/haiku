/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */
#ifndef _STATICMESH_H
#define _STATICMESH_H

#include "Mesh.h"
#include "Vector3.h"


class StaticMesh : public Mesh {
public:
					StaticMesh(const char* fileName);
	virtual			~StaticMesh();

	virtual	Face&	GetFace(uint32 index) const;
	virtual uint32	FaceCount() const;

protected:
	void	_ReadText(const char* fileName);
	void	_WriteBinary(const char* fileName);
	void	_ReadBinary(const char* fileName);
	void	_ReadResource(const char* resourceName);

	Face*	fFaces;
	uint32	fFaceCount;
};

#endif /* _STATICMESH_H */
