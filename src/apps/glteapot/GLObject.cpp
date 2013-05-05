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


#include "GLObject.h"

#include <Application.h>
#include <GL/gl.h>
#include <InterfaceKit.h>
#include <Resources.h>

#include "glob.h"


struct material {
	float ambient[3], diffuse[3], specular[3];
};

float *colors[] = {NULL, white, yellow, blue, red, green};

material materials[] = {
	// Null
	{
		{0.1745, 0.03175, 0.03175},
		{0.61424, 0.10136, 0.10136},
		{0.727811, 0.626959, 0.626959}
	},
	// White
	{
		{0.1745, 0.1745, 0.1745},
		{0.61424, 0.61424, 0.61424},
		{0.727811, 0.727811, 0.727811}
	},
	// Yellow
	{
		{0.1745, 0.1745, 0.03175},
		{0.61424, 0.61424, 0.10136},
		{0.727811, 0.727811, 0.626959}
	},
	// Blue
	{
		{0.03175, 0.03175, 0.1745},
		{0.10136, 0.10136, 0.61424},
		{0.626959, 0.626959, 0.727811}
	},
	// Red
	{
		{0.1745, 0.03175, 0.03175},
		{0.61424, 0.10136, 0.10136},
		{0.727811, 0.626959, 0.626959}
	},
	// Green
	{
		{0.03175, 0.1745, 0.03175},
		{0.10136, 0.61424, 0.10136},
		{0.626959, 0.727811, 0.626959}
	},
};

#define USE_QUAD_STRIPS 1

extern long setEvent(sem_id event);


GLObject::GLObject(ObjectView* ov)
	:
	x(0),
	y(0),
	z(-2.0),
	fRotation(0.0f, 0.0f, 0.0f, 1.0f),
	spinX(2),
	spinY(2),	
	solidity(0),
	color(4),	
	changed(false),
	fObjView(ov)
{
}


GLObject::~GLObject()
{
}


void
GLObject::MenuInvoked(BPoint point)
{
	BPopUpMenu* m = new BPopUpMenu("Object",false,false);
	BMenuItem* i;

	int c = 1;
	m->AddItem(i = new BMenuItem("White",NULL));
	if (color == c++)
		i->SetMarked(true);
	m->AddItem(i = new BMenuItem("Yellow",NULL));
	if (color == c++)
		i->SetMarked(true);
	m->AddItem(i = new BMenuItem("Blue",NULL));
	if (color == c++)
		i->SetMarked(true);
	m->AddItem(i = new BMenuItem("Red",NULL));
	if (color == c++)
		i->SetMarked(true);
	m->AddItem(i = new BMenuItem("Green",NULL));
	if (color == c++)
		i->SetMarked(true);
	m->AddSeparatorItem();

	c = 0;
	m->AddItem(i = new BMenuItem("Solid",NULL));
	if (solidity == c++)
		i->SetMarked(true);
	m->AddItem(i = new BMenuItem("Translucent",NULL));
	if (solidity == c++)
		i->SetMarked(true);
	m->AddItem(i = new BMenuItem("Transparent",NULL));
	if (solidity == c++)
		i->SetMarked(true);

	i = m->Go(point);
	int32 index = m->IndexOf(i);
	delete m;
	
	if (index < 5) {
		color = index+1;
	} else if (index > 5) {
		solidity = index-6;
	}
	changed = true;
	setEvent(fObjView->drawEvent);
}


int
GLObject::Solidity() const
{
	return solidity;
}



bool
GLObject::SpinIt()
{
	bool c = changed;
	c = c || ((spinX != 0.0f) || (spinY != 0.0f));
	
	if (c)
		RotateWorldSpace(spinY, spinX);
	
	return c;
}


void
GLObject::Spin(float rx, float ry)
{
	spinX = rx;
	spinY = ry;	
}


void
GLObject::RotateWorldSpace(float rx, float ry)
{
	fRotation = Quaternion(Vector3(0.0f, 1.0f, 0.0f), 0.01f * rx) * fRotation;
	fRotation = Quaternion(Vector3(1.0f, 0.0f, 0.0f), 0.01f * ry) * fRotation;	
	changed = true;
}


void
GLObject::Draw(bool forID, float IDcolor[])
{
	glPushMatrix();
		glTranslatef(x, y, z);
				
		float mat[4][4];
		fRotation.toOpenGLMatrix(mat);
		glMultMatrixf((GLfloat*)mat);
		
		if (forID) {
			glColor3fv(IDcolor);
		}
		
		DoDrawing(forID);

	glPopMatrix();

	changed = false;
}


