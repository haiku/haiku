/*
 * Copyright 2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexandre Deckner
 *
 */

/*
 * Original Be Sample source modified to use a quaternion for the object's orientation
 */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "ObjectView.h"

#include <Application.h>
#include <Catalog.h>
#include <Cursor.h>
#include <InterfaceKit.h>
#include <FindDirectory.h>

#include "FPS.h"
#include "GLObject.h"
#include "ResScroll.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "ObjectView"

float displayScale = 1.0;
float depthOfView = 30.0;
float zRatio = 10.0;

float white[3] = {1.0, 1.0, 1.0};
float dimWhite[3] = {0.25, 0.25, 0.25};
float black[3] = {0.0, 0.0, 0.0};
float foggy[3] = {0.4, 0.4, 0.4};
float blue[3] = {0.0, 0.0, 1.0};
float dimBlue[3] = {0.0, 0.0, 0.5};
float yellow[3] = {1.0, 1.0, 0.0};
float dimYellow[3] = {0.5, 0.5, 0.0};
float green[3] = {0.0, 1.0, 0.0};
float dimGreen[3] = {0.0, 0.5, 0.0};
float red[3] = {1.0, 0.0, 0.0};

float* bgColor = black;

const char *kNoResourceError = B_TRANSLATE("The Teapot 3D model was "
									"not found in application resources. "
									"Please repair the program installation.");

struct light {
	float *ambient;
	float *diffuse;
	float *specular;
};


light lights[] = {
	{NULL, NULL, NULL},
	{dimWhite, white, white},
	{dimWhite, yellow, yellow},
	{dimWhite, red, red},
	{dimWhite, blue, blue},
	{dimWhite, green, green}
};



long
signalEvent(sem_id event)
{
	long c;
	get_sem_count(event,&c);
	if (c < 0)
		release_sem_etc(event,-c,0);

	return 0;
}


long
setEvent(sem_id event)
{
	long c;
	get_sem_count(event,&c);
	if (c < 0)
	  release_sem_etc(event,-c,0);

	return 0;
}


long
waitEvent(sem_id event)
{
	acquire_sem(event);

	long c;
	get_sem_count(event,&c);
	if (c > 0)
		acquire_sem_etc(event,c,0,0);

	return 0;
}


static int32
simonThread(void* cookie)
{
	ObjectView* objectView = reinterpret_cast<ObjectView*>(cookie);

	int noPause = 0;
	while (acquire_sem_etc(objectView->quittingSem, 1, B_TIMEOUT, 0) == B_NO_ERROR) {
		if (objectView->SpinIt()) {
			objectView->DrawFrame(noPause);
			release_sem(objectView->quittingSem);
			noPause = 1;
		} else {
			release_sem(objectView->quittingSem);
			noPause = 0;
			waitEvent(objectView->drawEvent);
		}
	}
	return 0;
}


ObjectView::ObjectView(BRect rect, const char *name, ulong resizingMode,
	ulong options)
	: BGLView(rect, name, resizingMode, 0, options),
	fHistEntries(0),
	fOldestEntry(0),
	fFps(true),
	fLastGouraud(true),
	fGouraud(true),
	fLastZbuf(true),
	fZbuf(true),
	fLastCulling(true),
	fCulling(true),
	fLastLighting(true),
	fLighting(true),
	fLastFilled(true),
	fFilled(true),
	fLastPersp(false),
	fPersp(false),
	fLastTextured(false),
	fTextured(false),
	fLastFog(false),
	fFog(false),
	fForceRedraw(false),
	fLastYXRatio(1),
	fYxRatio(1)
{
	fTrackingInfo.isTracking = false;
	fTrackingInfo.pickedObject = NULL;
	fTrackingInfo.buttons = 0;
	fTrackingInfo.lastX = 0.0f;
	fTrackingInfo.lastY = 0.0f;
	fTrackingInfo.lastDx = 0.0f;
	fTrackingInfo.lastDy = 0.0f;

	fLastObjectDistance = fObjectDistance = depthOfView / 8;
	quittingSem = create_sem(1, "quitting sem");
	drawEvent = create_sem(0, "draw event");

	TriangleObject *Tri = new TriangleObject(this);
	if (Tri->InitCheck() == B_OK) {
		fObjListLock.Lock();
		fObjects.AddItem(Tri);
		fObjListLock.Unlock();
	} else {
		BAlert *NoResourceAlert	= new BAlert(B_TRANSLATE("Error"),
						kNoResourceError, B_TRANSLATE("OK"), NULL, NULL,
						B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_STOP_ALERT);
		NoResourceAlert->Go();
		delete Tri;
	}
}


