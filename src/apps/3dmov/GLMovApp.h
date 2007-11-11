/* 
 * Copyright 2004-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol <revol@free.fr>
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
