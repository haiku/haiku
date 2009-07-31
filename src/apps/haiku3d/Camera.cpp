/*
 * Copyright 2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */

#include "Camera.h"


Camera::Camera(const Vector3& position, const Quaternion& orientation,
	float fov, float near, float far)
	:
	fPosition(position),
	fOrientation(orientation),
	fFieldOfView(fov),
	fNear(near),
	fFar(far)
{
}


Camera::~Camera()
{
}
