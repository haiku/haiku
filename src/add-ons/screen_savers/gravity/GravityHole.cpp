/* 
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 * 
 * Authors:
 * 		Tri-Edge AI <triedgeai@gmail.com>
 */ 

#include "Particle.hpp"
#include "GravityHole.hpp"

#include <math.h>
#include <stdlib.h>

#define frand() ((float)rand() / (float)RAND_MAX)

GravityHole::GravityHole()
{
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
	
	vx = 0.0f;
	vy = 0.0f;
	vz = 0.0f;
	
	ax = frand() * 30.0f - 15.0f;
	ay = frand() * 30.0f - 15.0f;
	az = frand() * 10.0f - 5.0f;
}


void
GravityHole::Run()
{
	float dx = ax - x;
	float dy = ay - y;
	float dz = az - z;
	
	float d = dx * dx + dy * dy + dz * dz;
	
	vx += dx * 0.005f;
	vy += dy * 0.005f;
	vz += dz * 0.005f;
	
	x += vx;
	y += vy;
	z += vz;
	
	vx *= 0.95f;
	vy *= 0.95f;
	vz *= 0.95f;
	
	if (dx * dx + dy * dy + dz * dz < 10.0f) {
		ax = frand() * 30.0f - 15.0f;
		ay = frand() * 30.0f - 15.0f;
		az = frand() * 10.0f - 5.0f;
	}
	
	for (uint32 i = 0; i < Particle::list.size(); i++) {
		dx = x - Particle::list[i]->x;
		dy = y - Particle::list[i]->y;
		dz = z - Particle::list[i]->z;
		
		d = dx * dx + dy * dy + dz * dz;
		
		Particle::list[i]->vx += dx / d * 0.5f;
		Particle::list[i]->vy += dy / d * 0.5f;
		Particle::list[i]->vz += dz / d * 0.5f;
		Particle::list[i]->vr += 1.0f / d;
	}
}


void
GravityHole::Draw()
{
	//...
}
