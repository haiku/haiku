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


// TipManagerImpl.cpp
// e.moon 13may99

#include <algorithm>

#include "TipManager.h"
#include "TipManagerImpl.h"
#include "TipWindow.h"

#include <Autolock.h>
#include <Debug.h>
#include <MessageRunner.h>
#include <Region.h>
#include <Screen.h>

//#include "debug_tools.h"

using namespace std;

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //

// [e.moon 13oct99] now matches entry by pointer
class entry_target_matches_view { public:
	const BView* pView;
	entry_target_matches_view(const BView* p) : pView(p) {}
	bool operator()(const _ViewEntry* view) const {
		return view->target() == pView;
	}
};

// -------------------------------------------------------- //
// _ViewEntry impl.
// -------------------------------------------------------- //

// [e.moon 13oct99] delete tips & children
_ViewEntry::~_ViewEntry() {
	for(list<_ViewEntry*>::iterator it = m_childViews.begin();
		it != m_childViews.end(); ++it) {
		delete *it;
	}
	for(tip_entry_set::iterator it = m_tips.begin();
		it != m_tips.end(); ++it) {
		delete *it;
	}
}

// add the given entry for the designated view
// (which may be the target view or a child.)
// returns B_OK on success, B_ERROR if the given view is
// NOT a child .

status_t _ViewEntry::add(BView* pView, const tip_entry& tipEntry) {

	// walk up the view's parent tree, building a child-
	// hierarchy list and looking for my target view.
	// The list should be in descending order: the last
	// entry is pView.
	// +++++ move to separate method
	list<BView*> parentOrder;
	BView* pCurView = pView;
	while(pCurView && pCurView != m_target) {
		parentOrder.push_front(pCurView);
		pCurView = pCurView->Parent();
	}
	if(pCurView != m_target)
		return B_ERROR; // +++++ ever so descriptive
		
	// walk down the child hierarchy, making ViewEntries as
	// needed
	_ViewEntry* viewEntry = this;
	
	// [e.moon 13oct99] clone tipEntry
	tip_entry* newTipEntry = new tip_entry(tipEntry);

	for(list<BView*>::iterator itView = parentOrder.begin();
		itView != parentOrder.end(); itView++) {

		// look for this view in children of the current entry
		list<_ViewEntry*>::iterator itEntry =
			find_if(
				viewEntry->m_childViews.begin(),
				viewEntry->m_childViews.end(),
				entry_target_matches_view(*itView));
		
		// add new _ViewEntry if necessary
		if(itEntry == viewEntry->m_childViews.end()) {
			viewEntry->m_childViews.push_back(new _ViewEntry(*itView, viewEntry));
			viewEntry = viewEntry->m_childViews.back();
		} else
			viewEntry = *itEntry;
	}
	
	// found a home; can it hold the tip?
	if(viewEntry->m_tips.size() &&
		!(*viewEntry->m_tips.begin())->rect.IsValid()) {
		// [e.moon 13oct99] clean up
		delete newTipEntry;
		return B_ERROR; // +++++ error: full-view tip leaves no room
	}
	
	// remove matching tip if any, then add the new one:
	// [e.moon 13oct99] ref'd by pointer
	tip_entry_set::iterator itFound = viewEntry->m_tips.find(newTipEntry);
	if(itFound != viewEntry->m_tips.end()) {
		delete *itFound;
		viewEntry->m_tips.erase(itFound);
	}
	
	pair<tip_entry_set::iterator, bool> ret;
	ret = viewEntry->m_tips.insert(newTipEntry);

	// something's terribly wrong if insert() failed
	ASSERT(ret.second);
	
	return B_OK;
}

