/*
 * Copyright 2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */

#include "RenderView.h"

#include "BitmapTexture.h"
#include "Camera.h"
#include "MeshInstance.h"
#include "StaticMesh.h"
#include "VideoFileTexture.h"

#include <GL/gl.h>
#include <GL/glu.h>

#include <TranslationKit.h>
#include <TranslationUtils.h>

#include <stdio.h>

RenderView::RenderView(BRect frame)
	:
	BGLView(frame, "renderView", B_FOLLOW_ALL, B_WILL_DRAW,
		BGL_RGB | BGL_DOUBLE | BGL_DEPTH),
	fMainCamera(NULL),
	fRenderThread(-1),
	fStopRendering(false),
	fRes(0, 0),
	fNextRes(0, 0),
	fLastFrameTime(0)
{
}


RenderView::~RenderView()
{
}


void
RenderView::AttachedToWindow()
{
	BGLView::AttachedToWindow();

	_CreateScene();
	_InitGL();
	if (_CreateRenderThread() != B_OK)
		printf("Error trying to start the render thread!\n");
}


void
RenderView::DetachedFromWindow()
{
	_StopRenderThread();
	_DeleteScene();

	BGLView::DetachedFromWindow();
}


uint32
RenderView::_CreateRenderThread()
{
	fRenderThread = spawn_thread(RenderView::_RenderThreadEntry, "renderThread",
		B_NORMAL_PRIORITY, this);

	if (fRenderThread < 0)
		return fRenderThread;

	return resume_thread(fRenderThread);
}


void
RenderView::_StopRenderThread()
{
	LockGL();
	fStopRendering = true;
	UnlockGL();

	if (fRenderThread >= 0)
		wait_for_thread(fRenderThread, NULL);
}


int32
RenderView::_RenderThreadEntry(void* pointer)
{
	return reinterpret_cast<RenderView*>(pointer)->_RenderLoop();
}


int32
RenderView::_RenderLoop()
{
	fLastFrameTime = system_time();

	while (_Render()) {
		snooze(10000);
	}
	return B_OK;
}


void RenderView::_InitGL(void)
{
	LockGL();

	float position[] = {0.0, 3.0, 6.0, 0.0};
	float local_view[] = {0.0, 0.0};

	glLightfv(GL_LIGHT0, GL_POSITION, position);
	glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, local_view);

	float white[3] = {1.0, 1.0, 1.0};

	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, white);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
	glLightfv(GL_LIGHT0, GL_AMBIENT, white);

	glMaterialf(GL_FRONT, GL_SHININESS, 0.6 * 128.0);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);

	fNextRes.Set(Bounds().Width(), Bounds().Height());
	_UpdateViewport();

	UnlockGL();
}


void
RenderView::_CreateScene()
{
	LockGL();

	Texture* texture = new BitmapTexture(
		BTranslationUtils::GetBitmap(B_PNG_FORMAT, "texture"));

	float spacing = 1.6f;
	float timeSpacing = 5.0f;
	float yOffset = -1.0f;
	float zOffset = -16;

	Mesh* mesh = new StaticMesh("LetterH");
	MeshInstance* instance = new MeshInstance(mesh, texture,
		Vector3(-3.6 * spacing, yOffset, zOffset),
		Quaternion(0, 0, 0, 1), 0.0f);
	fMeshInstances.push_back(instance);
	mesh->ReleaseReference();

	mesh = new StaticMesh("LetterA");
	instance = new MeshInstance(mesh, texture,
		Vector3(-1.6 * spacing, yOffset, zOffset),
		Quaternion(0, 0, 0, 1), 1.0f * timeSpacing);
	fMeshInstances.push_back(instance);
	mesh->ReleaseReference();

	mesh = new StaticMesh("LetterI");
	instance = new MeshInstance(mesh, texture,
		Vector3(0 * spacing, yOffset, zOffset),
		Quaternion(0, 0, 0, 1), 2.0f * timeSpacing);
	fMeshInstances.push_back(instance);
	mesh->ReleaseReference();

	mesh = new StaticMesh("LetterK");
	instance = new MeshInstance(mesh, texture,
		Vector3(1.5 * spacing, yOffset, zOffset),
		Quaternion(0, 0, 0, 1), 3.0f * timeSpacing);
	fMeshInstances.push_back(instance);
	mesh->ReleaseReference();

	mesh = new StaticMesh("LetterU");
	instance = new MeshInstance(mesh, texture,
		Vector3(3.4 * spacing, yOffset, zOffset),
		Quaternion(0, 0, 0, 1), 4.0f * timeSpacing);
	fMeshInstances.push_back(instance);
	mesh->ReleaseReference();
	texture->ReleaseReference();

	fMainCamera = new Camera(Vector3(0, 0, 0), Quaternion(0, 0, 0, 1), 50);

	UnlockGL();
}


void
RenderView::_DeleteScene()
{
	LockGL();

	MeshInstanceList::iterator it = fMeshInstances.begin();
	for (; it != fMeshInstances.end(); it++) {
		delete (*it);
	}
	fMeshInstances.clear();

	UnlockGL();
}


void
RenderView::_UpdateViewport()
{
	if (fNextRes != fRes && fNextRes.x >= 1.0 && fNextRes.y >= 1.0) {
		glViewport(0, 0, (GLint) fNextRes.x + 1, (GLint) fNextRes.y + 1);
		fRes = fNextRes;
		_UpdateCamera();
	}
}


void
RenderView::_UpdateCamera()
{
	// TODO: take camera orientation into account
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fMainCamera->FieldOfView(), fNextRes.x / fNextRes.y,
		fMainCamera->Near(), fMainCamera->Far());
	glMatrixMode(GL_MODELVIEW);
}


bool
RenderView::_Render()
{
	LockGL();

	_UpdateViewport();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	bigtime_t time = system_time();
	float deltaTime = 0.000001 * (float)(time - fLastFrameTime);
	fLastFrameTime = time;

	MeshInstanceList::iterator it = fMeshInstances.begin();
	for (; it != fMeshInstances.end(); it++) {
		(*it)->Update(deltaTime);
		(*it)->Render();
	}

	if (fStopRendering) {
		UnlockGL();
		return false;
	}
	UnlockGL();
	SwapBuffers(false); // true = vsync
	return true;
}


void
RenderView::FrameResized(float width, float height)
{
	LockGL();
	fNextRes.Set(width, height);
	UnlockGL();
	BGLView::FrameResized(width, height);
}


void
RenderView::ErrorCallback(unsigned long error)
{
	fprintf(stderr, "OpenGL error (%lu): %s\n", error, gluErrorString(error));
}
