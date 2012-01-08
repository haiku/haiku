/***********************************************************
 *      Copyright (C) 1997, Be Inc.  Copyright (C) 1999, Jake Hamby.
 *
 * This program is freely distributable without licensing fees
 * and is provided without guarantee or warrantee expressed or
 * implied. This program is -not- in the public domain.
 *
 *	DESCRIPTION:	all the routines for dealing with GlutWindows
 ***********************************************************/

#include <stdlib.h>

#include <GL/glut.h>

#include "glutint.h"
#include "glutState.h"
#include "glutBlocker.h"


/*!	Helper function to get a new window slot */
static int
getUnusedWindowSlot()
{
	int i;

	/* Look for allocated, unused slot. */
	for (i = 0; i < gState.windowListSize; i++) {
		if (!gState.windowList[i])
			return i;
	}

	/* Allocate a new slot. */
	gState.windowListSize++;
	gState.windowList = (GlutWindow **)realloc(gState.windowList,
		gState.windowListSize * sizeof(GlutWindow *));
	if (gState.windowList == NULL)
		__glutFatalError("out of memory.");

	gState.windowList[gState.windowListSize - 1] = NULL;
	return gState.windowListSize - 1;
}


/*!	Default display function */
static void
__glutDefaultDisplay(void)
{
	/* XXX Remove the warning after GLUT 3.0. */
	__glutWarning("The following is a new check for GLUT 3.0; update your "
		"code.");
	__glutFatalError("redisplay needed for window %d, but no display callback.",
		gState.currentWindow->num + 1);
}


/*!	Default reshape function */
void
__glutDefaultReshape(int width, int height)
{
	/* Adjust the viewport of the window */
	glViewport(0, 0, (GLsizei) width, (GLsizei) height);
}


//	#pragma mark -


/*!	Creates a new GLUT window
	Note: subwindows don't resize, but top-level windows
	follow all sides.
*/
GlutWindow::GlutWindow(GlutWindow *nparent, const char *name,
		int x, int y, int width, int height, ulong options)
	: BGLView(nparent != NULL ? BRect(x, y, x + width - 1, y + height - 1)
			: BRect(0, 0, width - 1, height - 1), name,
		nparent != NULL ? B_FOLLOW_NONE : B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE | B_PULSE_NEEDED,
		options)
{
	// add myself to window list
	num = getUnusedWindowSlot();
	gState.windowList[num] = this;

	// set up parent/children relationships
	parent = nparent;
	if (parent) {
		siblings = parent->children;
		parent->children = this;
	} else {
		siblings = 0;
	}
	children = 0;

	// initialize variables
	cursor = GLUT_CURSOR_INHERIT;	// default cursor
	for (int i = 0; i < GLUT_MAX_MENUS; i++) {
		menu[i] = 0;
	}
	m_width = width;
	m_height = height;
	m_buttons = 0;

	// clear callbacks
	display = __glutDefaultDisplay;
	reshape = __glutDefaultReshape;
	mouse = NULL;
	motion = NULL;
	passive = NULL;
	entry = NULL;
	keyboard = NULL;
	keyboardUp = NULL;
	visibility = NULL;
	special = NULL;
	specialUp = NULL;
	windowStatus = NULL;

	// clear event counters
	anyevents = 1;
	displayEvent = 1;	// get a reshape and a display event right away
	reshapeEvent = 1;
	mouseEvent = 0;
	motionEvent = 0;
	passiveEvent = 0;
	entryEvent = 0;
	keybEvent = 0;
	keybUpEvent = 0;
	windowStatusEvent = 0; // DirectConnected() will report change in
	visState = -1;         // visibility
	specialEvent = 0;
	specialUpEvent = 0;
	statusEvent = 0;
	menuEvent = 0;
	visible = true;
	ignoreKeyRepeat = (gState.keyRepeatMode == GLUT_KEY_REPEAT_OFF);

	gBlock.QuickNewEvent();

	// if i'm a subwindow, add me to my parent view
	if (parent) {
		parent->Window()->Lock();
		parent->AddChild(this);
		parent->Window()->Unlock();
	} else {
		// if I'm a top-level window, create my BWindow
		GlutBWindow *mybwindow = new GlutBWindow(
			BRect(x, y, x + width - 1, y + height - 1), name);
		mybwindow->AddChild(this);
		mybwindow->bgl = this;
		mybwindow->Show();
	}

	// give me the keyboard focus (focus follows mouse, X style, as
	// implemented in GlutWindow::MouseMoved())
	Window()->Lock();
	MakeFocus();
	Window()->Unlock();

	// make myself the default window
	__glutSetWindow(this);
}


