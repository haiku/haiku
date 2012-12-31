/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef GRAVITY_SOURCE_H
#define GRAVITY_SOURCE_H


class GravitySource
{
public:
	float 	x;
	float 	y;
	float 	z;
	float	r;

	float 	vx;
	float 	vy;
	float 	vz;

	float 	tx;
	float 	ty;
	float 	tz;

			GravitySource();

	void 	Tick();

private:

};


#endif
