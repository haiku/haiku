/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Tri-Edge AI <triedgeai@gmail.com>
 */
#ifndef _PARTICLE_HPP_
#define _PARTICLE_HPP_


#include <GLView.h>

#include <math.h>
#include <vector>
#include <cstdlib>


using namespace std;


class Particle
{
public:
	static vector<Particle*> list;
	
	static void Initialize(int32 size, int32 shade);
	static void Terminate();
	static void RunAll();
	static void DrawAll();

	float x;
	float y;
	float z;
	float r;
	
	float vx;
	float vy;
	float vz;
	float vr;
	
	float red;
	float green;
	float blue;
	
	Particle(float x, float y, float z, float r);
	
private:
	void Run();
	void Draw() const;
};


#endif /* _PARTICLE_HPP_ */
