/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdio.h>
#include <stdlib.h>

#include <InterfaceKit.h>
#include <FindDirectory.h>

#include "ObjectView.h"
#include "ResScroll.h"
#include "GLObject.h"
#include "FPS.h"

#define teapotData "teapot.data"
char teapotPath[PATH_MAX];

float displayScale = 1.0;
float depthOfView = 30.0;
float zRatio = 10.0;

float white[3] = {1.0,1.0,1.0};
float dimWhite[3] = {0.25,0.25,0.25};
float black[3] = {0.0,0.0,0.0};
float foggy[3] = {0.4,0.4,0.4};
float blue[3] = {0.0,0.0,1.0};
float dimBlue[3] = {0.0,0.0,0.5};
float yellow[3] = {1.0,1.0,0.0};
float dimYellow[3] = {0.5,0.5,0.0};
float green[3] = {0.0,1.0,0.0};
float dimGreen[3] = {0.0,0.5,0.0};
float red[3] = {1.0,0.0,0.0};

float *bgColor = black;

struct light {
	float *ambient;
	float *diffuse;
	float *specular;
};

light lights[] = {
	{NULL,NULL,NULL},
	{dimWhite,white,white},
	{dimWhite,yellow,yellow},
	{dimWhite,red,red},
	{dimWhite,blue,blue},
	{dimWhite,green,green}
};

long signalEvent(sem_id event)
{
	long c;
	get_sem_count(event,&c);
	if (c<0)
		release_sem_etc(event,-c,0);
	
	return 0;
};

long setEvent(sem_id event)
{
	long c;
	get_sem_count(event,&c);
	if (c<0)
	  release_sem_etc(event,-c,0);

	return 0;
};


long waitEvent(sem_id event)
{
	acquire_sem(event);

	long c;
	get_sem_count(event,&c);
	if (c>0)
		acquire_sem_etc(event,c,0,0);
	
	return 0;
};

long simonThread(ObjectView *ov)
{
    int noPause=0;
    while (acquire_sem_etc(ov->quittingSem,1,B_TIMEOUT,0) == B_NO_ERROR) {
		if (ov->SpinIt()) {
			ov->DrawFrame(noPause);
			release_sem(ov->quittingSem);
			noPause = 1;
		} else {
			release_sem(ov->quittingSem);
			noPause = 0;
			waitEvent(ov->drawEvent);
		};
	};

	return 0;
};

ObjectView::ObjectView(BRect r, char *name, ulong resizingMode, ulong options)
	: BGLView(r,name,resizingMode,0,options)
{
    histEntries = 0;
	oldestEntry = 0;
    lastGouraud = gouraud = true;
	lastZbuf = zbuf = true;
	lastCulling = culling = true;
	lastLighting = lighting = true;
	lastFilled = filled = true;
	lastPersp = persp = false;
	lastFog = fog = false;
	lastTextured = textured = false;
	lastObjectDistance = objectDistance = depthOfView/8;
	fps = true;
	forceRedraw = false;
	lastYXRatio = yxRatio = 1;
	
	quittingSem = create_sem(1,"quitting sem");
	drawEvent = create_sem(0,"draw event");

	char findDir[PATH_MAX];
	find_directory(B_BEOS_ETC_DIRECTORY,-1, TRUE,findDir,PATH_MAX);
	sprintf(teapotPath,"%s/%s",findDir,teapotData);
	objListLock.Lock();
	objects.AddItem(new TriangleObject(this,teapotPath));
	objListLock.Unlock();
};

ObjectView::~ObjectView()
{
	delete_sem(quittingSem);
	delete_sem(drawEvent);
};

void ObjectView::AttachedToWindow()
{
	float position[] = {0.0, 3.0, 3.0, 0.0};
	float position1[] = {-3.0, -3.0, 3.0, 0.0};
	float position2[] = {3.0, 0.0, 0.0, 0.0};
	float local_view[] = {0.0,0.0};
//	float ambient[] = {0.1745, 0.03175, 0.03175};
//	float diffuse[] = {0.61424, 0.10136, 0.10136};
//	float specular[] = {0.727811, 0.626959, 0.626959};
//	rgb_color black = {0,0,0,255};
	BRect r = Bounds();

	BGLView::AttachedToWindow();
	Window()->SetPulseRate(100000);
	
	LockGL();

	glEnable(GL_DITHER);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);

	glShadeModel(GL_SMOOTH);
	
	glLightfv(GL_LIGHT0, GL_POSITION, position);
	glLightfv(GL_LIGHT0+1, GL_POSITION, position1);
	glLightfv(GL_LIGHT0+2, GL_POSITION, position2);
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
	
	glMaterialf(GL_FRONT, GL_SHININESS, 0.6*128.0);

	glClearColor(bgColor[0],bgColor[1],bgColor[2], 1.0);
	glColor3f(1.0, 1.0, 1.0);
		
	glViewport(0, 0, (GLint)r.IntegerWidth()+1, (GLint)r.IntegerHeight()+1);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float scale=displayScale;
	//    glOrtho (0.0, 16.0, 0, 16.0*(GLfloat)300/(GLfloat)300,
	//			         -10.0, 10.0);
	glOrtho(-scale, scale, -scale, scale, -scale*depthOfView,
			scale*depthOfView);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	UnlockGL();

	drawThread = spawn_thread((thread_entry)simonThread,
		"Simon",B_NORMAL_PRIORITY,(void*)this);
	resume_thread(drawThread);
	forceRedraw = true;
	setEvent(drawEvent);
};

