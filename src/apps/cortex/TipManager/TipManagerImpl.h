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


// TipManagerImpl.h
// e.moon 13may99
//
// HISTORY
//   13may99	e.moon		moved TipManager internals here

#ifndef __TIPMANAGERIMPL_H__
#define __TIPMANAGERIMPL_H__

#include <set>
#include <list>
#include <utility>

#include <Debug.h>
#include <SupportDefs.h>
#include <Font.h>
#include <Point.h>
#include <Rect.h>
#include <GraphicsDefs.h>
#include <String.h>
#include <View.h>

class BWindow;
class BMessageRunner;

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// tip_entry
// -------------------------------------------------------- //

class tip_entry {
public:													// *** interface
	// if rect is invalid, the entry represents the view's entire
	// frame area
	tip_entry(
		BRect												_rect,
		const char*									_text,
		TipManager::offset_mode_t		_offsetMode,
		BPoint											_offset,
		uint32											_flags) :
			
		rect(_rect),
		text(_text),
		offsetMode(_offsetMode),
		offset(_offset),
		flags(_flags) {}
		
	// useful for comparisons
	tip_entry(
		BRect												_rect) :
		rect(_rect) {}

	// give the compiler some slack:
	tip_entry() {}
	
	void dump(int indent) {
		BString s;
		s.SetTo('\t', indent);
		PRINT((
			"%stip_entry '%s'\n",
			s.String(),
			text.String()));
	}
		
	// +++++ copy ctors and other treats for lame compilers go here +++++
		
	// members
	BRect													rect;		
	BString												text;
		
	TipManager::offset_mode_t			offsetMode;
	BPoint 												offset;
		
	uint32 												flags;
};

// ordering criteria: by top-left corner of the rect,
//                    invalid rects last (unsorted)
// [13may99: inverted -- which I suppose causes elements to
//           be stored in increasing 'order', and therefore
//           position]

// [e.moon 13oct99] tip_entry instances now stored and compared
// by pointer

class tip_entry_ptr_less_fn {		public:
	bool operator()(
		const tip_entry*						a,
		const tip_entry*						b) const {
		if(a->rect.IsValid())
			if(b->rect.IsValid()) {
				if(a->rect.left == b->rect.left)
					return a->rect.top > b->rect.top;
				return a->rect.left > b->rect.left;
			}
			else
				return true;
		else
			return false;		
	}
};

typedef std::set<tip_entry*, tip_entry_ptr_less_fn > tip_entry_set;

// -------------------------------------------------------- //
// _ViewEntry
// -------------------------------------------------------- //

class _ViewEntry {
public:													// *** interface

	virtual ~_ViewEntry();
	_ViewEntry() {}
	_ViewEntry(
		BView*											target,
		_ViewEntry*									parent) :
		m_target(target),
		m_parent(parent) {}

	// +++++ copy ctors and other treats for lame compilers go here +++++

	// add the given entry for the designated view
	// (which may be the target view or a child.)
	// returns B_OK on success, B_ERROR if:
	// - the given view is NOT a child of the target view
	// - or if tips can't be added to this view due to it,
	//   or one of its parents, having a full-frame tip.
	
	status_t add(
		BView*											view,
		const tip_entry&						entry);

	// remove tip matching the given rect's upper-left corner or
	// all tips if rect is invalid.
	// returns B_ERROR on failure -- if there are no entries for
	// the given view -- or B_OK otherwise.

	status_t remove(
		BView*											view,
		const BRect&								rect);
		
	// match the given point (in target's view coordinates)
	// against tips in this view and child views.

	std::pair<BView*, const tip_entry*> match(
		BPoint											point,
		BPoint											screenPoint);

	// retrieve current frame rect (in parent view's coordinates)
	BRect Frame();
		
	BView* target() const { return m_target; }
	_ViewEntry* parent() const { return m_parent; }
	
	size_t countTips() const;
	
	void dump(int indent);

private:
	// returns pointer to sole entry in the set if it's
	// a full-frame tip, or 0 if there's no full-frame tip
	const tip_entry* fullFrameTip() const;

	// members
	BView*												m_target;
	_ViewEntry*										m_parent;
	
	// [e.moon 13oct99] child view list now stores pointers
	std::list<_ViewEntry*>							m_childViews;
	tip_entry_set									m_tips;
};

// -------------------------------------------------------- //
// _WindowEntry
// -------------------------------------------------------- //

class _WindowEntry {
public:													// *** interface

