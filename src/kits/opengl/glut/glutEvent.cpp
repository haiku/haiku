/***********************************************************
 *      Copyright (C) 1997, Be Inc.  Copyright (C) 1999, Jake Hamby.
 *
 * This program is freely distributable without licensing fees
 * and is provided without guarantee or warrantee expressed or
 * implied. This program is -not- in the public domain.
 *
 *
 *  FILE:	glutEvent.cpp
 *
 *	DESCRIPTION:	here it is, the BeOS GLUT event loop
 ***********************************************************/

/***********************************************************
 *	Headers
 ***********************************************************/
#include <GL/glut.h>
#include "glutint.h"
#include "glutState.h"
#include "glutBlocker.h"

/***********************************************************
 *	CLASS:	GLUTtimer
 *
 *	DESCRIPTION:	list of timer callbacks
 ***********************************************************/
struct GLUTtimer {
	GLUTtimer *next;	// list of timers
	bigtime_t timeout;	// time to be called
	GLUTtimerCB func;	// function to call
	int value;			// value
};

/***********************************************************
 *	Private variables
 ***********************************************************/
static GLUTtimer *__glutTimerList = 0;			// list of timer callbacks
static GLUTtimer *freeTimerList = 0;

/***********************************************************
 *	FUNCTION:	glutTimerFunc (7.19)
 *
 *	DESCRIPTION:  register a new timer callback
 ***********************************************************/
void APIENTRY 
glutTimerFunc(unsigned int interval, GLUTtimerCB timerFunc, int value)
{
  GLUTtimer *timer, *other;
  GLUTtimer **prevptr;

  if (!timerFunc)
    return;

  if (freeTimerList) {
    timer = freeTimerList;
    freeTimerList = timer->next;
  } else {
    timer = new GLUTtimer();
    if (!timer)
      __glutFatalError("out of memory.");
  }

  timer->func = timerFunc;
  timer->value = value;
  timer->next = NULL;
  timer->timeout = system_time() + (interval*1000);	// 1000 ticks in a millisecond
  prevptr = &__glutTimerList;
  other = *prevptr;
  while (other && (other->timeout < timer->timeout)) {
    prevptr = &other->next;
    other = *prevptr;
  }
  timer->next = other;
  *prevptr = timer;
}

/***********************************************************
 *	FUNCTION:	handleTimeouts
 *
 *	DESCRIPTION:  private function to handle outstanding timeouts
 ***********************************************************/
static void
handleTimeouts(void)
{
  bigtime_t now;
  GLUTtimer *timer;

  /* Assumption is that __glutTimerList is already determined
     to be non-NULL. */
  now = system_time();
  while (__glutTimerList->timeout <= now) {
    timer = __glutTimerList;
    if(gState.currentWindow)
	    gState.currentWindow->LockGL();
    timer->func(timer->value);
    if(gState.currentWindow)
	    gState.currentWindow->UnlockGL();
    __glutTimerList = timer->next;
    timer->next = freeTimerList;
    freeTimerList = timer;
    if (!__glutTimerList)
      break;
  }
}


/***********************************************************
 *	FUNCTION:	processEventsAndTimeouts
 *
 *	DESCRIPTION:  clear gBlock, then check all windows for events
 ***********************************************************/
