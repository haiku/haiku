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


#include "Particle.h"

#include <stdlib.h>


#define frand() ((float)rand() / (float)RAND_MAX)


BObjectList<Particle>* Particle::list;


/*static*/ void
Particle::Initialize(int32 size, int32 shade)
{
	list = new BObjectList<Particle>(2048);

	for (int32 i = 0; i < size; i++) {
		Particle* p = new Particle();
		Particle::_FillParticle(p, size, shade);
		list->AddItem(p);
	}
}


/*static*/ void
Particle::AddParticles(int32 size, int32 shade)
{
	for (int32 i = list->CountItems(); i < size; i++) {
		Particle* p = new Particle();
		Particle::_FillParticle(p, size, shade);
		list->AddItem(p);
	}
}


/*static*/ void
Particle::RemoveParticles(int32 size, int32 shade)
{
	while (list->CountItems() > size)
		delete list->RemoveItemAt(list->CountItems() - 1);
}


/*static*/ void
Particle::ColorParticles(int32 size, int32 shade)
{
	for (int32 i = 0; i < size; i++) {
		Particle* p = list->ItemAt(i);
		Particle::_ColorParticle(p, size, shade);
	}
}


/*static*/ void
Particle::Terminate()
{
	list->MakeEmpty();
	delete list;
}


/*static*/ void
Particle::Tick()
{
	for (int32 i = 0; i < list->CountItems(); i++) {
		Particle* p = list->ItemAt(i);
		p->_Logic();
		p->_Render();
	}
}


void
Particle::_Logic()
{
	// Motion
	x += vx;
	y += vy;
	z += vz;
	r += vr;

	// Friction
	vx *= 0.98f;
	vy *= 0.98f;
	vz *= 0.98f;
	vr *= 0.98f;
}


void
Particle::_Render() const
{
	glPushMatrix();
	glTranslatef(x, y, z);
	glRotatef(r, 0.0f, 0.0f, 1.0f);
	glColor3f(red, green, blue);
	glBegin(GL_QUADS);
	glVertex3f(-0.25f,  0.25f,  0.0f);
	glVertex3f(-0.25f, -0.25f,  0.0f);
	glVertex3f( 0.25f, -0.25f,  0.0f);
	glVertex3f( 0.25f,  0.25f,  0.0f);
	glEnd();
	glPopMatrix();
}


/*static*/ void
Particle::_FillParticle(Particle* p, int32 size, int32 shade)
{
	p->x = frand() * 30.0f - 15.0f;
	p->y = frand() * 30.0f - 15.0f;
	p->z = frand() * 5.0f;
	p->r = frand() * 360.0f;

	p->vx = frand() - 0.5f;
	p->vy = frand() - 0.5f;
	p->vz = frand() - 0.5f;
	p->vr = (frand() - 0.5f) * 180.0f;

	Particle::_ColorParticle(p, size, shade);
}


/*static*/ void
Particle::_ColorParticle(Particle* p, int32 size, int32 shade)
{
	switch(shade) {
		case 0:
			// Red
			p->red   = 0.1f + frand() * 0.2f;
			p->green = 0.0f;
			p->blue  = frand() * 0.05f;
			break;

		case 1:
			// Green
			p->red   = 0;
			p->green = 0.1f + frand() * 0.2f;
			p->blue  = frand() * 0.05f;
			break;

		case 2:
		default:
			// Blue
			p->red   = 0;
			p->green = frand() * 0.05f;
			p->blue  = 0.1f + frand() * 0.2f;
			break;

		case 3:
			// Orange
			p->red   = 0.1f + frand() * 0.1f;
			p->green = 0.05f + frand() * 0.1f;
			p->blue  = 0.0f;
			break;

		case 4:
			// Purple
			p->red   = 0.1f + frand() * 0.2f;
			p->green = 0.0f;
			p->blue  = 0.1f + frand() * 0.2f;
			break;

		case 5:
			// White
			p->red = p->green = p->blue = 0.1f + frand() * 0.2f;
			break;

		case 6:
			// Rainbow
			p->red   = 0.1f + frand() * 0.2f;
			p->green = 0.1f + frand() * 0.2f;
			p->blue  = 0.1f + frand() * 0.2f;
			break;
	}
}
