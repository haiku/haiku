/*
	
	cl_wind.h
	
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef	_CL_VIEW_H_
#include "cl_view.h"
#endif

class TClockWindow : public BWindow {

public:
				TClockWindow(BRect, const char*);
virtual	bool	QuitRequested( void );

TOnscreenView	*theOnscreenView;
};
