/***********************************************************
 *      Copyright (C) 1997, Be Inc.  Copyright (C) 1999, Jake Hamby.
 *
 * This program is freely distributable without licensing fees
 * and is provided without guarantee or warrantee expressed or
 * implied. This program is -not- in the public domain.
 *
 *
 *  FILE:	glutCursor.cpp
 *
 *	DESCRIPTION:	code for handling custom mouse cursors
 ***********************************************************/

/***********************************************************
 *	Headers
 ***********************************************************/
#include <GL/glut.h>
#include "glutint.h"
#include "glutState.h"
#include "glutCursors.h"

static const unsigned char *cursorTable[] = {
  XC_arrow,		  /* GLUT_CURSOR_RIGHT_ARROW */
  XC_top_left_arrow,	  /* GLUT_CURSOR_LEFT_ARROW */
  XC_hand1,		  /* GLUT_CURSOR_INFO */
  XC_pirate,		  /* GLUT_CURSOR_DESTROY */
  XC_question_arrow,	  /* GLUT_CURSOR_HELP */
  XC_exchange,		  /* GLUT_CURSOR_CYCLE */
  XC_spraycan,		  /* GLUT_CURSOR_SPRAY */
  XC_watch,		  /* GLUT_CURSOR_WAIT */
  XC_xterm,		  /* GLUT_CURSOR_TEXT */
  XC_crosshair,		  /* GLUT_CURSOR_CROSSHAIR */
  XC_sb_v_double_arrow,	  /* GLUT_CURSOR_UP_DOWN */
  XC_sb_h_double_arrow,	  /* GLUT_CURSOR_LEFT_RIGHT */
  XC_top_side,		  /* GLUT_CURSOR_TOP_SIDE */
  XC_bottom_side,	  /* GLUT_CURSOR_BOTTOM_SIDE */
  XC_left_side,		  /* GLUT_CURSOR_LEFT_SIDE */
  XC_right_side,	  /* GLUT_CURSOR_RIGHT_SIDE */
  XC_top_left_corner,	  /* GLUT_CURSOR_TOP_LEFT_CORNER */
  XC_top_right_corner,	  /* GLUT_CURSOR_TOP_RIGHT_CORNER */
  XC_bottom_right_corner, /* GLUT_CURSOR_BOTTOM_RIGHT_CORNER */
  XC_bottom_left_corner,  /* GLUT_CURSOR_BOTTOM_LEFT_CORNER */
};

/***********************************************************
 *	FUNCTION:	glutSetCursor (4.13)
 *
 *	DESCRIPTION:  set a new mouse cursor for current window
 ***********************************************************/
void glutSetCursor(int cursor) {
	gState.currentWindow->Window()->Lock();
	gState.currentWindow->cursor = cursor;
	__glutSetCursor(cursor);
	gState.currentWindow->Window()->Unlock();
}

/***********************************************************
 *	FUNCTION:	__glutSetCursor
 *
 *	DESCRIPTION:  the actual cursor changing routine
 ***********************************************************/
void __glutSetCursor(int cursor) {
	int realcursor = cursor;
	if (cursor < 0 || cursor > GLUT_CURSOR_BOTTOM_LEFT_CORNER) {
		switch(cursor) {
		case GLUT_CURSOR_INHERIT:
			return;		// don't change cursor
		case GLUT_CURSOR_NONE:
			// this hides the cursor until the user moves the mouse
			// change it to HideCursor() AT YOUR OWN RISK!
			be_app->ObscureCursor();
			return;
		case GLUT_CURSOR_FULL_CROSSHAIR:
			realcursor = GLUT_CURSOR_CROSSHAIR;
			break;
		default:
			__glutWarning("unknown cursor\n");
			return;
		}
	}
	be_app->SetCursor(cursorTable[realcursor]);
}

/***********************************************************
 *	FUNCTION:	glutWarpPointer (x.xx)
 *
 *	DESCRIPTION:  move the mouse pointer to a new location
 *		(note:  can't do this in BeOS!)
 ***********************************************************/
void glutWarpPointer(int x, int y) { }
