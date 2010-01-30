/*	PROJECT:		3Dmov
	AUTHORS:		Zenja Solaja
	COPYRIGHT:		2009 Haiku Inc
	DESCRIPTION:	Haiku version of the famous BeInc demo 3Dmov
					Just drag'n'drop media files to the 3D objects
*/

#ifndef _VIEW_BOOK_H_
#define _VIEW_BOOK_H_

#ifndef _VIEW_OBJECT_H_
#include "ViewObject.h"
#endif

class Paper;

/**********************************
	ViewBook displays a book with turnable pages
***********************************/
class ViewBook : public ViewObject
{
public:
			ViewBook(BRect frame);
			~ViewBook();
	
	void	AttachedToWindow();
	void	MouseDown(BPoint);
	void	MouseMoved(BPoint p, uint32 transit, const BMessage *message);
	void	MouseUp(BPoint);
	void	Render();
	bool	SurfaceUpdate(MediaSource *source, float mouse_x, float mouse_y);
		 
private:
	enum
	{
		PAGE_LEFT = 0,
		PAGE_MIDDLE = 1,
		PAGE_RIGHT = 2,
		NUMBER_PAGES = 3,
		NUMBER_SOURCES = 4,
	};
	Paper 			*GetPage(float mouse_x, float mouse_y, int *side);
	
	bool			fMouseTracking;
	float			fPageAngle;
	bigtime_t		fStartTime;
	Paper			*fPages[NUMBER_PAGES];
	MediaSource		*fMediaSources[NUMBER_SOURCES];
};

#endif	//#ifndef _VIEW_BOOK_H_
