/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "GravityView.h"

#include "Gravity.h"
#include "GravitySource.h"
#include "Particle.h"

#include <GL/glu.h>


GravityView::GravityView(Gravity* parent, BRect rect)
	:
	BGLView(rect, B_EMPTY_STRING, B_FOLLOW_NONE, 0,
		BGL_RGB | BGL_DEPTH | BGL_DOUBLE)
{
	fParent = parent;

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
		realCount = 128;

	Particle::Initialize(realCount, parent->Config.ShadeID);

	LockGL();

	glClearDepth(1.0f);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, rect.Width() / rect.Height(), 2.0f, 20000.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(0.0f, 0.0f, -30.0f);

	glDepthMask(GL_FALSE);

	UnlockGL();

	fGravSource = new GravitySource();
}


GravityView::~GravityView()
{
	Particle::Terminate();
	delete fGravSource;
}


void
GravityView::AttachedToWindow()
{
	LockGL();
	BGLView::AttachedToWindow();
	UnlockGL();
}


void
GravityView::DirectDraw()
{
	LockGL();

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Particle::Tick();
	fGravSource->Tick();

	SwapBuffers();

	UnlockGL();
}

