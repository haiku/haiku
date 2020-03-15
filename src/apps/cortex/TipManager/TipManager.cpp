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


// TipManager.cpp
// e.moon 12may99

#include "TipManager.h"
#include "TipManagerImpl.h"
#include "TipWindow.h"

#include <Autolock.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Region.h>
#include <float.h>

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// constants
// -------------------------------------------------------- //

// static instance (created on first call to TipManager::Instance().)
TipManager* TipManager::s_instance = 0;
BLocker TipManager::s_instanceLock("TipManager::s_instanceLock");

// special point value set to highly improbable position
const BPoint TipManager::s_useDefaultOffset(FLT_MIN, FLT_MIN);

// default tip position
const BPoint TipManager::s_defaultOffset(8.0, 8.0);

const bigtime_t		TipManager::s_defIdleTime		= 750000LL;
const bigtime_t		TipManager::s_sleepPeriod 	= 250000LL;

// -------------------------------------------------------- //
// *** message filter
// -------------------------------------------------------- //

filter_result ignore_quit_key(
	BMessage* message,
	BHandler** target,
	BMessageFilter* filter)
{
	switch(message->what)
	{
		// filter command-Q
		case B_KEY_DOWN:
		{
			if((modifiers() & B_COMMAND_KEY))
			{
				int8 key;
				message->FindInt8("byte", &key);
				if(key == 'q')
					return B_SKIP_MESSAGE;
			}
			break;
		}
	}
	return B_DISPATCH_MESSAGE;
}

// -------------------------------------------------------- //
// *** dtor
// -------------------------------------------------------- //

TipManager::~TipManager() {}

// -------------------------------------------------------- //
// *** singleton access
// -------------------------------------------------------- //

/*static*/
TipManager* TipManager::Instance() {
	BAutolock _l(s_instanceLock);
	if(!s_instance)
		s_instance = new TipManager();

	return s_instance;
}

// kill current instance if any
/*static*/
void TipManager::QuitInstance() {
	BAutolock _l(s_instanceLock);
	if(s_instance) {
		s_instance->Lock();
		s_instance->Quit();
		s_instance = 0;
	}
}

// -------------------------------------------------------- //
// hidden constructor (use Instance() to access
// a single instance)
// -------------------------------------------------------- //

TipManager::TipManager() :

	BWindow(
		BRect(-100,-100,-100,-100),
		"TipManager",
		B_NO_BORDER_WINDOW_LOOK,
		B_FLOATING_ALL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AVOID_FOCUS),
	m_view(0) {

	AddCommonFilter(
		new BMessageFilter(
			B_PROGRAMMED_DELIVERY,
			B_ANY_SOURCE,
			&ignore_quit_key));

	m_view = new _TipManagerView(
		new TipWindow(),
		this,
		s_sleepPeriod,
		s_defIdleTime);
	AddChild(m_view);

	// start the window thread
	Show();
}


// -------------------------------------------------------- //
// add and remove tips
// -------------------------------------------------------- //

// add or modify a tip:

status_t TipManager::setTip(
	const BRect&			rect,
	const char*				text,
	BView*						view,
	offset_mode_t			offsetMode	/*=LEFT_OFFSET_FROM_RECT*/,
	BPoint						offset			/*=s_useDefaultOffset*/,
	uint32 						flags				/*=NONE*/) {

	ASSERT(text);
	ASSERT(m_view);

	BAutolock _l(this);
	return m_view->setTip(
		rect, text, view, offsetMode, offset, flags);
}


status_t TipManager::setTip(
	const char*				text,
	BView*						view,
	offset_mode_t			offsetMode	/*=LEFT_OFFSET_FROM_RECT*/,
	BPoint						offset			/*=s_useDefaultOffset*/,
	uint32 						flags				/*=NONE*/) {

	return setTip(
		BRect(), text, view, offsetMode, offset, flags);
}

// Remove all tips matching the given rectangle and/or child
// view.  Returns the number of tips removed.

