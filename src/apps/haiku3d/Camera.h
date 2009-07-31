/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */
#ifndef _CAMERA_H
#define _CAMERA_H

#include "Vector3.h"
#include "Quaternion.h"


class Camera {
public:
					Camera(const Vector3& position,
						const Quaternion& orientation, float fov = 50.0f, float near = 1.0f,
						float far = 100.0f);
	virtual			~Camera();

	const Vector3&		Position() const;
	const Quaternion&	Orientation() const;
	float				FieldOfView() const;
	float				Near() const;
	float				Far() const;

protected:
	Vector3			fPosition;
	Quaternion		fOrientation;
	float			fFieldOfView;
	float			fNear;
	float			fFar;
	bool			fOrtho;
};


inline const Vector3&
Camera::Position() const
{
	return fPosition;
}


inline const Quaternion&
Camera::Orientation() const
{
	return fOrientation;
}


inline float
Camera::FieldOfView() const
{
	return fFieldOfView;
}


inline float
Camera::Near() const
{
	return fNear;
}


inline float
Camera::Far() const
{
	return fFar;
}

#endif /* _CAMERA_H */
