/*
	
	clock.h
	
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _APPLICATION_H
#include <Application.h>
#endif

#include "cl_view.h"
#include "cl_wind.h"

#define SHOW_SECONDS	'ssec'

extern const char *app_signature;

class THelloApplication : public BApplication {

public:
						THelloApplication();
virtual	void			MessageReceived(BMessage *msg);

private:
		TClockWindow*	myWindow;
		TOnscreenView	*myView;
};