status_t TipManager::removeTip(
	const BRect&		rect,
	BView*					view) {

	ASSERT(view);
	ASSERT(m_view);

	BAutolock _l(this);
	return m_view->removeTip(rect, view);
}

// If more than one tip is mapped to pChild, all are removed:

status_t TipManager::removeAll(
	BView*					view) {

	return removeTip(BRect(), view);
}

status_t TipManager::removeAll(
	BWindow*				window) {

//	PRINT((
//		"### TipManager::removeAll(): %p, %p\n", this, m_view->Looper()));

	ASSERT(window);
	ASSERT(m_view);
	ASSERT(m_view->Looper() == this); // +++++

	BAutolock _l(this);
	return m_view->removeAll(window);
}

// -------------------------------------------------------- //
// *** manual tip arming
// -------------------------------------------------------- //

// [e.moon 19oct99]
// Call when the mouse has entered a particular region of
// the screen for which you want a tip to be displayed.
// The tip will be displayed if the mouse stops moving
// for idleTime microseconds within the rectangle screenRect.

status_t TipManager::showTip(
	const char*						text,
	BRect									screenRect,
	offset_mode_t					offsetMode	/*=LEFT_OFFSET_FROM_RECT*/,
	BPoint								offset			/*=s_useDefaultOffset*/,
	uint32 								flags				/*=NONE*/) {

	ASSERT(text);
	ASSERT(m_view);

	BAutolock _l(this);
	return m_view->armTip(
	  screenRect, text, offsetMode, offset, flags);
}

// [e.moon 22oct99]
// Call to immediately hide a visible tip.  You need to know
// the screen rectangle for which the tip was shown (which is easy
// if was displayed due to a showTip() call -- pass the same
// screenRect argument.)
// If the tip was found & hidden, returns B_OK; if there's
// no visible tip or it was triggered by a different rectangle,
// returns B_BAD_VALUE.

status_t TipManager::hideTip(
	BRect									screenRect) {

	ASSERT(m_view);

	BAutolock _l(this);
	return m_view->hideTip(screenRect);
}


// -------------------------------------------------------- //
// *** BWindow
// -------------------------------------------------------- //

// -------------------------------------------------------- //
// *** BLooper
// -------------------------------------------------------- //

bool TipManager::QuitRequested() {
	// ignored, since I receive key events bound for other apps
	return false;
}

// -------------------------------------------------------- //
// *** BHandler
// -------------------------------------------------------- //

void TipManager::MessageReceived(
	BMessage*							message) {

	switch(message->what) {
		default:
			_inherited::MessageReceived(message);
	}
}

