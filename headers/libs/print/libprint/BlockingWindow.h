/*

InterfaceUtils.cpp

Copyright (c) 2002 Haiku.

Author: 
	Michael Pfeiffer

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/
#ifndef _INTERFACE_UTILS_H
#define _INTERFACE_UTILS_H


#include <AppDefs.h>
#include <MessageFilter.h>
#include <Window.h>


class BHandler;
class BMessage;


class EscapeMessageFilter : public BMessageFilter
{
public:
							EscapeMessageFilter(BWindow *window, int32 what);
			filter_result	Filter(BMessage *msg, BHandler **target);

private:
			BWindow*		fWindow;
			int32			fWhat;
};


class HWindow : public BWindow
{
	typedef BWindow 		inherited;
public:
							HWindow(BRect frame, const char *title,
								window_type type, uint32 flags,
								uint32 workspace = B_CURRENT_WORKSPACE,
								uint32 escape_msg = B_QUIT_REQUESTED);
							HWindow(BRect frame, const char *title,
								window_look look, window_feel feel, uint32 flags,
								uint32 workspace = B_CURRENT_WORKSPACE,
								uint32 escape_msg = B_QUIT_REQUESTED);
	virtual					~HWindow() {}

	virtual void			MessageReceived(BMessage* m);
	virtual void			AboutRequested();
	virtual const char*		AboutText() const { return NULL; }

protected:
			void			Init(uint32 escape_msg);
};


class BlockingWindow : public HWindow
{
	typedef HWindow 		inherited;
public:
							BlockingWindow(BRect frame, const char *title,
								window_type type, uint32 flags,
								uint32 workspace = B_CURRENT_WORKSPACE,
								uint32 escape_msg = B_QUIT_REQUESTED);
							BlockingWindow(BRect frame, const char *title,
								window_look look, window_feel feel, uint32 flags,
								uint32 workspace = B_CURRENT_WORKSPACE,
								uint32 escape_msg = B_QUIT_REQUESTED);
	virtual					~BlockingWindow();

	virtual bool			QuitRequested();
	// Quit() is called by child class with result code
			void			Quit(status_t result);
	// Show window and wait for it to quit, returns result code
	virtual status_t		Go();
	// Or quit window e.g. something went wrong in constructor
	virtual void			Quit();
	// Sets the result that is returned when the user closes the window.
	// Default is B_OK.
			void			SetUserQuitResult(status_t result);

private:
			void			Init(const char* title);

private:
			status_t		fUserQuitResult;
			bool			fReadyToQuit;
			sem_id			fExitSem;
			status_t*		fResult;
};

#endif