ObjectView::~ObjectView()
{
	delete_sem(quittingSem);
	delete_sem(drawEvent);
}


void
ObjectView::AttachedToWindow()
{
	float position[] = {0.0, 3.0, 3.0, 0.0};
	float position1[] = {-3.0, -3.0, 3.0, 0.0};
	float position2[] = {3.0, 0.0, 0.0, 0.0};
	float local_view[] = {0.0, 0.0};
//	float ambient[] = {0.1745, 0.03175, 0.03175};
//	float diffuse[] = {0.61424, 0.10136, 0.10136};
//	float specular[] = {0.727811, 0.626959, 0.626959};
//	rgb_color black = {0, 0, 0, 255};
	BRect bounds = Bounds();

	BGLView::AttachedToWindow();
	Window()->SetPulseRate(100000);

	LockGL();

	glEnable(GL_DITHER);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);

	glShadeModel(GL_SMOOTH);

	glLightfv(GL_LIGHT0, GL_POSITION, position);
	glLightfv(GL_LIGHT0 + 1, GL_POSITION, position1);
	glLightfv(GL_LIGHT0 + 2, GL_POSITION, position2);
	glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, local_view);

	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lights[lightWhite].specular);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,lights[lightWhite].diffuse);
	glLightfv(GL_LIGHT0, GL_AMBIENT,lights[lightWhite].ambient);
	glEnable(GL_LIGHT1);
	glLightfv(GL_LIGHT1, GL_SPECULAR, lights[lightBlue].specular);
	glLightfv(GL_LIGHT1, GL_DIFFUSE,lights[lightBlue].diffuse);
	glLightfv(GL_LIGHT1, GL_AMBIENT,lights[lightBlue].ambient);

	glFrontFace(GL_CW);
	glEnable(GL_LIGHTING);
	glEnable(GL_AUTO_NORMAL);
	glEnable(GL_NORMALIZE);

	glMaterialf(GL_FRONT, GL_SHININESS, 0.6 * 128.0);

	glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.0);
	glColor3f(1.0, 1.0, 1.0);

	glViewport(0, 0, (GLint)bounds.IntegerWidth() + 1,
				(GLint)bounds.IntegerHeight() + 1);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	float scale = displayScale;
	glOrtho(-scale, scale, -scale, scale, -scale * depthOfView,
			scale * depthOfView);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	UnlockGL();

	fDrawThread = spawn_thread(simonThread, "Simon", B_NORMAL_PRIORITY, this);
	resume_thread(fDrawThread);
	fForceRedraw = true;
	setEvent(drawEvent);
}


void
ObjectView::DetachedFromWindow()
{
	BGLView::DetachedFromWindow();

	long dummy;
	long locks = 0;

	while (Window()->IsLocked()) {
		locks++;
		Window()->Unlock();
	}

	acquire_sem(quittingSem);
	release_sem(drawEvent);
	wait_for_thread(fDrawThread, &dummy);
	release_sem(quittingSem);

	while (locks--)
		Window()->Lock();
}


void
ObjectView::Pulse()
{
	Window()->Lock();
	BRect parentBounds = Parent()->Bounds();
	BRect bounds = Bounds();
	parentBounds.OffsetTo(0, 0);
	bounds.OffsetTo(0, 0);
	if (bounds != parentBounds) {
		ResizeTo(parentBounds.right - parentBounds.left,
				 parentBounds.bottom - parentBounds.top);
	}
	Window()->Unlock();
}


