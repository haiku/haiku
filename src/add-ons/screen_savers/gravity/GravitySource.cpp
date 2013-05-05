/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "GravitySource.h"

#include "Particle.h"

#include <stdlib.h>


#define frand() ((float)rand() / (float)RAND_MAX)


GravitySource::GravitySource()
{
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
	r = 0.0f;

	vx = 0.0f;
	vy = 0.0f;
	vz = 0.0f;

	tx = frand() * 30.0f - 15.0f;
	ty = frand() * 30.0f - 15.0f;
	tz = frand() * 10.0f - 5.0f;
}


void
GravitySource::Tick()
{
	float dx = tx - x;
	float dy = ty - y;
	float dz = tz - z;

	float d = dx * dx + dy * dy + dz * dz;

	vx += dx * 0.003f;
	vy += dy * 0.003f;
	vz += dz * 0.003f;

	x += vx;
	y += vy;
	z += vz;

	vx *= 0.98f;
	vy *= 0.98f;
	vz *= 0.98f;

	if (dx * dx + dy * dy + dz * dz < 1.0f) {
		tx = frand() * 20.0f - 10.0f;
		ty = frand() * 20.0f - 10.0f;
		tz = frand() * 10.0f - 5.0f;
	}

	for (int32 i = 0; i < Particle::list->CountItems(); i++) {
		Particle* p = (Particle*)Particle::list->ItemAt(i);
		dx = x - p->x;
		dy = y - p->y;
		dz = z - p->z;

		d = dx * dx + dy * dy + dz * dz;

		p->vx += dx / d * 0.25f;
		p->vy += dy / d * 0.25f;
		p->vz += dz / d * 0.25f;
		p->vr += 1.0f / d;
	}
}
