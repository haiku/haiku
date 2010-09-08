/***********************************************************
 *      Copyright (C) 1997, Be Inc.  Copyright (C) 1999, Jake Hamby.
 *
 * This program is freely distributable without licensing fees
 * and is provided without guarantee or warrantee expressed or
 * implied. This program is -not- in the public domain.
 *
 *
 *  FILE:	glutGet.cpp
 *
 *	DESCRIPTION:	get state information from GL
 ***********************************************************/

/***********************************************************
 *	Headers
 ***********************************************************/
#include <GL/glut.h>
#include <string.h>
#include <Autolock.h>
#include <Screen.h>

#include "glutint.h"
#include "glutState.h"

/***********************************************************
 *	Global variables
 ***********************************************************/
// rough guess, since we don't know how big the monitor really is
const float dots_per_mm = (72/25.4);	// dots per millimeter

/***********************************************************
 *	FUNCTION:	glutGet (9.1)
 *
 *	DESCRIPTION:  retrieve window and GL state
 ***********************************************************/
int glutGet(GLenum state) {
	switch(state) {
	case GLUT_WINDOW_X:
		{BAutolock winlock(gState.currentWindow->Window());	// need to lock the window
		if (gState.currentWindow->parent)
			return (int)gState.currentWindow->Frame().left;
		else
			return (int)gState.currentWindow->Window()->Frame().left;
		}
	case GLUT_WINDOW_Y:
		{BAutolock winlock(gState.currentWindow->Window());
		if (gState.currentWindow->parent)
			return (int)gState.currentWindow->Frame().top;
		else
			return (int)gState.currentWindow->Window()->Frame().top;
		}
	case GLUT_WINDOW_WIDTH:
		{BAutolock winlock(gState.currentWindow->Window());
		return gState.currentWindow->m_width;
		}
	case GLUT_WINDOW_HEIGHT:
		{BAutolock winlock(gState.currentWindow->Window());
		return gState.currentWindow->m_height;
		}
	case GLUT_WINDOW_PARENT:
		{BAutolock winlock(gState.currentWindow->Window());
		if(gState.currentWindow->parent)
			return gState.currentWindow->parent->num + 1;
		else
			return 0;
		}
	case GLUT_WINDOW_NUM_CHILDREN:
		{BAutolock winlock(gState.currentWindow->Window());
		int num = 0;
		GlutWindow *children = gState.currentWindow->children;
		while (children) {
			num++;
			children = children->siblings;
		}
		return num;
		}
  case GLUT_WINDOW_BUFFER_SIZE:	// best guesses
  case GLUT_WINDOW_DEPTH_SIZE:
        return 32;

  case GLUT_WINDOW_STENCIL_SIZE:
  case GLUT_WINDOW_RED_SIZE:		// always 24-bit color
  case GLUT_WINDOW_GREEN_SIZE:
  case GLUT_WINDOW_BLUE_SIZE:
  case GLUT_WINDOW_ALPHA_SIZE:
  case GLUT_WINDOW_ACCUM_RED_SIZE:
  case GLUT_WINDOW_ACCUM_GREEN_SIZE:
  case GLUT_WINDOW_ACCUM_BLUE_SIZE:
  case GLUT_WINDOW_ACCUM_ALPHA_SIZE:
        return 8;

  case GLUT_WINDOW_DOUBLEBUFFER:	// always double-buffered RGBA
  case GLUT_WINDOW_RGBA:
        return 1;

  case GLUT_WINDOW_COLORMAP_SIZE:	// don't support these
  case GLUT_WINDOW_NUM_SAMPLES:
  case GLUT_WINDOW_STEREO:
        return 0;

 	case GLUT_WINDOW_CURSOR:
 		return gState.currentWindow->cursor;	// don't need to lock window since it won't change

	case GLUT_SCREEN_WIDTH:
		return (int)(BScreen().Frame().Width()) + 1;
	case GLUT_SCREEN_HEIGHT:
		return (int)(BScreen().Frame().Height()) + 1;
	case GLUT_SCREEN_WIDTH_MM:
		return (int)((BScreen().Frame().Width() + 1) / dots_per_mm);
	case GLUT_SCREEN_HEIGHT_MM:
		return (int)((BScreen().Frame().Height() + 1) / dots_per_mm);
	case GLUT_MENU_NUM_ITEMS:
		return gState.currentMenu->num;
	case GLUT_DISPLAY_MODE_POSSIBLE:
		return __glutConvertDisplayMode(0);	// returns 1 if possible
	case GLUT_INIT_DISPLAY_MODE:
		return gState.displayMode;
	case GLUT_INIT_WINDOW_X:
		return gState.initX;
	case GLUT_INIT_WINDOW_Y:
		return gState.initY;
	case GLUT_INIT_WINDOW_WIDTH:
		return gState.initWidth;
	case GLUT_INIT_WINDOW_HEIGHT:
		return gState.initHeight;
	case GLUT_ELAPSED_TIME:
		bigtime_t elapsed, beginning, now;
		__glutInitTime(&beginning);
		now = system_time();
		elapsed = now - beginning;
		return (int) (elapsed / 1000);	// 1000 ticks in a millisecond
	default:
		__glutWarning("invalid glutGet parameter: %d", state);
		return -1;
	}
}

