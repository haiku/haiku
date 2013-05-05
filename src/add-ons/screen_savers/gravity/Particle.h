/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PARTICLE_H
#define PARTICLE_H


#include <GLView.h>

class BList;


class Particle
{
public:
	static	BList* 	list;

	static	void 	Initialize(int32 size, int32 shade);
	static	void 	Terminate();
	static	void 	Tick();

			float 	x;
			float 	y;
			float 	z;
			float 	r;

			float 	vx;
			float 	vy;
			float 	vz;
			float 	vr;

			float 	red;
			float 	green;
			float 	blue;

private:
			void 	_Logic();
			void 	_Render() const;

};


#endif
