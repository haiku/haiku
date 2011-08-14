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


// NodeManager.h (Cortex)
//
// * PURPOSE
//   Provides a Media Kit application with a straightforward
//   way to keep track of media nodes and the connections
//   between them.  Nodes are collected into sets via the
//   NodeGroup class; these sets can be controlled in tandem.
//
// * GROUPING NOTES
//   A new group is created with the following information:
//   - time source (defaults to the DAC time source)
//   - a user-provided name
//
//   New nodes can be added to a group via NodeGroup methods.  When a
//   node is added to a group, it will automatically be assigned the
//   group's time source.  Unless the node has a run mode set, it will
//   also be assigned the group's run mode.  (If the group is in B_OFFLINE
//   mode, this will be assigned to all nodes even if they specify something
//   else.)  If a node is added to a group whose transport is running, it
//   will automatically be seeked and started (unless one or both of those
//   operations has been disabled.)
//   
// * SYNCHRONIZATION NOTES
//   Each NodeManager object, including all the NodeGroup and NodeRef
//   objects in its care, is synchronized by a single semaphore.
//   Most operations in these three classes require that the object
//   be locked.
//
// * UI HOOKS
//   NodeManager resends any Media Roster messages to all observers
//   *after* processing them: the NodeRef corresponding to a newly-
//   created node, for example, must exist by the time that a
//   NodeManager observer receives B_MEDIA_NODE_CREATED.
//
//
// * HISTORY
//   e.moon		7nov99		1) added hooks for Media Roster message processing
//											2) improved NodeGroup handling
//   e.moon		6nov99		safe node instantiation (via addon-host
//											application)
//   e.moon		11aug99		Expanded findConnection() methods.
//   e.moon		6jul99		Begun

#ifndef __NodeManager_H__
#define __NodeManager_H__

#include "ILockable.h"
#include "ObservableLooper.h"
#include "observe.h"

#include <Looper.h>
#include <MediaDefs.h>
#include <MediaNode.h>

#include <vector>
#include <map>

class BMediaRoster;

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class Connection;
class NodeGroup;
class NodeRef;

