/*
 * Copyright 2010, Haiku Inc.
 * Authors:
 *		Philippe Houdoin <phoudoin %at% haiku-os %dot% org>
 *
 * Distributed under the terms of the MIT License.
 */


#include "glutGameMode.h"
#include "glutint.h"
#include "glutState.h"

#include <GL/glut.h>
#include <String.h>
#include <stdio.h>


// GlutGameMode class

GlutGameMode::GlutGameMode()
	:
	fActive(false),
	fDisplayChanged(false),
	fWidth(-1),
	fHeight(-1),
	fPixelDepth(-1),
	fRefreshRate(-1),
	fModesList(NULL),
	fModesCount(0),
	fGameModeWorkspace(-1),
	fGameModeWindow(0),
	fPreviousWindow(0)
{
}


GlutGameMode::~GlutGameMode()
{
	free(fModesList);
}


status_t
GlutGameMode::Set(const char* modeString)
{
    // String format: [WxH][:Bpp][@Hz]

   	const char* templates[] = {
    	"%1$ix%2$i:%3$i@%4$i",	// WxH:Bpp@Hz
    	"%1$ix%2$i:%3$i",		// WxH:Bpp
    	"%1$ix%2$i@%4$i",		// WxH@Hz
    	"%1$ix%2$i",			// WxH
    	":%3$i@%4$i",			// :Bpp@Hz
    	":%3$i",				// :Bpp
    	"@%4$i",				// @Hz
    	NULL
    };

	// Find matching string format template, if any
	for (int i=0; templates[i]; i++) {
		// count expected arguments in this template
		int expectedArgs = 0;
		const char *p = templates[i];
		while (*p) {
			if (*p++ == '%')
				expectedArgs++;
		}

		// printf("Trying %s pattern\n", templates[i]);

		fWidth = fHeight = fPixelDepth = fRefreshRate = -1;

		if (sscanf(modeString, templates[i],
			&fWidth, &fHeight, &fPixelDepth, &fRefreshRate) == expectedArgs) {
			// printf("match!\n");
			return B_OK;
		}
	}

	return B_BAD_VALUE;
}


bool
GlutGameMode::IsPossible()
{
	display_mode* mode = _FindMatchingMode();
	return mode != NULL;
}


status_t
GlutGameMode::Enter()
{
	display_mode* mode = _FindMatchingMode();
	if (!mode)
		return B_BAD_VALUE;

	BScreen screen;
	if (!fActive) {
		// First enter: remember this workspace original mode...
		fGameModeWorkspace = current_workspace();
		screen.GetMode(fGameModeWorkspace, &fOriginalMode);
	}

	// Don't make it new default mode for this workspace...
	status_t status = screen.SetMode(fGameModeWorkspace, mode, false);
	if (status != B_OK)
		return status;

	// Retrieve the new active display mode, which could be
	// a sligth different than the one we asked for...
	screen.GetMode(fGameModeWorkspace, &fCurrentMode);

	if (!fGameModeWindow) {
		// create a new window
		fPreviousWindow = glutGetWindow();
		fGameModeWindow = glutCreateWindow("glutGameMode");
		if (!fGameModeWindow)
			return Leave();
	} else
		// make sure it's the current window
		glutSetWindow(fGameModeWindow);

	BDirectWindow *directWindow
		= dynamic_cast<BDirectWindow*>(gState.currentWindow->Window());
	if (directWindow == NULL)
		// Hum?!
		return B_ERROR;

	// Give it some useless title, except for debugging (thread name).
	BString name;
	name << "Game Mode " << fCurrentMode.virtual_width
		<< "x" << fCurrentMode.virtual_height
		<< ":" << _GetModePixelDepth(&fCurrentMode)
		<< "@" << _GetModeRefreshRate(&fCurrentMode);

	// force the game mode window to fullscreen
	directWindow->Lock();
	directWindow->SetTitle(name.String());
	directWindow->SetFullScreen(true);
	directWindow->Unlock();

	fDisplayChanged = true;
	fActive = true;

	return B_OK;
}


status_t
GlutGameMode::Leave()
{
	if (!fActive)
		return B_OK;

	if (fGameModeWorkspace < 0)
		return B_ERROR;

	if (fGameModeWindow) {
		glutDestroyWindow(fGameModeWindow);
		fGameModeWindow = 0;
		if (fPreviousWindow)
			glutSetWindow(fPreviousWindow);
	}

	if (_CompareModes(&fOriginalMode, &fCurrentMode)) {
		// Restore original display mode
		BScreen screen;
		// Make restored mode the default one,
		// as it was before entering game mode...
		screen.SetMode(fGameModeWorkspace, &fOriginalMode, true);
	}

	fActive = false;
	return B_OK;
}