// remove tip matching the given rect's upper-left corner or
// all tips if rect is invalid.
// returns B_OK on success, B_ERROR on failure
status_t _ViewEntry::remove(
	BView* pView, const BRect& rect) {

	// walk up the view's parent tree, building a child-
	// hierarchy list and looking for my target view.
	// The list should be in descending order: the last
	// entry is pView.
	// +++++ move to separate method
	list<BView*> parentOrder;
	BView* pCurView = pView;
	while(pCurView && pCurView != m_target) {
		parentOrder.push_front(pCurView);
		pCurView = pCurView->Parent();
	}
	if(pCurView != m_target)
		return B_ERROR; // +++++ ever so descriptive
	
	// walk down the child tree to the entry for the
	// target view
	_ViewEntry* viewEntry = this;
	for(list<BView*>::iterator itView = parentOrder.begin();
		itView != parentOrder.end(); itView++) {

		// look for this view in children of the current entry
		list<_ViewEntry*>::iterator itEntry =
			find_if(
				viewEntry->m_childViews.begin(),
				viewEntry->m_childViews.end(),
				entry_target_matches_view(*itView));
		
		// it'd better be there!
		if(itEntry == viewEntry->m_childViews.end())
			return B_ERROR;
		
		viewEntry = *itEntry;
	}
	
	// remove matching entries:
	// [13oct99 e.moon] now ref'd by pointer; find and erase all matching tips
	if(rect.IsValid()) {
		tip_entry matchEntry(rect);
		tip_entry_set::iterator it = viewEntry->m_tips.lower_bound(&matchEntry);
		tip_entry_set::iterator itEnd = viewEntry->m_tips.upper_bound(&matchEntry);
		while(it != itEnd) {
			// found one; erase it
			delete *it;
			viewEntry->m_tips.erase(it++);
		}
	}
	else {
		// invalid rect == wildcard

//		PRINT((
//			"### _ViewEntry::remove(): WILDCARD MODE\n"));

		// [13oct99 e.moon] free all tip entries
		for(
			tip_entry_set::iterator it = viewEntry->m_tips.begin();
			it != viewEntry->m_tips.end(); ++it) {
			delete *it;
		}
		viewEntry->m_tips.clear();
		
//		PRINT((
//			"### - freed all tips\n"));

		// [27oct99 e.moon] remove all child views		
		for(
			list<_ViewEntry*>::iterator itChild = viewEntry->m_childViews.begin();
			itChild != viewEntry->m_childViews.end(); ++itChild) {

			delete *itChild;
		}
		viewEntry->m_childViews.clear();

//		PRINT((
//			"### - freed all child views\n"));
		
		// remove the view entry if possible		
		if(viewEntry->m_parent) {
			PRINT((
				"### - removing view entry from %p\n",
					viewEntry->m_parent));
		
			list<_ViewEntry*>::iterator it =
				find_if(
					viewEntry->m_parent->m_childViews.begin(),
					viewEntry->m_parent->m_childViews.end(),
					entry_target_matches_view(pView));
			ASSERT(it != viewEntry->m_parent->m_childViews.end());
			
			_ViewEntry* parent = viewEntry->m_parent;
			delete viewEntry;
			parent->m_childViews.erase(it);
		}
	}
	
	return B_OK;
}

// match the given point (in target's view coordinates)
// against tips in this view and child views.  recurse.

pair<BView*, const tip_entry*> _ViewEntry::match(
	BPoint point, BPoint screenPoint) {

	// fetch this view's current frame rect
	BRect f = Frame();
	
	// check for a full-frame tip:
	
	const tip_entry* pFront = fullFrameTip();
	if(pFront) {
		// match, and stop recursing here; children can't have tips.
		m_target->ConvertFromParent(&f);
		return make_pair(m_target, f.Contains(point) ? pFront : 0);
	}		

	// match against tips for my target view	
	if(m_tips.size()) {
	
		tip_entry matchEntry(BRect(point, point));
		tip_entry_set::iterator itCur = m_tips.lower_bound(&matchEntry);
//		tip_entry_set::iterator itCur = m_tips.begin();
		tip_entry_set::iterator itEnd = m_tips.end();
	
		while(itCur != itEnd) {
			// match:
			const tip_entry* entry = *itCur;
			if(entry->rect.Contains(point))
				return pair<BView*, const tip_entry*>(m_target, entry);
			
			++itCur;
		}
	}
	
	// recurse through children	
	for(list<_ViewEntry*>::iterator it = m_childViews.begin();
		it != m_childViews.end(); it++) {

		_ViewEntry* entry = *it;
		BPoint childPoint =
			entry->target()->ConvertFromParent(point);
		
		pair<BView*, const tip_entry*> ret = entry->match(
			childPoint,
			screenPoint);

		if(ret.second)
			return ret;
	}
	
	// nothing found
	return pair<BView*, const tip_entry*>(0, 0);
}

