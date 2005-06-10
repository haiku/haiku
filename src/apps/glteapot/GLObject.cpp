/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <stdlib.h>
#include <GL/gl.h>

#include <InterfaceKit.h>

#include "GLObject.h"
#include "glob.h"

struct material {
	float ambient[3],diffuse[3],specular[3];
};

float *colors[] =
{
	NULL,white,yellow,blue,red,green
};

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

extern long 	setEvent(sem_id event);

GLObject::GLObject(ObjectView *ov)
{
	rotX = rotY = lastRotX = lastRotY = 0;
	spinX = spinY = 2;
	x = y = 0;
	z = -2.0;
	color = 4;
	solidity = 0;
	changed = false;
	objView = ov;
};

GLObject::~GLObject()
{
};

void GLObject::MenuInvoked(BPoint point)
{
	BPopUpMenu *m = new BPopUpMenu("Object",false,false);
	BMenuItem *i;

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
	};
	changed = true;
	setEvent(objView->drawEvent);
};

bool GLObject::SpinIt()
{
	rotX += spinX;
	rotY += spinY;
	bool c = changed;
	c = c || ((rotX != lastRotX) || (rotY != lastRotY));
	lastRotX = rotX;
	lastRotY = rotY;
	
	return c;
};

void GLObject::Draw(bool forID, float IDcolor[])
{
	glPushMatrix();
		glTranslatef(x, y, z);
		glRotatef(rotY, 0.0,1.0,0.0);
		glRotatef(rotX, 1.0,0.0,0.0);

		if (forID) {
			glColor3fv(IDcolor);
		};
		
		DoDrawing(forID);

	glPopMatrix();

	changed = false;
};

TriangleObject::TriangleObject(ObjectView *ov, char *filename)
	: 	GLObject(ov), 
		points(100,100),
		triangles(100,100),
		qs(50,50)
{
	float maxp=0;
	int numPt,numTri;
	
	FILE *f = fopen(filename,"r");
	fscanf(f,"%d",&numPt);
//	printf("Points: %d\n",numPt);
	for (int i=0;i<numPt;i++) {
		point p;
		fscanf(f,"%f %f %f %f %f %f",
			&p.x,
			&p.y,
			&p.z,
			&p.nx,
			&p.ny,
			&p.nz);
		if (fabs(p.x) > maxp)
			maxp = fabs(p.x);
		if (fabs(p.y) > maxp)
			maxp = fabs(p.y);
		if (fabs(p.z) > maxp)
			maxp = fabs(p.z);
		points.add(p);
	};
	for (int i=0;i<points.num_items;i++) {
		points[i].x /= maxp;
		points[i].y /= maxp;
		points[i].z /= maxp;
	};
		
	fscanf(f,"%d",&numTri);
//	printf("Triangles: %d\n",numTri);
	int tpts=0;
	for (int i=0;i<numTri;i++) {
		tri t;
		fscanf(f,"%d %d %d",
			&t.p1,
			&t.p2,
			&t.p3);
		triangles.add(t);
		tpts+=3;
	};

	int qpts=4;
	int qp[1024];
	quadStrip q;
	q.pts = qp;
	q.numpts = 4;
	q.pts[2] = triangles[0].p1;
	q.pts[0] = triangles[0].p2;
	q.pts[1] = triangles[0].p3;
	q.pts[3] = triangles[1].p3;

	for (int i=2;i<numTri;i+=2) {
		if ((triangles[i-1].p1 == triangles[i].p2) &&
			(triangles[i-1].p3 == triangles[i].p3)) {
			q.pts[q.numpts++] = triangles[i+1].p1;
			q.pts[q.numpts++] = triangles[i+1].p3;
			qpts+=2;
		} else {
			int *np = (int*)malloc(sizeof(int)*q.numpts);
			memcpy(np,qp,q.numpts*sizeof(int));
			quadStrip nqs;
			nqs.numpts = q.numpts;
			nqs.pts = np;
			qs.add(nqs);
		
			qpts+=4;
			q.numpts = 4;
			q.pts[2] = triangles[i].p1;
			q.pts[0] = triangles[i].p2;
			q.pts[1] = triangles[i].p3;
			q.pts[3] = triangles[i+1].p3;
		};
	};

	int *np = (int*)malloc(sizeof(int)*q.numpts);
	memcpy(np,qp,q.numpts*sizeof(int));
	quadStrip nqs;
	nqs.numpts = q.numpts;
	nqs.pts = np;
	qs.add(nqs);

	fclose(f);
};

TriangleObject::~TriangleObject()
{
	for (int i=0;i<qs.num_items;i++) {
		free(qs[i].pts);
	};
};

void TriangleObject::DoDrawing(bool forID)
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
		};
		glMaterialfv(GL_FRONT, GL_AMBIENT, c[0]);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, c[1]);
		glMaterialfv(GL_FRONT, GL_SPECULAR, c[2]);
	} else {
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	};

#if USE_QUAD_STRIPS
		for (int i=0;i<qs.num_items;i++) {
			glBegin(GL_QUAD_STRIP);
			for (int j=0;j<qs[i].numpts;j++) {
				glNormal3f(
					points[qs[i].pts[j]].nx,
					points[qs[i].pts[j]].ny,
					points[qs[i].pts[j]].nz
				);
				glVertex3f(
					points[qs[i].pts[j]].x,
					points[qs[i].pts[j]].y,
					points[qs[i].pts[j]].z
				);
			};
			glEnd();
		};
#else
		glBegin(GL_TRIANGLES);
		for (int i=0;i<triangles.num_items;i++) {
			int v3 = triangles[i].p1;
			int v1 = triangles[i].p2;
			int v2 = triangles[i].p3;
	  		glNormal3f(
				points[v1].nx,
				points[v1].ny,
				points[v1].nz
			);
			glVertex3f(
				points[v1].x,
				points[v1].y,
				points[v1].z
			);
			glNormal3f(
				points[v2].nx,
				points[v2].ny,
				points[v2].nz
			);
			glVertex3f(
				points[v2].x,
				points[v2].y,
				points[v2].z
			);
			glNormal3f(
				points[v3].nx,
				points[v3].ny,
				points[v3].nz
			);
			glVertex3f(
				points[v3].x,
				points[v3].y,
				points[v3].z
			);
		};
		glEnd();
#endif
};

