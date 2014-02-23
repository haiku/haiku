/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Tri-Edge AI
 *		John Scipione, jscipione@gmail.com
 */

#ifndef GRAVITY_SOURCE_H
#define GRAVITY_SOURCE_H


struct GravitySource {
	float	x;
	float	y;
	float	z;
	float	r;

	float	vx;
	float	vy;
	float	vz;

	float	tx;
	float	ty;
	float	tz;

			GravitySource();

	void	Tick();
};


#endif	// GRAVITY_SOURCE_H
