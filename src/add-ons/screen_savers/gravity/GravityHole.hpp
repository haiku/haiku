/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Tri-Edge AI <triedgeai@gmail.com>
 */

#ifndef _GRAVITY_HOLE_HPP_
#define _GRAVITY_HOLE_HPP_

#include <GLView.h>

class GravityHole
{
public:
	float x;
	float y;
	float z;
	
	float vx;
	float vy;
	float vz;
	
	float ax;
	float ay;
	float az;
	
	GravityHole();
	
	void Run();
	void Draw();
};

#endif /* _GRAVITY_HOLE_HPP */