void ObjectView::DetachedFromWindow()
{
	BGLView::DetachedFromWindow();

	long dummy;
	long locks=0;
	
	while (Window()->IsLocked()) {
		locks++;
		Window()->Unlock();
	};
	
	acquire_sem(quittingSem);
	release_sem(drawEvent);
	wait_for_thread(drawThread,&dummy);
	release_sem(quittingSem);

	while (locks--) Window()->Lock();
};

void ObjectView::Pulse()
{
  Window()->Lock();
  BRect p = Parent()->Bounds();
  BRect b = Bounds();
  p.OffsetTo(0,0);
  b.OffsetTo(0,0);
  if (b != p) {
	ResizeTo(p.right-p.left,p.bottom-p.top);
  };
  Window()->Unlock();
};

void ObjectView::MessageReceived(BMessage *msg)
{
	BMenuItem *i;
	bool *b;

	switch (msg->what) {
   	    case bmsgFPS:
		    fps = (fps)?false:true;
			msg->FindPointer("source", (void **)&i);
			i->SetMarked(fps);
			forceRedraw = true;
			setEvent(drawEvent);
			break;
	    case bmsgAddModel:
		    objListLock.Lock();
			objects.AddItem(new TriangleObject(this,teapotPath));
			objListLock.Unlock();
			setEvent(drawEvent);
			break;
		case bmsgLights:
		{
			msg->FindPointer("source",(void **)&i);
			long lightNum = msg->FindInt32("num");
			long color = msg->FindInt32("color");
			BMenu *m = i->Menu();
			long index = m->IndexOf(i);
			m->ItemAt(index)->SetMarked(true);
			for (int i=0;i<m->CountItems();i++) {
				if (i != index)
					m->ItemAt(i)->SetMarked(false);
			};
			
			LockGL();
			if (color != lightNone) {
				glEnable(GL_LIGHT0+lightNum-1);
				glLightfv(GL_LIGHT0+lightNum-1, GL_SPECULAR, lights[color].specular);
				glLightfv(GL_LIGHT0+lightNum-1, GL_DIFFUSE,lights[color].diffuse);
				glLightfv(GL_LIGHT0+lightNum-1, GL_AMBIENT,lights[color].ambient);
			} else {
				glDisable(GL_LIGHT0+lightNum-1);
			};
			UnlockGL();
			forceRedraw = true;
			setEvent(drawEvent);
			break;
		}
		case bmsgGouraud:
			b = &gouraud;
			goto stateChange;
		case bmsgZBuffer:
			b = &zbuf;
			goto stateChange;
		case bmsgCulling:
			b = &culling;
			goto stateChange;
		case bmsgLighting:
			b = &lighting;
			goto stateChange;
		case bmsgFilled:
			b = &filled;
			goto stateChange;
		case bmsgPerspective:
			b = &persp;
			goto stateChange;
		case bmsgFog:
			b = &fog;
			goto stateChange;
		stateChange:
			msg->FindPointer("source",(void **)&i);
			if (!i) return;

			if (*b) {
				i->SetMarked(*b = false);
			} else {
				i->SetMarked(*b = true);
			};
			setEvent(drawEvent);
			break;
		default:
			BGLView::MessageReceived(msg);
	};		
};

int ObjectView::ObjectAtPoint(BPoint p)
{
	LockGL();
	glShadeModel(GL_FLAT);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glClearColor(black[0],black[1],black[2],1.0);
	glClear(GL_COLOR_BUFFER_BIT|(zbuf?GL_DEPTH_BUFFER_BIT:0));

	float f[3];
	f[1] = f[2] = 0;
	for (int i=0;i<objects.CountItems();i++) {
		f[0] = (255-i)/255.0;
		((GLObject*)objects.ItemAt(i))->Draw(true, f);
	};
	glReadBuffer(GL_BACK);
	uchar pixel[256];
	glReadPixels((GLint)p.x,(GLint)(Bounds().bottom-p.y),1,1,GL_RGB,GL_UNSIGNED_BYTE,pixel);
	int objNum = pixel[0];
	objNum = 255-objNum;
		
	EnforceState();
	UnlockGL();

	return objNum;
};