static void
processEventsAndTimeouts(void)
{
	gBlock.WaitEvent();		// if there is already an event, returns
							// immediately, otherwise wait forever
	gBlock.ClearEvents();
	
	if(gState.quitAll)
		exit(0);		// exit handler cleans up windows and quits nicely
	
	if (gState.currentWindow)
		gState.currentWindow->LockGL();
	for(int i=0; i<gState.windowListSize; i++) {
		if (gState.windowList[i]) {
			GlutWindow *win = gState.windowList[i];
			// NOTE: we can use win as a shortcut for gState.windowList[i]
			// in callbacks, EXCEPT we need to check the original variable
			// after each callback to make sure the window hasn't been destroyed
			if (win->anyevents) {
				win->anyevents = false;
				if (win->reshapeEvent) {
					win->reshapeEvent = false;
					__glutSetWindow(win);
					win->reshape(win->m_width, win->m_height);
				}
				if (!gState.windowList[i])
					continue;	// window was destroyed by callback!

				if (win->displayEvent) {
					win->displayEvent = false;
					__glutSetWindow(win);
					win->display();
				}
				if (!gState.windowList[i])
					continue;	// window was destroyed by callback!

				if (win->mouseEvent) {
					win->mouseEvent = false;
					__glutSetWindow(win);
					if (win->mouse) {
						gState.modifierKeys = win->modifierKeys;
						win->mouse(win->button, win->mouseState, win->mouseX, win->mouseY);
						gState.modifierKeys = ~0;
					}
				}
				if (!gState.windowList[i])
					continue;	// window was destroyed by callback!

				if (win->menuEvent) {
					win->menuEvent = false;
					__glutSetWindow(win);
					GlutMenu *menu = __glutGetMenuByNum(win->menuNumber);
					if (menu) {
						gState.currentMenu = menu;
						menu->select(win->menuValue);
					}
				}
				if (!gState.windowList[i])
					continue;	// window was destroyed by callback!

				if (win->statusEvent) {
					win->statusEvent = false;
					__glutSetWindow(win);
					if (gState.menuStatus) {
						gState.currentMenu = __glutGetMenuByNum(win->menuNumber);
						gState.menuStatus(win->menuStatus, win->statusX, win->statusY);
					}
				}
				if (!gState.windowList[i])
					continue;	// window was destroyed by callback!

				if (win->motionEvent) {
					win->motionEvent = false;
					__glutSetWindow(win);
					if (win->motion)
						win->motion(win->motionX, win->motionY);
				}
				if (!gState.windowList[i])
					continue;	// window was destroyed by callback!

				if (win->passiveEvent) {
					win->passiveEvent = false;
					__glutSetWindow(win);
					if (win->passive)
						win->passive(win->passiveX, win->passiveY);
				}
				if (!gState.windowList[i])
					continue;	// window was destroyed by callback!

				if (win->keybEvent) {
					win->keybEvent = false;
					__glutSetWindow(win);
					if (win->keyboard) {
						gState.modifierKeys = win->modifierKeys;
						win->keyboard(win->key, win->keyX, win->keyY);
						gState.modifierKeys = ~0;
					}
				}
				if (!gState.windowList[i])
					continue;	// window was destroyed by callback!

				if (win->specialEvent) {
					win->specialEvent = false;
					__glutSetWindow(win);
					if (win->special) {
						gState.modifierKeys = win->modifierKeys;
						win->special(win->specialKey, win->specialX, win->specialY);
						gState.modifierKeys = ~0;
					}
				}
				if (!gState.windowList[i])
					continue;	// window was destroyed by callback!

				if (win->entryEvent) {
					win->entryEvent = false;
					__glutSetWindow(win);
					if (win->entry)
						win->entry(win->entryState);
				}
				if (!gState.windowList[i])
					continue;	// window was destroyed by callback!

				if (win->windowStatusEvent) {
					win->windowStatusEvent = false;
					__glutSetWindow(win);
					if (win->windowStatus)
						win->windowStatus(win->visState);
				}
				if (!gState.windowList[i])
					continue;	// window was destroyed by callback!
			}
		}
	}
	if (gState.currentWindow)
		gState.currentWindow->UnlockGL();

	// This code isn't necessary since BGLView automatically traps errors
#if 0
	if(gState.debug) {
		for(int i=0; i<gState.windowListSize; i++) {
			if (gState.windowList[i]) {
				gState.windowList[i]->LockGL();
				glutReportErrors();
				gState.windowList[i]->UnlockGL();
			}
		}
	}
#endif
	if (__glutTimerList) {
      handleTimeouts();
    }
}

/***********************************************************
 *	FUNCTION:	waitForSomething
 *
 *	DESCRIPTION:  use gBlock to wait for a new event or timeout
 ***********************************************************/
static void
waitForSomething(void)
{
	bigtime_t timeout = __glutTimerList->timeout;
	bigtime_t now = system_time();
	
	if (gBlock.PendingEvent())
		goto immediatelyHandleEvent;
	
	if(timeout>now)
		gBlock.WaitEvent(timeout-now);
	if (gBlock.PendingEvent()) {
	immediatelyHandleEvent:
		processEventsAndTimeouts();
	} else {
		if (__glutTimerList)
			handleTimeouts();
	}
}

/***********************************************************
 *	FUNCTION:	idleWait
 *
 *	DESCRIPTION:  check for events, then call idle function
 ***********************************************************/
