// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        Media.h
//  Author:      Sikosis, Jérôme Duval
//  Description: Media Preferences
//  Created :    June 25, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~


#ifndef __MEDIA_H__
#define __MEDIA_H__

// Constants ------------------------------------------------------------------------------------------------- //
const char *APP_SIGNATURE = "application/x-vnd.Haiku.MediaPrefs";  // Application Signature

#include <Application.h>
#include "MediaWindow.h"

class Media : public BApplication
{
	public:
    	Media();
    	~Media();
    	virtual void MessageReceived(BMessage *message);
	    	    
	private:
		MediaWindow   *mWindow;
		
};

#endif
