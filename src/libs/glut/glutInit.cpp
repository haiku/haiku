/***********************************************************
 *      Copyright (C) 1997, Be Inc.  Copyright (C) 1999, Jake Hamby.
 *
 * This program is freely distributable without licensing fees
 * and is provided without guarantee or warrantee expressed or
 * implied. This program is -not- in the public domain.
 *
 *
 *  FILE:	glutInit.cpp
 *
 *	DESCRIPTION:	initialize GLUT state
 ***********************************************************/

/***********************************************************
 *	Headers
 ***********************************************************/
#include <GL/glut.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "glutint.h"
#include "glutState.h"
#include "glutBlocker.h"
#include "beos_x11.h"

/***********************************************************
 *	Global variables
 ***********************************************************/
GlutState gState;
char *__glutProgramName = NULL;

/***********************************************************
 *	Private variables
 ***********************************************************/
static int __glutArgc;
static char **__glutArgv;

/***********************************************************
 *	FUNCTION:	__glutInitTime
 *
 *	DESCRIPTION:  set up start time variable
 ***********************************************************/
void __glutInitTime(bigtime_t *beginning)
{
  static int beenhere = 0;
  static bigtime_t genesis;

  if (!beenhere) {
    genesis = system_time();
    beenhere = 1;
  }
  *beginning = genesis;
}

/***********************************************************
 *	FUNCTION:	removeArgs
 *
 *	DESCRIPTION:  helper function for glutInit to remove args
 *		from argv variable passed in
 ***********************************************************/
static void
removeArgs(int *argcp, char **argv, int numToRemove)
{
  int i, j;

  for (i = 0, j = numToRemove; argv[j]; i++, j++) {
    argv[i] = argv[j];
  }
  argv[i] = NULL;
  *argcp -= numToRemove;
}

/***********************************************************
 *	FUNCTION:	bAppThread
 *
 *	DESCRIPTION:  starts the BApplication message loop running
 ***********************************************************/
static int32 bAppThread(void *arg) {
	be_app->Lock();
	return be_app->Run();
}

/***********************************************************
 *	FUNCTION:	sigHandler
 *
 *	DESCRIPTION:  shuts down the app on CTRL-C
 ***********************************************************/
static void sigHandler(int) {
  gState.quitAll = true;
  gBlock.NewEvent();
}

/***********************************************************
 *	FUNCTION:	glutInit (2.1)
 *
 *	DESCRIPTION:  create BApplication, parse cmd-line arguments,
 *		and set up gState structure.
 ***********************************************************/