static void
idleWait(void)
{
  if (gBlock.PendingEvent()) {
    processEventsAndTimeouts();
  } else {
    if (__glutTimerList)
      handleTimeouts();
  }
  /* Make sure idle func still exists! */
  if(gState.currentWindow)
	  gState.currentWindow->LockGL();
  if (gState.idle) {
    gState.idle();
  }
  if(gState.currentWindow)
	  gState.currentWindow->UnlockGL();
}

/***********************************************************
 *	FUNCTION:	glutMainLoop (3.1)
 *
 *	DESCRIPTION:  enter the event processing loop
 ***********************************************************/
void glutMainLoop()
{
  if (!gState.windowListSize)
    __glutFatalUsage("main loop entered with no windows created.");

  if(gState.currentWindow)
	  gState.currentWindow->UnlockGL();

  for (;;) {
    if (gState.idle) {
      idleWait();
    } else {
      if (__glutTimerList) {
        waitForSomething();
      } else {
        processEventsAndTimeouts();
      }
    }
  }
}

/***********************************************************
 *	CLASS:		GlutWindow
 *
 *	FUNCTION:	KeyDown
 *
 *	DESCRIPTION:  handles keyboard and special events
 ***********************************************************/
void GlutWindow::KeyDown(const char *s, int32 slen)
{
  ulong aChar = s[0];
  BGLView::KeyDown(s,slen);
  
  BPoint p;
	
	switch (aChar) {
		case B_FUNCTION_KEY:
		switch(Window()->CurrentMessage()->FindInt32("key")) {
			case B_F1_KEY:
				aChar = GLUT_KEY_F1;
				goto specialLabel;
			case B_F2_KEY:
				aChar = GLUT_KEY_F2;
				goto specialLabel;
			case B_F3_KEY:
				aChar = GLUT_KEY_F3;
				goto specialLabel;
			case B_F4_KEY:
				aChar = GLUT_KEY_F4;
				goto specialLabel;
			case B_F5_KEY:
				aChar = GLUT_KEY_F5;
				goto specialLabel;
			case B_F6_KEY:
				aChar = GLUT_KEY_F6;
				goto specialLabel;
			case B_F7_KEY:
				aChar = GLUT_KEY_F7;
				goto specialLabel;
			case B_F8_KEY:
				aChar = GLUT_KEY_F8;
				goto specialLabel;
			case B_F9_KEY:
				aChar = GLUT_KEY_F9;
				goto specialLabel;
			case B_F10_KEY:
				aChar = GLUT_KEY_F10;
				goto specialLabel;
			case B_F11_KEY:
				aChar = GLUT_KEY_F11;
				goto specialLabel;
			case B_F12_KEY:
				aChar = GLUT_KEY_F12;
				goto specialLabel;
			default:
				return;
		}
		case B_LEFT_ARROW:
			aChar = GLUT_KEY_LEFT;
			goto specialLabel;
		case B_UP_ARROW:
			aChar = GLUT_KEY_UP;
			goto specialLabel;
		case B_RIGHT_ARROW:
			aChar = GLUT_KEY_RIGHT;
			goto specialLabel;
		case B_DOWN_ARROW:
			aChar = GLUT_KEY_DOWN;
			goto specialLabel;
		case B_PAGE_UP:
			aChar = GLUT_KEY_PAGE_UP;
			goto specialLabel;
		case B_PAGE_DOWN:
			aChar = GLUT_KEY_PAGE_DOWN;
			goto specialLabel;
		case B_HOME:
			aChar = GLUT_KEY_HOME;
			goto specialLabel;
		case B_END:
			aChar = GLUT_KEY_END;
			goto specialLabel;
		case B_INSERT:
            aChar = GLUT_KEY_INSERT;
specialLabel:	
			if (special) {
				anyevents = specialEvent = true;
				GetMouse(&p,&m_buttons);
				specialKey = aChar;
				specialX = (int)p.x;
				specialY = (int)p.y;
				goto setModifiers;	// set the modifier variable
			}
			return;

		default:
			break;
	}
		
	if (keyboard) {
		anyevents = keybEvent = true;
		GetMouse(&p,&m_buttons);
		key = aChar;
		keyX = (int)p.x;
		keyY = (int)p.y;
setModifiers:
		modifierKeys = 0;
		uint32 beMod = Window()->CurrentMessage()->FindInt32("modifiers");
		if(beMod & B_SHIFT_KEY)
			modifierKeys |= GLUT_ACTIVE_SHIFT;
		if(beMod & B_CONTROL_KEY)
			modifierKeys |= GLUT_ACTIVE_CTRL;
		if(beMod & B_OPTION_KEY) {
			// since the window traps B_COMMAND_KEY, we'll have to settle
			// for the option key.. but we need to get the raw character,
			// not the Unicode-enhanced version
			key = Window()->CurrentMessage()->FindInt32("raw_char");
			modifierKeys |= GLUT_ACTIVE_ALT;
		}
		gBlock.NewEvent();
	}
}

