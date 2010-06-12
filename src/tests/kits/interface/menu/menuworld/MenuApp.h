//--------------------------------------------------------------------
//	
//	MenuApp.h
//
//	Written by: Owen Smith
//	
//--------------------------------------------------------------------

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _MenuApp_h
#define _MenuApp_h

#include <Application.h>

//====================================================================
//	CLASS: MenuApp

class MenuApp : public BApplication
{
	//----------------------------------------------------------------
	//	Constructors, destructors, operators

public:
				MenuApp();
	
	//----------------------------------------------------------------
	//	Virtual function overrides

public:
	void		AboutRequested(void);	
	void		ReadyToRun(void);
	
};

#endif /* _MenuApp_h */