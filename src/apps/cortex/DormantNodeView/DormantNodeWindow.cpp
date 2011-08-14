/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// DormantNodeWindow.cpp
// e.moon 2jun99

#include "DormantNodeWindow.h"
// DormantNodeView
#include "DormantNodeView.h"

#include "RouteWindow.h"

// Application Kit
#include <Application.h>
// Interface Kit
#include <Screen.h>
#include <ScrollBar.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_ALLOC(x)	//PRINT (x)		// ctor/dtor
#define D_HOOK(x) //PRINT (x)		// BWindow impl.
#define D_MESSAGE(x) //PRINT (x)	// MessageReceived()
#define D_INTERNAL(x) //PRINT (x)	// internal operations

// -------------------------------------------------------- //
// constants
// -------------------------------------------------------- //

// this should be a bit more sophisticated :)
const BRect DormantNodeWindow::s_initFrame(500.0, 350.0, 640.0, 480.0);

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

DormantNodeWindow::DormantNodeWindow(
	BWindow* parent)
	: BWindow(s_initFrame, "Media add-ons",
			  B_FLOATING_WINDOW_LOOK,
			  B_FLOATING_SUBSET_WINDOW_FEEL,
			  B_WILL_ACCEPT_FIRST_CLICK|B_AVOID_FOCUS|B_ASYNCHRONOUS_CONTROLS),
	  m_parent(parent),
	  m_zoomed(false),
	  m_zooming(false) {
	D_ALLOC(("DormantNodeWindow::DormantNodeWindow()\n"));

	ASSERT(m_parent);
	AddToSubset(m_parent);

	// Create the ListView
	BRect r = Bounds();
	r.right -= B_V_SCROLL_BAR_WIDTH;
	m_listView = new DormantNodeView(r, "Dormant Node ListView", B_FOLLOW_ALL_SIDES);

	// Add the vertical ScrollBar
	r.left = r.right + 1.0;
	r.right = r.left + B_V_SCROLL_BAR_WIDTH;
	r.InsetBy(0.0, -1.0);
	BScrollBar *scrollBar;
	AddChild(scrollBar = new BScrollBar(r, "", m_listView, 0.0, 0.0, B_VERTICAL));

	// Add the ListView
	AddChild(m_listView);
	_constrainToScreen();
}

DormantNodeWindow::~DormantNodeWindow() {
	D_ALLOC(("DormantNodeWindow::~DormantNodeWindow()\n"));

}

// -------------------------------------------------------- //
// BWindow impl.
// -------------------------------------------------------- //

bool DormantNodeWindow::QuitRequested() {
	D_HOOK(("DormantNodeWindow::QuitRequested()\n"));

	// [e.moon 29nov99] the RouteWindow is now responsible for
	// closing me
	m_parent->PostMessage(RouteWindow::M_TOGGLE_DORMANT_NODE_WINDOW);	
	return false;
}

void DormantNodeWindow::Zoom(
	BPoint origin,
	float width,
	float height) {
	D_HOOK(("DormantNodeWindow::Zoom()\n"));

	m_zooming = true;

	BScreen screen(this);
	if (!screen.Frame().Contains(Frame())) {
		m_zoomed = false;
	}

	if (!m_zoomed) {
		// resize to the ideal size
		m_manualSize = Bounds();
		m_listView->GetPreferredSize(&width, &height);
		ResizeTo(width + B_V_SCROLL_BAR_WIDTH, height);
		m_zoomed = true;
		_constrainToScreen();
	}
	else {
		// resize to the most recent manual size
		ResizeTo(m_manualSize.Width(), m_manualSize.Height());
		m_zoomed = false;
	}
}

// -------------------------------------------------------- //
// internal operations
// -------------------------------------------------------- //

void DormantNodeWindow::_constrainToScreen() {
	D_INTERNAL(("DormantNodeWindow::_constrainToScreen()\n"));
	
	BScreen screen(this);
	BRect screenRect = screen.Frame();
	BRect windowRect = Frame();

	// if the window is outside the screen rect
	// move it to the default position
	if (!screenRect.Intersects(windowRect)) {
		windowRect.OffsetTo(screenRect.LeftTop());
		MoveTo(windowRect.LeftTop());
		windowRect = Frame();
	}

	// if the window is larger than the screen rect
	// resize it to fit at each side
	if (!screenRect.Contains(windowRect)) {
		if (windowRect.left < screenRect.left) {
			windowRect.left = screenRect.left + 5.0;
			MoveTo(windowRect.LeftTop());
			windowRect = Frame();
		}
		if (windowRect.top < screenRect.top) {
			windowRect.top = screenRect.top + 5.0;
			MoveTo(windowRect.LeftTop());
			windowRect = Frame();
		}
		if (windowRect.right > screenRect.right) {
			windowRect.right = screenRect.right - 5.0;
		}
		if (windowRect.bottom > screenRect.bottom) {
			windowRect.bottom = screenRect.bottom - 5.0;
		}
		ResizeTo(windowRect.Width(), windowRect.Height());
	}
}

// END -- DormantNodeWindow.cpp --
