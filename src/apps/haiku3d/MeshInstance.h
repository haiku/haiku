/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */

#ifndef _MESH_INSTANCE_H
#define _MESH_INSTANCE_H

#include "Mesh.h"
#include "Texture.h"
#include "Vector3.h"
#include "Quaternion.h"


class MeshInstance {
public:
				MeshInstance(Mesh* mesh, Texture* texture,
					const Vector3& position, const Quaternion& orientation,
					float animOffset);
				~MeshInstance();

	void		Update(float dt);
	void		Render();

protected:
	BReference<Mesh>	fMeshReference;
	BReference<Texture>	fTextureReference;

	Vector3		fPosition;
	Quaternion	fOrientation;
	float		fScale;

	// TODO: manage the animation externally
	float		fTime;
	float		fAnimOffset;

	bool		fDoubleSided;
	bool		fDrawNormals;
};

#endif /* _MESH_INSTANCE_H */
