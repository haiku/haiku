/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "Particle.h"

#include <List.h>

#include <stdlib.h>


#define frand() ((float)rand() / (float)RAND_MAX)


BList* Particle::list;


void
Particle::Initialize(int32 size, int32 shade)
{
	list = new BList();

	for (int32 i = 0; i < size; i++) {
		Particle* p = new Particle();

		p->x = frand() * 30.0f - 15.0f;
		p->y = frand() * 30.0f - 15.0f;
		p->z = frand() * 5.0f;
		p->r = frand() * 360.0f;

		p->vx = frand() - 0.5f;
		p->vy = frand() - 0.5f;
		p->vz = frand() - 0.5f;
		p->vr = (frand() - 0.5f) * 180.0f;

		if (shade == 0) {
			// Red
			p->red = 0.1f + frand() * 0.2f;
			p->green = 0.0f;
			p->blue = frand() * 0.05f;
		} else if (shade == 1) {
			// Green
			p->red = 0;
			p->green =  0.1f + frand() * 0.2f;
			p->blue = frand() * 0.05f;
		} else if (shade == 2) {
			// Blue
			p->red = 0;
			p->green = frand() * 0.05f;
			p->blue = 0.1f + frand() * 0.2f;
		} else if (shade == 3) {
			// Orange
			p->red = 0.1f + frand() * 0.1f;
			p->green =  0.05f + frand() * 0.1f;
			p->blue = 0.0f;
		} else if (shade == 4) {
			// Purple
			p->red = 0.1f + frand() * 0.2f;
			p->green = 0.0f;
			p->blue = 0.1f + frand() * 0.2f;
		} else if (shade == 5) {
			// White
			p->red = p->green = p->blue = 0.1f + frand() * 0.2f;
		} else if (shade == 6) {
			// Rainbow
			p->red = 0.1f + frand() * 0.2f;
			p->green = 0.1f + frand() * 0.2f;
			p->blue = 0.1f + frand() * 0.2f;
		} else {
			// Man, this shouldn't even happen.. Blue.
			p->red = 0;
			p->green = frand() * 0.05f;
			p->blue = 0.1f + frand() * 0.2f;
		}

		list->AddItem(p);
	}
}


void
Particle::Terminate()
{
	for (int32 i = 0; i < list->CountItems(); i++)
		delete (Particle*)list->ItemAt(i);

	list->MakeEmpty();
	delete list;
}


void
Particle::Tick()
{
	for (int32 i = 0; i < list->CountItems(); i++) {
		Particle* p = (Particle*)list->ItemAt(i);
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
