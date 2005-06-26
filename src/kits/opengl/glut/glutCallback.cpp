/***********************************************************
 *      Copyright (C) 1997, Be Inc.  Copyright (C) 1999, Jake Hamby.
 *
 * This program is freely distributable without licensing fees
 * and is provided without guarantee or warrantee expressed or
 * implied. This program is -not- in the public domain.
 *
 *
 *  FILE:	glutCallback.cpp
 *
 *	DESCRIPTION:	put all the callback setting routines in
 *		one place
 ***********************************************************/

/***********************************************************
 *	Headers
 ***********************************************************/
#include <GL/glut.h>
#include "glutint.h"
#include "glutState.h"

/***********************************************************
 *	Window related callbacks
 ***********************************************************/
void APIENTRY 
glutDisplayFunc(GLUTdisplayCB displayFunc)
{
  /* XXX Remove the warning after GLUT 3.0. */
  if (!displayFunc)
    __glutFatalError("NULL display callback not allowed in GLUT 3.0; update your code.");
  gState.currentWindow->display = displayFunc;
}

void APIENTRY 
glutKeyboardFunc(GLUTkeyboardCB keyboardFunc)
{
  gState.currentWindow->keyboard = keyboardFunc;
}

void APIENTRY 
glutSpecialFunc(GLUTspecialCB specialFunc)
{
  gState.currentWindow->special = specialFunc;
}

void APIENTRY 
glutMouseFunc(GLUTmouseCB mouseFunc)
{
  gState.currentWindow->mouse = mouseFunc;
}

void APIENTRY 
glutMotionFunc(GLUTmotionCB motionFunc)
{
  gState.currentWindow->motion = motionFunc;
}

void APIENTRY 
glutPassiveMotionFunc(GLUTpassiveCB passiveMotionFunc)
{
  gState.currentWindow->passive = passiveMotionFunc;
}

void APIENTRY 
glutEntryFunc(GLUTentryCB entryFunc)
{
  gState.currentWindow->entry = entryFunc;
  if (!entryFunc) {
    gState.currentWindow->entryState = -1;
  }
}

void APIENTRY
glutWindowStatusFunc(GLUTwindowStatusCB windowStatusFunc)
{
  gState.currentWindow->windowStatus = windowStatusFunc;
}

static void
visibilityHelper(int status)
{
  if (status == GLUT_HIDDEN || status == GLUT_FULLY_COVERED)
    gState.currentWindow->visibility(GLUT_NOT_VISIBLE);
  else
    gState.currentWindow->visibility(GLUT_VISIBLE);
}

void APIENTRY 
glutVisibilityFunc(GLUTvisibilityCB visibilityFunc)
{
  gState.currentWindow->visibility = visibilityFunc;
  if (visibilityFunc)
    glutWindowStatusFunc(visibilityHelper);
  else
    glutWindowStatusFunc(NULL);
}

void APIENTRY 
glutReshapeFunc(GLUTreshapeCB reshapeFunc)
{
  if (reshapeFunc) {
    gState.currentWindow->reshape = reshapeFunc;
  } else {
    gState.currentWindow->reshape = __glutDefaultReshape;
  }
}

/***********************************************************
 *	General callbacks (timer callback in glutEvent.cpp)
 ***********************************************************/
/* DEPRICATED, use glutMenuStatusFunc instead. */
void APIENTRY
glutMenuStateFunc(GLUTmenuStateCB menuStateFunc)
{
  gState.menuStatus = (GLUTmenuStatusCB) menuStateFunc;
}

void APIENTRY
glutMenuStatusFunc(GLUTmenuStatusCB menuStatusFunc)
{
  gState.menuStatus = menuStatusFunc;
}

void APIENTRY
glutIdleFunc(GLUTidleCB idleFunc)
{
  gState.idle = idleFunc;
}

/***********************************************************
 *	Unsupported callbacks
 ***********************************************************/
void APIENTRY
glutOverlayDisplayFunc(GLUTdisplayCB displayFunc)
{
}

void APIENTRY
glutSpaceballMotionFunc(GLUTspaceMotionCB spaceMotionFunc)
{
}

void APIENTRY
glutSpaceballRotateFunc(GLUTspaceRotateCB spaceRotateFunc)
{
}

void APIENTRY
glutSpaceballButtonFunc(GLUTspaceButtonCB spaceButtonFunc)
{
}

void APIENTRY
glutButtonBoxFunc(GLUTbuttonBoxCB buttonBoxFunc)
{
}

void APIENTRY
glutDialsFunc(GLUTdialsCB dialsFunc)
{
}

void APIENTRY
glutTabletMotionFunc(GLUTtabletMotionCB tabletMotionFunc)
{
}

void APIENTRY
glutTabletButtonFunc(GLUTtabletButtonCB tabletButtonFunc)
{
}
