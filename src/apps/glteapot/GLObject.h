/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "ObjectView.h"

struct point {
	float x,y,z;
	float nx,ny,nz;
	float tu,tv;
};

struct tri {
	int p1,p2,p3;
};

struct quadStrip {
	int numpts;
	int *pts;
};

#include "util.h"

class GLObject {
public:
float 				rotX,rotY,spinX,spinY,lastRotX,lastRotY;
float				x,y,z;
int					color;
int					solidity;
bool				changed;
ObjectView *		objView;

					GLObject(ObjectView *ov);
virtual				~GLObject();

virtual void		Draw(bool forID, float IDcolor[]);
virtual bool		SpinIt();
virtual void		MenuInvoked(BPoint point);

virtual void		DoDrawing(bool forID) {};
};

class TriangleObject : public GLObject {
public:
BufferArray<point>		points;
BufferArray<tri>		triangles;
BufferArray<quadStrip>	qs;

					TriangleObject(ObjectView *ov, char *filename);
virtual				~TriangleObject();

virtual void		DoDrawing(bool forID);
};
