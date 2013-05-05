/***********************************************************
 *      Copyright (C) 1997, Be Inc.  Copyright (C) 1999, Jake Hamby.
 *
 * This program is freely distributable without licensing fees
 * and is provided without guarantee or warrantee expressed or
 * implied. This program is -not- in the public domain.
 *
 *  FILE:	glutBlocker.h
 *
 *	DESCRIPTION:	helper class for GLUT event loop.
 *		if a window receives an event, wake up the event loop.
 ***********************************************************/

/***********************************************************
 *	Headers
 ***********************************************************/
#include <kernel/OS.h>

/***********************************************************
 *	CLASS:	GlutBlocker
 *
 *	DESCRIPTION:  Fairly naive, but safe implementation.
 *		global semaphore controls access to state
 *		event semaphore blocks WaitEvent() call if necessary
 *		(this is basically a condition variable class)
 ***********************************************************/
class GlutBlocker {
public:
	GlutBlocker();
	~GlutBlocker();
	void WaitEvent();		// wait for new event
	void WaitEvent(bigtime_t usecs);	// wait with timeout
	void NewEvent();		// new event from a window (may need to wakeup main thread)
	void QuickNewEvent() { events = true; }	// new event from main thread
	void ClearEvents() { events = false; }		// clear counter at beginning of event loop
	bool PendingEvent() { return events; }		// XPending() equivalent
private:
	sem_id gSem;
	sem_id eSem;
	bool events;	// are there any new events?
	bool sleeping;	// is someone sleeping on eSem?
};

/***********************************************************
 *	Global variable
 ***********************************************************/
extern GlutBlocker gBlock;
