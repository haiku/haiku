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

#ifndef GL_OBJECT_H
#define GL_OBJECT_H

#include "ObjectView.h"
#include "util.h"
#include "Quaternion.h"

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
		void			Spin(float rx, float ry);
		void 			RotateWorldSpace(float rx, float ry);		
		virtual void	MenuInvoked(BPoint point);
		virtual void	DoDrawing(bool forID) {};
		int				Solidity() const;

		float			x, y, z;
		Quaternion		fRotation;
		
	protected:
		
		float			spinX, spinY;
		int				solidity;
		int				color;
		bool			changed;

	private:
		ObjectView*		fObjView;
};

class TriangleObject : public GLObject {
	public:
							TriangleObject(ObjectView* ov);
		virtual				~TriangleObject();		
		status_t			InitCheck() const;	
		virtual void		DoDrawing(bool forID);
		
	private:
		status_t				fStatus;
		BufferArray<point>		fPoints;
		BufferArray<tri>		fTriangles;
		BufferArray<quadStrip>	fQs;
};

#endif // GL_OBJECT_H