// get frame rect (in parent view's coordinates)
BRect _ViewEntry::Frame() {
	ASSERT(m_target);
//	ASSERT(m_target->Parent());

	// +++++ if caching or some weird proxy mechanism
	//       works out, return a cached BRect here
	//       rather than asking every view every time!

	BRect f = m_target->Frame();
	return f;
}

// returns pointer to sole entry in the set if it's
// a full-frame tip, or 0 if there's no full-frame tip
const tip_entry* _ViewEntry::fullFrameTip() const {
	if(m_tips.size()) {
		const tip_entry* front = *m_tips.begin();
		if(!front->rect.IsValid()) {
			return front;
		}
	}
	return 0;
}

size_t _ViewEntry::countTips() const {

	size_t tips = m_tips.size();
	for(list<_ViewEntry*>::const_iterator it = m_childViews.begin();
		it != m_childViews.end(); it++) {
		tips += (*it)->countTips();
	}
	
	return tips;
}


void _ViewEntry::dump(int indent) {
	BString s;
	s.SetTo('\t', indent);
	PRINT((
		"%s_ViewEntry '%s'\n",
		s.String(),
		m_target->Name()));

	for(tip_entry_set::iterator it = m_tips.begin();
		it != m_tips.end(); ++it) {
		(*it)->dump(indent + 1);
	}
	for(list<_ViewEntry*>::iterator it = m_childViews.begin();
		it != m_childViews.end(); it++) {
		(*it)->dump(indent + 1);
	}
}

// -------------------------------------------------------- //
// _WindowEntry impl
// -------------------------------------------------------- //

_WindowEntry::~_WindowEntry() {
	for(list<_ViewEntry*>::iterator it = m_views.begin();
		it != m_views.end(); ++it) {
		delete *it;
	}
}

// add the given entry for the designated view (which must
// be attached to the target window)
// returns B_OK on success, B_ERROR if:
// - the given view is NOT attached to the target window, or
// - tips can't be added to this view due to it, or one of its
//   parents, having a full-frame tip.
	
status_t _WindowEntry::add(
	BView*											view,
	const tip_entry&						entry) {

	ASSERT(view);
	if(view->Window() != target())
		return B_ERROR;

	// find top-level view
	BView* parent = view;
	while(parent && parent->Parent())
		parent = parent->Parent();
	
	// look for a _ViewEntry matching the parent & hand off
	for(list<_ViewEntry*>::iterator it = m_views.begin();
		it != m_views.end(); ++it)
		if((*it)->target() == parent)
			return (*it)->add(view, entry);
	
	// create _ViewEntry for the parent & hand off
	_ViewEntry* v = new _ViewEntry(parent, 0);
	m_views.push_back(v);

	return v->add(view, entry);
}

// remove tip matching the given rect's upper-left corner or
// all tips if rect is invalid.
// returns B_ERROR on failure -- if there are no entries for
// the given view -- or B_OK otherwise.

status_t _WindowEntry::remove(
	BView*											view,
	const BRect&								rect) {

	ASSERT(view);
	if(view->Window() != target())
		return B_ERROR;

	// find top-level view
	BView* parent = view;
	while(parent && parent->Parent())
		parent = parent->Parent();
	
	// look for a matching _ViewEntry	& hand off
	for(list<_ViewEntry*>::iterator it = m_views.begin();
		it != m_views.end(); ++it)
		if((*it)->target() == parent) {

			// do it
			status_t ret = (*it)->remove(view, rect);

			if(!(*it)->countTips()) {
				// remove empty entry
				delete *it;
				m_views.erase(it);
			}
			return ret;
		}

	// not found
	PRINT((
		"!!! _WindowEntry::remove(): no matching view\n"));
	return B_ERROR;	
}
		
