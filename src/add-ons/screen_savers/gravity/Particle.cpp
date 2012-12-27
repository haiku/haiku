/* 
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 * 
 * Authors:
 * 		Tri-Edge AI <triedgeai@gmail.com>
 */ 


#include "Particle.hpp"


#define frand() ((float)rand() / (float)RAND_MAX)

vector<Particle*> Particle::list;


void
Particle::Initialize(int32 size, int32 shade)
{	
	for (int32 i = 0; i < size; i++) {
		Particle* p = new Particle(frand() * 30.0f - 15.0f, 
			frand() * 30.0f - 15.0f, frand() * 5.0f, frand() * 360.0f);
		
		p->vx = frand() - 0.5f;
		p->vy = frand() - 0.5f;
		p->vz = frand() - 0.5f;
		p->vr = (frand() - 0.5f) * 180.0f;
		
		if (shade == 0) { // Red
			p->red = 0.1f + frand() * 0.2f;
			p->green = 0.0f;
			p->blue = frand() * 0.05f; 
		} else if (shade == 1) { // Green
			p->red = 0;
			p->green =  0.1f + frand() * 0.2f;
			p->blue = frand() * 0.05f;
		} else if (shade == 2) { // Blue
			p->red = 0;
			p->green = frand() * 0.05f;
			p->blue = 0.1f + frand() * 0.2f;
		} else if (shade == 3) { // Orange
			p->red = 0.1f + frand() * 0.1f;
			p->green =  0.05f + frand() * 0.1f;
			p->blue = 0.0f;
		} else if (shade == 4) { // Purple
			p->red = 0.1f + frand() * 0.2f;
			p->green = 0.0f;
			p->blue = 0.1f + frand() * 0.2f;
		} else if (shade == 5) { // White 
			p->red = p->green = p->blue = 0.1f + frand() * 0.2f;
		} else if (shade == 6) { // Rainbow
			p->red = 0.1f + frand() * 0.2f;
			p->green = 0.1f + frand() * 0.2f;
			p->blue = 0.1f + frand() * 0.2f;
		} else {
			// Man, this shouldn't even happen..	Blue.
			p->red = 0;
			p->green = frand() * 0.05f;
			p->blue = 0.1f + frand() * 0.2f;
		}
		
		list.push_back(p);
	}
}


void
Particle::Terminate()
{
	for (uint32 i = 0; i < list.size(); i++)
		delete list[i];	
	
	list.clear();
}


void
Particle::RunAll()
{
	for (uint32 i = 0; i < list.size(); i++)
		list[i]->Run();
}


void
Particle::DrawAll()
{
	for (uint32 i = 0; i < list.size(); i++)
		list[i]->Draw();
}


Particle::Particle(float x, float y, float z, float r)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->r = r;
}


void
Particle::Run()
{
	x += vx;
	y += vy;
	z += vz;
	r += vr;
	
	vx *= 0.98f;
	vy *= 0.98f;
	vz *= 0.98f;
	vr *= 0.98f;
}


void
Particle::Draw() const
{
	glPushMatrix();
	glTranslatef(x, y, z);
	glRotatef(r, 0.0f, 0.0f, 1.0f);
	glBegin(GL_QUADS);
	glColor3f(red, green, blue);
	glVertex3f(-0.5f,  0.5f,  0.0f);
	glVertex3f(-0.5f, -0.5f,  0.0f);
	glVertex3f( 0.5f, -0.5f,  0.0f);
	glVertex3f( 0.5f,  0.5f,  0.0f);
	glEnd();
	glPopMatrix();
}
