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


// MediaRoutingView.h
// c.lenz 9oct99
//
// PURPOSE
//   Provide a simple interface for the BeOS/Genki media system.
//   Displays all the currently running ('live') media nodes,
//   and represents the connections between them visually.
//
// NOTES
//
// *** 9oct99: replaced grid-based version
//
// HISTORY
//   e.moon 6may99: first stab
//   c.lenz 6oct99: starting change to DiagramView impl

#ifndef __MediaRoutingView__H__
#define __MediaRoutingView__H__

// DiagramView
#include "DiagramView.h"

// Media Kit
#include "MediaDefs.h"

#include <Entry.h>
#include <List.h>
#include <Message.h>

#include "IStateArchivable.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

// MediaRoutingView
class MediaNodePanel;
class MediaWire;
// NodeManager
class RouteAppNodeManager;
class NodeGroup;
class NodeRef;
class Connection;
// RouteApp
class NodeSetIOContext;

class MediaRoutingView :
	public		DiagramView,
	public		IStateArchivable {
	typedef		DiagramView _inherited;
	
public:					// *** constants

	// [e.moon 26oct99] moved definitions to MediaRoutingView.cpp
	static float	M_CLEANUP_H_GAP;
	static float	M_CLEANUP_V_GAP;
	static float	M_CLEANUP_H_MARGIN;
	static float	M_CLEANUP_V_MARGIN;

	// [e.moon 7dec99] enum now a named type
	enum layout_t
	{
		M_ICON_VIEW = 1,
		M_MINI_ICON_VIEW
	};

public:					//  messages

	enum message_t {
		// INBOUND
		// "layout"		int32:			M_ICON_VIEW / M_MINI_ICON_VIEW
		M_LAYOUT_CHANGED,

		// INBOUND
		M_CLEANUP_REQUESTED,
		
		// INBOUND
		// release/delete node
		// "nodeID"		[u]int32:		node to release
		M_RELEASE_NODE,
		
		// INBOUND
		M_SELECT_ALL,
		
		// INBOUND
		M_DELETE_SELECTION,
		
		// OUTBOUND
		// describes a selected node (sent to owning window)
		// "nodeID"		int32
		M_NODE_SELECTED,
		
		// OUTBOUND
		// describes a selected group (sent to owning window)
		// "groupID"	int32
		M_GROUP_SELECTED,
		
		// INBOUND
		// requests that the currently selected node/group be broadcast
		// back to the owning window
		M_BROADCAST_SELECTION,

		// INBOUND
		// request to change the selected nodes cycling mode (on/off)
		// "cycle"		bool
		M_NODE_CHANGE_CYCLING,

		// INBOUND
		// request to change the selected nodes run mode
		// "run_mode"	int32
		M_NODE_CHANGE_RUN_MODE,
		
		// INBOUND
		// request to start/stop the selected node(s) as a time source
		// instantly
		// [e.moon 5dec99]
		M_NODE_START_TIME_SOURCE,
		M_NODE_STOP_TIME_SOURCE,
		
		// INBOUND
		// call BControllable::StartControlPanel for the node specified
		// in the field "nodeID" (int32)
		// [c.lenz 24dec99]
		M_NODE_START_CONTROL_PANEL,
		
		// INBOUND
		// set the given group's GROUP_LOCKED flag
		// [em 1feb00]
		// "groupID"  int32
		// "locked"   bool
		M_GROUP_SET_LOCKED,

		// INBOUND
		// open ParameterWindow for selected nodes
		// [c.lenz 17feb2000]
		M_NODE_TWEAK_PARAMETERS,

		// INBOUND
		// sent to the RouteWindow for displaying error
		// messages in the status bar if available
		// "text"	string
		// "error"	bool	(optional)
		M_SHOW_ERROR_MESSAGE
	};

public:						// *** members

	RouteAppNodeManager* const	manager;
	
public:						// *** ctor/dtor

							MediaRoutingView(
								RouteAppNodeManager *nodeManager,
								BRect frame,
								const char *name,
								uint32 resizeMode = B_FOLLOW_ALL_SIDES);

	virtual					~MediaRoutingView();
	
public:						// *** DiagramView impl

	virtual void			connectionAborted(
								DiagramEndPoint *fromWhich);

	virtual void			connectionEstablished(
								DiagramEndPoint *fromWhich,
								DiagramEndPoint *toWhich);

	DiagramWire			   *createWire(
								DiagramEndPoint *fromWhich,
								DiagramEndPoint *woWhich);

	DiagramWire			   *createWire(
								DiagramEndPoint *fromWhich);

	virtual void			BackgroundMouseDown(
								BPoint point,
								uint32 buttons,
								uint32 clicks);

	virtual void			MessageDropped(
								BPoint point,
								BMessage *message);

	virtual void			SelectionChanged();

public:						// *** BView impl

	virtual void			AttachedToWindow();

	virtual void			AllAttached();

	virtual void			DetachedFromWindow();
	
	virtual void			KeyDown(
								const char *bytes,
								int32 count);

	virtual void			Pulse();	

public:						// *** BHandler impl

	virtual void			MessageReceived(
								BMessage *message);

public:						// *** accessors

	layout_t				getLayout() const
							{ return m_layout; }

public:						// *** operations

	// returns coordinates for a free area where the panel
	// could be positioned; uses the M_CLEANUP_* settings
	// and positions producers at the left, consumer at the
	// right and filters in the middle
	BPoint					findFreePositionFor(
								const MediaNodePanel *panel) const;

public:												// *** IStateArchivable

	status_t importState(
		const BMessage*						archive);
	
	status_t exportState(
		BMessage*									archive) const;
		
	// [e.moon 8dec99] subset support
	status_t importStateFor(
		const NodeSetIOContext*		context,
		const BMessage*						archive);

	status_t exportStateFor(
		const NodeSetIOContext*		context,
		BMessage*									archive) const;

protected:					// *** operations

	// adjust the default object sizes to the current
	// layout and font size and rearrange if necessary
	void					layoutChanged(
								layout_t layout);

	// aligns the panels on a grid according to their node_kind
	void					cleanUp();

	// displays a context menu at a given position
	void					showContextMenu(
								BPoint point);

	// will currently display a BAlert; this should change sometime
	// in the future!
	void					showErrorMessage(
								BString message,
								status_t error);

private:					// *** children management

	// adds a panel representation of a live media node to the view
	status_t				_addPanelFor(
								media_node_id id,
								BPoint point);
	
	// tries to find the panel to a given media node
	status_t				_findPanelFor(
								media_node_id id,
								MediaNodePanel **outPanel) const;

	// removes the panel of a given media node
	status_t				_removePanelFor(
								media_node_id);

	// adds a wire represenation of a media kit connection
	status_t				_addWireFor(
								Connection &connection);
								
	// finds the ui rep of a given Connection object
	status_t				_findWireFor(
								uint32	connectionID,
								MediaWire **wire) const;

	// removes the wire
	status_t				_removeWireFor(
								uint32 connectionID);

private:					// *** internal methods

	// iterates through all selected MediaNodePanels and sets the
	// 'cycling' state for each to cycle
	void					_changeCyclingForSelection(
								bool cycle);

	// iterates through all selected MediaNodePanels and sets the
	// RunMode for each to mode; 0 is interpreted as '(same as group)'
	void					_changeRunModeForSelection(
								uint32 mode);

	void					_openInfoWindowsForSelection();

	void					_openParameterWindowsForSelection();

	void					_startControlPanelsForSelection();

	// tries to release every node in the current selection, or to
	// disconnect wires if those were selected
	void					_deleteSelection();

	void					_addShortcuts();

	void					_initLayout();

	// populates the view with all nodes currently in the NodeManager
	void					_initContent();

	void 					_checkDroppedFile(
								entry_ref *ref,
								BPoint dropPoint);

	void					_changeBackground(
								entry_ref *ref);

	void					_changeBackground(
								rgb_color color);

	// adjust scroll bar ranges
	void					_adjustScrollBars();

	void					_broadcastSelection() const;
	
	// find & remove an entry in m_inactiveNodeState
	status_t				_fetchInactiveNodeState(
								MediaNodePanel* forPanel,
								BMessage* outMessage);
								
	void					_emptyInactiveNodeState();

private:

	// the current layout
	layout_t				m_layout;

	// current new-group-name index
	uint32					m_nextGroupNumber;

	// holds the id of the node instantiated last thru d&d
	media_node_id			m_lastDroppedNode;
	
	// the point at which above node was dropped
	BPoint					m_lastDropPoint;

	// holds a pointer to the currently dragged wire (if any)
	MediaWire			   *m_draggedWire;
	
	// stores location of the background bitmap (invalid if no
	// background bitmap has been set.)
	// [e.moon 1dec99]
	BEntry					m_backgroundBitmapEntry;
	
	// state info for currently inactive nodes (cached from importState())
	BList           m_inactiveNodeState;
};

__END_CORTEX_NAMESPACE
#endif /* __MediaRoutingView_H__ */
