/*
 * DialogWindow.h
 * Copyright 2004 Michael Pfeiffer. All Rights Reserved.
 */
 
#ifndef __DIALOG_WINDOW_H
#define __DIALOG_WINDOW_H

#include <OS.h>
#include <Window.h>

class DialogWindow : public BWindow {
public:
	DialogWindow(BRect frame,
			const char *title, 
			window_type type,
			uint32 flags,
			uint32 workspace = B_CURRENT_WORKSPACE);
	
	DialogWindow(BRect frame,
			const char *title, 
			window_look look,
			window_feel feel,
			uint32 flags,
			uint32 workspace = B_CURRENT_WORKSPACE);
			
	status_t Go();
	
	void SetResult(status_t result);

	void MessageReceived(BMessage* msg);
	
	enum {
		kGetThreadId = 'dwti' // request thread id from window
	};
		
private:
	status_t           fPreviousResult; // holds the result as long as fResult == NULL
	volatile status_t *fResult;
};

#endif