// match the given point (in screen coordinates)
// against tips in this window's views.

pair<BView*, const tip_entry*> _WindowEntry::match(
	BPoint											screenPoint) {

	for(list<_ViewEntry*>::iterator it = m_views.begin();
		it != m_views.end(); ++it) {
		
		// +++++ failing on invalid views? [e.moon 27oct99]
		
		BView* target = (*it)->target();
		if(target->Window() != m_target) {
			PRINT((
				"!!! _WindowEntry::match(): unexpected window for target view (%p)\n",
				target));

			// skip it
			return pair<BView*,const tip_entry*>(0,0);
		}
		pair<BView*,const tip_entry*> ret = (*it)->match(
			(*it)->target()->ConvertFromScreen(screenPoint),
			screenPoint);
		if(ret.second)
			return ret;
	}
	
	return pair<BView*,const tip_entry*>(0,0);
}

void _WindowEntry::dump(int indent) {
	BString s;
	s.SetTo('\t', indent);
	PRINT((
		"%s_WindowEntry '%s'\n",
		s.String(),
		m_target->Name()));

	for(list<_ViewEntry*>::iterator it = m_views.begin();
		it != m_views.end(); it++) {
		(*it)->dump(indent + 1);
	}
}


// -------------------------------------------------------- //
// _TipManagerView
// -------------------------------------------------------- //

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

_TipManagerView::~_TipManagerView() {
	for(list<_WindowEntry*>::iterator it = m_windows.begin();
		it != m_windows.end(); ++it) {
		delete *it;
	}
	if(m_messageRunner)
		delete m_messageRunner;
		
	// clean up the tip-display window
	m_tipWindow->Lock();
	m_tipWindow->Quit();
}

_TipManagerView::_TipManagerView(
	TipWindow*									tipWindow,
	TipManager*									manager,
	bigtime_t										updatePeriod,
	bigtime_t										idlePeriod) :
	BView(
		BRect(0,0,0,0),
		"_TipManagerView",
		B_FOLLOW_NONE,
		B_PULSE_NEEDED),
	m_tipWindow(tipWindow),
	m_manager(manager),
	m_messageRunner(0),
	m_tipWindowState(TIP_WINDOW_HIDDEN),
	m_updatePeriod(updatePeriod),
	m_idlePeriod(idlePeriod),
	m_lastEventTime(0LL),
	m_triggered(false),
	m_armedTip(0) {
	
	ASSERT(m_tipWindow);
	ASSERT(m_manager);

	// +++++ is this cheating?
	m_tipWindow->Run();

	// request to be sent all mouse & keyboard events
	SetEventMask(B_POINTER_EVENTS | B_KEYBOARD_EVENTS);
	
	// don't draw
	SetViewColor(B_TRANSPARENT_COLOR);
}
	

// -------------------------------------------------------- //
// *** operations
// -------------------------------------------------------- //

// Prepare a 'one-off' tip: this interrupts normal mouse-watching
// behavior while the mouse remains in the given screen rectangle.
// If it idles long enough, the tip window is displayed.

status_t _TipManagerView::armTip(
	const BRect&								screenRect,
	const char*									text,
	TipManager::offset_mode_t		offsetMode,
	BPoint											offset,
	uint32 											flags) {
	
	ASSERT(Looper()->IsLocked());
	
	ASSERT(text);
	if(!screenRect.IsValid())
		return B_BAD_VALUE;
	
	// cancel previous manual tip operation
	if(m_armedTip) {
		ASSERT(m_tipWindowState == TIP_WINDOW_ARMED);
		delete m_armedTip;
		m_armedTip = 0;
	}

	// is a tip with the same screen rect visible? update it:
	if(m_tipWindowState == TIP_WINDOW_VISIBLE &&
		m_visibleTipRect == screenRect) {
		BAutolock _l(m_tipWindow);
		m_tipWindow->setText(text);
		return B_OK;
	}
	
	// create new entry; enter 'armed' state
	m_armedTip = new tip_entry(
		screenRect,
		text,
		offsetMode,
		offset,
		flags);
	m_tipWindowState = TIP_WINDOW_ARMED;
	
	return B_OK;
}
		