void glutInit(int *argcp, char **argv) {
  char *str, *geometry = NULL;
  int i;

  if (gState.display) {
    __glutWarning("glutInit being called a second time.");
    return;
  }
  /* Determine temporary program name. */
  str = strrchr(argv[0], '/');
  if (str == NULL) {
    __glutProgramName = argv[0];
  } else {
    __glutProgramName = str + 1;
  }

  /* Make private copy of command line arguments. */
  __glutArgc = *argcp;
  __glutArgv = (char **) malloc(__glutArgc * sizeof(char *));
  if (!__glutArgv)
    __glutFatalError("out of memory.");
  for (i = 0; i < __glutArgc; i++) {
    __glutArgv[i] = strdup(argv[i]);
    if (!__glutArgv[i])
      __glutFatalError("out of memory.");
  }

  /* determine permanent program name */
  str = strrchr(__glutArgv[0], '/');
  if (str == NULL) {
    __glutProgramName = __glutArgv[0];
  } else {
    __glutProgramName = str + 1;
  }

  /* parse arguments for standard options */
  for (i = 1; i < __glutArgc; i++) {
    if (!strcmp(__glutArgv[i], "-display")) {
      __glutWarning("-display option only valid for X glut.");
      if (++i >= __glutArgc) {
        __glutFatalError(
          "follow -display option with X display name.");
      }
      removeArgs(argcp, &argv[1], 2);
    } else if (!strcmp(__glutArgv[i], "-geometry")) {
      if (++i >= __glutArgc) {
        __glutFatalError(
          "follow -geometry option with geometry parameter.");
      }
      geometry = __glutArgv[i];
      removeArgs(argcp, &argv[1], 2);
    } else if (!strcmp(__glutArgv[i], "-direct")) {
      __glutWarning("-direct option only valid for X glut.");
      removeArgs(argcp, &argv[1], 1);
    } else if (!strcmp(__glutArgv[i], "-indirect")) {
      __glutWarning("-indirect option only valid for X glut.");
      removeArgs(argcp, &argv[1], 1);
    } else if (!strcmp(__glutArgv[i], "-iconic")) {
      __glutWarning("-iconic option doesn't make sense in BeOS.");
      removeArgs(argcp, &argv[1], 1);
    } else if (!strcmp(__glutArgv[i], "-gldebug")) {
      gState.debug = true;
      removeArgs(argcp, &argv[1], 1);
    } else if (!strcmp(__glutArgv[i], "-sync")) {
      __glutWarning("-sync option only valid for X glut.");
      removeArgs(argcp, &argv[1], 1);
    } else {
      /* Once unknown option encountered, stop command line
         processing. */
      break;
    }
  }

  __glutInit();  /* Create BApplication first so DisplayWidth() works */
  if (geometry) {
    int flags, x, y, width, height;

    /* Fix bogus "{width|height} may be used before set"
       warning */
    width = 0;
    height = 0;

    flags = XParseGeometry(geometry, &x, &y,
      (unsigned int *) &width, (unsigned int *) &height);
    if (WidthValue & flags) {
      /* Careful because X does not allow zero or negative
         width windows */
      if (width > 0)
        gState.initWidth = width;
    }
    if (HeightValue & flags) {
      /* Careful because X does not allow zero or negative
         height windows */
      if (height > 0)
        gState.initHeight = height;
    }
    if (XValue & flags) {
      if (XNegative & flags)
        x = DisplayWidth() + x - gState.initWidth;
      /* Play safe: reject negative X locations */
      if (x >= 0)
        gState.initX = x;
    }
    if (YValue & flags) {
      if (YNegative & flags)
        y = DisplayHeight() + y - gState.initHeight;
      /* Play safe: reject negative Y locations */
      if (y >= 0)
        gState.initY = y;
    }
  }
}

/***********************************************************
 *	FUNCTION:	__glutInit
 *
 *	DESCRIPTION:  create BApplication, parse cmd-line arguments,
 *		and set up gState structure.
 ***********************************************************/
void __glutInit() {
  // open BApplication
  gState.display = new BApplication("application/x-glut-demo");
  be_app->Unlock();
  gState.appthread = spawn_thread(bAppThread, "BApplication", B_NORMAL_PRIORITY, 0);
  resume_thread(gState.appthread);

  bigtime_t unused;
  __glutInitTime(&unused);

  /* set atexit() function to cleanup before exiting */
  if(atexit(__glutExitCleanup))
  	__glutFatalError("can't set exit handler");

  /* similarly, destroy all windows on CTRL-C */
  signal(SIGINT, sigHandler);
}


void
__glutExitCleanup()
{
	if (glutGameModeGet(GLUT_GAME_MODE_ACTIVE) > 0)
		// Try to restore initial screen mode...
		glutLeaveGameMode();

	__glutDestroyAllWindows();
}

/***********************************************************
 *	FUNCTION:	glutInitWindowPosition (2.2)
 *
 *	DESCRIPTION:  set initial window position
 ***********************************************************/
void glutInitWindowPosition(int x, int y) {
	gState.initX = x;
	gState.initY = y;
}

/***********************************************************
 *	FUNCTION:	glutInitWindowSize (2.2)
 *
 *	DESCRIPTION:  set initial window size
 ***********************************************************/
void glutInitWindowSize(int width, int height) {
	gState.initWidth = width;
	gState.initHeight = height;
}

/***********************************************************
 *	FUNCTION:	glutInitDisplayMode (2.3)
 *
 *	DESCRIPTION:  set initial display mode
 ***********************************************************/
void glutInitDisplayMode(unsigned int mode) {
	gState.displayMode = mode;
}