/***********************************************************
 *	FUNCTION:	glutLayerGet (9.2)
 *
 *	DESCRIPTION:  since we don't support layers, this is easy
 ***********************************************************/
int glutLayerGet(GLenum info) {
	switch(info) {
	case GLUT_OVERLAY_POSSIBLE:
	case GLUT_HAS_OVERLAY:
		return 0;
	case GLUT_LAYER_IN_USE:
		return GLUT_NORMAL;
	case GLUT_TRANSPARENT_INDEX:
		return -1;
	case GLUT_NORMAL_DAMAGED:
		return gState.currentWindow->displayEvent;
	case GLUT_OVERLAY_DAMAGED:
		return -1;
	default:
		__glutWarning("invalid glutLayerGet param: %d", info);
		return -1;
	}
}

/***********************************************************
 *	FUNCTION:	glutDeviceGet (9.3)
 *
 *	DESCRIPTION:  get info about I/O devices we support
 *		easy, since BeOS only supports a keyboard and mouse now
 ***********************************************************/
int glutDeviceGet(GLenum info) {
	switch(info) {
	case GLUT_HAS_KEYBOARD:
	case GLUT_HAS_MOUSE:
		return 1;

	case GLUT_HAS_SPACEBALL:
	case GLUT_NUM_SPACEBALL_BUTTONS:
	case GLUT_HAS_DIAL_AND_BUTTON_BOX:
	case GLUT_NUM_DIALS:
	case GLUT_NUM_BUTTON_BOX_BUTTONS:
	case GLUT_HAS_TABLET:
	case GLUT_NUM_TABLET_BUTTONS:
	case GLUT_HAS_JOYSTICK:
	case GLUT_JOYSTICK_POLL_RATE:
	case GLUT_JOYSTICK_BUTTONS:
	case GLUT_JOYSTICK_AXES:
		return 0;

	case GLUT_NUM_MOUSE_BUTTONS:
	    {
		int32 mouseButtons = 3;		// good guess
		if(get_mouse_type(&mouseButtons) != B_OK) {
			__glutWarning("error getting number of mouse buttons");
		}
		return mouseButtons;
	    }

	case GLUT_DEVICE_IGNORE_KEY_REPEAT:
		if (gState.currentWindow)
			return gState.currentWindow->ignoreKeyRepeat;
 		return 0;

	case GLUT_DEVICE_KEY_REPEAT:
		return GLUT_KEY_REPEAT_DEFAULT;

	default:
		__glutWarning("invalid glutDeviceGet parameter: %d", info);
		return -1;
	}
}

/***********************************************************
 *	FUNCTION:	glutGetModifiers (9.4)
 *
 *	DESCRIPTION:  get the modifier key state for the current window
 ***********************************************************/
int glutGetModifiers() {
	if(gState.modifierKeys == (int) ~0) {
	  __glutWarning(
		"glutCurrentModifiers: do not call outside core input callback.");
    return 0;
	}
	return gState.modifierKeys;
}


#ifdef __HAIKU__

extern "C" {
GLUTproc __glutGetProcAddress(const char* procName);
}

GLUTproc
__glutGetProcAddress(const char* procName)
{
	if (gState.currentWindow)
		return (GLUTproc) gState.currentWindow->GetGLProcAddress(procName);
	return NULL;
}

#endif