// Hide tip corresponding to the given screenRect, if any.
// [e.moon 29nov99] Cancel 'one-off' tip for the given rect if any.

status_t _TipManagerView::hideTip(
	const BRect&								screenRect) {
	
	ASSERT(Looper()->IsLocked());
	
	// check for armed tip
	if(m_armedTip) {
		ASSERT(m_tipWindowState == TIP_WINDOW_ARMED);
		if(m_armedTip->rect == screenRect) {
			// cancel it
			delete m_armedTip;
			m_armedTip = 0;
			m_tipWindowState = TIP_WINDOW_HIDDEN;
			return B_OK;
		}
	}
	
	// check for visible tip
	if(m_tipWindowState != TIP_WINDOW_VISIBLE)
		return B_BAD_VALUE;
	
	if(m_visibleTipRect != screenRect)
		return B_BAD_VALUE;
	
	_hideTip();
	m_tipWindowState = TIP_WINDOW_HIDDEN;

	return B_OK;
}

status_t _TipManagerView::setTip(
	const BRect&								rect,
	const char*									text,
	BView*											view,
	TipManager::offset_mode_t		offsetMode,
	BPoint											offset,
	uint32 											flags) {

	ASSERT(text);
	ASSERT(view);
	ASSERT(Looper()->IsLocked());
	
	BWindow* window = view->Window();
	if(!window)
		return B_ERROR; // can't add non-attached views
	
	// construct & add an entry
	tip_entry e(rect, text, offsetMode, offset, flags);

	for(
		list<_WindowEntry*>::iterator it = m_windows.begin();
		it != m_windows.end(); ++it) {
		if((*it)->target() == window)
			return (*it)->add(view, e);
	}
	
	// create new window entry
	_WindowEntry* windowEntry = new _WindowEntry(window);
	m_windows.push_back(windowEntry);
	
	return windowEntry->add(view, e);
}

// [e.moon 27oct99]
// +++++ broken for 'remove all' mode (triggered by invalid rect):
// doesn't remove entries.
status_t _TipManagerView::removeTip(
	const BRect&								rect,
	BView*											view) {
	
	ASSERT(view);
	ASSERT(Looper()->IsLocked());

	BWindow* window = view->Window();
	if(!window) {
		PRINT((
			"!!! _TipManagerView::removeTip(): not attached !!!\n"));
		return B_ERROR; // can't add non-attached views
	}

	// hand off to the entry for the containing window
	for(
		list<_WindowEntry*>::iterator it = m_windows.begin();
		it != m_windows.end(); ++it) {
		if((*it)->target() == window) {

//			PRINT((
//				"### _TipManagerView::removeTip(%.0f,%.0f - %.0f,%.0f)\n"
//				"    * BEFORE\n\n",
//				rect.left, rect.top, rect.right, rect.bottom));
//			(*it)->dump(1);

			// remove
			status_t ret = (*it)->remove(view, rect);
			
			if(!(*it)->countViews()) {

				// emptied window entry; remove it
				delete *it;
				m_windows.erase(it);
//				PRINT((
//					"    (removed window entry)\n"));
			}
//			else {
//				PRINT((
//					"    * AFTER\n\n"));
//				(*it)->dump(1);
//			}
			return ret;
		}
	}
	
	PRINT((
		"!!! _TipManagerView::removeTip(): window entry not found!\n\n"));
	return B_ERROR;
}

status_t _TipManagerView::removeAll(
	BWindow*										window) {
	
	ASSERT(window);
	ASSERT(Looper()->IsLocked());
	
//	PRINT((
//		"### _TipManagerView::removeAll()\n"));

	for(
		list<_WindowEntry*>::iterator it = m_windows.begin();
		it != m_windows.end(); ++it) {
		if((*it)->target() == window) {
			delete *it;
			m_windows.erase(it);
			return B_OK;
		}
	}
	
	PRINT((
		"!!! _TipManagerView::removeAll(): window entry not found!\n"));
	return B_ERROR;
}

