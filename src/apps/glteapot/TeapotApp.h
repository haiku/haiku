/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _TEAPOT_APP_
#define _TEAPOT_APP_


#include <Application.h>

#include "TeapotWindow.h"


class TeapotApp : public BApplication {
public:
							TeapotApp(const char* sign);
	virtual	void			MessageReceived(BMessage* msg);

private:
			TeapotWindow*	fTeapotWindow;
};

#endif
