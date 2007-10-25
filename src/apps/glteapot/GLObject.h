/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef GL_OBJECT_H
#define GL_OBJECT_H

#include "ObjectView.h"
#include "util.h"

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


class GLObject {
	public:
						GLObject(ObjectView* ov);
		virtual			~GLObject();
		virtual void	Draw(bool forID, float IDcolor[]);
		virtual bool	SpinIt();
		virtual void	MenuInvoked(BPoint point);
		virtual void	DoDrawing(bool forID) {};

		float			rotX, rotY, spinX, spinY;
		float			x, y, z;
		int				solidity;

	protected:
		float			lastRotX, lastRotY;
		int				color;
		bool			changed;

	private:
		ObjectView*		fObjView;
};

class TriangleObject : public GLObject {
	public:
							TriangleObject(ObjectView* ov, char* filename);
		virtual				~TriangleObject();		
		virtual void		DoDrawing(bool forID);

	private:
		BufferArray<point>		fPoints;
		BufferArray<tri>		fTriangles;
		BufferArray<quadStrip>	fQs;
};

#endif // GL_OBJECT_H