TriangleObject::TriangleObject(ObjectView* ov)
		: 	
		GLObject(ov),
		fStatus(B_NO_INIT),
		fPoints(100, 100),
		fTriangles(100, 100),
		fQs(50, 50)
{
	BResources *res = BApplication::AppResources();
	if (res == NULL)
		return;

	size_t size = 0;
	int32 *arrayOfPoints
					= (int32*)res->LoadResource(B_RAW_TYPE, "points", &size);
	if (arrayOfPoints == NULL)
		return;

	float maxp = 0;
	size_t numPt = size / sizeof(int32);
	for (size_t i = 0; i < numPt; i += 6) {
		point p;
		p.x = 1e-6 * arrayOfPoints[i];
		p.y = 1e-6 * arrayOfPoints[i + 1];
		p.z = 1e-6 * arrayOfPoints[i + 2];
		p.nx = 1e-6 * arrayOfPoints[i + 3];
		p.ny = 1e-6 * arrayOfPoints[i + 4];
		p.nz = 1e-6 * arrayOfPoints[i + 5];

		if (fabs(p.x) > maxp)
			maxp = fabs(p.x);
		if (fabs(p.y) > maxp)
			maxp = fabs(p.y);
		if (fabs(p.z) > maxp)
			maxp = fabs(p.z);

		fPoints.add(p);
	}

	for (int i = 0; i < fPoints.num_items; i++) {
		fPoints[i].x /= maxp;
		fPoints[i].y /= maxp;
		fPoints[i].z /= maxp;
	}

	int32 *arrayOfTriangles
					= (int32*)res->LoadResource(B_RAW_TYPE, "triangles", &size);
	if (arrayOfTriangles == NULL)
		return;

	size_t numTriPoints = size / sizeof(int32);
	for (size_t i = 0; i < numTriPoints; i += 3) {
		tri t;
		t.p1 = arrayOfTriangles[i];
		t.p2 = arrayOfTriangles[i + 1];
		t.p3 = arrayOfTriangles[i + 2];
		fTriangles.add(t);
	}

	size_t numTri = numTriPoints / 3;

	int qpts = 4;
	int qp[1024];
	quadStrip q;
	q.pts = qp;
	q.numpts = 4;
	q.pts[2] = fTriangles[0].p1;
	q.pts[0] = fTriangles[0].p2;
	q.pts[1] = fTriangles[0].p3;
	q.pts[3] = fTriangles[1].p3;

	for (size_t i = 2; i < numTri; i += 2) {
		if ((fTriangles[i - 1].p1 == fTriangles[i].p2) &&
			(fTriangles[i - 1].p3 == fTriangles[i].p3)) {
			q.pts[q.numpts++] = fTriangles[i + 1].p1;
			q.pts[q.numpts++] = fTriangles[i + 1].p3;
			qpts+=2;
		} else {
			int *np = (int*)malloc(sizeof(int)*q.numpts);
			memcpy(np, qp, q.numpts * sizeof(int));
			quadStrip nqs;
			nqs.numpts = q.numpts;
			nqs.pts = np;
			fQs.add(nqs);

			qpts += 4;
			q.numpts = 4;
			q.pts[2] = fTriangles[i].p1;
			q.pts[0] = fTriangles[i].p2;
			q.pts[1] = fTriangles[i].p3;
			q.pts[3] = fTriangles[i + 1].p3;
		}
	}

	int* np = (int*)malloc(sizeof(int)*q.numpts);
	memcpy(np, qp, q.numpts * sizeof(int));
	quadStrip nqs;
	nqs.numpts = q.numpts;
	nqs.pts = np;
	fQs.add(nqs);

	fStatus = B_OK;
}


TriangleObject::~TriangleObject()
{
	for (int i = 0; i < fQs.num_items; i++) {
		free(fQs[i].pts);
	}
}


status_t
TriangleObject::InitCheck() const
{
	return fStatus;
}


void
TriangleObject::DoDrawing(bool forID)
{
	if (!forID) {
		float c[3][4];
		c[0][0] = materials[color].ambient[0];
		c[0][1] = materials[color].ambient[1];
		c[0][2] = materials[color].ambient[2];
		c[1][0] = materials[color].diffuse[0];
		c[1][1] = materials[color].diffuse[1];
		c[1][2] = materials[color].diffuse[2];
		c[2][0] = materials[color].specular[0];
		c[2][1] = materials[color].specular[1];
		c[2][2] = materials[color].specular[2];

		float alpha = 1;
		if (solidity == 0)
			alpha = 1.0;
		else if (solidity == 1)
			alpha = 0.95;
		else if (solidity == 2)
			alpha = 0.6;
		c[0][3] = c[1][3] = c[2][3] = alpha;
		if (solidity != 0) {
			glBlendFunc(GL_SRC_ALPHA,GL_ONE);
			glEnable(GL_BLEND);
			glDepthMask(GL_FALSE);
			glDisable(GL_CULL_FACE);
		} else {
			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);
		}
		glMaterialfv(GL_FRONT, GL_AMBIENT, c[0]);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, c[1]);
		glMaterialfv(GL_FRONT, GL_SPECULAR, c[2]);
	} else {
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}

#if USE_QUAD_STRIPS
		for (int i = 0; i < fQs.num_items; i++) {
 			glBegin(GL_QUAD_STRIP);
			for (int j = 0; j < fQs[i].numpts; j++) {
 				glNormal3f(
					fPoints[fQs[i].pts[j]].nx,
					fPoints[fQs[i].pts[j]].ny,
					fPoints[fQs[i].pts[j]].nz
				);
 				glVertex3f(
					fPoints[fQs[i].pts[j]].x,
					fPoints[fQs[i].pts[j]].y,
					fPoints[fQs[i].pts[j]].z
				);
			}
			glEnd();
		}
#else
 		glBegin(GL_TRIANGLES);
		for (int i = 0; i < fTriangles.num_items; i++) {
			int v3 = fTriangles[i].p1;
			int v1 = fTriangles[i].p2;
			int v2 = fTriangles[i].p3;
 	  		glNormal3f(
				fPoints[v1].nx,
				fPoints[v1].ny,
				fPoints[v1].nz
			);
 			glVertex3f(
				fPoints[v1].x,
				fPoints[v1].y,
				fPoints[v1].z
			);
 			glNormal3f(
				fPoints[v2].nx,
				fPoints[v2].ny,
				fPoints[v2].nz
			);
 			glVertex3f(
				fPoints[v2].x,
				fPoints[v2].y,
				fPoints[v2].z
			);
 			glNormal3f(
				fPoints[v3].nx,
				fPoints[v3].ny,
				fPoints[v3].nz
			);
			glVertex3f(
				fPoints[v3].x,
				fPoints[v3].y,
				fPoints[v3].z
			);
		}
		glEnd();
 #endif
}