class NodeManager :
	public	ObservableLooper,
	public	ILockable {

	// primary parent class:
	typedef	ObservableLooper _inherited;
	
	friend class NodeGroup;
	friend class NodeRef;
	friend class Connection;

public:				// *** messages
	// [13aug99]
	//   NodeManager retransmits Media Roster messages to its listeners,
	//   after processing each message.
	//
	///  B_MEDIA_CONNECTION_BROKEN
	//     This message, as sent by the Media Roster, contains only
	//     source/destination information.  NodeManager adds these fields
	//     to the message:
	//     __connection_id:        (uint32) id of the Connection; 0 if no
	//                             matching Connection was found.
	//     __source_node_id:			 media_node_id of the node corresponding to
	//                        		 the source; 0 if no matching Connection was
	//                        		 found.
	//     __destination_node_id:	 media_node_id of the node corresponding to
	//                         		 the source; 0 if no matching Connection was
	//                         		 found.
	//
	//   B_MEDIA_FORMAT_CHANGED
	//     NodeManager add these fields as above:
	//     __connection_id
	//     __source_node_id
	//     __destination_node_id

	enum outbound_message_t {
		M_OBSERVER_ADDED						=NodeManager_message_base,
		M_OBSERVER_REMOVED,
		M_RELEASED,

		// groupID: int32
		M_GROUP_CREATED,
		M_GROUP_DELETED,
		
		// groupID: int32 x2 (the first is the original)
		M_GROUP_SPLIT,
		
		// groupID: int32 x2
		M_GROUPS_MERGED
	};

public:				// *** default group names

	static const char* const			s_defaultGroupPrefix;
	static const char* const			s_timeSourceGroup;
	static const char* const			s_audioInputGroup;
	static const char* const			s_videoInputGroup;
	static const char* const			s_audioMixerGroup;
	static const char* const			s_videoOutputGroup;

public:				// *** hooks

	// [e.moon 7nov99] these hooks are called during processing of
	// BMediaRoster messages, before any notification is sent to
	// observers.  For example, if a B_MEDIA_NODES_CREATED message
	// were received describing 3 new nodes, nodeCreated() would be
	// called 3 times before the notification was sent.
	
	virtual void nodeCreated(
		NodeRef*											ref);
	
	virtual void nodeDeleted(
		const NodeRef*								ref);
		
	virtual void connectionMade(
		Connection*										connection);

	virtual void connectionBroken(
		const Connection*							connection);

	virtual void connectionFailed(
		const media_output &							output,
		const media_input &								input,
		const media_format &							format,
		status_t										error);

public:				// *** ctor/dtor

	NodeManager(
		bool													useAddOnHost=false);

	// don't directly delete NodeManager;
	// use IObservable::release()
	virtual ~NodeManager();

public:				// *** const members

	// cached roster pointer
	::BMediaRoster*	const						roster;
	
public:				// *** operations

	// * ACCESS

	// fetches NodeRef corresponding to a given ID; returns
	// B_BAD_VALUE if no matching entry was found (and writes
	// a 0 into the provided pointer.)

	status_t getNodeRef(
		media_node_id									id,
		NodeRef**											outRef) const;
	
	// [13aug99]	
	// fetches Connection corresponding to a given source/destination
	// on a given node.  Returns an invalid connection and B_BAD_VALUE
	// if no matching connection was found.
	
	status_t findConnection(
		media_node_id									node,
		const media_source&						source,
		Connection*										outConnection) const;
	
	status_t findConnection(
		media_node_id									node,
		const media_destination&			destination,
		Connection*										outConnection) const;
		
	// [e.moon 28sep99]
	// fetches a Connection matching the given source and destination
	// nodes.  Returns an invalid connection and B_BAD_VALUE if
	// no matching connection was found
	
	status_t findConnection(
		media_node_id									sourceNode,
		media_node_id									destinationNode,
		Connection*										outConnection) const;
		
	// [e.moon 28sep99]
	// tries to find a route from 'nodeA' to 'nodeB'; returns
	// true if one exists, false if not.  If nodeA and nodeB
	// are the same node, only returns true if it's actually
	// connected to itself.
	
	bool findRoute(
		media_node_id									nodeA,
		media_node_id									nodeB);
		
private:	
	// implementation of above
	class _find_route_state;
	bool _find_route_recurse(
		NodeRef*													origin,
		media_node_id											target,
		_find_route_state*								state);
	
public:	
	// fetches Connection corresponding to a given source or
	// destination; Returns an invalid connection and B_BAD_VALUE if
	// none found.
	// *   Note: this is the slowest possible way to look up a
	//     connection.  Since the low-level source/destination
	//     structures don't include a node ID, and a destination
	//     port can differ from its node's control port, a linear
	//     search of all known connections is performed.  Only
	//     use these methods if you have no clue what node the
	//     connection corresponds to.
	
	status_t findConnection(
		const media_source&						source,
		Connection*										outConnection) const;
		
	status_t findConnection(
		const media_destination&			destination,
		Connection*										outConnection) const;
	
	// fetch NodeRefs for system nodes (if a particular node doesn't
	// exist, these methods return 0)
	
	NodeRef* audioInputNode() const;
	NodeRef* videoInputNode() const;
	NodeRef* audioMixerNode() const;
	NodeRef* audioOutputNode() const;
	NodeRef* videoOutputNode() const;
	
	// * GROUP CREATION

	NodeGroup* createGroup(
		const char*										name,
		BMediaNode::run_mode					runMode=BMediaNode::B_INCREASE_LATENCY);

	// fetch groups by index
	// - you can write-lock the manager during sets of calls to these methods;
	//   this ensures that the group set won't change.  The methods do lock
	//   the group internally, so locking isn't explicitly required.
	
	uint32 countGroups() const;
	NodeGroup* groupAt(
		uint32												index) const;

	// look up a group by unique ID; returns B_BAD_VALUE if no
	// matching group was found
	
	status_t findGroup(
		uint32												id,
		NodeGroup**										outGroup) const;

	// look up a group by name; returns B_NAME_NOT_FOUND if
	// no group matching the name was found.

	status_t findGroup(
		const char*										name,
		NodeGroup**										outGroup) const;
		
	// merge the given source group to the given destination;
	// empties and releases the source group
	
	status_t mergeGroups(
		NodeGroup*										sourceGroup,
		NodeGroup*										destinationGroup);
	
	// [e.moon 28sep99]
	// split group: given two nodes currently in the same group
	// that are not connected (directly OR indirectly),
	// this method removes outsideNode, and all nodes connected
	// to outsideNode, from the common group.  These nodes are
	// then added to a new group, returned in 'outGroup'.  The
	// new group has " split" appended to the end of the original
	// group's name.
	//
	// Returns B_NOT_ALLOWED if any of the above conditions aren't
	// met (ie. the nodes are in different groups or an indirect
	// route exists from one to the other), or B_OK if the group
	// was split successfully.
	
	status_t splitGroup(
		NodeRef*											insideNode,
		NodeRef*											outsideNode,
		NodeGroup**										outGroup);

	// * INSTANTIATION & CONNECTION
	//   Use these calls rather than the associated BMediaRoster()
	//   methods to assure that the nodes and connections you set up
	//   can be properly serialized & reconstituted.

	// basic BMediaRoster::InstantiateDormantNode() wrapper
	// - writes a 0 into *outRef if the instantiation fails
	// - [e.moon 23oct99]
	//   returns B_BAD_INDEX if InstantiateDormantNode() returns
	//   success, but doesn't hand back a viable media_node
	// - [e.moon 6nov99] +++++ 'distributed' instantiate:
	//   wait for an external app to create the node; allow for
	//   failure.
	
	status_t instantiate(
		const dormant_node_info&			info,
		NodeRef**											outRef=0,
		bigtime_t											timeout=B_INFINITE_TIMEOUT,
		uint32												nodeFlags=0);
		
	// SniffRef/Instantiate.../SetRefFor: a one-call interface
	// to create a node capable of playing a given media file.
	// - writes a 0 into *outRef if the instantiation fails; on the
	//   other hand, if instantiation succeeds, but SetRefFor() fails,
	//   a NodeRef will still be returned.
	
	status_t instantiate(
		const entry_ref&							file,
		uint64												requireNodeKinds,
		NodeRef**											outRef,
		bigtime_t											timeout=B_INFINITE_TIMEOUT,
		uint32												nodeFlags=0,
		bigtime_t*										outDuration=0);

	// use this method to reference nodes created within your
	// application.  These nodes can't be automatically reconstituted
	// by the cortex serializer yet.

	status_t reference(
		BMediaNode*										node,
		NodeRef**											outRef,
		uint32												nodeFlags=0);

	// the most flexible form of connect(): set the template
	// format as you would for BMediaRoster::Connect().
	
	status_t connect(
		const media_output&						output,
		const media_input&						input,
		const media_format&						templateFormat,
		Connection*										outConnection=0);
		
	// format-guessing form of connect(): tries to find
	// a common format between output & input before connection;
	// returns B_MEDIA_BAD_FORMAT if no common format type found.
	//
	// NOTE: the specifics of the input and output formats are ignored;
	//       this method only looks at the format type, and properly
	//       handles wildcards at that level (B_MEDIA_NO_TYPE).
	
	status_t connect(
		const media_output&						output,
		const media_input&						input,
		Connection*										outConnection=0);

	// disconnects the connection represented by the provided
	// Connection object.  if successful, returns B_OK.
	
	status_t disconnect(
		const Connection&							connection);

public:				// *** node/connection iteration
							// *** MUST BE LOCKED for any of these calls
	
	// usage:
	//   For the first call, pass 'cookie' a pointer to a void* set to 0.
	//   Returns B_BAD_INDEX when the set of nodes has been exhausted (and
	//   invalidates the cookie, so don't try to use it after this point.)
	
	status_t getNextRef(
		NodeRef**											outRef,
		void**												cookie);
		
	// if you want to stop iterating, call this method to avoid leaking
	// memory
	void disposeRefCookie(
		void**												cookie);

	status_t getNextConnection(
		Connection*										outConnection,
		void**												cookie);

	void disposeConnectionCookie(
		void**												cookie);
		
public:				// *** BHandler impl
	void MessageReceived(BMessage*	message); //nyi

public:				// *** IObservable hooks
	virtual void observerAdded(
		const BMessenger&				observer);
		
	virtual void observerRemoved(
		const BMessenger&				observer);

	virtual void notifyRelease();

	virtual void releaseComplete();


public:				// *** ILockable impl.
	virtual bool lock(
		lock_t												type=WRITE,
		bigtime_t											timeout=B_INFINITE_TIMEOUT);
	
	virtual bool unlock(
		lock_t												type=WRITE);
	
	virtual bool isLocked(
		lock_t												type=WRITE) const;
		

protected:			// *** internal operations (LOCK REQUIRED)


	void _initCommonNodes();
	
	void _addRef(
		NodeRef*											ref);
	void _removeRef(
		media_node_id									id);
		
	// create, add, return a NodeRef for the given external node;
	// must not already exist
	NodeRef* _addRefFor(
		const media_node&							node,
		uint32												nodeFlags,
		uint32												nodeImplFlags=0);

	void _addConnection(
		Connection*										connection);
	void _removeConnection(
		const Connection&							connection);
		
	void _addGroup(
		NodeGroup*										group);
	void _removeGroup(
		NodeGroup*										group);
	
	// dtor helpers
	inline void _clearGroup(
		NodeGroup*										group);

	inline void _freeConnection(
		Connection*										connection);
		
	// message handlers
	
	// now returns B_OK iff the message should be relayed to observers
	// [e.moon 11oct99]
	inline status_t _handleNodesCreated(
		BMessage*											message);

	inline void _handleNodesDeleted(
		BMessage*											message);

	inline void _handleConnectionMade(
		BMessage*											message);

	inline void _handleConnectionBroken(
		BMessage*											message);
		
	inline void _handleFormatChanged(
		BMessage*											message);
	// return flags appropriate for an external
	// node with the given 'kind'

	inline uint32 _userFlagsForKind(
		uint32												kind);

	inline uint32 _implFlagsForKind(
		uint32												kind);
	
	// [e.moon 28sep99] latency updating
	// These methods must set the recording-mode delay for
	// any B_RECORDING nodes they handle.
	
	// +++++ abstract to 'for each' and 'for each from'
	//       methods (template or callback?)

	
	// refresh cached latency for every node in the given group
	// (or all nodes if no group given.)
	
	inline void _updateLatencies(
		NodeGroup*										group =0);
	
	// refresh cached latency for every node attached to
	// AND INCLUDING the given origin node.
	// if 'recurse' is true, affects indirectly attached
	// nodes as well.
	
	inline void _updateLatenciesFrom(
		NodeRef*											origin,
		bool													recurse =false);
		
	// a bit of unpleasantness [e.moon 13oct99]
	inline void _lockAllGroups();
	inline void _unlockAllGroups();
	
private:				// *** internal messages
	enum int_message_t {
		//  groupID: int32
		_M_GROUP_RELEASED					= NodeManager_int_message_base
	};
	
private:				// *** members
	// the main NodeRef store
	typedef std::map<media_node_id, NodeRef*> node_ref_map;
	node_ref_map					m_nodeRefMap;
	
	// the Connection stores (connections are indexed by both
	// source and destination node ID)
	typedef std::multimap<media_node_id, Connection*> con_map;
	con_map								m_conSourceMap;
	con_map								m_conDestinationMap;
	
	// the NodeGroup store
	typedef std::vector<NodeGroup*> node_group_set;
	node_group_set				m_nodeGroupSet;
	
	// common system nodes
	NodeRef*							m_audioInputNode;
	NodeRef*							m_videoInputNode;
	NodeRef*							m_audioMixerNode;
	NodeRef*							m_audioOutputNode;
	NodeRef*							m_videoOutputNode;
	
	// next unique connection ID
	uint32								m_nextConID;

	// false until the first 'nodes created' message is
	// received from the Media Roster, and pre-existing connection
	// info is filled in:
	bool									m_existingNodesInit;
	
	// true if nodes should be launched via an external application
	// (using AddOnHost)
	bool									m_useAddOnHost;
};

__END_CORTEX_NAMESPACE
#endif /*__NodeManager_H__*/
