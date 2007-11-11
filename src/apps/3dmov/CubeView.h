/* 
 * Copyright 2004-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol <revol@free.fr>
 */
#ifndef _CUBEVIEW_H
#define _CUBEVIEW_H

#include "GLMovView.h"

class CubeView: public GLMovView
{
	public:
					CubeView(BRect frame);
					~CubeView();
	virtual void	AttachedToWindow();
	
};

#endif /* _CUBEVIEW_H */