void ObjectView::MouseDown(BPoint p)
{
	BPoint op=p,np=p;
	BRect bounds = Bounds();
	float lastDx=0,lastDy=0;
	GLObject *o = NULL;
	uint32 mods;

	BMessage *m = Window()->CurrentMessage();
	uint32 buttons = m->FindInt32("buttons");
	o = ((GLObject*)objects.ItemAt(ObjectAtPoint(p)));

	long locks=0;
	
	while (Window()->IsLocked()) {
		locks++;
		Window()->Unlock();
	};

	if (buttons == B_SECONDARY_MOUSE_BUTTON) {
	  if (o) {
		while (buttons) {
		  lastDx = np.x-op.x;
		  lastDy = np.y-op.y;
		  if (lastDx || lastDy) {
			float xinc = (lastDx*2*displayScale/bounds.Width());
			float yinc = (-lastDy*2*displayScale/bounds.Height());
			float zinc = 0;
			if (persp) {
			  zinc = yinc*(o->z/(displayScale));
			  xinc *= -(o->z*4/zRatio);
			  yinc *= -(o->z*4/zRatio);
			};
			o->x += xinc;
			mods = modifiers();
			if (mods & B_SHIFT_KEY)
			  o->z += zinc;
			else
			  o->y += yinc;
			op = np;
			forceRedraw = true;
			setEvent(drawEvent);
		  };
		  
		  snooze(25000);
		  
		  Window()->Lock();
		  GetMouse(&np, &buttons, true);
		  Window()->Unlock();
		};
	  };
	} else if (buttons == B_PRIMARY_MOUSE_BUTTON) {
	  float llx=0,lly=0;
	  lastDx = lastDy = 0;
	  if (o) {
		o->spinX = 0;
		o->spinY = 0;
		while (buttons) {
		  llx = lastDx;
		  lly = lastDy;
		  lastDx = np.x-op.x;
		  lastDy = np.y-op.y;
		  if (lastDx || lastDy) {
			o->rotY += lastDx;
			o->rotX += lastDy;
			op = np;
			setEvent(drawEvent);
		  };
		  
		  snooze(25000);
		  
		  Window()->Lock();
		  GetMouse(&np, &buttons, true);
		  Window()->Unlock();
		};
		o->spinY = lastDx+llx;
		o->spinX = lastDy+lly;
	  };
	} else {
		if (o) {
			BPoint point = p;
			Window()->Lock();
			ConvertToScreen(&point);
			Window()->Unlock();
			o->MenuInvoked(point);
		};
	};

	while (locks--)
		Window()->Lock();
	
	setEvent(drawEvent);
};

void ObjectView::FrameResized(float w, float h)
{
	LockGL();

	BGLView::FrameResized(w,h);

	BRect b = Bounds();
	w = b.Width();
	h = b.Height();
	yxRatio = h/w;
    glViewport(0, 0, (GLint)w+1, (GLint)h+1);

	// To prevent weird buffer contents
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float scale=displayScale;
	if (persp) {
	  gluPerspective(60,1.0/yxRatio,0.15,120);
	} else {
	  if (yxRatio < 1) {
		glOrtho(-scale/yxRatio, scale/yxRatio, -scale, scale,
				-1.0, depthOfView*4);
	  } else {
		glOrtho(-scale, scale, -scale*yxRatio, scale*yxRatio,
				-1.0, depthOfView*4);
	  };
	};

	lastYXRatio = yxRatio;
	
	glMatrixMode(GL_MODELVIEW);
	
	UnlockGL();
	
	forceRedraw = true;
	setEvent(drawEvent);
}

bool ObjectView::RepositionView()
{
	if (!(persp != lastPersp) &&
		!(lastObjectDistance != objectDistance) &&
		!(lastYXRatio != yxRatio)) {
		return false;
	};

	LockGL();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float scale=displayScale;
	if (persp) {
	  gluPerspective(60,1.0/yxRatio,0.15,120);
	} else {
	  if (yxRatio < 1) {
		glOrtho(-scale/yxRatio, scale/yxRatio, -scale, scale,
				-1.0, depthOfView*4);
	  } else {
		glOrtho(-scale, scale, -scale*yxRatio, scale*yxRatio,
				-1.0, depthOfView*4);
	  };
	};

	glMatrixMode(GL_MODELVIEW);

	UnlockGL();
	
	lastObjectDistance = objectDistance;
	lastPersp = persp;
	lastYXRatio = yxRatio;
	return true;
};

