/***********************************************************
 *      Copyright (C) 1997, Be Inc.  Copyright (C) 1999, Jake Hamby.
 *
 * This program is freely distributable without licensing fees
 * and is provided without guarantee or warrantee expressed or
 * implied. This program is -not- in the public domain.
 *
 *
 *  FILE:	glutColor.cpp
 *
 *	DESCRIPTION:	we don't support indexed color, so this code is
 *		really simple
 ***********************************************************/

/***********************************************************
 *	Headers
 ***********************************************************/
#include <GL/glut.h>
#include "glutint.h"

void glutSetColor(int cell, GLfloat red, GLfloat green, GLfloat blue) {
	__glutWarning("glutSetColor: current window is RGBA");
}

GLfloat glutGetColor(int cell, int component) {
	__glutWarning("glutGetColor: current window is RGBA");
	return -1.0;
}

void glutCopyColormap(int win) {
	__glutWarning("glutCopyColormap: color index not supported in BeOS");
}
