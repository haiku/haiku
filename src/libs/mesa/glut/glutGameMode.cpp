/*
 * Copyright 2010, Haiku Inc.
 * Authors:
 *		Philippe Houdoin <phoudoin %at% haiku-os %dot% org>
 *
 * Distributed under the terms of the MIT License.
 */


#include <GL/glut.h>
#include "glutint.h"
#include "glutState.h"
#include "glutBlocker.h"
#include <stdio.h>


void APIENTRY
glutGameModeString(const char *string)
{
	// TODO
}


int APIENTRY
glutEnterGameMode(void)
{
	// TODO
	return 0;
}


void APIENTRY
glutLeaveGameMode(void)
{
	// TODO
}


int APIENTRY
glutGameModeGet(GLenum pname)
{
	int ret = -1;

	switch( pname ) {
    	case GLUT_GAME_MODE_ACTIVE:
    		ret = gState.gameMode != NULL;
        	break;

    	case GLUT_GAME_MODE_POSSIBLE:
        	// TODO
        	break;

    	case GLUT_GAME_MODE_WIDTH:
    		if (gState.gameMode)
    			ret = gState.gameMode->width;
        	break;

    	case GLUT_GAME_MODE_HEIGHT:
    		if (gState.gameMode)
	    		ret = gState.gameMode->height;
	        break;

	    case GLUT_GAME_MODE_PIXEL_DEPTH:
    		if (gState.gameMode)
	    		ret = gState.gameMode->pixelDepth;
	        break;

	    case GLUT_GAME_MODE_REFRESH_RATE:
    		if (gState.gameMode)
	    		ret = gState.gameMode->refreshRate;
    	    break;

	    case GLUT_GAME_MODE_DISPLAY_CHANGED:
			// TODO
	        break;

    	default:
        	__glutWarning( "Unknown gamemode get: %d", pname );
        	break;
    }

    return ret;
}


void APIENTRY
glutForceJoystickFunc(void)
{
	/*
	Forces a joystick poll and callback.

	Forces the OpenGLUT joystick code to poll your
	joystick(s) and to call your joystick callbacks
	with the result.  The operation completes, including
	callbacks, before glutForceJoystickFunc() returns.

    See also glutJoystickFunc()
	*/

	// TODO
}