void ObjectView::EnforceState()
{
	glShadeModel(gouraud?GL_SMOOTH:GL_FLAT);
	if (zbuf) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
	if (culling) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
	if (lighting) glEnable(GL_LIGHTING); else glDisable(GL_LIGHTING);
	if (filled) {
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	};
	if (fog) {
		glFogf(GL_FOG_START,10.0);
		glFogf(GL_FOG_DENSITY,0.2);
		glFogf(GL_FOG_END,depthOfView);
		glFogfv(GL_FOG_COLOR,foggy);
		glEnable(GL_FOG);
		bgColor = foggy;
		glClearColor(bgColor[0],bgColor[1],bgColor[2], 1.0);
	} else {
		glDisable(GL_FOG);
		bgColor = black;
		glClearColor(bgColor[0],bgColor[1],bgColor[2], 1.0);
	};
};

bool ObjectView::SpinIt()
{
	bool changed = false;

	if (gouraud != lastGouraud) {
		LockGL();
		glShadeModel(gouraud?GL_SMOOTH:GL_FLAT);
		UnlockGL();
		lastGouraud = gouraud;
		changed = true;
	};
	if (zbuf != lastZbuf) {
		LockGL();
		if (zbuf) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
		UnlockGL();
		lastZbuf = zbuf;
		changed = true;
	};
	if (culling != lastCulling) {
		LockGL();
		if (culling) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
		UnlockGL();
		lastCulling = culling;
		changed = true;
	};
	if (lighting != lastLighting) {
		LockGL();
		if (lighting) glEnable(GL_LIGHTING); else glDisable(GL_LIGHTING);
		UnlockGL();
		lastLighting = lighting;
		changed = true;
	};
	if (filled != lastFilled) {
		LockGL();
		if (filled) {
			glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
		} else {
			glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
		};
		UnlockGL();
		lastFilled = filled;
		changed = true;
	};
	if (fog != lastFog) {
		if (fog) {
			glFogf(GL_FOG_START,1.0);
			glFogf(GL_FOG_DENSITY,0.2);
			glFogf(GL_FOG_END,depthOfView);
			glFogfv(GL_FOG_COLOR,foggy);
			glEnable(GL_FOG);
			bgColor = foggy;
			glClearColor(bgColor[0],bgColor[1],bgColor[2], 1.0);
		} else {
			glDisable(GL_FOG);
			bgColor = black;
			glClearColor(bgColor[0],bgColor[1],bgColor[2], 1.0);
		};
		lastFog = fog;
		changed = true;
	};
	
	changed = changed || RepositionView();
	changed = changed || forceRedraw;
	forceRedraw = false;

	for (int i=0;i<objects.CountItems();i++) {
		bool hack = ((GLObject*)objects.ItemAt(i))->SpinIt();
		changed = changed || hack;
	};	

	return changed;
};

void ObjectView::DrawFrame(bool noPause)
{
	LockGL();
	glClear(GL_COLOR_BUFFER_BIT|(zbuf?GL_DEPTH_BUFFER_BIT:0));

	objListLock.Lock();
	for (int i=0;i<objects.CountItems();i++) {
	  GLObject *o = ((GLObject*)objects.ItemAt(i));
		if (o->solidity==0)
			o->Draw(false, NULL);
	};
	EnforceState();
	for (int i=0;i<objects.CountItems();i++) {
		GLObject *o = ((GLObject*)objects.ItemAt(i));
		if (o->solidity!=0)
			o->Draw(false, NULL);
	};
	objListLock.Unlock();

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);

	if (noPause) {
	  uint64 now = system_time();
	  float f = 1.0/((now - lastFrame)/1000000.0);
	  lastFrame = now;
	  int e;
	  if (histEntries < HISTSIZE) {
		e = (oldestEntry + histEntries)%HISTSIZE;
		histEntries++;
	  } else {
		e = oldestEntry;
		oldestEntry = (oldestEntry+1)%HISTSIZE;
	  };

	  fpsHistory[e] = f;
	  
	  if (histEntries > 5) {
		  f = 0;
		  for (int i=0;i<histEntries;i++)
			f += fpsHistory[(oldestEntry+i)%HISTSIZE];
		  f = f/histEntries;
		  if (fps) {

			glPushAttrib( GL_ENABLE_BIT | GL_LIGHTING_BIT );			
			glPushMatrix();
			glLoadIdentity();
			glTranslatef( -0.9, -0.9, 0 );
			glScalef( 0.10, 0.10, 0.10 );
			glDisable( GL_LIGHTING );
			glDisable( GL_DEPTH_TEST );
			glDisable( GL_BLEND );
			glColor3f( 1.0, 1.0, 0 );
			
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadIdentity();
			glMatrixMode( GL_MODELVIEW );

			FPS::drawCounter( f );

			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode( GL_MODELVIEW );
			glPopMatrix();
			glPopAttrib();
		  };
	  };
	} else {
	  histEntries = 0;
	  oldestEntry = 0;
	};
	SwapBuffers();
	UnlockGL();
};