//// -------------------------------------------------------- //
//// BasicThread impl.
//// -------------------------------------------------------- //
//
//// +++++
//// 12aug99: a locking bug seems to cause occasional
////          crashes on shutdown after the looper's been deleted.
//// +++++
//// 23sep99: probably fixed; the TipManager needs to be manually
////          killed before the window is deleted.
//
//void TipManager::run() {
//
//	BPoint point, lastPoint, screenPoint;
//	bigtime_t curTime, lastTime;
//	uint32 buttons;
//
//	bool bTipVisible = false;
//	BRect tipScreenRect;
//
//	lastTime = 0;
//	curTime = 0;
//
//	// [e.moon 27sep99]
//	// store whether the tip has fired at the current point
//	bool fired = false;
//
//	ASSERT(m_tree);
//	BView* pOwningView = m_tree->target();
//
//	while(!stopping()) {
//		snooze(s_sleepPeriod);
//		if(stopping())
//			break;
//
//		// wait for the view to show up
//		if(!pOwningView->Parent() || !pOwningView->Window())
//			continue;
//
//		// get current mouse position
//		pOwningView->LockLooper();
//
//		pOwningView->GetMouse(&point, &buttons, false);
//		screenPoint = pOwningView->ConvertToScreen(point);
//
//		pOwningView->UnlockLooper();
//
//		// has it been sitting in one place long enough?
//		bool bMoved = (point != lastPoint);
//
//		if(bMoved) {
//			lastTime = curTime;
//			fired = false;
//		}
//		else if(fired) {
//			// [27sep99 e.moon] the tip has already fired, and
//			// the mouse hasn't moved; bail out now
//			continue;
//		}
//
//		curTime = system_time();
//		bool bIdle = !bMoved && lastTime && (curTime - lastTime) > m_idleTime;
//		lastPoint = point;
//
//		if(bTipVisible) {
//			// hide tip once mouse moves outside its rectangle
//			if(!tipScreenRect.Contains(screenPoint)) {
//				m_tipWindow->Lock();
//				if(!m_tipWindow->IsHidden()) // tip may hide itself [7sep99]
//					m_tipWindow->Hide();
//				bTipVisible = false;
//				m_tipWindow->Unlock();
//			}
//		} else if(bIdle) {
//
//			// mouse has idled at a given point long enough;
//			// look for a tip at that position and display one if found:
//
//			fired = true;
//
//			pOwningView->LockLooper();
//
//			// make sure this part of the view is actually visible
//			if(!pOwningView->Window()->Frame().Contains(screenPoint)) {
//				pOwningView->UnlockLooper();
//				continue;
//			}
//
//			// look for a tip under the mouse
//			m_tipWindow->Lock();
//			pair<BView*, const tip_entry*> found =
//				m_tree->match(point, screenPoint);
//
//			if(!found.second) {
//				// none found; move on
//				pOwningView->UnlockLooper();
//				m_tipWindow->Unlock();
//				continue;
//			}
//
//			BView* pTipTarget = found.first;
//			const tip_entry& entry = *found.second;
//
//			// test the screen point against the view's clipping region;
//			// if no match, the given point is likely covered by another
//			// window (so stop recursing)
//
//			BRegion clipRegion;
//			pTipTarget->GetClippingRegion(&clipRegion);
//			if(!clipRegion.Contains(
//				pTipTarget->ConvertFromScreen(screenPoint))) {
//				// move on
//				pOwningView->UnlockLooper();
//				m_tipWindow->Unlock();
//				continue;
//			}
//
//			// found one; set up the tip window:
//			BRect entryFrame = pTipTarget->ConvertToScreen(entry.rect);
//
//			// set text (this has the side effect of resizing the
//			// window)
//
//			ASSERT(m_tipWindow);
//			m_tipWindow->setText(entry.text.String());
//
//			// figure out where to display it:
//
//			BPoint offset = (entry.offset == s_useDefaultOffset) ?
//				s_defaultOffset :
//				entry.offset;
//
//			BPoint p;
//			switch(entry.offsetMode) {
//				case LEFT_OFFSET_FROM_RECT:
//					p = entryFrame.RightTop() + offset;
//					break;
//				case LEFT_OFFSET_FROM_POINTER:
//					p = screenPoint + offset;
//					break;
//				case RIGHT_OFFSET_FROM_RECT:
//					p = entryFrame.LeftTop();
//					p.x -= offset.x;
//					p.y += offset.y;
//					p.x -= m_tipWindow->Frame().Width();
//					break;
//				case RIGHT_OFFSET_FROM_POINTER:
//					p = screenPoint;
//					p.x -= offset.x;
//					p.y += offset.y;
//					p.x -= m_tipWindow->Frame().Width();
//					break;
//				default:
//					ASSERT(!"bad offset mode");
//			}
//
//			// do it:
//
//			m_tipWindow->MoveTo(p);
//			m_tipWindow->Show();
//
//			bTipVisible = true;
//			tipScreenRect = entryFrame;
//
//			m_tipWindow->Unlock();
//			pOwningView->UnlockLooper();
//
//		} // if(bIdle ...
//	} // while(!stopping ...
//}

// END -- TipManager.cpp --

