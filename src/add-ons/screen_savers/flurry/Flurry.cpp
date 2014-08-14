/*

Copyright (c) 2002, Calum Robinson
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the author nor the names of its contributors may be used
  to endorse or promote products derived from this software without specific
  prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/*
 * Copyright Karsten Heimrich, host.haiku@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Flurry.h"

#include <new>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <Catalog.h>

#include "Shared.h"
#include "Smoke.h"
#include "Spark.h"
#include "Star.h"
#include "Texture.h"


using namespace BPrivate;


//	#pragma mark - FlurryView


FlurryView::FlurryView(BRect bounds)
	:
	BGLView(bounds, (const char *)NULL, B_FOLLOW_ALL,
		B_FRAME_EVENTS | B_WILL_DRAW,
		BGL_RGB | BGL_ALPHA | BGL_DEPTH | BGL_DOUBLE),
	fOldFrameTime(-1.0),
	fFlurryInfo(NULL)
{
	B_TRANSLATE_MARK_SYSTEM_NAME_VOID("Flurry");

	fWidth = bounds.Width();
	fHeight = bounds.Height();
	fStartTime = _CurrentTime();

	LockGL();
	_SetupFlurryBaseInfo();
	UnlockGL();
}


FlurryView::~FlurryView()
{
	if (fFlurryInfo != NULL) {
		LockGL();

		free(fFlurryInfo->s);
		free(fFlurryInfo->star);
		for (int32 i = 0; i < MAX_SPARKS; ++i)
			free(fFlurryInfo->spark[i]);
		free(fFlurryInfo);

		UnlockGL();
	}
}


status_t
FlurryView::InitCheck() const
{
	return (fFlurryInfo != NULL) ? B_OK : B_ERROR;
}


void
FlurryView::AttachedToWindow()
{
	LockGL();

	BGLView::AttachedToWindow();

	MakeTexture();

	glDisable(GL_DEPTH_TEST);
	glAlphaFunc(GL_GREATER, 0.0);
	glEnable(GL_ALPHA_TEST);
	glShadeModel(GL_FLAT);
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);

	glViewport(0, 0, int(fWidth), int(fHeight));
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, fWidth, 0.0, fHeight);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	UnlockGL();
}


void
FlurryView::DrawFlurryScreenSaver()
{
	double deltaFrameTime = 0.0;
	const double newFrameTime = _CurrentTime();

	GLfloat alpha = 1.0;
	if (fOldFrameTime >= 0.0) {
		deltaFrameTime = newFrameTime - fOldFrameTime;
		alpha = 5.0 * deltaFrameTime;

		if (alpha > 0.2)
			alpha = 0.2;
	}

	fOldFrameTime = newFrameTime;

	LockGL();

	// TODO: enable once double buffering is supported
	//glDrawBuffer(GL_BACK);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4f(0.0, 0.0, 0.0, alpha);
	glRectd(0.0, 0.0, fWidth, fHeight);

	fFlurryInfo->dframe++;
	fFlurryInfo->fOldTime = fFlurryInfo->fTime;
	fFlurryInfo->fTime = _SecondsSinceStart() + fFlurryInfo->randomSeed;
	fFlurryInfo->fDeltaTime = fFlurryInfo->fTime - fFlurryInfo->fOldTime;
	fFlurryInfo->drag = (float)pow(0.9965, fFlurryInfo->fDeltaTime * 85.0);

	UpdateStar(fFlurryInfo, fFlurryInfo->star);

	glEnable(GL_BLEND);
	glShadeModel(GL_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	for (int32 i = 0; i < fFlurryInfo->numStreams; ++i) {
		fFlurryInfo->spark[i]->color[0] = 1.0;
		fFlurryInfo->spark[i]->color[1] = 1.0;
		fFlurryInfo->spark[i]->color[2] = 1.0;
		fFlurryInfo->spark[i]->color[3] = 1.0;

		UpdateSpark(fFlurryInfo, fFlurryInfo->spark[i]);
	}

	UpdateSmoke_ScalarBase(fFlurryInfo, fFlurryInfo->s);

	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	const double brite = pow(deltaFrameTime, 0.75) * 10.0;
	DrawSmoke_Scalar(fFlurryInfo, fFlurryInfo->s,
		brite * fFlurryInfo->briteFactor);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	glFinish();

	SwapBuffers();
	UnlockGL();
}


void
FlurryView::FrameResized(float newWidth, float newHeight)
{
	LockGL();

	BGLView::FrameResized(newWidth, newHeight);

	if (fFlurryInfo != NULL) {
		fWidth = newWidth;
		fHeight = newHeight;

		fFlurryInfo->sys_glWidth = fWidth;
		fFlurryInfo->sys_glHeight = fHeight;

		glViewport(0, 0, int(fWidth), int(fHeight));
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0.0, fWidth, 0.0, fHeight);
		glMatrixMode(GL_MODELVIEW);

		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);

		glFlush();
	}

	UnlockGL();
}


void
FlurryView::_SetupFlurryBaseInfo()
{
	fFlurryInfo = (flurry_info_t*)malloc(sizeof(flurry_info_t));

	if (fFlurryInfo == NULL)
		return;

	fFlurryInfo->next = NULL;
	fFlurryInfo->randomSeed = RandFlt(0.0, 300.0);

	fFlurryInfo->dframe = 0;
	fFlurryInfo->fOldTime = 0.0;
	fFlurryInfo->sys_glWidth = fWidth;
	fFlurryInfo->sys_glHeight = fHeight;
	fFlurryInfo->fTime = _SecondsSinceStart() + fFlurryInfo->randomSeed;
	fFlurryInfo->fDeltaTime = fFlurryInfo->fTime - fFlurryInfo->fOldTime;

	fFlurryInfo->numStreams = 5;
	fFlurryInfo->briteFactor = 1.0;
	fFlurryInfo->streamExpansion = 10000.0;
	fFlurryInfo->currentColorMode = tiedyeColorMode;

	fFlurryInfo->s = (SmokeV*)malloc(sizeof(SmokeV));
	InitSmoke(fFlurryInfo->s);

	fFlurryInfo->star = (Star*)malloc(sizeof(Star));
	InitStar(fFlurryInfo->star);

	fFlurryInfo->star->rotSpeed = 1.0;

	for (int32 i = 0; i < MAX_SPARKS; ++i) {
		fFlurryInfo->spark[i] = (Spark*)malloc(sizeof(Spark));
		InitSpark(fFlurryInfo->spark[i]);
		fFlurryInfo->spark[i]->mystery = 1800 * (i + 1) / 13;
		UpdateSpark(fFlurryInfo, fFlurryInfo->spark[i]);
	}

	for (int32 i = 0; i < NUMSMOKEPARTICLES / 4; ++i) {
		for (int32 k = 0; k < 4; ++k)
			fFlurryInfo->s->p[i].dead.i[k] = 1;
	}
}


double
FlurryView::_CurrentTime() const
{
	return double(BDateTime::CurrentDateTime(B_LOCAL_TIME).Time_t() +
		double(BTime::CurrentTime(B_LOCAL_TIME).Millisecond() / 1000.0));
}


double
FlurryView::_SecondsSinceStart() const
{
	return _CurrentTime() - fStartTime;
}


// #pragma mark - Flurry


extern "C" BScreenSaver*
instantiate_screen_saver(BMessage* archive, image_id imageId)
{
	return new Flurry(archive, imageId);
}


Flurry::Flurry(BMessage* archive, image_id imageId)
	:
	BScreenSaver(archive, imageId),
	fFlurryView(NULL)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	srand((999 * tv.tv_sec) + (1001 * tv.tv_usec) + (1003 * getpid()));
}


Flurry::~Flurry()
{
}


status_t
Flurry::InitCheck()
{
	return B_OK;
}


status_t
Flurry::StartSaver(BView* view, bool preview)
{
	status_t result = B_ERROR;

	SetTickSize(50000);

	fFlurryView = new (std::nothrow) FlurryView(view->Bounds());
	if (fFlurryView != NULL) {
		if (fFlurryView->InitCheck() != B_OK) {
			delete fFlurryView;
			fFlurryView = NULL;
		} else {
			result = B_OK;
			view->AddChild(fFlurryView);
		}
	}

	return result;
}


void
Flurry::StopSaver()
{
	if (fFlurryView != NULL)
		fFlurryView->EnableDirectMode(false);
}


void
Flurry::Draw(BView*, int32 frame)
{
	fFlurryView->DrawFlurryScreenSaver();
}


void
Flurry::DirectConnected(direct_buffer_info* info)
{
	if (fFlurryView != NULL) {
		fFlurryView->DirectConnected(info);
		fFlurryView->EnableDirectMode(true);
	}
}


void
Flurry::StartConfig(BView* configView)
{
}


void
Flurry::StopConfig()
{
}


status_t
Flurry::SaveState(BMessage* into) const
{
	return B_ERROR;
}