/*!	Creates a new GLUT window */
int
glutCreateWindow(const char *name)
{
	if (!be_app)
		__glutInit();

	ulong options;
	if (!__glutConvertDisplayMode(&options)) {
		__glutWarning("visual with necessary capabilities not found.");
	}

	// if X or Y is negative, then start at a reasonable position
	bool defaultxy = gState.initX < 0 || gState.initY < 0;

	GlutWindow *window = new GlutWindow(0, name,
		defaultxy ? 50 : gState.initX, defaultxy ? 50 : gState.initY,
		gState.initWidth, gState.initHeight, options);

	return window->num + 1;
}


/*!	Creates a new GLUT subwindow
	Note: a subwindow is a GlutWindow (which is actually
	a BGLView) without its own BWindow
*/
int
glutCreateSubWindow(int win, int x, int y, int width, int height)
{
	ulong options;
	if (!__glutConvertDisplayMode(&options)) {
		__glutFatalError("visual with necessary capabilities not found.");
	}

	GlutWindow *window = new GlutWindow(gState.windowList[win-1], "child",
		x, y, width, height, options);

	return window->num + 1;
}


/*!	Set the current window (utility function) */
void
__glutSetWindow(GlutWindow *window)
{
	if (gState.currentWindow)
		gState.currentWindow->UnlockGL();
	gState.currentWindow = window;
	gState.currentWindow->LockGL();
}


/*!	Set and get the current window */
void
glutSetWindow(int win)
{
	GlutWindow *window;

	if (win < 1 || win > gState.windowListSize) {
		__glutWarning("glutSetWindow attempted on bogus window.");
		return;
	}

	window = gState.windowList[win - 1];
	if (!window) {
		__glutWarning("glutSetWindow attempted on bogus window.");
		return;
	}
	__glutSetWindow(window);
}


int
glutGetWindow()
{
	if (gState.currentWindow)
		return gState.currentWindow->num + 1;

	return 0;
}


/*!	Recursively set entries to 0. */
static void
__glutDestroyWindow(GlutWindow *window, GlutWindow *initialWindow)
{
	// first, find all children recursively and set their entries to 0
	GlutWindow *cur = window->children;
	while (cur) {
		GlutWindow *siblings = cur->siblings;
		__glutDestroyWindow(cur, initialWindow);
		cur = siblings;
	}

	/* Remove from parent's children list (only necessary for
	non-initial windows and subwindows!). */
	GlutWindow *parent = window->parent;
	if (parent && parent == initialWindow->parent) {
		GlutWindow **prev = &parent->children;
		cur = parent->children;
		while (cur) {
			if (cur == window) {
				*prev = cur->siblings;
				break;
			}
			prev = &(cur->siblings);
			cur = cur->siblings;
		}
	}

	// finally, check if we are the current window, and set to 0
	if (gState.currentWindow == window)
		gState.currentWindow = 0;

	gState.windowList[window->num] = 0;
}


/*!	Destroy window and all its children. */
void
glutDestroyWindow(int win)
{
	// can't destroy a window if another window has the GL context
	if (gState.currentWindow)
		gState.currentWindow->UnlockGL();

	// lock the window
	GlutWindow *window = gState.windowList[win - 1];
	BWindow *bwindow = window->Window();
	bwindow->Lock();

	// if win is the current window, set current window to 0
	if (gState.currentWindow == window)
		gState.currentWindow = 0;

	// recursively set child entries to 0
	__glutDestroyWindow(window, window);

	// try flushing OpenGL
	window->LockGL();
	glFlush();
	window->UnlockGL();

	// now, if the window was top-level, delete its BWindow
	if (!window->parent) {
		bwindow->Quit();
	} else {
		// else, detach it from the BWindow and delete it
		window->RemoveSelf();
		delete window;
		bwindow->Unlock();
	}

	// relock GL if the current window is still valid
	if (gState.currentWindow)
		gState.currentWindow->LockGL();
}


