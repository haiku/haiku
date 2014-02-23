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
#ifndef PARTICLE_H
#define PARTICLE_H


#include <ObjectList.h>
#include <GLView.h>


class Particle {
public:
	static	BObjectList<Particle>*	list;

	static	void					Initialize(int32 size, int32 shade);
	static	void					AddParticles(int32 size, int32 shade);
	static	void					RemoveParticles(int32 size, int32 shade);
	static	void					ColorParticles(int32 size, int32 shade);
	static	void					Terminate();
	static	void					Tick();

			float					x;
			float					y;
			float					z;
			float					r;

			float					vx;
			float					vy;
			float					vz;
			float					vr;

			float					red;
			float					green;
			float					blue;

private:
			void					_Logic();
			void					_Render() const;

	static	void					_FillParticle(Particle* p, int32 size,
										int32 shade);
	static	void					_ColorParticle(Particle* p, int32 size,
										int32 shade);
};


#endif	// PARTICLE_H