/***********************************************************
 *	CLASS:		GlutWindow
 *
 *	FUNCTION:	MouseDown
 *
 *	DESCRIPTION:  handles mouse and menustatus events
 ***********************************************************/
void GlutWindow::MouseDown(BPoint point)
{
	BGLView::MouseDown(point);
	MouseCheck();
}

/***********************************************************
 *	CLASS:		GlutWindow
 *
 *	FUNCTION:	MouseCheck
 *
 *	DESCRIPTION:  checks for button state changes
 ***********************************************************/
void GlutWindow::MouseCheck()
{
	if (mouseEvent)
		return;		// we already have an outstanding mouse event

	BPoint point;
	uint32 newButtons;
	GetMouse(&point, &newButtons);
	if (m_buttons != newButtons) {
		if (newButtons&B_PRIMARY_MOUSE_BUTTON && !(m_buttons&B_PRIMARY_MOUSE_BUTTON)) {
			button = GLUT_LEFT_BUTTON;
			mouseState = GLUT_DOWN;
		} else if (m_buttons&B_PRIMARY_MOUSE_BUTTON && !(newButtons&B_PRIMARY_MOUSE_BUTTON)) {
			button = GLUT_LEFT_BUTTON;
			mouseState = GLUT_UP;
		} else if (newButtons&B_SECONDARY_MOUSE_BUTTON && !(m_buttons&B_SECONDARY_MOUSE_BUTTON)) {
			button = GLUT_RIGHT_BUTTON;
			mouseState = GLUT_DOWN;
		} else if (m_buttons&B_SECONDARY_MOUSE_BUTTON && !(newButtons&B_SECONDARY_MOUSE_BUTTON)) {
			button = GLUT_RIGHT_BUTTON;
			mouseState = GLUT_UP;
		} else if (newButtons&B_TERTIARY_MOUSE_BUTTON && !(m_buttons&B_TERTIARY_MOUSE_BUTTON)) {
			button = GLUT_MIDDLE_BUTTON;
			mouseState = GLUT_DOWN;
		} else if (m_buttons&B_TERTIARY_MOUSE_BUTTON && !(newButtons&B_TERTIARY_MOUSE_BUTTON)) {
			button = GLUT_MIDDLE_BUTTON;
			mouseState = GLUT_UP;
		}
	} else {
		return;		// no change, return
	}
	m_buttons = newButtons;

	if (mouseState == GLUT_DOWN) {
		BWindow *w = Window();
		GlutMenu *m = __glutGetMenuByNum(menu[button]);
		if (m) {
			if (gState.menuStatus) {
				anyevents = statusEvent = true;
				menuNumber = menu[button];
				menuStatus = GLUT_MENU_IN_USE;
				statusX = (int)point.x;
				statusY = (int)point.y;
				gBlock.NewEvent();
			}		
			BRect bounds = w->Frame();
			point.x += bounds.left;
			point.y += bounds.top;
			GlutPopUp *bmenu = static_cast<GlutPopUp*>(m->CreateBMenu());	// start menu
			bmenu->point = point;
			bmenu->win = this;
			thread_id menu_thread = spawn_thread(MenuThread, "menu thread", B_NORMAL_PRIORITY, bmenu);
			resume_thread(menu_thread);
			return;
		}
	}

	if (mouse) {
		anyevents = mouseEvent = true;
		mouseX = (int)point.x;
		mouseY = (int)point.y;
		modifierKeys = 0;
		uint32 beMod = modifiers();
		if(beMod & B_SHIFT_KEY)
			modifierKeys |= GLUT_ACTIVE_SHIFT;
		if(beMod & B_CONTROL_KEY)
			modifierKeys |= GLUT_ACTIVE_CTRL;
		if(beMod & B_OPTION_KEY) {
			modifierKeys |= GLUT_ACTIVE_ALT;
		}
		gBlock.NewEvent();
	}
}

