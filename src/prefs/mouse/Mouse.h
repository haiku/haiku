// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        Mouse.h
//  Author:      Jérôme Duval, Andrew McCall (mccall@digitalparadise.co.uk)
//  Description: Media Preferences
//  Created :    
//	Modified : December 10, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
#ifndef MOUSE_H
#define MOUSE_H


#include <Application.h>


class MouseApplication : public BApplication 
{
	public:
		MouseApplication();

		virtual void AboutRequested(void);
};

#endif	/* MOUSE_H */
