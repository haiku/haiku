/* 
 * Copyright 2004-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol <revol@free.fr>
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

void
GLMovView::AttachedToWindow()
{
	
}