display_mode*
GlutGameMode::_FindMatchingMode()
{
	if (fWidth == -1 && fHeight == -1 && fPixelDepth == -1
		&& fRefreshRate == -1)
		// nothing to match!
		return NULL;

	if (!fModesList) {
		// Lazy retrieval of supported modes...
		BScreen screen;
		if (screen.GetModeList(&fModesList, &fModesCount) == B_OK) {
			// sort modes in decrease order (resolution, depth, frequency)
			qsort(fModesList, fModesCount, sizeof(display_mode), _CompareModes);
		} else {
			// bad luck, no modes can be retrieved!
			fModesList = NULL;
			fModesCount = 0;
		}
	}

	if (!fModesList)
		return NULL;

	float bestRefreshDiff = 999999;
	int bestMode = -1;
	for (uint32 i =0; i < fModesCount; i++) {

		printf("[%ld]: %d x %d, %d Bpp, %d Hz\n", i,
			fModesList[i].virtual_width, fModesList[i].virtual_height,
			_GetModePixelDepth(&fModesList[i]),
			_GetModeRefreshRate(&fModesList[i]));

		if (fWidth > 0 && fModesList[i].virtual_width != fWidth)
			continue;

		if (fHeight > 0 && fModesList[i].virtual_height != fHeight)
			continue;

		if (fPixelDepth > 0
			&& _GetModePixelDepth(&fModesList[i]) != fPixelDepth)
			continue;

		float refreshDiff = fabs(_GetModeRefreshRate(&fModesList[i])
			- fRefreshRate);
		if (fRefreshRate > 0 && (refreshDiff > 0.006 * fRefreshRate)) {
			// not exactly the same, but maybe the best similar mode so far?
			if (refreshDiff < bestRefreshDiff) {
				bestRefreshDiff = refreshDiff;
				bestMode = i;
			}
			continue;
		}

		// Hey, this one match everything!
		return &fModesList[i];
	}

	if (bestMode == -1)
		return NULL;

	return &fModesList[bestMode];
}


int
GlutGameMode::_GetModePixelDepth(const display_mode* mode)
{
	switch (mode->space) {
		case B_RGB32:	return 32;
		case B_RGB24:	return 24;
		case B_RGB16:	return 16;
		case B_RGB15:	return 15;
		case B_CMAP8:	return 8;
		default:		return 0;
	}
}


int
GlutGameMode::_GetModeRefreshRate(const display_mode* mode)
{
	// we have to be catious as refresh rate cannot be controlled directly,
	// so it suffers under rounding errors and hardware restrictions
	return rint(10 * float(mode->timing.pixel_clock * 1000)
		/ float(mode->timing.h_total * mode->timing.v_total)) / 10.0;
}


int
GlutGameMode::_CompareModes(const void* _mode1, const void* _mode2)
{
	display_mode *mode1 = (display_mode *)_mode1;
	display_mode *mode2 = (display_mode *)_mode2;

	if (mode1->virtual_width != mode2->virtual_width)
		return mode2->virtual_width - mode1->virtual_width;

	if (mode1->virtual_height != mode2->virtual_height)
		return mode2->virtual_height - mode1->virtual_height;

	if (mode1->space != mode2->space)
		return _GetModePixelDepth(mode2) - _GetModePixelDepth(mode1);

	return _GetModeRefreshRate(mode2) - _GetModeRefreshRate(mode1);
}


// #pragma mark GLUT game mode API

void APIENTRY
glutGameModeString(const char* string)
{
	gState.gameMode.Set(string);
}


int APIENTRY
glutEnterGameMode(void)
{
	return gState.gameMode.Enter() == B_OK;
}


void APIENTRY
glutLeaveGameMode(void)
{
	gState.gameMode.Leave();
}


int APIENTRY
glutGameModeGet(GLenum pname)
{
	switch( pname ) {
    	case GLUT_GAME_MODE_ACTIVE:
    		return gState.gameMode.IsActive();

    	case GLUT_GAME_MODE_POSSIBLE:
        	return gState.gameMode.IsPossible();

    	case GLUT_GAME_MODE_WIDTH:
			return gState.gameMode.Width();

    	case GLUT_GAME_MODE_HEIGHT:
    		return gState.gameMode.Height();

	    case GLUT_GAME_MODE_PIXEL_DEPTH:
			return gState.gameMode.PixelDepth();

	    case GLUT_GAME_MODE_REFRESH_RATE:
    		return gState.gameMode.RefreshRate();

	    case GLUT_GAME_MODE_DISPLAY_CHANGED:
    		return gState.gameMode.HasDisplayChanged();

    	default:
        	__glutWarning( "Unknown gamemode get: %d", pname );
        	return -1;
    }
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
