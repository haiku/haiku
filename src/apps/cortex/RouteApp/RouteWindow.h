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


// RouteWindow.h
// e.moon 14may99
//
// PURPOSE
//   Window class containing a MediaRoutingView for
//   inspection & manipulation of Media Kit nodes.
//
// HISTORY
//   14may99		e.moon		Created from routeApp.cpp
//	 21may00		c.lenz		added StatusView to the window

#ifndef __ROUTEWINDOW_H__
#define __ROUTEWINDOW_H__

#include <list>

#include <Node.h>
#include <ScrollBar.h>
#include <Window.h>

#include "IStateArchivable.h"

#include "cortex_defs.h"

class BMenu;

__BEGIN_CORTEX_NAMESPACE

class MediaRoutingView;
class StatusView;
class RouteAppNodeManager;
class DormantNodeWindow;
class TransportWindow;

class RouteWindow :
	public		BWindow,
	public		IStateArchivable {

	typedef	BWindow _inherited;

public:													// messages
	enum message_t {
		M_TOGGLE_TRANSPORT_WINDOW		=RouteWindow_message_base,
		
		// nodeID: int32
		M_SHOW_NODE_INSPECTOR,
		
		// groupID: int32
		M_REFRESH_TRANSPORT_SETTINGS,
		
		// [e.moon 17nov99]
		M_TOGGLE_PULLING_PALETTES,
		
		// [e.moon 29nov99]
		M_TOGGLE_DORMANT_NODE_WINDOW,
		
		// [e.moon 1dec99]
		M_TOGGLE_GROUP_ROLLING
	};

public:													// ctor/dtor
	virtual ~RouteWindow();
	RouteWindow(
		RouteAppNodeManager*			manager);
		
public:													// palette-window operations

	// enable/disable palette position-locking (when the main
	// window is moved, all palettes follow)
	bool isPullPalettes() const;
	void setPullPalettes(
		bool												enabled);
		
	// [e.moon 2dec99] force window & palettes on-screen
	void constrainToScreen();
	
public:													// BWindow impl

	// [e.moon 17nov99] 'palette-pulling' impl
	virtual void FrameMoved(
		BPoint											point);

	// [c.lenz 1mar2000] added for better Zoom() support
	virtual void FrameResized(
		float											width,
		float											height);

	bool QuitRequested();
	
	// [c.lenz 1mar2000] resize to MediaRoutingView's preferred size
	virtual void Zoom(
		BPoint											origin,
		float											width,
		float											height);

public:													// BHandler impl
	void MessageReceived(
		BMessage*										message);

public:												// *** IStateArchivable

	status_t importState(
		const BMessage*						archive);
	
	status_t exportState(
		BMessage*									archive) const;

private:												// implementation
	friend class RouteApp;

	// resizes the window to fit in the current screen rect
	void _constrainToScreen();

	void _toggleTransportWindow();

	void _handleGroupSelected(
		BMessage*										message);

	// [c.lenz 21may00]
	void _handleShowErrorMessage(
		BMessage*										message);

	// [e.moon 17nov99]
	void _togglePullPalettes();
		
	void _toggleDormantNodeWindow();

	// refresh the transport window for the given group, if any		
	void _refreshTransportSettings(
		BMessage*										message);
		
	void _closePalettes();
	
	// [e.moon 17nov99] move all palette windows by the
	// specified amounts
	void _movePalettesBy(
		float												xDelta,
		float												yDelta);
		
	// [e.moon 1dec99] toggle group playback
	void _toggleGroupRolling();
	
private:												// members
	MediaRoutingView*							m_routingView;
	BScrollBar*										m_hScrollBar;
	BScrollBar*										m_vScrollBar;
		
	StatusView*										m_statusView;

	BMenuItem*										m_transportWindowItem;
	BRect													m_transportWindowFrame;
	TransportWindow*							m_transportWindow;
//	list<InspectorWindow*>				m_nodeInspectorWindows;
	
	BMenuItem*										m_dormantNodeWindowItem;
	BRect													m_dormantNodeWindowFrame;
	DormantNodeWindow*						m_dormantNodeWindow;

//	BPoint												m_nodeInspectorBasePosition;
		
	// all items in this menu control the routing view
	BMenu*												m_viewMenu;
	
	// cached window position [e.moon 17nov99]
	BMenuItem*										m_pullPalettesItem;
	BPoint												m_lastFramePosition;
	
	// ID of currently selected group [e.moon 1dec99]
	uint32												m_selectedGroupID;

	BRect												m_manualSize;

	// true if window was zoomed to MediaRoutingView's preferred size
	// [c.lenz 1mar2000]
	bool												m_zoomed;

	// true if a resize operation resulted from a call to Zoom()
	// [c.lenz 1mar2000]
	bool												m_zooming;
	

private:												// constants
	static const BRect						s_initFrame;
	static const char* const			s_windowName;
};

__END_CORTEX_NAMESPACE
#endif /* __ROUTEWINDOW_H__ */
