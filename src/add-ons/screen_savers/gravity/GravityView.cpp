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


#include "GravityView.h"

#include "Gravity.h"
#include "GravitySource.h"
#include "Particle.h"

#include <GL/glu.h>


GravityView::GravityView(BRect frame, Gravity* parent)
	:
	BGLView(frame, B_EMPTY_STRING, B_FOLLOW_NONE, 0,
		BGL_RGB | BGL_DEPTH | BGL_DOUBLE),
	fParent(parent),
	fGravitySource(new GravitySource()),
	fSize(128 << parent->Config.ParticleCount),
	fShade(parent->Config.ShadeID)
{
	Particle::Initialize(fSize, fShade);
}


GravityView::~GravityView()
{
	Particle::Terminate();
	delete fGravitySource;
}


void
GravityView::AttachedToWindow()
{
	LockGL();
	BGLView::AttachedToWindow();

	glClearDepth(1.0f);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, Bounds().Width() / Bounds().Height(), 2.0f, 20000.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(0.0f, 0.0f, -30.0f);

	glDepthMask(GL_FALSE);

	UnlockGL();
}


void
GravityView::DirectDraw()
{
	int32 size = 128 << fParent->Config.ParticleCount;
	int32 shade = fParent->Config.ShadeID;

	// resize particle list if needed
	if (size > fSize)
		Particle::AddParticles(size, shade);
	else if (size < fSize)
		Particle::RemoveParticles(size, shade);

	// recolor particles if needed
	if (shade != fShade)
		Particle::ColorParticles(size, shade);

	fSize = size;
	fShade = shade;

	LockGL();

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Particle::Tick();
	fGravitySource->Tick();

	SwapBuffers();

	UnlockGL();
}