/***********************************************************
 *	CLASS:		GlutWindow
 *
 *	FUNCTION:	MouseMoved
 *
 *	DESCRIPTION:  handles entry, motion, and passive events
 ***********************************************************/
void GlutWindow::MouseMoved(BPoint point,
						ulong transit, const BMessage *msg)
{
	BGLView::MouseMoved(point,transit,msg);
	
	if(transit != B_INSIDE_VIEW) {
		if (entry) {
			anyevents = entryEvent = true;
			gBlock.NewEvent();
		}
		if (transit == B_ENTERED_VIEW) {
			entryState = GLUT_ENTERED;
			MakeFocus();	// make me the current focus
			__glutSetCursor(cursor);
		} else
			entryState = GLUT_LEFT;
	}
	
	MouseCheck();
	if(m_buttons) {
		if(motion) {
			anyevents = motionEvent = true;
			motionX = (int)point.x;
			motionY = (int)point.y;
			gBlock.NewEvent();
		}
	} else {
		if(passive) {
			anyevents = passiveEvent = true;
			passiveX = (int)point.x;
			passiveY = (int)point.y;
			gBlock.NewEvent();
		}
	}
}

/***********************************************************
 *	CLASS:		GlutWindow
 *
 *	FUNCTION:	FrameResized
 *
 *	DESCRIPTION:  handles reshape event
 ***********************************************************/
void GlutWindow::FrameResized(float width, float height)
{
	BGLView::FrameResized(width, height);
	if (visible) {
		anyevents = reshapeEvent = true;
		m_width = (int)(width)+1;
		m_height = (int)(height)+1;
		gBlock.NewEvent();
	}
}

/***********************************************************
 *	CLASS:		GlutWindow
 *
 *	FUNCTION:	Draw
 *
 *	DESCRIPTION:  handles reshape and display events
 ***********************************************************/
void GlutWindow::Draw(BRect updateRect)
{
	BGLView::Draw(updateRect);
	BRect frame = Frame();
	if (m_width != (frame.Width()+1) || m_height != (frame.Height()+1)) {
		FrameResized(frame.Width(), frame.Height());
	}
	Window()->Lock();
	if (visible) {
		anyevents = displayEvent = true;
		gBlock.NewEvent();
	}
	Window()->Unlock();
}

/***********************************************************
 *	CLASS:		GlutWindow
 *
 *	FUNCTION:	Pulse
 *
 *	DESCRIPTION:  handles mouse up event (MouseUp is broken)
 ***********************************************************/
void GlutWindow::Pulse()
{
	BGLView::Pulse();
	if (m_buttons) {	// if there are buttons pressed
		MouseCheck();
	}
}

/***********************************************************
 *	CLASS:		GlutWindow
 *
 *	FUNCTION:	ErrorCallback
 *
 *	DESCRIPTION:  handles GL error messages
 ***********************************************************/
void GlutWindow::ErrorCallback(GLenum errorCode) {
	__glutWarning("GL error: %s", gluErrorString(errorCode));
}

/***********************************************************
 *	CLASS:		GlutWindow
 *
 *	FUNCTION:	MenuThread
 *
 *	DESCRIPTION:  a new thread to launch popup menu, wait
 *			wait for response, then clean up afterwards and
 *			send appropriate messages
 ***********************************************************/
long GlutWindow::MenuThread(void *m) {
	GlutPopUp *bmenu = static_cast<GlutPopUp*>(m);
	GlutWindow *win = bmenu->win;	// my window
	GlutBMenuItem *result = (GlutBMenuItem*)bmenu->Go(bmenu->point);
	win->Window()->Lock();
	win->anyevents = win->statusEvent = true;
	win->menuStatus = GLUT_MENU_NOT_IN_USE;
	win->menuNumber = bmenu->menu;
	BPoint cursor;
	uint32 buttons;
	win->GetMouse(&cursor, &buttons);
	win->statusX = (int)cursor.x;
	win->statusY = (int)cursor.y;
	if(result && result->menu) {
		win->menuEvent = true;
		win->menuNumber = result->menu;  // in case it was a submenu
		win->menuValue = result->value;
	}
	win->Window()->Unlock();
	gBlock.NewEvent();
	delete bmenu;
	return 0;
}
