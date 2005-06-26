/***********************************************************
 *	Copyright (C) 1997, Be Inc.  Copyright (C) 1999, Jake Hamby.
 *
 * This program is freely distributable without licensing fees
 * and is provided without guarantee or warrantee expressed or
 * implied. This program is -not- in the public domain.
 *
 *  FILE:	glutBlocker.cpp
 *
 *	DESCRIPTION:	helper class for GLUT event loop.
 *		if a window receives an event, wake up the event loop.
 ***********************************************************/

/***********************************************************
 *	Headers
 ***********************************************************/
#include "glutBlocker.h"

/***********************************************************
 *	Global variable
 ***********************************************************/
GlutBlocker gBlock;

/***********************************************************
 *	Member functions
 ***********************************************************/
GlutBlocker::GlutBlocker() {
	gSem = create_sem(1, "gSem");
	eSem = create_sem(0, "eSem");
	events = false;
	sleeping = false;
}

GlutBlocker::~GlutBlocker() {
	delete_sem(eSem);
	delete_sem(gSem);
}

void GlutBlocker::WaitEvent() {
	acquire_sem(gSem);
	if(!events) {			// wait for new event
		sleeping = true;
		release_sem(gSem);
		acquire_sem(eSem);	// next event will release eSem
	} else {
		release_sem(gSem);
	}
}

void GlutBlocker::WaitEvent(bigtime_t usecs) {
	acquire_sem(gSem);
	if(!events) {			// wait for new event
		sleeping = true;
		release_sem(gSem);
		acquire_sem_etc(eSem, 1, B_TIMEOUT, usecs);	// wait for next event or timeout
	} else {
		release_sem(gSem);
	}
}

void GlutBlocker::NewEvent() {
	acquire_sem(gSem);
	events = true;		// next call to WaitEvent returns immediately
	if(sleeping) {
		sleeping = false;
		release_sem(eSem);	// if event loop is blocking, wake it up
	}
	release_sem(gSem);
}