// -------------------------------------------------------- //
// *** BView
// -------------------------------------------------------- //

void _TipManagerView::AttachedToWindow() {

//	PRINT((
//		"### _TipManagerView::AttachedToWindow()\n"));

	// start the updates flowing
	m_messageRunner = new BMessageRunner(
		BMessenger(this),
		new BMessage(M_TIME_PASSED),
		m_updatePeriod);
}

void _TipManagerView::KeyDown(
	const char*									bytes,
	int32												count) {

	// no longer attached?
	if(!Window())
		return;

	// hide the tip
	if(m_tipWindowState == TIP_WINDOW_VISIBLE) {
		_hideTip();
		m_tipWindowState = TIP_WINDOW_HIDDEN;
	}	

	m_lastEventTime = system_time();
}

void _TipManagerView::MouseDown(
	BPoint											point) {

	// no longer attached?
	if(!Window())
		return;

	// hide the tip
	if(m_tipWindowState == TIP_WINDOW_VISIBLE) {
		_hideTip();
		m_tipWindowState = TIP_WINDOW_HIDDEN;
	}	

	m_lastEventTime = system_time();
	ConvertToScreen(&point);
	m_lastMousePoint = point;
}

void _TipManagerView::MouseMoved(
	BPoint											point,
	uint32											orientation,
	const BMessage*							dragMessage) {

//	PRINT((
//		"### _TipManagerView::MouseMoved()\n"));

	// no longer attached?
	if(!Window())
		return;

	ConvertToScreen(&point);

	bool moved = (point != m_lastMousePoint);
	
	if(m_tipWindowState == TIP_WINDOW_ARMED) {
		ASSERT(m_armedTip);
		if(moved && !m_armedTip->rect.Contains(point)) {
			// mouse has moved outside the tip region,
			// disarming this manually-armed tip.
			m_tipWindowState = TIP_WINDOW_HIDDEN;
			delete m_armedTip;
			m_armedTip = 0;
		}
	}
	else if(m_tipWindowState == TIP_WINDOW_VISIBLE) {
		ASSERT(m_visibleTipRect.IsValid());

		if(moved && !m_visibleTipRect.Contains(point)) {
			// hide the tip
			_hideTip();
			m_tipWindowState = TIP_WINDOW_HIDDEN;
		}
		
		// don't reset timing state until the tip is closed
		return;
	}
	
	// if the mouse has moved, reset timing state:
	if(moved) {
		m_lastMousePoint = point;
		m_lastEventTime = system_time();
		m_triggered = false;
	}
}

// -------------------------------------------------------- //
// *** BHandler
// -------------------------------------------------------- //

void _TipManagerView::MessageReceived(
	BMessage*										message) {
	switch(message->what) {
		case M_TIME_PASSED:
			_timePassed();
			break;

		default:
			_inherited::MessageReceived(message);
	}
}

// -------------------------------------------------------- //
// implementation
// -------------------------------------------------------- //

