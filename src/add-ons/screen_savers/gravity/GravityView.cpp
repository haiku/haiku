/* 
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 * 
 * Authors:
 * 		Tri-Edge AI <triedgeai@gmail.com>
 */ 


#include "GravityView.hpp"

#include <GL/glu.h>


GravityView::GravityView(GravityScreenSaver* parent, BRect rect)
	:
	BGLView(rect, B_EMPTY_STRING, B_FOLLOW_NONE, 0, 
		BGL_RGB | BGL_DEPTH | BGL_DOUBLE),
	fRect(rect)
{
	this->parent = parent;
	
	int realCount;
	
	if (parent->Config.ParticleCount == 0)
		realCount = 128;
	else if (parent->Config.ParticleCount == 1)
		realCount = 256;	
	else if (parent->Config.ParticleCount == 2)
		realCount = 512;	
	else if (parent->Config.ParticleCount == 3)
		realCount = 1024;	
	else if (parent->Config.ParticleCount == 4)
		realCount = 2048;	
	else
		realCount = 128; // This shouldn't be happening either.	
	
	Particle::Initialize(realCount, parent->Config.ShadeID);
	
	LockGL();

	glClearDepth(1.0f);
	
	glEnable(GL_TEXTURE_2D);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, rect.Width() / rect.Height(), 2.0f, 20000.0f);
	glTranslatef(0.0f, 0.0f, -30.0f);
	
	glDepthMask(GL_FALSE);
	
	glMatrixMode(GL_MODELVIEW);
	
	UnlockGL();
	
	ghole = new GravityHole();
}


GravityView::~GravityView()
{
	delete ghole;
	Particle::Terminate();
}


void
GravityView::AttachedToWindow()
{
	LockGL();
	BGLView::AttachedToWindow();
	UnlockGL();
}


void
GravityView::Draw(BRect rect)
{
	LockGL();
	
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	Particle::DrawAll();
	ghole->Draw();

	SwapBuffers();
	UnlockGL();
}


void
GravityView::Run()
{
	Particle::RunAll();
	ghole->Run();
}
