/***********************************************************
 *      Copyright (C) 1997, Be Inc.  Copyright (C) 1999, Jake Hamby.
 *
 * This program is freely distributable without licensing fees
 * and is provided without guarantee or warrantee expressed or
 * implied. This program is -not- in the public domain.
 *
 *
 *  FILE:	glutOverlay.cpp
 *
 *	DESCRIPTION:	we don't support overlays, so this code is
 *		really simple
 ***********************************************************/

/***********************************************************
 *	Headers
 ***********************************************************/
#include <GL/glut.h>
#include "glutint.h"

void glutEstablishOverlay() {
	__glutFatalError("BeOS lacks overlay support.");
}

void glutUseLayer(GLenum layer) {
	// ignore
}

void glutRemoveOverlay() {
	// ignore
}

void glutPostOverlayRedisplay() {
	// ignore
}

void glutShowOverlay() {
	// ignore
}

void glutHideOverlay() {
	// ignore
}

void glutPostWindowOverlayRedisplay(int win) {
	// ignore
}