inline void _TipManagerView::_timePassed() {

//	PRINT((
//		"### _TipManagerView::_timePassed()\n"));

	// no longer attached?
	if(!Window())
		return;

	// has the mouse already triggered at this point?
	if(m_triggered)
		// yup; nothing more to do
		return;

	// see if the mouse has idled for long enough to trigger
	bigtime_t now = system_time();
	if(now - m_lastEventTime < m_idlePeriod)
		// nope
		return;

	// trigger!
	m_triggered = true;
	
	if(m_tipWindowState == TIP_WINDOW_ARMED) {
		// a tip has been manually set
		ASSERT(m_armedTip);
		m_visibleTipRect = m_armedTip->rect;
		_showTip(m_armedTip);
		m_tipWindowState = TIP_WINDOW_VISIBLE;
		
		// clean up
		delete m_armedTip;
		m_armedTip = 0;
		return;
	}

	// look for a tip under the current mouse point
	for(
		list<_WindowEntry*>::iterator it = m_windows.begin();
		it != m_windows.end(); ++it) {
	
		// lock the window
		BWindow* window = (*it)->target();
		ASSERT(window);

		// [e.moon 21oct99] does autolock work in this context?
		//BAutolock _l(window);
		window->Lock();
			
		// match
		pair<BView*, const tip_entry*> found =
			(*it)->match(m_lastMousePoint);
			
		// if no tip found, or the view's no longer attached, bail:
		if(!found.second || found.first->Window() != window) {
			window->Unlock();
			continue;
		}
		
		// found a tip under the mouse; see if it's obscured
		// by another window
		BRegion clipRegion;
		found.first->GetClippingRegion(&clipRegion);
		if(!clipRegion.Contains(
			found.first->ConvertFromScreen(m_lastMousePoint))) {
			// view hidden; don't show tip
			window->Unlock();
			continue;
		}
		
		// show the tip
		if(found.second->rect.IsValid())
			m_visibleTipRect = found.first->ConvertToScreen(
				found.second->rect);
		else
			m_visibleTipRect = found.first->ConvertToScreen(
				found.first->Bounds());

		_showTip(found.second);
		m_tipWindowState = TIP_WINDOW_VISIBLE;
		
		window->Unlock();
		break;
	}
}

inline void _TipManagerView::_showTip(
	const tip_entry*						entry) {
	
//	PRINT((
//		"### _TipManagerView::_showTip()\n"));
	
	ASSERT(m_tipWindow);
	ASSERT(m_tipWindowState != TIP_WINDOW_VISIBLE);
	ASSERT(entry);
	
	BAutolock _l(m_tipWindow);

	// set text	
	m_tipWindow->SetWorkspaces(B_ALL_WORKSPACES);
	m_tipWindow->setText(entry->text.String());
	
	// figure position
	BPoint offset = (entry->offset == TipManager::s_useDefaultOffset) ?
		TipManager::s_defaultOffset :
		entry->offset;

	BPoint p;
	switch(entry->offsetMode) {
		case TipManager::LEFT_OFFSET_FROM_RECT:
			p = m_visibleTipRect.RightTop() + offset;
			break;
		case TipManager::LEFT_OFFSET_FROM_POINTER:
			p = m_lastMousePoint + offset;
			break;
		case TipManager::RIGHT_OFFSET_FROM_RECT:
			p = m_visibleTipRect.LeftTop();
			p.x -= offset.x;
			p.y += offset.y;
			p.x -= m_tipWindow->Frame().Width();
			break;
		case TipManager::RIGHT_OFFSET_FROM_POINTER:
			p = m_lastMousePoint;
			p.x -= offset.x;
			p.y += offset.y;
			p.x -= m_tipWindow->Frame().Width();
			break;
		default:
			ASSERT(!"bad offset mode");
	}

	// constrain window to be on-screen:
	m_tipWindow->MoveTo(p);
	
	BRect screenR = BScreen(m_tipWindow).Frame();
	BRect tipR = m_tipWindow->Frame();

	if(tipR.left < screenR.left)
		tipR.left = screenR.left;
	else if(tipR.right > screenR.right)
		tipR.left = screenR.right - tipR.Width();
	
	if(tipR.top < screenR.top)
		tipR.top = screenR.top;
	else if(tipR.bottom > screenR.bottom)
		tipR.top = screenR.bottom - tipR.Height();
	
	if(tipR.LeftTop() != p)
		m_tipWindow->MoveTo(tipR.LeftTop());

	if(m_tipWindow->IsHidden())
		m_tipWindow->Show();
}

inline void _TipManagerView::_hideTip() {
//	PRINT((
//		"### _TipManagerView::_hideTip()\n"));
	
	ASSERT(m_tipWindow);
	ASSERT(m_tipWindowState == TIP_WINDOW_VISIBLE);
	BAutolock _l(m_tipWindow);
	
	if(m_tipWindow->IsHidden())
		return;
		
	m_tipWindow->Hide();
}

// END -- TipManagerImpl.cpp --
