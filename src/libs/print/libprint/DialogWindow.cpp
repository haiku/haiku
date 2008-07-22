/*
 * DialogWindow.h
 * Copyright 2004 Michael Pfeiffer. All Rights Reserved.
 */
 
#include "DialogWindow.h"

#include <Messenger.h>

DialogWindow::DialogWindow(BRect frame,
			const char *title, 
			window_type type,
			uint32 flags,
			uint32 workspace)
	: BWindow(frame, title, type, flags, workspace)
	, fPreviousResult(B_OK)
	, fResult(NULL)
{
	// nothing to do
}

DialogWindow::DialogWindow(BRect frame,
			const char *title, 
			window_look look,
			window_feel feel,
			uint32 flags,
			uint32 workspace)
	: BWindow(frame, title, look, feel, flags, workspace)
	, fPreviousResult(B_OK)
	, fResult(NULL)
{
	// nothing to do
}

void DialogWindow::MessageReceived(BMessage *msg)
{
	if (msg->what == kGetThreadId) {
		BMessage reply;
		reply.AddInt32("thread_id", Thread());
		msg->SendReply(&reply);
		return;
	}
	BWindow::MessageReceived(msg);
}
			
status_t DialogWindow::Go()
{
	BMessenger messenger(this, this);
	// store result in local variable and
	// initialize it with previous result
	volatile status_t result = fPreviousResult;
	// new results are stored on the stack
	fResult = &result;
	
	// show the window
	Show();
	// at this point we must not access member variables,
	// because this object (the window) could already be deleted.

	// get thread id of window thread
	BMessage reply;
	if (messenger.SendMessage(kGetThreadId, &reply) != B_OK) {
		return B_ERROR;
	}
	thread_id windowThread;
	if (reply.FindInt32("thread_id", &windowThread) != B_OK) {
		return B_ERROR;
	}
	
	// wait for window thread to die
	// The window thread will crash if the image holding the
	// code used by the window thread is unloaded while the thread is 
	// still running!!!
	status_t status = B_ERROR;
	wait_for_thread(windowThread, &status);
	
	return result;
}
	
void DialogWindow::SetResult(status_t result)
{
	if (fResult != NULL) {
		*fResult = result;
	} else {
		fPreviousResult = result;
	}
}