	virtual ~_WindowEntry();
	_WindowEntry() {}
	_WindowEntry(
		BWindow*										target) :
		m_target(target) {}

	// add the given entry for the designated view (which must
	// be attached to the target window)
	// returns B_OK on success, B_ERROR if:
	// - the given view is NOT attached to the target window, or
	// - tips can't be added to this view due to it, or one of its
	//   parents, having a full-frame tip.
	
	status_t add(
		BView*											view,
		const tip_entry&						entry);

	// remove tip matching the given rect's upper-left corner or
	// all tips if rect is invalid.
	// returns B_ERROR on failure -- if there are no entries for
	// the given view -- or B_OK otherwise.

	status_t remove(
		BView*											view,
		const BRect&								rect);
		
	// match the given point (in screen coordinates)
	// against tips in this view and child views.

	std::pair<BView*, const tip_entry*> match(
		BPoint											screenPoint);

	BWindow* target() const { return m_target; }
	
	size_t countViews() const { return m_views.size(); }
	
	void dump(int indent);

private:
	// the target window
	BWindow*											m_target;

	// view subtrees with tip entries
	std::list<_ViewEntry*>							m_views;
};

// -------------------------------------------------------- //
// _TipManagerView
// -------------------------------------------------------- //

class _TipManagerView :
	public	BView {
	typedef	BView _inherited;

public:													// *** messages
	enum message_t {
		// sent 
		M_TIME_PASSED
	};

public:													// *** ctor/dtor
	virtual ~_TipManagerView();
	_TipManagerView(
		TipWindow*									tipWindow,
		TipManager*									manager,
		bigtime_t										updatePeriod,
		bigtime_t										idlePeriod);

public:													// *** operations

	// Prepare a 'one-off' tip: this interrupts normal mouse-watching
	// behavior while the mouse remains in the given screen rectangle.
	// If it idles long enough, the tip window is displayed.

	status_t armTip(
		const BRect&								screenRect,
		const char*									text,
		TipManager::offset_mode_t		offsetMode,
		BPoint											offset,
		uint32 											flags);
		
	// Hide tip corresponding to the given screenRect, if any.
	// [e.moon 29nov99] Cancel 'one-off' tip for the given rect if any.
	
	status_t hideTip(
		const BRect&								screenRect);

	// Add/replace a tip in the window/view-entry tree

	status_t setTip(
		const BRect&								rect,
		const char*									text,
		BView*											view,
		TipManager::offset_mode_t		offsetMode,
		BPoint											offset,
		uint32 											flags);

	// Remove a given tip from a particular view in the entry tree
	// (if the rect is invalid, removes all tips for that view.)
	
	status_t removeTip(
		const BRect&								rect,
		BView*											view);

	// Remove all tips for the given window from the entry tree.

	status_t removeAll(
		BWindow*										window);

public:													// *** BView

	void AttachedToWindow();

	void KeyDown(
		const char*									bytes,
		int32												count);

	void MouseDown(
		BPoint											point);

	void MouseMoved(
		BPoint											point,
		uint32											orientation,
		const BMessage*							dragMessage);

public:													// *** BHandler

	void MessageReceived(
		BMessage*										message);

private:												// implementation

	// the tip window to be displayed
	TipWindow*										m_tipWindow;

	// owner
	TipManager*										m_manager;
	
	// set of window entries containing one or more bound tip rectangles
	std::list<_WindowEntry*>						m_windows;
	
	// update message source
	BMessageRunner*								m_messageRunner;

	// window state
	enum tip_window_state_t {
		TIP_WINDOW_HIDDEN,
		TIP_WINDOW_VISIBLE,
		TIP_WINDOW_ARMED
	};
	tip_window_state_t						m_tipWindowState;

	// how often to check for mouse idleness
	bigtime_t											m_updatePeriod;

	// how long it takes a tip to fire
	bigtime_t											m_idlePeriod;

	// mouse-watching state
	BPoint												m_lastMousePoint;
	bigtime_t											m_lastEventTime;
	bool													m_triggered;
	
	// once a tip has been shown, it remains visible while the
	// mouse stays within this screen rect
	BRect													m_visibleTipRect;
	
	// armTip() state
	tip_entry*										m_armedTip;

private:

	inline void _timePassed();
	
	inline void _showTip(
		const tip_entry*						entry);

	inline void _hideTip();
};

__END_CORTEX_NAMESPACE
#endif /* __TIPMANAGERIMPL_H__ */
