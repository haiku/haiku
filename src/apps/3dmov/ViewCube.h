/*	PROJECT:		3Dmov
	AUTHORS:		Zenja Solaja
	COPYRIGHT:		2009 Haiku Inc
	DESCRIPTION:	Haiku version of the famous BeInc demo 3Dmov
					Just drag'n'drop media files to the 3D objects
*/

#ifndef _VIEW_CUBE_H_
#define _VIEW_CUBE_H_

#ifndef _VIEW_OBJECT_H_
#include "ViewObject.h"
#endif

class Cube;

/**********************************
	ViewCube displays a rotating box.
***********************************/
class ViewCube : public ViewObject
{
public:
			ViewCube(BRect frame);
			~ViewCube();
	
	void	AttachedToWindow();
	void	MouseDown(BPoint);
	void	MouseMoved(BPoint p, uint32 transit, const BMessage *message);
	void	MouseUp(BPoint);
	void	Render();
		 
protected:
	bool	SurfaceUpdate(MediaSource *source, float mouse_x, float mouse_y);
	
private:
	enum
	{
		NUMBER_FACES = 6,
	};
	bigtime_t		fStartTime;
	MediaSource		*fMediaSources[NUMBER_FACES];
	Cube			*fCube;
	float			fCubeAngle;
	float			fSpeed;
	BPoint			fMousePosition;
	bool			fMouseTracking;	
};

#endif	//#ifndef _VIEW_CUBE_H_
