/*
  -----------------------------------------------------------------------------

	File:	GLMovView.cpp
	
	Date:	10/20/2004

	Description:	3DMov reloaded...
	
	Author:	François Revol.
	
	
	Copyright 2004, yellowTAB GmbH, All rights reserved.
	

  -----------------------------------------------------------------------------
*/

#include <opengl/GL/gl.h>
#include <opengl/GL/glu.h>
#include "GLMovView.h"

GLMovView::GLMovView(BRect frame)
	: BGLView(frame, "glview", B_FOLLOW_ALL, B_WILL_DRAW, BGL_RGB|BGL_DOUBLE)
{
}

GLMovView::~GLMovView()
{
}

void GLMovView::AttachedToWindow()
{
	
}
