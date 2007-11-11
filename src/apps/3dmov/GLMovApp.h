/*
  -----------------------------------------------------------------------------

	File:	GLMovApp.h
	
	Date:	10/20/2004

	Description:	3DMov reloaded...
	
	Author:	François Revol.
	
	
	Copyright 2004, yellowTAB GmbH, All rights reserved.
	

  -----------------------------------------------------------------------------
*/
#ifndef _GLMOVAPP_H
#define _GLMOVAPP_H

#include <Application.h>

class GLMovApp: public BApplication
{
	public:
					GLMovApp();
					~GLMovApp();
	virtual void	ReadyToRun();
	virtual void	AboutRequested();
	virtual void	MessageReceived(BMessage *message);
	
};

#endif /* _GLMOVAPP_H */