/*!	Destroy all windows when exit() is called this seems to be necessary
	to avoid delays and crashes when using BDirectWindow.
*/
void
__glutDestroyAllWindows()
{
	for (int i = 0; i < gState.windowListSize; i++) {
		if (gState.windowList[i])
			glutDestroyWindow(i + 1);
	}
	gState.display->Lock();
	gState.display->Quit();
	gState.display->Unlock();

	status_t ignored;
	wait_for_thread(gState.appthread, &ignored);
}


/*!	Mark window as needing redisplay */
void
glutPostRedisplay()
{
	gState.currentWindow->Window()->Lock();
	gState.currentWindow->anyevents = true;
	gState.currentWindow->displayEvent = true;
	gState.currentWindow->Window()->Unlock();
	gBlock.QuickNewEvent();
}


/*!	Mark window as needing redisplay */
void
glutPostWindowRedisplay(int win)
{
	GlutWindow *gwin = gState.windowList[win - 1];
	gwin->Window()->Lock();
	gwin->anyevents = true;
	gwin->displayEvent = true;
	gwin->Window()->Unlock();
	gBlock.QuickNewEvent();
}


void
glutSwapBuffers()
{
	gState.currentWindow->SwapBuffers();
}


/*!	Move window */
void
glutPositionWindow(int x, int y)
{
	BDirectWindow *win = dynamic_cast<BDirectWindow*>(gState.currentWindow->Window());
	if (win == NULL)
		return;

	win->Lock();
	if (gState.currentWindow->parent)
		gState.currentWindow->MoveTo(x, y);	// move the child view
	else {
		if (win->IsFullScreen())
			win->SetFullScreen(false);

		win->MoveTo(x, y);  // move the window
	}
	win->Unlock();
}


/*!	Reshape window (we'll catch the callback when the view gets
	a Draw() message).
*/
void
glutReshapeWindow(int width, int height)
{
	BDirectWindow *win = dynamic_cast<BDirectWindow*>(gState.currentWindow->Window());
	if (win == NULL)
		return;

	win->Lock();
	if (gState.currentWindow->parent) {
		// resize the child
		gState.currentWindow->ResizeTo(width - 1, height - 1);
	} else {
		if (win->IsFullScreen())
			win->SetFullScreen(false);

		// resize the parent
		win->ResizeTo(width - 1, height - 1);
	}
	win->Unlock();
}


/*!	Makes the window full screen */
void
glutFullScreen()
{
	BDirectWindow *win = dynamic_cast<BDirectWindow*>(gState.currentWindow->Window());
	if (win == NULL)
		return;

	win->Lock();
	win->SetFullScreen(true);
	win->Unlock();
}


/*!	Supposed to change the stacking order of the current window
	NOTE: I can't figure out how to do this for windows,
		and there is no concept of "stacking order" for
		subwindows, so these are currently no-ops.
*/
void
glutPopWindow()
{
}


/*!	Same problem as glutPopWindow() */
void
glutPushWindow()
{
}


void
glutShowWindow()
{
	gState.currentWindow->Window()->Lock();
	if (gState.currentWindow->parent)	// subwindow
		gState.currentWindow->Show();
	else {
		if(gState.currentWindow->Window()->IsHidden())
			gState.currentWindow->Window()->Show();	// show the actual BWindow
		gState.currentWindow->Window()->Minimize(false);
	}
	gState.currentWindow->Window()->Unlock();
}


void
glutHideWindow()
{
	gState.currentWindow->Window()->Lock();

	if (gState.currentWindow->parent)	// subwindow
		gState.currentWindow->Hide();
	else
		gState.currentWindow->Window()->Hide();	// show the actual BWindow

	gState.currentWindow->Window()->Unlock();
}


/*!	Minimizes window */
void
glutIconifyWindow()
{
	if (gState.currentWindow->parent)
		__glutFatalError("can't iconify a subwindow");

	gState.currentWindow->Window()->Lock();
	gState.currentWindow->Window()->Minimize(true);
	gState.currentWindow->Window()->Unlock();
}


/*!	Sets the window title */
void
glutSetWindowTitle(const char *name)
{
	if (gState.currentWindow->parent)
		__glutFatalError("glutSetWindowTitle: isn't a top-level window");

	gState.currentWindow->Window()->Lock();
	gState.currentWindow->Window()->SetTitle(name);
	gState.currentWindow->Window()->Unlock();
}


