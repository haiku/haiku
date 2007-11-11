/*
  -----------------------------------------------------------------------------

	File:	CubeView.h
	
	Date:	10/20/2004

	Description:	3DMov reloaded...
	
	Author:	François Revol.
	
	
	Copyright 2004, yellowTAB GmbH, All rights reserved.
	

  -----------------------------------------------------------------------------
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
