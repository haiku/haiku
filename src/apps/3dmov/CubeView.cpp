/*
  -----------------------------------------------------------------------------

	File:	CubeView.cpp
	
	Date:	10/20/2004

	Description:	3DMov reloaded...
	
	Author:	François Revol.
	
	
	Copyright 2004, yellowTAB GmbH, All rights reserved.
	

  -----------------------------------------------------------------------------
*/

#include <opengl/GL/gl.h>
#include <opengl/GL/glu.h>
#include "CubeView.h"

CubeView::CubeView(BRect frame)
	: GLMovView(frame)
{
}

CubeView::~CubeView()
{
}

void CubeView::AttachedToWindow()
{
	
}