void
ObjectView::MessageReceived(BMessage* msg)
{
	BMenuItem* item = NULL;
	bool toggleItem = false;

	switch (msg->what) {
		case kMsgFPS:
			fFps = (fFps) ? false : true;
			msg->FindPointer("source", reinterpret_cast<void**>(&item));
			item->SetMarked(fFps);
			fForceRedraw = true;
			setEvent(drawEvent);
			break;
		case kMsgAddModel: 
		{
			TriangleObject *Tri = new TriangleObject(this);
			if (Tri->InitCheck() == B_OK) {
				fObjListLock.Lock();
				fObjects.AddItem(Tri);
				fObjListLock.Unlock();
			} else {
				BAlert *NoResourceAlert	= new BAlert(B_TRANSLATE("Error"),
						kNoResourceError, B_TRANSLATE("OK"), NULL, NULL,
						B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_STOP_ALERT);
				NoResourceAlert->Go();
				delete Tri;
			}
			setEvent(drawEvent);
			break;
		}
		case kMsgLights:
		{
			msg->FindPointer("source", reinterpret_cast<void**>(&item));
			long lightNum = msg->FindInt32("num");
			long color = msg->FindInt32("color");
			BMenu *menu = item->Menu();
			long index = menu->IndexOf(item);
			menu->ItemAt(index)->SetMarked(true);
			for (int i = 0; i < menu->CountItems(); i++) {
				if (i != index)
					menu->ItemAt(i)->SetMarked(false);
			}

			LockGL();
			if (color != lightNone) {
				glEnable(GL_LIGHT0 + lightNum - 1);
				glLightfv(GL_LIGHT0 + lightNum - 1, GL_SPECULAR,
					lights[color].specular);
				glLightfv(GL_LIGHT0 + lightNum - 1, GL_DIFFUSE,
					lights[color].diffuse);
				glLightfv(GL_LIGHT0 + lightNum - 1, GL_AMBIENT,
					lights[color].ambient);
			} else {
				glDisable(GL_LIGHT0 + lightNum - 1);
			}
			UnlockGL();
			fForceRedraw = true;
			setEvent(drawEvent);
			break;
		}
		case kMsgGouraud:
			fGouraud = !fGouraud;
			toggleItem = true;
			break;
		case kMsgZBuffer:
			fZbuf = !fZbuf;
			toggleItem = true;
			break;
		case kMsgCulling:
			fCulling = !fCulling;
			toggleItem = true;
			break;
		case kMsgLighting:
			fLighting = !fLighting;
			toggleItem = true;
			break;
		case kMsgFilled:
			fFilled = !fFilled;
			toggleItem = true;
			break;
		case kMsgPerspective:
			fPersp = !fPersp;
			toggleItem = true;
			break;
		case kMsgFog:
			fFog = !fFog;
			toggleItem = true;
			break;
	}

	if (toggleItem && msg->FindPointer("source", reinterpret_cast<void**>(&item)) == B_OK){
		item->SetMarked(!item->IsMarked());
		setEvent(drawEvent);
	}

	BGLView::MessageReceived(msg);
}


int
ObjectView::ObjectAtPoint(const BPoint &point)
{
	LockGL();
	glShadeModel(GL_FLAT);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glClearColor(black[0], black[1], black[2], 1.0);
	glClear(GL_COLOR_BUFFER_BIT | (fZbuf ? GL_DEPTH_BUFFER_BIT : 0));

	float idColor[3];
	idColor[1] = idColor[2] = 0;
	for (int i = 0; i < fObjects.CountItems(); i++) {
		// to take into account 16 bits colorspaces,
		// only use the 5 highest bits of the red channel
		idColor[0] = (255 - (i << 3)) / 255.0;
		reinterpret_cast<GLObject*>(fObjects.ItemAt(i))->Draw(true, idColor);
	}
	glReadBuffer(GL_BACK);
	uchar pixel[256];
	glReadPixels((GLint)point.x, (GLint)(Bounds().bottom - point.y), 1, 1,
		GL_RGB, GL_UNSIGNED_BYTE, pixel);
	int objNum = pixel[0];
	objNum = (255 - objNum) >> 3;

	EnforceState();
	UnlockGL();

	return objNum;
}


