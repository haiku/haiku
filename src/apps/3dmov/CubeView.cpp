/* 
 * Copyright 2004-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol <revol@free.fr>
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
