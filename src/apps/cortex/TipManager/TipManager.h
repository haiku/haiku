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


// TipManager.h
//
// PURPOSE
//   Maintains a set of pop-up tips bound to rectangular
//   regions of any number of views.  Also provides for
//   simple 'manual' operation: call showTip() with text and
//   a screen rectangle, and the tip will be displayed after
//   the mouse has idled in that rectangle.
//
// HISTORY
//   e.moon 27oct99: Substantial bugfixes (removal of entire
//                   view hierarchies' tips now works).
//
//   e.moon 19oct99: TipManager now derives from BWindow.
//
//   e.moon 17oct99: reworked the tip window: now exposed via the
//                   TipWindow & TipView classes.
//
//   e.moon 27sep99: optimized TipManager::run() (no longer pounds
//                   the CPU when idling in a view)
//
//   e.moon 13may99: moved TipManager's privates into
//                   TipManagerImpl.h
//
//   e.moon 12may99: expanded to TipManager
//   e.moon 11may99: begun as TipTriggerThread

#ifndef __TipManager_H__
#define __TipManager_H__

#include <SupportDefs.h>
#include <Font.h>
#include <Point.h>
#include <Rect.h>
#include <GraphicsDefs.h>
#include <String.h>

#include <Locker.h>
#include <Window.h>

class BView;

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class TipWindow;

class _TipManagerView;
class _ViewEntry;
class _WindowEntry;

class TipManager :
	protected	BWindow {
	typedef		BWindow _inherited;
	
public:	
	static const BPoint			s_useDefaultOffset;
	static const BPoint			s_defaultOffset;

	static const bigtime_t	s_defIdleTime;
	static const bigtime_t	s_sleepPeriod;

public:										// *** types & constants
	enum flag_t {
		NONE
	};
	
	enum offset_mode_t {
		// offset determines left/top point of tip window
		LEFT_OFFSET_FROM_RECT,		// from the right bound
		LEFT_OFFSET_FROM_POINTER,
		
		// offset determines right/top point of tip window
		// (x offset is inverted; y isn't)
		RIGHT_OFFSET_FROM_RECT,		// from the left bound
		RIGHT_OFFSET_FROM_POINTER
	};

public:										// *** dtor
	virtual ~TipManager();

public:										// *** singleton access
	static TipManager* Instance();
	static void QuitInstance();
	
private:									// hidden constructor (use Instance() to access
													// a single instance)
	TipManager();

public:										// *** add and remove tips

	// add or modify a tip:

	// child allows tips to be added to child views of the main
	// target view.  rect is in view coordinates; only one tip
	// may exist for a particular view with a given top-left
	// corner -- you don't want tip rectangles to overlap in general,
	// but TipManager won't stop you from trying.  Yet.
	// [13may99]

	status_t setTip(
		const BRect&					rect,
		const char*						text,
		BView*								view,
		offset_mode_t					offsetMode	=LEFT_OFFSET_FROM_POINTER,
		BPoint								offset			=s_useDefaultOffset,
		uint32 								flags				=NONE);
	
	// This version of setTip() maps a tip to the entire frame
	// rectangle of a child view.  This call will fail if tips
	// are already being managed for that view; once a
	// full-view tip has been added future attempts call any
	// version of setTip() for that view will also fail.
	// [13may99]

	status_t setTip(
		const char*						text,
		BView*								view,
		offset_mode_t					offsetMode	=LEFT_OFFSET_FROM_POINTER,
		BPoint								offset			=s_useDefaultOffset,
		uint32 								flags				=NONE);
		
	// Remove all tips matching the given rectangle and/or child
	// view.
	
	status_t removeTip(
		const BRect&					rect,
		BView*								view);

	status_t removeAll(
		BView*								view);
	
	status_t removeAll(
		BWindow*							window);

public:										// *** manual tip arming

	// [e.moon 19oct99]
	// Call when the mouse has entered a particular region of
	// the screen for which you want a tip to be displayed.
	// The tip will be displayed if the mouse stops moving
	// for idleTime microseconds within the rectangle screenRect.

	status_t showTip(
		const char*						text,
		BRect									screenRect,
		offset_mode_t					offsetMode	=LEFT_OFFSET_FROM_POINTER,
		BPoint								offset			=s_useDefaultOffset,
		uint32 								flags				=NONE);
	
	// [e.moon 22oct99]
	// Call to immediately hide a visible tip.  You need to know
	// the screen rectangle for which the tip was shown (which is easy
	// if was displayed due to a showTip() call -- pass the same
	// screenRect argument.)
	// If the tip was found & hidden, returns B_OK; if there's
	// no visible tip or it was triggered by a different rectangle,
	// returns B_BAD_VALUE.
	
	status_t hideTip(
		BRect									screenRect);

public:										// *** BWindow
		
public:										// *** BLooper

	virtual bool QuitRequested();
	
public:										// *** BHandler

	virtual void MessageReceived(
		BMessage*							message);
	
private:

	// --------------------------------------------------------------- //
	//                           *** GUTS ***
	// --------------------------------------------------------------- //

	// implements TipManager & enjoys direct (non-polling) access to
	// mouse events:
	_TipManagerView*				m_view;
	
	
private:
	static TipManager*			s_instance;
	static BLocker					s_instanceLock;
};

__END_CORTEX_NAMESPACE
#endif /*__TipManager_H__*/