/*!	Same as glutSetWindowTitle() */
void
glutSetIconTitle(const char *name)
{
	glutSetWindowTitle(name);
}


/*!	Converts the current display mode into a BGLView
	display mode, printing warnings as appropriate.

	\param options if non-NULL, the current display mode is
		returned in it.
	\return 1 if the current display mode is possible, else 0
*/
int
__glutConvertDisplayMode(unsigned long *options)
{
	if (gState.displayString) {
		/* __glutDisplayString should be NULL except if
       glutInitDisplayString has been called to register a
       different display string.  Calling glutInitDisplayString
       means using a string instead of an integer mask determine
       the visual to use.  This big ugly code is in glutDstr.cpp */
       return __glutConvertDisplayModeFromString(options);
    }

	if (options) {
		ulong newoptions = 0;
		if (gState.displayMode & GLUT_ACCUM)
			newoptions |= BGL_ACCUM;
		if (gState.displayMode & GLUT_ALPHA)
			newoptions |= BGL_ALPHA;
		if (gState.displayMode & GLUT_DEPTH)
			newoptions |= BGL_DEPTH;
		if (gState.displayMode & GLUT_DOUBLE)
			newoptions |= BGL_DOUBLE;
		if (gState.displayMode & GLUT_STENCIL)
			newoptions |= BGL_STENCIL;
		*options = newoptions;
	}

	if (gState.displayMode & GLUT_INDEX) {
		__glutWarning("BeOS doesn't support indexed color");
		return 0;
	}
	if (gState.displayMode & GLUT_MULTISAMPLE) {
		return 1;	// try to go without multisampling
	}
	if (gState.displayMode & GLUT_STEREO) {
		__glutWarning("BeOS doesn't support stereo windows");
		return 0;
	}
	if (gState.displayMode & GLUT_LUMINANCE) {
		__glutWarning("BeOS doesn't support luminance color model");
		return 0;
	}
	return 1;	// visual supported
}


//	#pragma mark -


/*!	Very thin wrapper around BWindow */
GlutBWindow::GlutBWindow(BRect frame, const char *name)
	: BDirectWindow(frame, name, B_TITLED_WINDOW, 0)
{
	fConnectionDisabled = false;
	bgl = 0;
	SetPulseRate(100000);

	if (!SupportsWindowMode()) {
		__glutFatalError("video card doesn't support windowed operation");
	}
}


void
GlutBWindow::DirectConnected(direct_buffer_info *info)
{
	if (!bgl)
		return;

	bgl->DirectConnected(info);
	if (!fConnectionDisabled)
		bgl->EnableDirectMode(true);

	int newVisState;
	if ((info->buffer_state & B_DIRECT_MODE_MASK) == B_DIRECT_START)
		bgl->visible = true;

	if (!bgl->visible || info->buffer_state == B_DIRECT_STOP)
		newVisState = GLUT_HIDDEN;
	else {
		if (info->clip_list_count == 0)
			newVisState = GLUT_FULLY_COVERED;
		else if (info->clip_list_count == 1)
			newVisState = GLUT_FULLY_RETAINED;
		else
			newVisState = GLUT_PARTIALLY_RETAINED;
	}

	if (newVisState != bgl->visState) {
		bgl->visState = newVisState;
		bgl->anyevents = bgl->windowStatusEvent = true;
		gBlock.NewEvent();
	}
}


GlutBWindow::~GlutBWindow()
{
	fConnectionDisabled = true;
	if (bgl)
		bgl->EnableDirectMode(false);

	if (!IsHidden())
		Hide();

	Sync();
}


bool
GlutBWindow::QuitRequested()
{
	gState.quitAll = true;
	gBlock.NewEvent();
	return false;
		// don't quit now, wait for main thread to do it
}


void
GlutBWindow::Minimize(bool minimize)
{
	bgl->visible = !minimize;
	BWindow::Minimize(minimize);
}


void
GlutBWindow::Hide()
{
	BWindow::Hide();
	bgl->visible = false;
}


void
GlutBWindow::Show()
{
	BWindow::Show();
	bgl->visible = true;
}
