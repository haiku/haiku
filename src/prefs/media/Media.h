/*

Media Header by Sikosis

(C)2003

*/

#ifndef __MEDIA_H__
#define __MEDIA_H__

// Constants ------------------------------------------------------------------------------------------------- //
const char *APP_SIGNATURE = "application/x-vnd.OBOS.Media";  // Application Signature and Title

#include <Application.h>
#include "MediaWindow.h"

class Media : public BApplication
{
	public:
    	Media();
	    	    
	private:
		MediaWindow   *mWindow;
		
};

#endif