void
ObjectView::MouseDown(BPoint point)
{
	GLObject* object = NULL;

	BMessage *msg = Window()->CurrentMessage();
	uint32 buttons = msg->FindInt32("buttons");
	object = reinterpret_cast<GLObject*>(fObjects.ItemAt(ObjectAtPoint(point)));

	if (object != NULL){
		if (buttons == B_PRIMARY_MOUSE_BUTTON || buttons == B_SECONDARY_MOUSE_BUTTON) {
			fTrackingInfo.pickedObject = object;
			fTrackingInfo.buttons = buttons;
			fTrackingInfo.isTracking = true;
			fTrackingInfo.lastX = point.x;
			fTrackingInfo.lastY = point.y;
			fTrackingInfo.lastDx = 0.0f;
			fTrackingInfo.lastDy = 0.0f;
			fTrackingInfo.pickedObject->Spin(0.0f, 0.0f);


			SetMouseEventMask(B_POINTER_EVENTS,
						B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);

			BCursor grabbingCursor(B_CURSOR_ID_GRABBING);
			SetViewCursor(&grabbingCursor);
		} else {
			ConvertToScreen(&point);
			object->MenuInvoked(point);
		}
	}
}


void
ObjectView::MouseUp(BPoint point)
{
	if (fTrackingInfo.isTracking) {

		//spin the teapot on release, TODO: use a marching sum and divide by time
		if (fTrackingInfo.buttons == B_PRIMARY_MOUSE_BUTTON
			&& fTrackingInfo.pickedObject != NULL
			&& (fabs(fTrackingInfo.lastDx) > 1.0f
				|| fabs(fTrackingInfo.lastDy) > 1.0f) ) {

			fTrackingInfo.pickedObject->Spin(0.5f * fTrackingInfo.lastDy, 0.5f * fTrackingInfo.lastDx);

			setEvent(drawEvent);
		}

		//stop tracking
		fTrackingInfo.isTracking = false;
		fTrackingInfo.buttons = 0;
		fTrackingInfo.pickedObject = NULL;
		fTrackingInfo.lastX = 0.0f;
		fTrackingInfo.lastY = 0.0f;
		fTrackingInfo.lastDx = 0.0f;
		fTrackingInfo.lastDy = 0.0f;

		BCursor grabCursor(B_CURSOR_ID_GRAB);
		SetViewCursor(&grabCursor);
	}
}


void
ObjectView::MouseMoved(BPoint point, uint32 transit, const BMessage *msg)
{
	if (fTrackingInfo.isTracking && fTrackingInfo.pickedObject != NULL) {

		float dx = point.x - fTrackingInfo.lastX;
		float dy = point.y - fTrackingInfo.lastY;
		fTrackingInfo.lastX = point.x;
		fTrackingInfo.lastY = point.y;

		if (fTrackingInfo.buttons == B_PRIMARY_MOUSE_BUTTON) {

			fTrackingInfo.pickedObject->Spin(0.0f, 0.0f);
			fTrackingInfo.pickedObject->RotateWorldSpace(dx,dy);
			fTrackingInfo.lastDx = dx;
			fTrackingInfo.lastDy = dy;

			setEvent(drawEvent);

		} else if (fTrackingInfo.buttons == B_SECONDARY_MOUSE_BUTTON) {

			float xinc = (dx * 2 * displayScale / Bounds().Width());
			float yinc = (-dy * 2 * displayScale / Bounds().Height());
			float zinc = 0;

			if (fPersp) {
				zinc = yinc * (fTrackingInfo.pickedObject->z / displayScale);
				xinc *= -(fTrackingInfo.pickedObject->z * 4 / zRatio);
				yinc *= -(fTrackingInfo.pickedObject->z * 4 / zRatio);
			}

			fTrackingInfo.pickedObject->x += xinc;
			if (modifiers() & B_SHIFT_KEY)
				fTrackingInfo.pickedObject->z += zinc;
			else
	  			fTrackingInfo.pickedObject->y += yinc;

			fForceRedraw = true;
			setEvent(drawEvent);
		}
	} else {
		void* object = fObjects.ItemAt(ObjectAtPoint(point));
		BCursor cursor(object != NULL
			? B_CURSOR_ID_GRAB : B_CURSOR_ID_SYSTEM_DEFAULT);
		SetViewCursor(&cursor);
	}
}


