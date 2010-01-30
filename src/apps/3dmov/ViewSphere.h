/*	PROJECT:		3Dmov
	AUTHORS:		Zenja Solaja
	COPYRIGHT:		2009 Haiku Inc
	DESCRIPTION:	Haiku version of the famous BeInc demo 3Dmov
					Just drag'n'drop media files to the 3D objects
*/

#ifndef _VIEW_SPHERE_H_
#define _VIEW_SPHERE_H_

#ifndef _VIEW_OBJECT_H_
#include "ViewObject.h"
#endif

class Sphere;

/**********************************
	ViewSphere displays a rotating sphere.
***********************************/
class ViewSphere : public ViewObject
{
public:
			ViewSphere(BRect frame);
			~ViewSphere();
	
	void	AttachedToWindow();
	void	MouseDown(BPoint);
	void	MouseMoved(BPoint p, uint32 transit, const BMessage *message);
	void	MouseUp(BPoint);
	void	Render();
	
protected:
	bool	SurfaceUpdate(MediaSource *source, float mouse_x, float mouse_y);
		 
private:
	bigtime_t		fStartTime;
	MediaSource		*fMediaSource;
	Sphere			*fSphere;
	bool			fMouseTracking;
	BPoint			fMousePosition;
	float			fAngleY, fAngleZ;
	float			fSpeedY, fSpeedZ;
};

#endif	//#ifndef _VIEW_SPHERE_H_