void
ObjectView::FrameResized(float width, float height)
{
	LockGL();

	BGLView::FrameResized(width, height);

	width = Bounds().Width();
	height = Bounds().Height();
	fYxRatio = height / width;
    glViewport(0, 0, (GLint)width + 1, (GLint)height + 1);

	// To prevent weird buffer contents
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float scale = displayScale;

	if (fPersp) {
		gluPerspective(60, 1.0 / fYxRatio, 0.15, 120);
	} else {
		if (fYxRatio < 1) {
			glOrtho(-scale / fYxRatio, scale / fYxRatio, -scale, scale, -1.0,
				depthOfView * 4);
		} else {
			glOrtho(-scale, scale, -scale * fYxRatio, scale * fYxRatio, -1.0,
				depthOfView * 4);
		}
	}

	fLastYXRatio = fYxRatio;

	glMatrixMode(GL_MODELVIEW);

	UnlockGL();

	fForceRedraw = true;
	setEvent(drawEvent);
}


bool
ObjectView::RepositionView()
{
	if (!(fPersp != fLastPersp) &&
		!(fLastObjectDistance != fObjectDistance) &&
		!(fLastYXRatio != fYxRatio)) {
		return false;
	}

	LockGL();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float scale = displayScale;

	if (fPersp) {
		gluPerspective(60, 1.0 / fYxRatio, 0.15, 120);
	} else {
		if (fYxRatio < 1) {
			glOrtho(-scale / fYxRatio, scale / fYxRatio, -scale, scale, -1.0,
				depthOfView * 4);
		} else {
			glOrtho(-scale, scale, -scale * fYxRatio, scale * fYxRatio, -1.0,
				depthOfView * 4);
		}
	}

	glMatrixMode(GL_MODELVIEW);

	UnlockGL();

	fLastObjectDistance = fObjectDistance;
	fLastPersp = fPersp;
	fLastYXRatio = fYxRatio;
	return true;
}


void
ObjectView::EnforceState()
{
	glShadeModel(fGouraud ? GL_SMOOTH : GL_FLAT);

	if (fZbuf)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);

	if (fCulling)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);

	if (fLighting)
		glEnable(GL_LIGHTING);
	else
		glDisable(GL_LIGHTING);

	if (fFilled)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	if (fFog) {
		glFogf(GL_FOG_START, 10.0);
		glFogf(GL_FOG_DENSITY, 0.2);
		glFogf(GL_FOG_END, depthOfView);
		glFogfv(GL_FOG_COLOR, foggy);
		glEnable(GL_FOG);
		bgColor = foggy;
		glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.0);
	} else {
		glDisable(GL_FOG);
		bgColor = black;
		glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.0);
	}
}


bool
ObjectView::SpinIt()
{
	bool changed = false;

	if (fGouraud != fLastGouraud) {
		LockGL();
		glShadeModel(fGouraud ? GL_SMOOTH : GL_FLAT);
		UnlockGL();
		fLastGouraud = fGouraud;
		changed = true;
	}

	if (fZbuf != fLastZbuf) {
		LockGL();
		if (fZbuf)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);
		UnlockGL();
		fLastZbuf = fZbuf;
		changed = true;
	}

	if (fCulling != fLastCulling) {
		LockGL();
		if (fCulling)
			glEnable(GL_CULL_FACE);
		else
			glDisable(GL_CULL_FACE);
		UnlockGL();
		fLastCulling = fCulling;
		changed = true;
	}

	if (fLighting != fLastLighting) {
		LockGL();
		if (fLighting)
			glEnable(GL_LIGHTING);
		else
			glDisable(GL_LIGHTING);
		UnlockGL();
		fLastLighting = fLighting;
		changed = true;
	}

	if (fFilled != fLastFilled) {
		LockGL();
		if (fFilled) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		} else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		UnlockGL();
		fLastFilled = fFilled;
		changed = true;
	}

	if (fFog != fLastFog) {
		if (fFog) {
			glFogf(GL_FOG_START, 1.0);
			glFogf(GL_FOG_DENSITY, 0.2);
			glFogf(GL_FOG_END, depthOfView);
			glFogfv(GL_FOG_COLOR, foggy);
			glEnable(GL_FOG);
			bgColor = foggy;
			glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.0);
		} else {
			glDisable(GL_FOG);
			bgColor = black;
			glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.0);
		}
		fLastFog = fFog;
		changed = true;
	}

	changed = changed || RepositionView();
	changed = changed || fForceRedraw;
	fForceRedraw = false;

	for (int i = 0; i < fObjects.CountItems(); i++) {
		bool hack = reinterpret_cast<GLObject*>(fObjects.ItemAt(i))->SpinIt();
		changed = changed || hack;
	}

	return changed;
}


void
ObjectView::DrawFrame(bool noPause)
{
	LockGL();
	glClear(GL_COLOR_BUFFER_BIT | (fZbuf ? GL_DEPTH_BUFFER_BIT : 0));

	fObjListLock.Lock();
	for (int i = 0; i < fObjects.CountItems(); i++) {
	  GLObject *object = reinterpret_cast<GLObject*>(fObjects.ItemAt(i));
		if (object->Solidity() == 0)
			object->Draw(false, NULL);
	}
	EnforceState();
	for (int i = 0; i < fObjects.CountItems(); i++) {
		GLObject *object = reinterpret_cast<GLObject*>(fObjects.ItemAt(i));
		if (object->Solidity() != 0)
			object->Draw(false, NULL);
	}
	fObjListLock.Unlock();

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);

	if (noPause) {
		uint64 now = system_time();
		float fps = 1.0 / ((now - fLastFrame) / 1000000.0);
		fLastFrame = now;
		int entry;
		if (fHistEntries < HISTSIZE) {
			entry = (fOldestEntry + fHistEntries) % HISTSIZE;
			fHistEntries++;
		} else {
			entry = fOldestEntry;
			fOldestEntry = (fOldestEntry + 1) % HISTSIZE;
		}

		fFpsHistory[entry] = fps;

		if (fHistEntries > 5) {
			fps = 0;
			for (int i = 0; i < fHistEntries; i++)
				fps += fFpsHistory[(fOldestEntry + i) % HISTSIZE];

			fps /= fHistEntries;

			if (fFps) {
				glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT);
				glPushMatrix();
				glLoadIdentity();
				glTranslatef(-0.9, -0.9, 0);
				glScalef(0.10, 0.10, 0.10);
				glDisable(GL_LIGHTING);
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_BLEND);
				glColor3f(1.0, 1.0, 0);
				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();
				glMatrixMode(GL_MODELVIEW);

				FPS::drawCounter(fps);

				glMatrixMode(GL_PROJECTION);
				glPopMatrix();
				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();
				glPopAttrib();
			}
		}
	} else {
		fHistEntries = 0;
		fOldestEntry = 0;
	}
	SwapBuffers();
	UnlockGL();
}
