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


// NodeManager.cpp

#include "NodeManager.h"

#include "AddOnHost.h"
#include "Connection.h"
#include "NodeGroup.h"
#include "NodeRef.h"

#include <Debug.h>
#include <MediaRoster.h>

#include <algorithm>
#include <cstring>
#include <functional>
#include <list>
#include <set>

#include "set_tools.h"
#include "functional_tools.h"

#include "node_manager_impl.h"

using namespace std;

__USE_CORTEX_NAMESPACE

#define D_METHOD(x) //PRINT (x)
#define D_MESSAGE(x) //PRINT (x)
#define D_ROSTER(x) //PRINT (x)
#define D_LOCK(x) //PRINT (x)

// -------------------------------------------------------- //
// messaging constants
// -------------------------------------------------------- //

// B_MEDIA_CONNECTION_BROKEN
const char* const _connectionField = "__connection_id";
const char* const _sourceNodeField = "__source_node_id";
const char* const _destNodeField = "__destination_node_id";




// -------------------------------------------------------- //
// *** hooks
// -------------------------------------------------------- //

// [e.moon 7nov99] these hooks are called during processing of
// BMediaRoster messages, before any notification is sent to
// observers.  For example, if a B_MEDIA_NODES_CREATED message
// were received describing 3 new nodes, nodeCreated() would be
// called 3 times before the notification was sent.

void NodeManager::nodeCreated(
	NodeRef*											ref) {}

void NodeManager::nodeDeleted(
	const NodeRef*								ref) {}

void NodeManager::connectionMade(
	Connection*										connection) {}

void NodeManager::connectionBroken(
	const Connection*							connection) {}

void NodeManager::connectionFailed(
	const media_output &							output,
	const media_input &								input,
	const media_format &							format,
	status_t error) {}

// -------------------------------------------------------- //
// helpers
// -------------------------------------------------------- //

class _for_each_state {
public:
	// marks nodes visited
	set<media_node_id>		visited;
};

// [e.moon 28sep99] graph crawling
// - does NOT apply operation to origin node.
// - if inGroup is non-0, visits only nodes in that group.
//
// [e.moon 13oct99]: no longer supports locking (use _lockAllGroups() to
// be sure all nodes are locked, if necessary.)

template<class Op>
void _do_for_each_connected(
	NodeManager*											manager,
	NodeRef*													origin,
	NodeGroup*												inGroup,
	bool															recurse,
	Op																operation,
	_for_each_state*									state) {

//	PRINT(("### _do_for_each_connected()\n"));

	ASSERT(manager->IsLocked());

	ASSERT(origin);
	ASSERT(state);
	status_t err;

	if(state->visited.find(origin->id()) != state->visited.end()) {
//		PRINT(("### already visited\n"));
		// already visited
		return;
	}

	// used to walk connections
	vector<Connection>		connections;

	// mark visited
	state->visited.insert(origin->id());

	// walk input connections
	origin->getInputConnections(connections);
	for(uint32 n = 0; n < connections.size(); ++n) {

		if(!connections[n].isValid())
			continue;

//		PRINT(("# source: %ld\n", connections[n].sourceNode()));

		NodeRef* targetRef;
		err = manager->getNodeRef(
			connections[n].sourceNode(),
			&targetRef);
		ASSERT(err == B_OK);
		ASSERT(targetRef);

		if(inGroup && targetRef->group() != inGroup) {
//			PRINT(("# .group mismatch\n"));
			// don't need to visit
			return;
		}

		// invoke operation
//		if(lockRef)
//			targetRef->lock();
		operation(targetRef);
//		if(lockRef)
//			targetRef->unlock();

		// recurse?
		if(recurse)
			_do_for_each_connected(
				manager,
				targetRef,
				inGroup,
				true,
				operation,
				state);
	}

	// walk output connections
	connections.clear();
	origin->getOutputConnections(connections);
	for(uint32 n = 0; n < connections.size(); ++n) {
//		PRINT(("# dest: %ld\n", connections[n].destinationNode()));

		if(!connections[n].isValid())
			continue;

		NodeRef* targetRef;
		err = manager->getNodeRef(
			connections[n].destinationNode(),
			&targetRef);
		ASSERT(err == B_OK);
		ASSERT(targetRef);

		if(inGroup && targetRef->group() != inGroup) {
//			PRINT(("# .group mismatch\n"));
			// don't need to visit
			return;
		}

		// invoke operation
//		if(lockRef)
//			targetRef->lock();
		operation(targetRef);
//		if(lockRef)
//			targetRef->unlock();

		// recurse?
		if(recurse)
			_do_for_each_connected(
				manager,
				targetRef,
				inGroup,
				true,
				operation,
				state);
	}
}

// dtor helpers
inline void NodeManager::_clearGroup(
	NodeGroup*										group) {
	Autolock _l(group);
	D_METHOD((
		"NodeManager::_clearGroup()\n"));

	// stop group before clearing [10aug99]
	group->_stop();

	int32 n;
	while((n = group->countNodes()) > 0) {
		group->removeNode(n-1);
	}

//	// [e.moon 7nov99] release the group
//	status_t err = remove_observer(this, group);
//	if(err < B_OK) {
//		// spew diagnostics
//		PRINT((
//			"!!! NodeManager::_clearGroup(): remove_observer(group %ld):\n"
//			"    %s\n",
//			group->id(),
//			strerror(err)));
//	}
}

inline void NodeManager::_freeConnection(
	Connection*										connection) {
	ASSERT(connection);

	D_METHOD((
		"NodeManager::_freeConnection(%ld)\n", connection->id()));
	status_t err;

	// break if internal & still valid
	if(
		connection->isValid() &&
		connection->flags() & Connection::INTERNAL &&
		!(connection->flags() & Connection::LOCKED)) {

		D_METHOD((
			"! breaking connection:\n"
			"  source node:  %ld\n"
			"  source id:    %ld\n"
			"  source port:  %ld\n"
			"  dest node:    %ld\n"
			"  dest id:      %ld\n"
			"  dest port:    %ld\n",
			connection->sourceNode(),
			connection->source().id, connection->source().port,
			connection->destinationNode(),
			connection->destination().id, connection->destination().port));

		// do it
		D_ROSTER(("# roster->Disconnect()\n"));
		err = roster->Disconnect(
			connection->sourceNode(),
			connection->source(),
			connection->destinationNode(),
			connection->destination());

		if(err < B_OK) {
			D_METHOD((
				"!!! BMediaRoster::Disconnect('%s' -> '%s') failed:\n"
				"    %s\n",
				connection->outputName(), connection->inputName(),
				strerror(err)));
		}
	}

	// delete
	delete connection;
}

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //


NodeManager::~NodeManager() {
	D_METHOD((
		"~NodeManager()\n"));
	ASSERT(IsLocked());

	// make a list of nodes to be released
	list<NodeRef*> deadNodes;

	for(node_ref_map::const_iterator it = m_nodeRefMap.begin();
		it != m_nodeRefMap.end(); ++it) {
		deadNodes.push_back((*it).second);
	}

	// ungroup all nodes

// [e.moon 13oct99] making PPC compiler happy
//	for_each(
//		m_nodeGroupSet.begin(),
//		m_nodeGroupSet.end(),
//		bound_method(
//			*this,
//			&NodeManager::_clearGroup
//		)
//	);
	for(node_group_set::iterator it = m_nodeGroupSet.begin();
		it != m_nodeGroupSet.end(); ++it) {
		_clearGroup(*it);
	}

	// delete groups
	ptr_set_delete(
		m_nodeGroupSet.begin(),
		m_nodeGroupSet.end());
	m_nodeGroupSet.clear();


	// deallocate all connections; disconnect internal nodes
// [e.moon 13oct99] making PPC compiler happy
//	for_each(
//		m_conSourceMap.begin(),
//		m_conSourceMap.end(),
//		unary_map_function(
//			m_conSourceMap,
//			bound_method(
//				*this,
//				&NodeManager::_freeConnection
//			)
//		)
//	);
	for(con_map::iterator it = m_conSourceMap.begin();
		it != m_conSourceMap.end(); ++it) {
		_freeConnection((*it).second);
	}
	m_conSourceMap.clear();
	m_conDestinationMap.clear();

	// release all nodes
	for(list<NodeRef*>::const_iterator it = deadNodes.begin();
		it != deadNodes.end(); ++it) {
		(*it)->release();
	}

	if(m_nodeRefMap.size()) {
		// +++++ nodes will only remain if they have observers; cope!
		PRINT(("*** %ld nodes remaining!\n", m_nodeRefMap.size()));

		deadNodes.clear();
		for(node_ref_map::const_iterator it = m_nodeRefMap.begin();
			it != m_nodeRefMap.end(); ++it)
				deadNodes.push_back((*it).second);

		ptr_set_delete(
			deadNodes.begin(),
			deadNodes.end());
	}

//	for_each(
//		m_nodeRefMap.begin(),
//		m_nodeRefMap.end(),
//		unary_map_function(
//			m_nodeRefMap,
//			mem_fun(
//				&NodeRef::release
//			)
//		)
//	);
//
//	// delete all nodes
//	ptr_map_delete(
//		m_nodeRefMap.begin(),
//		m_nodeRefMap.end());
//
//
//	PRINT((
//		"~NodeManager() done\n"));
//
}

const char* const			NodeManager::s_defaultGroupPrefix = "No Name";
const char* const			NodeManager::s_timeSourceGroup = "Time Sources";
const char* const			NodeManager::s_audioInputGroup = "System Audio Input";
const char* const			NodeManager::s_videoInputGroup = "System Video Input";
const char* const			NodeManager::s_audioMixerGroup = "System Audio Mixer";
const char* const			NodeManager::s_videoOutputGroup = "System Video Output";

NodeManager::NodeManager(
	bool													useAddOnHost) :

	ObservableLooper("NodeManager"),
	roster(BMediaRoster::Roster()),
	m_audioInputNode(0),
	m_videoInputNode(0),
	m_audioMixerNode(0),
	m_audioOutputNode(0),
	m_videoOutputNode(0),
	m_nextConID(1),
	m_existingNodesInit(false),
	m_useAddOnHost(useAddOnHost) {

	D_METHOD((
		"NodeManager()\n"));

	ASSERT(roster);

	// create refs for common nodes
	_initCommonNodes();

	// start the looper
	Run();

	// initialize connection to the media roster
	D_ROSTER(("# roster->StartWatching(%p)\n", this));
	roster->StartWatching(BMessenger(this));
}

// -------------------------------------------------------- //
// *** operations
// -------------------------------------------------------- //

// * ACCESS

// fetches NodeRef corresponding to a given ID; returns
// B_BAD_VALUE if no matching entry was found (and writes
// a 0 into the provided pointer.)

status_t NodeManager::getNodeRef(
	media_node_id									id,
	NodeRef**											outRef) const {
	Autolock _l(this);

	D_METHOD((
		"NodeManager::getNodeRef(%ld)\n", id));

	node_ref_map::const_iterator it = m_nodeRefMap.find(id);
	if(it == m_nodeRefMap.end()) {
		*outRef = 0;
		return B_BAD_VALUE;
	}

	*outRef = (*it).second;
	return B_OK;
}

// [13aug99]
// fetches Connection corresponding to a given source/destination
// on a given node.  Returns an invalid connection and B_BAD_VALUE
// if no matching connection was found.

status_t NodeManager::findConnection(
	media_node_id									node,
	const media_source&						source,
	Connection*										outConnection) const {
	Autolock _l(this);

	D_METHOD((
		"NodeManager::findConnection()\n"));
	ASSERT(source != media_source::null);

	con_map::const_iterator it = m_conSourceMap.lower_bound(node);
	con_map::const_iterator itEnd = m_conSourceMap.upper_bound(node);
	for(; it != itEnd; ++it)
		if((*it).second->source() == source) {
			// copy connection
			*outConnection = *((*it).second);
			return B_OK;
		}

	*outConnection = Connection();
	return B_BAD_VALUE;
}

status_t NodeManager::findConnection(
	media_node_id									node,
	const media_destination&			destination,
	Connection*										outConnection) const {
	Autolock _l(this);

	D_METHOD((
		"NodeManager::findConnection()\n"));
	ASSERT(destination != media_destination::null);

	con_map::const_iterator it = m_conDestinationMap.lower_bound(node);
	con_map::const_iterator itEnd = m_conDestinationMap.upper_bound(node);
	for(; it != itEnd; ++it)
		if((*it).second->destination() == destination) {
			// copy connection
			*outConnection = *((*it).second);
			return B_OK;
		}

	*outConnection = Connection();
	return B_BAD_VALUE;
}

// [e.moon 28sep99]
// fetches a Connection matching the given source and destination
// nodes.  Returns an invalid connection and B_BAD_VALUE if
// no matching connection was found

status_t NodeManager::findConnection(
	media_node_id									sourceNode,
	media_node_id									destinationNode,
	Connection*										outConnection) const {
	Autolock _l(this);

	D_METHOD((
		"NodeManager::findConnection(source %ld, dest %ld)\n", sourceNode, destinationNode));

	con_map::const_iterator it = m_conSourceMap.lower_bound(sourceNode);
	con_map::const_iterator itEnd = m_conSourceMap.upper_bound(sourceNode);
	for(; it != itEnd; ++it) {
		if((*it).second->destinationNode() == destinationNode) {
			*outConnection = *((*it).second);
			return B_OK;
		}
	}

	*outConnection = Connection();
	return B_BAD_VALUE;
}

// [e.moon 28sep99]
// tries to find a route from 'nodeA' to 'nodeB'; returns
// true if one exists, false if not.

class NodeManager::_find_route_state {
public:
	set<media_node_id>						visited;
};

bool NodeManager::findRoute(
	media_node_id									nodeA,
	media_node_id									nodeB) {
	Autolock _l(this);

	D_METHOD((
		"NodeManager::findRoute(%ld, %ld)\n", nodeA, nodeB));
	status_t err;

	NodeRef* ref;
	err = getNodeRef(nodeA, &ref);
	if(err < B_OK) {
		PRINT((
			"!!! NodeManager::findRoute(%ld, %ld): no ref for node %ld\n",
			nodeA, nodeB, nodeA));
		return false;
	}

	_find_route_state st;
	return _find_route_recurse(ref, nodeB, &st);
}

// implementation of above

bool NodeManager::_find_route_recurse(
	NodeRef*													origin,
	media_node_id											target,
	_find_route_state*								state) {

	ASSERT(IsLocked());
	ASSERT(origin);
	ASSERT(state);
	status_t err;

	// node already visited?
	if(state->visited.find(origin->id()) != state->visited.end()) {
		return false;
	}

	// mark node visited
	state->visited.insert(origin->id());

	vector<Connection> connections;

	// walk input connections
	origin->getInputConnections(connections);
	for(uint32 n = 0; n < connections.size(); ++n) {

		if(!connections[n].isValid())
			continue;

		// test against target
		if(connections[n].sourceNode() == target)
			return true; // SUCCESS

		// recurse
		NodeRef* ref;
		err = getNodeRef(
			connections[n].sourceNode(),
			&ref);
		ASSERT(err == B_OK);
		ASSERT(ref);

		if(_find_route_recurse(
			ref,
			target,
			state))
			return true; // SUCCESS
	}

	// walk output connections
	connections.clear();
	origin->getOutputConnections(connections);
	for(uint32 n = 0; n < connections.size(); ++n) {

		if(!connections[n].isValid())
			continue;

		// test against target
		if(connections[n].destinationNode() == target)
			return true; // SUCCESS

		// recurse
		NodeRef* ref;
		err = getNodeRef(
			connections[n].destinationNode(),
			&ref);
		ASSERT(err == B_OK);
		ASSERT(ref);

		if(_find_route_recurse(
			ref,
			target,
			state))
			return true; // SUCCESS
	}

	return false; // FAILED
}



// fetches Connection corresponding to a given source or
// destination; Returns an invalid connection and B_BAD_VALUE if
// none found.
// * Note: this is the slowest possible way to look up a
//   connection.  Since the low-level source/destination
//   structures don't include a node ID, and a destination
//   port can differ from its node's control port, a linear
//   search of all known connections is performed.  Only
//   use these methods if you have no clue what node the
//   connection corresponds to.

status_t NodeManager::findConnection(
	const media_source&						source,
	Connection*										outConnection) const {
	Autolock _l(this);

	D_METHOD((
		"NodeManager::findConnection()\n"));
	ASSERT(source != media_source::null);

	for(con_map::const_iterator it = m_conSourceMap.begin();
		it != m_conSourceMap.end(); ++it) {
		if((*it).second->source() == source) {
			// copy connection
			*outConnection = *((*it).second);
			return B_OK;
		}
	}

	*outConnection = Connection();
	return B_BAD_VALUE;
}

status_t NodeManager::findConnection(
	const media_destination&			destination,
	Connection*										outConnection) const {
	Autolock _l(this);

	D_METHOD((
		"NodeManager::findConnection()\n"));
	ASSERT(destination != media_destination::null);

	for(con_map::const_iterator it = m_conDestinationMap.begin();
		it != m_conDestinationMap.end(); ++it) {
		if((*it).second->destination() == destination) {
			// copy connection
			*outConnection = *((*it).second);
			return B_OK;
		}
	}

	*outConnection = Connection();
	return B_BAD_VALUE;
}


// fetch NodeRefs for system nodes (if a particular node doesn't
// exist, these methods return 0)

NodeRef* NodeManager::audioInputNode() const {
	Autolock _l(this);
	return m_audioInputNode;
}
NodeRef* NodeManager::videoInputNode() const {
	Autolock _l(this);
	return m_videoInputNode;
}
NodeRef* NodeManager::audioMixerNode() const {
	Autolock _l(this);
	return m_audioMixerNode;
}
NodeRef* NodeManager::audioOutputNode() const {
	Autolock _l(this);
	return m_audioOutputNode;
}
NodeRef* NodeManager::videoOutputNode() const {
	Autolock _l(this);
	return m_videoOutputNode;
}

// fetch groups by index
// - you can write-lock the manager during sets of calls to these methods;
//   this ensures that the group set won't change.  The methods do lock
//   the group internally, so locking isn't explicitly required.

uint32 NodeManager::countGroups() const {
	Autolock _l(this);
	D_METHOD((
		"NodeManager::countGroups()\n"));

	return m_nodeGroupSet.size();
}

NodeGroup* NodeManager::groupAt(
	uint32												index) const {
	Autolock _l(this);
	D_METHOD((
		"NodeManager::groupAt()\n"));

	return (index < m_nodeGroupSet.size()) ?
		m_nodeGroupSet[index] :
		0;
}

// look up a group by unique ID; returns B_BAD_VALUE if no
// matching group was found

class match_group_by_id :
	public binary_function<const NodeGroup*, uint32, bool> {
public:
	bool operator()(const NodeGroup* group, uint32 id) const {
		return group->id() == id;
	}
};

status_t NodeManager::findGroup(
	uint32												id,
	NodeGroup**										outGroup) const {
	Autolock _l(this);
	D_METHOD((
		"NodeManager::findGroup(id)\n"));

	node_group_set::const_iterator it =
		find_if(
			m_nodeGroupSet.begin(),
			m_nodeGroupSet.end(),
			bind2nd(match_group_by_id(), id)
		);

	if(it == m_nodeGroupSet.end()) {
		*outGroup = 0;
		return B_BAD_VALUE;
	}

	*outGroup = *it;
	return B_OK;
}

// look up a group by name; returns B_NAME_NOT_FOUND if
// no group matching the name was found.

class match_group_by_name :
	public binary_function<const NodeGroup*, const char*, bool> {
public:
	bool operator()(const NodeGroup* group, const char* name) const {
		return !strcmp(group->name(), name);
	}
};

status_t NodeManager::findGroup(
	const char*										name,
	NodeGroup**										outGroup) const {
	Autolock _l(this);
	D_METHOD((
		"NodeManager::findGroup(name)\n"));

	node_group_set::const_iterator it =
		find_if(
			m_nodeGroupSet.begin(),
			m_nodeGroupSet.end(),
			bind2nd(match_group_by_name(), name)
		);

	if(it == m_nodeGroupSet.end()) {
		*outGroup = 0;
		return B_BAD_VALUE;
	}

	*outGroup = *it;
	return B_OK;
}

// merge the given source group to the given destination;
// empties and releases the source group

status_t NodeManager::mergeGroups(
	NodeGroup*										sourceGroup,
	NodeGroup*										destinationGroup) {
	Autolock _l(this);
	D_METHOD((
		"NodeManager::mergeGroups(name)\n"));

	status_t err;

	// [5feb00 c.lenz] already merged
	if(sourceGroup->id() == destinationGroup->id())
		return B_OK;

	if(sourceGroup->isReleased() || destinationGroup->isReleased())
		return B_NOT_ALLOWED;

	for(uint32 n = sourceGroup->countNodes(); n; --n) {
		NodeRef* node = sourceGroup->nodeAt(n-1);
		ASSERT(node);
		err = sourceGroup->removeNode(n-1);
		ASSERT(err == B_OK);
		err = destinationGroup->addNode(node);
		ASSERT(err == B_OK);
	}

	// [7nov99 e.moon] delete the source group
	_removeGroup(sourceGroup);
	sourceGroup->release();

	return B_OK;
}

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


class _changeNodeGroupFn :
	public	unary_function<NodeRef*, void> {
public:
	NodeGroup*										newGroup;

	_changeNodeGroupFn(
		NodeGroup*									_newGroup) : newGroup(_newGroup) {
		ASSERT(newGroup);
	}

	void operator()(
		NodeRef*										node) {

		PRINT((
			"_changeNodeGroupFn(): '%s'\n", node->name()));

		status_t err;
		NodeGroup* oldGroup = node->group();
		if(oldGroup) {
			err = oldGroup->removeNode(node);
			ASSERT(err == B_OK);
		}

		err = newGroup->addNode(node);
		ASSERT(err == B_OK);
	}
};

status_t NodeManager::splitGroup(
	NodeRef*											insideNode,
	NodeRef*											outsideNode,
	NodeGroup**										outGroup) {

	ASSERT(insideNode);
	ASSERT(outsideNode);

	Autolock _l(this);

	// ensure that no route exists from insideNode to outsideNode
	if(findRoute(insideNode->id(), outsideNode->id())) {
		PRINT((
			"!!! NodeManager::splitGroup(): route exists from %ld to %ld\n",
			insideNode->id(), outsideNode->id()));
		return B_NOT_ALLOWED;
	}

	// make sure the nodes share a common group
	NodeGroup* oldGroup = insideNode->group();
	if(!oldGroup) {
		PRINT((
			"!!! NodeManager::splitGroup(): invalid group\n",
			insideNode->id(), outsideNode->id()));
		return B_NOT_ALLOWED;
	}
	if(oldGroup != outsideNode->group()) {
		PRINT((
			"!!! NodeManager::splitGroup(): mismatched groups for %ld and %ld\n",
			insideNode->id(), outsideNode->id()));
		return B_NOT_ALLOWED;
	}

	Autolock _l_old_group(oldGroup);

	// create the new group
	BString nameBuffer = oldGroup->name();
	nameBuffer << " split";

	NodeGroup* newGroup = createGroup(
		nameBuffer.String(),
		oldGroup->runMode());
	*outGroup = newGroup;

	// move nodes connected to outsideNode from old to new group
	_changeNodeGroupFn fn(newGroup);
	fn(outsideNode);

	_for_each_state st;
	_do_for_each_connected(
		this,
		outsideNode,
		oldGroup,
		true,
		fn,
		&st);

	// [e.moon 1dec99] a single-node group takes that node's name
	if(newGroup->countNodes() == 1)
		newGroup->setName(newGroup->nodeAt(0)->name());

	if(oldGroup->countNodes() == 1)
		oldGroup->setName(oldGroup->nodeAt(0)->name());

	return B_OK;
}


// * INSTANTIATION & CONNECTION
//   Use these calls rather than the associated BMediaRoster()
//   methods to assure that the nodes and connections you set up
//   can be properly serialized & reconstituted.

// basic BMediaRoster::InstantiateDormantNode() wrapper

status_t NodeManager::instantiate(
	const dormant_node_info&			info,
	NodeRef**											outRef,
	bigtime_t											timeout,
	uint32												nodeFlags) {

	Autolock _l(this);
	status_t err;
	D_METHOD((
		"NodeManager::instantiate()\n"));

	// * instantiate

	media_node node;

	if(m_useAddOnHost) {
		err = AddOnHost::InstantiateDormantNode(
			info, &node, timeout);

		if(err < B_OK) {
			node = media_node::null;

			// attempt to relaunch
			BMessenger mess;
			err = AddOnHost::Launch(&mess);
			if(err < B_OK) {
				PRINT((
					"!!! NodeManager::instantiate(): giving up on AddOnHost\n"));

				m_useAddOnHost = false;
			}
			else {
				err = AddOnHost::InstantiateDormantNode(
					info, &node, timeout);
			}
		}
	}

	if(!m_useAddOnHost || node == media_node::null) {
		D_ROSTER((
			"# roster->InstantiateDormantNode()\n"));
		err = roster->InstantiateDormantNode(info, &node);
	}

	if(err < B_OK) {
		*outRef = 0;
		return err;
	}

	if(node == media_node::null) {
		// [e.moon 23oct99] +++++
		// instantiating a soundcard input/output (emu10k or sonic_vibes)
		// produces a node ID of -1 (R4.5.2 x86)
		//
		PRINT((
			"! InstantiateDormantNode(): invalid media node\n"));

		// bail out
		*outRef = 0;
		return B_BAD_INDEX;
	}

	// * create NodeRef
	NodeRef* ref = new NodeRef(
		node,
		this,
		nodeFlags, // | NodeRef::NO_ROSTER_WATCH, // +++++ e.moon 11oct99
		NodeRef::_INTERNAL);

	ref->_setAddonHint(&info);
	_addRef(ref);

	// * return it
	*outRef = ref;
	return B_OK;
}

// SniffRef/Instantiate.../SetRefFor: a one-call interface
// to create a node capable of playing a given media file.

status_t NodeManager::instantiate(
	const entry_ref&							file,
	uint64												requireNodeKinds,
	NodeRef**											outRef,
	bigtime_t											timeout,
	uint32												nodeFlags,
	bigtime_t*										outDuration) {

	D_METHOD((
		"NodeManager::instantiate(ref)\n"));

	// [no lock needed; calls the full form of instantiate()]

	status_t err;

	// * Find matching add-on
	dormant_node_info info;

	D_ROSTER((
		"# roster->SniffRef()\n"));
	err = roster->SniffRef(
		file,
		requireNodeKinds,
		&info);
	if(err < B_OK) {
		*outRef = 0;
		return err;
	}

	// * Instantiate
	err = instantiate(info, outRef, timeout, nodeFlags);

	if(err < B_OK)
		return err;

	ASSERT(*outRef);

	// * Set file to play
	bigtime_t dur;
	D_ROSTER(("# roster->SetRefFor()\n"));
	err = roster->SetRefFor(
		(*outRef)->node(),
		file,
		false,
		&dur);

	if(err < B_OK) {
		PRINT((
			"* SetRefFor() failed: %s\n", strerror(err)));
	}
	else if(outDuration)
		*outDuration = dur;

	// * update info [e.moon 29sep99]
	Autolock _l(*outRef);
	(*outRef)->_setAddonHint(&info, &file);

	return err;
}

// use this method to reference nodes internal to your
// application.

status_t NodeManager::reference(
	BMediaNode*										node,
	NodeRef**											outRef,
	uint32												nodeFlags) {

	Autolock _l(this);
	D_METHOD((
		"NodeManager::reference()\n"));

	// should this node be marked _NO_RELEASE?
	NodeRef* ref = new NodeRef(node->Node(), this, nodeFlags, 0);
	_addRef(ref);

	*outRef = ref;
	return B_OK;
}

// the most flexible form of connect(): set the template
// format as you would for BMediaRoster::Connect().

status_t NodeManager::connect(
	const media_output&						output,
	const media_input&						input,
	const media_format&						templateFormat,
	Connection*										outConnection /*=0*/) {

	Autolock _l(this);
	status_t err;
	D_METHOD((
		"NodeManager::connect()\n"));

	// * Find (& create if necessary) NodeRefs

	NodeRef* outputRef;
	if(getNodeRef(output.node.node, &outputRef) < B_OK)
		outputRef = _addRefFor(output.node, 0);

	NodeRef* inputRef;
	if(getNodeRef(input.node.node, &inputRef) < B_OK)
		inputRef = _addRefFor(input.node, 0);

	// * Connect the nodes

	media_output finalOutput;
	media_input finalInput;
	media_format finalFormat = templateFormat;

	D_ROSTER(("# roster->Connect()\n"));
	err = roster->Connect(
		output.source,
		input.destination,
		&finalFormat,
		&finalOutput,
		&finalInput);

	if(err < B_OK) {
		if(outConnection)
			*outConnection = Connection();
		connectionFailed(output, input, templateFormat, err);
		return err;
	}

	// * Create Connection instance; mark it INTERNAL
	//   to automatically remove it upon shutdown.

	D_METHOD((
		"! creating connection:\n"
		"  source id:    %ld\n"
		"  source port:  %ld\n"
		"  dest id:      %ld\n"
		"  dest port:    %ld\n",
		finalOutput.source.id, finalOutput.source.port,
		finalInput.destination.id, finalInput.destination.port));

	uint32 cflags = Connection::INTERNAL;
	if(outputRef->m_info.node.kind & B_FILE_INTERFACE) {
		// 3aug99:
		// workaround for bug 19990802-12798:
		//   connections involving a file-interface node aren't removed
		cflags |= Connection::LOCKED;
	}

	Connection* con = new Connection(
		m_nextConID++,
		output.node,
		finalOutput.source,
		finalOutput.name,
		input.node,
		finalInput.destination,
		finalInput.name,
		finalFormat,
		cflags);

	con->setOutputHint(
		output.name,
		output.format);

	con->setInputHint(
		input.name,
		input.format);

	con->setRequestedFormat(
		templateFormat);

	_addConnection(con);

	// [e.moon 10aug99]
	// fetch updated latencies;
	// [e.moon 28sep99]
	// scan all nodes connected directly OR indirectly to the
	// newly-connected nodes and update their latencies -- this includes
	// recalculating the node's 'producer delay' if in B_RECORDING mode.

	_updateLatenciesFrom(inputRef, true);

	// copy connection
	if(outConnection) {
		*outConnection = *con;
	}
	return B_OK;
}

// format-guessing form of connect(): tries to find
// a common format between output & input before connection;
// returns B_MEDIA_BAD_FORMAT if no common format appears
// possible.
//
// NOTE: the specifics of the input and output formats are ignored;
//       this method only looks at the format type, and properly
//       handles wildcards at that level (B_MEDIA_NO_TYPE).

status_t NodeManager::connect(
	const media_output&						output,
	const media_input&						input,
	Connection*										outConnection /*=0*/) {

	D_METHOD((
		"NodeManager::connect(guess)\n"));

	// [no lock needed; calls the full form of connect()]

	// defer to the pickier endpoint
	media_format f;

	if(output.format.type > B_MEDIA_UNKNOWN_TYPE) {
		f = output.format;
		if ((input.format.type > B_MEDIA_UNKNOWN_TYPE) &&
			(f.type != input.format.type)) {
			connectionFailed(output, input, f, B_MEDIA_BAD_FORMAT);
			return B_MEDIA_BAD_FORMAT;
		}
	}
	else if(input.format.type > B_MEDIA_UNKNOWN_TYPE) {
		f = input.format;
		// output node doesn't care
	}
	else {
		// about as non-picky as two nodes can possibly be
		f.type = B_MEDIA_UNKNOWN_TYPE;
	}

	// +++++ ? revert to wildcard ?

	// let the nodes try to work out a common format from here
	return connect(
		output,
		input,
		f,
		outConnection);
}

// disconnects the connection represented by the provided
// Connection object.  if successful, returns B_OK.

status_t NodeManager::disconnect(
	const Connection&								connection) {

	Autolock _l(this);
	status_t err;
	D_METHOD((
		"NodeManager::disconnect()\n"));

	// don't bother trying to disconnect an invalid connection
	if(!connection.isValid())
		return B_NOT_ALLOWED;

	// make sure connection can be released
	if(connection.flags() & Connection::LOCKED) {
		PRINT((
			"NodeManager::disconnect(): connection locked:\n"
			"  %ld:%s -> %ld:%s\n",
			connection.sourceNode(),
			connection.outputName(),
			connection.destinationNode(),
			connection.inputName()));
		return B_NOT_ALLOWED;
	}

	D_METHOD((
		"! breaking connection:\n"
		"  source node:  %ld\n"
		"  source id:    %ld\n"
		"  source port:  %ld\n"
		"  dest node:    %ld\n"
		"  dest id:      %ld\n"
		"  dest port:    %ld\n",
		connection.sourceNode(),
		connection.source().id, connection.source().port,
		connection.destinationNode(),
		connection.destination().id, connection.destination().port));

	// do it
	D_ROSTER(("# roster->Disconnect()\n"));
	err = roster->Disconnect(
		connection.sourceNode(),
		connection.source(),
		connection.destinationNode(),
		connection.destination());

	// mark disconnected
	if(err == B_OK) {
		con_map::iterator it = m_conSourceMap.lower_bound(connection.sourceNode());
		con_map::iterator itEnd = m_conSourceMap.upper_bound(connection.sourceNode());
		for(; it != itEnd; ++it)
			if((*it).second->id() == connection.id()) {
				(*it).second->m_disconnected = true;
				break;
			}
		ASSERT(it != itEnd);

		// [e.moon 28sep99]
		// fetch updated latencies;
		// scan all nodes connected directly OR indirectly to the
		// newly-connected nodes and update their latencies -- this includes
		// recalculating the node's 'producer delay' if in B_RECORDING mode.

		NodeRef* ref;
		if(getNodeRef(connection.sourceNode(), &ref) == B_OK)
			_updateLatenciesFrom(ref, true);
		if(getNodeRef(connection.destinationNode(), &ref) == B_OK)
			_updateLatenciesFrom(ref, true);

	} else {
		// +++++ if this failed, somebody somewhere is mighty confused
		PRINT((
			"NodeManager::disconnect(): Disconnect() failed:\n  %s\n",
			strerror(err)));
	}

	return err;
}

// * GROUP CREATION

NodeGroup* NodeManager::createGroup(
	const char*										name,
	BMediaNode::run_mode					runMode) {

	Autolock _l(this);
	D_METHOD((
		"NodeManager::createGroup()\n"));

	NodeGroup* g = new NodeGroup(name, this, runMode);
	_addGroup(g);

	return g;
}

// -------------------------------------------------------- //
// *** node/connection iteration
// *** MUST BE LOCKED for any of these calls
// -------------------------------------------------------- //

// usage:
//   For the first call, pass 'cookie' a pointer to a void* set to 0.
//   Returns B_BAD_INDEX when the set of nodes has been exhausted (and
//   re-zeroes the cookie, cleaning up any unused memory.)

status_t NodeManager::getNextRef(
	NodeRef**											ref,
	void**												cookie) {
	ASSERT(IsLocked());
	ASSERT(cookie);

	if(!*cookie)
		*cookie = new node_ref_map::iterator(m_nodeRefMap.begin());

	node_ref_map::iterator* pit = (node_ref_map::iterator*)*cookie;

	// at end of set?
	if(*pit == m_nodeRefMap.end()) {
		delete pit;
		*cookie = 0;
		return B_BAD_INDEX;
	}

	// return next entry
	*ref = (*(*pit)).second;
	++(*pit);
	return B_OK;
}

// +++++ reworked 13sep99: dtors wouldn't have been called with 'delete *cookie'! +++++
void NodeManager::disposeRefCookie(
	void**												cookie) {

	if(!cookie)
		return;

	node_ref_map::iterator* it =
		reinterpret_cast<node_ref_map::iterator*>(*cookie);
	ASSERT(it);
	if(it)
		delete it;
}

status_t NodeManager::getNextConnection(
	Connection*										connection,
	void**												cookie) {
	ASSERT(IsLocked());
	ASSERT(cookie);

	if(!*cookie)
		*cookie = new con_map::iterator(m_conSourceMap.begin());

	con_map::iterator* pit = (con_map::iterator*)*cookie;

	// at end of set?
	if(*pit == m_conSourceMap.end()) {
		delete pit;
		*cookie = 0;
		return B_BAD_INDEX;
	}

	// return next entry (ewww)
	*connection = *((*(*pit)).second);
	++(*pit);
	return B_OK;
}

// +++++ reworked 13sep99: dtors wouldn't have been called with 'delete *cookie'! +++++
void NodeManager::disposeConnectionCookie(
	void**												cookie) {

	if(!cookie)
		return;

	con_map::iterator* it =
		reinterpret_cast<con_map::iterator*>(*cookie);
	ASSERT(it);
	if(it)
		delete it;
}


// -------------------------------------------------------- //
// *** BHandler impl
// -------------------------------------------------------- //

// +++++ support all Media Roster messages! +++++

// [e.moon 7nov99] implemented observer pattern for NodeGroup
void NodeManager::MessageReceived(BMessage* message) {

	D_MESSAGE((
		"NodeManager::MessageReceived(): %c%c%c%c\n",
		 message->what >> 24,
		(message->what >> 16)	& 0xff,
		(message->what >> 8)	& 0xff,
		(message->what) 			& 0xff));

	switch(message->what) {

		// *** Media Roster messages ***

		case B_MEDIA_NODE_CREATED:
			if(_handleNodesCreated(message) == B_OK)
				notify(message);
			break;

		case B_MEDIA_NODE_DELETED:
			_handleNodesDeleted(message);
			notify(message);
			break;

		case B_MEDIA_CONNECTION_MADE:
			_handleConnectionMade(message);
			notify(message);
			break;

		case B_MEDIA_CONNECTION_BROKEN:
			_handleConnectionBroken(message); // augments message!
			notify(message);
			break;

		case B_MEDIA_FORMAT_CHANGED:
			_handleFormatChanged(message);
			notify(message);
			break;

		default:
			_inherited::MessageReceived(message);
			break;
	}
}

// -------------------------------------------------------- //
// *** IObservable hooks
// -------------------------------------------------------- //

void NodeManager::observerAdded(
	const BMessenger&				observer) {

	BMessage m(M_OBSERVER_ADDED);
	m.AddMessenger("target", BMessenger(this));
	observer.SendMessage(&m);
}


void NodeManager::observerRemoved(
	const BMessenger&				observer) {

	BMessage m(M_OBSERVER_REMOVED);
	m.AddMessenger("target", BMessenger(this));
	observer.SendMessage(&m);
}

void NodeManager::notifyRelease() {

	BMessage m(M_RELEASED);
	m.AddMessenger("target", BMessenger(this));
	notify(&m);
}

void NodeManager::releaseComplete() {
	// tear down media roster connection
	D_ROSTER(("# roster->StopWatching()\n"));
	status_t err = roster->StopWatching(
		BMessenger(this));
	if(err < B_OK) {
		PRINT((
			"* roster->StopWatching() failed: %s\n", strerror(err)));
	}
}


// -------------------------------------------------------- //
// *** ILockable impl.
// -------------------------------------------------------- //

bool NodeManager::lock(
	lock_t												type,
	bigtime_t											timeout) {

	D_LOCK(("*** NodeManager::lock(): %ld\n", find_thread(0)));

	status_t err = LockWithTimeout(timeout);


	D_LOCK(("*** NodeManager::lock() ACQUIRED: %ld\n", find_thread(0)));

	return err == B_OK;
}

bool NodeManager::unlock(
	lock_t												type) {

	D_LOCK(("*** NodeManager::unlock(): %ld\n", find_thread(0)));

	Unlock();

	D_LOCK(("*** NodeManager::unlock() RELEASED: %ld\n", find_thread(0)));

	return true;
}

bool NodeManager::isLocked(
	lock_t												type) const {

	return IsLocked();
}

// -------------------------------------------------------- //
// *** internal operations (LOCK REQUIRED)
// -------------------------------------------------------- //

void NodeManager::_initCommonNodes() {

	ASSERT(IsLocked());
	status_t err;
	media_node node;

	D_METHOD((
		"NodeManager::_initCommonNodes()\n"));

	uint32 disableTransport =
		(NodeRef::NO_START_STOP | NodeRef::NO_SEEK | NodeRef::NO_PREROLL);

	// video input
	D_ROSTER(("# roster->GetVideoInput()\n"));
	err = roster->GetVideoInput(&node);
	if(err == B_OK)
		m_videoInputNode = _addRefFor(
			node,
			_userFlagsForKind(node.kind),
			_implFlagsForKind(node.kind));

	// video output
	D_ROSTER(("# roster->GetVideoOutput()\n"));
	err = roster->GetVideoOutput(&node);
	if(err == B_OK) {
		if(m_videoInputNode && node.node == m_videoInputNode->id()) {
			// input and output nodes identical
			// [e.moon 20dec99]
			m_videoOutputNode = m_videoInputNode;
		}
		else {
			m_videoOutputNode = _addRefFor(
				node,
				_userFlagsForKind(node.kind) & ~NodeRef::NO_START_STOP,
				_implFlagsForKind(node.kind));
		}
	}

	// audio mixer
	D_ROSTER(("# roster->GetAudioMixer()\n"));
	err = roster->GetAudioMixer(&node);
	if(err == B_OK)
		m_audioMixerNode = _addRefFor(
			node,
			_userFlagsForKind(node.kind) | disableTransport,
			_implFlagsForKind(node.kind));

	// audio input
	D_ROSTER(("# roster->GetAudioInput()\n"));
	err = roster->GetAudioInput(&node);
	if(err == B_OK)
		m_audioInputNode = _addRefFor(
			node,
			_userFlagsForKind(node.kind),
			_implFlagsForKind(node.kind));

	// audio output
	D_ROSTER(("# roster->GetAudioOutput()\n"));
	err = roster->GetAudioOutput(&node);
	if(err == B_OK) {
		if(m_audioInputNode && node.node == m_audioInputNode->id()) {
			// input and output nodes identical
			// [e.moon 20dec99]
			m_audioOutputNode = m_audioInputNode;

			// disable transport controls (the default audio output must always
			// be running, and can't be seeked.)

			m_audioOutputNode->setFlags(
				m_audioOutputNode->flags() | disableTransport);
		}
		else {
			m_audioOutputNode = _addRefFor(
				node,
				_userFlagsForKind(node.kind) | disableTransport,
				_implFlagsForKind(node.kind));
		}
	}
}

void NodeManager::_addRef(
	NodeRef*											ref) {

	ASSERT(ref);
	ASSERT(IsLocked());

	D_METHOD((
		"NodeManager::_addRef()\n"));

	// precondition: no NodeRef yet exists for this node
	// +++++
	// [e.moon 21oct99]
	// <markjan@xs4all.nl> sez this fails on startup w/ MediaKit 10.5
	ASSERT(
		m_nodeRefMap.find(ref->id()) == m_nodeRefMap.end());

	// add it
	// [e.moon 13oct99] PPC-friendly
	m_nodeRefMap.insert(node_ref_map::value_type(ref->id(), ref));

	// [e.moon 8nov99] call hook
	nodeCreated(ref);
}

void NodeManager::_removeRef(
	media_node_id									id) {
	ASSERT(IsLocked());

	D_METHOD((
		"NodeManager::_removeRef()\n"));

	node_ref_map::iterator it = m_nodeRefMap.find(id);

	// precondition: a NodeRef w/ matching ID is in the map
	ASSERT(it != m_nodeRefMap.end());

	// [e.moon 8nov99] call hook
	nodeDeleted((*it).second);

	// remove it
	m_nodeRefMap.erase(it);
}

// create, add, return a NodeRef for the given external node;
// must not already exist
NodeRef* NodeManager::_addRefFor(
	const media_node&							node,
	uint32												nodeFlags,
	uint32												nodeImplFlags) {

	ASSERT(IsLocked());

	D_METHOD((
		"NodeManager::_addRefFor()\n"));

	// precondition: no NodeRef yet exists for this node
	ASSERT(
		m_nodeRefMap.find(node.node) == m_nodeRefMap.end());

//	// precondition: the node actually exists
//	live_node_info info;
//	D_ROSTER(("# roster->GetLiveNodeInfo()\n"));
//	ASSERT(roster->GetLiveNodeInfo(node, &info) == B_OK);

	// * create & add ref
	NodeRef* ref = new NodeRef(node, this, nodeFlags, nodeImplFlags);
	_addRef(ref);

	return ref;
}

void NodeManager::_addConnection(
	Connection*										connection) {

	ASSERT(connection);
	ASSERT(IsLocked());

	D_METHOD((
		"NodeManager::_addConnection()\n"));

	// precondition: not already entered in either source or dest map
	// +++++ a more rigorous test would be a very good thing
#ifdef DEBUG
	for(con_map::iterator it = m_conSourceMap.lower_bound(connection->sourceNode());
		it != m_conSourceMap.upper_bound(connection->sourceNode()); ++it) {
		ASSERT((*it).second->id() != connection->id());
	}
	for(con_map::iterator it = m_conDestinationMap.lower_bound(connection->destinationNode());
		it != m_conDestinationMap.upper_bound(connection->destinationNode()); ++it) {
		ASSERT((*it).second->id() != connection->id());
	}
#endif

	// add to both maps
	// [e.moon 13oct99] PPC-friendly
	m_conSourceMap.insert(
		con_map::value_type(
			connection->sourceNode(),
			connection));
	m_conDestinationMap.insert(
		con_map::value_type(
			connection->destinationNode(),
			connection));

	// [e.moon 8nov99] call hook
	connectionMade(connection);
}

void NodeManager::_removeConnection(
	const Connection&							connection) {

	ASSERT(IsLocked());
	con_map::iterator itSource, itDestination;

	D_METHOD((
		"NodeManager::_removeConnection()\n"));

	// [e.moon 8nov99] call hook
	connectionBroken(&connection);

	// precondition: one entry in both source & dest maps
	// +++++ a more rigorous test would be a very good thing

	for(
		itSource = m_conSourceMap.lower_bound(connection.sourceNode());
		itSource != m_conSourceMap.upper_bound(connection.sourceNode());
		++itSource)
		if((*itSource).second->id() == connection.id())
			break;

	ASSERT(itSource != m_conSourceMap.end());

	for(
		itDestination = m_conDestinationMap.lower_bound(connection.destinationNode());
		itDestination != m_conDestinationMap.upper_bound(connection.destinationNode());
		++itDestination)
		if((*itDestination).second->id() == connection.id())
			break;

	ASSERT(itDestination != m_conDestinationMap.end());

	// delete & remove from both maps
	delete (*itSource).second;
	m_conSourceMap.erase(itSource);
	m_conDestinationMap.erase(itDestination);
}

void NodeManager::_addGroup(
	NodeGroup*										group) {

	ASSERT(group);
	ASSERT(IsLocked());

	D_METHOD((
		"NodeManager::_addGroup()\n"));

	// precondition: group not already in set
	ASSERT(
		find(
			m_nodeGroupSet.begin(),
			m_nodeGroupSet.end(),
			group) == m_nodeGroupSet.end());

	// add
	m_nodeGroupSet.push_back(group);

//	// [e.moon 7nov99] observe
//	add_observer(this, group);
}

void NodeManager::_removeGroup(
	NodeGroup*										group) {

	ASSERT(group);
	ASSERT(IsLocked());

	D_METHOD((
		"NodeManager::_removeGroup()\n"));

	node_group_set::iterator it = find(
		m_nodeGroupSet.begin(),
		m_nodeGroupSet.end(),
		group);

	// precondition: group in set
	if(it == m_nodeGroupSet.end()) {
		PRINT((
			"* NodeManager::_removeGroup(%ld): group not in set.\n",
			group->id()));
		return;
	}

	// remove it
	m_nodeGroupSet.erase(it);
}

// -------------------------------------------------------- //
// *** Message Handlers ***
// -------------------------------------------------------- //


// now returns B_OK iff the message should be relayed to observers
// [e.moon 11oct99]

inline status_t NodeManager::_handleNodesCreated(
	BMessage*											message) {
	ASSERT(IsLocked());

	status_t err = B_OK;

	// fetch number of new nodes
	type_code type;
	int32 count;
	err = message->GetInfo("media_node_id", &type, &count);
	if(err < B_OK) {
		PRINT((
			"* NodeManager::_handleNodesCreated(): GetInfo() failed:\n"
			"  %s\n",
			strerror(err)));
		return err;
	}
	if(!count) {
		PRINT((
			"* NodeManager::_handleNodesCreated(): no node IDs in message.\n"));
		return err;
	}

	D_METHOD((
		"NodeManager::_handleNodesCreated(): %d nodes\n",
		count));

	// * Create NodeRef instances for the listed nodes.
	//   If this is the first message received from the Media Roster,
	//   no connection info will be received for these nodes; store them
	//   for now (indexed by service-thread port) and figure the connections
	//   afterwards.
	//   These nodes are mapped by port ID because that's the only criterion
	//   available for matching a media_source to a node.

	typedef map<port_id, NodeRef*> port_ref_map;
	port_ref_map* initialNodes = m_existingNodesInit ? 0 : new port_ref_map;

	bool refsCreated = false;

	for(int32 n = 0; n < count; ++n) {
		// fetch ID of next node
		int32 id;
		err = message->FindInt32("media_node_id", n, &id);
		if(err < B_OK) {
			PRINT((
				"* NodeManager::_handleNodesCreated(): FindInt32() failed:\n"
				"  %s", strerror(err)));
			continue;
		}

		// look up the node
		media_node node;
		err = roster->GetNodeFor(id, &node);
		if(err < B_OK) {
			PRINT((
				"* NodeManager::_handleNodesCreated(): roster->GetNodeFor(%ld) failed:\n"
				"  %s\n",
				id, strerror(err)));
			continue;
		}

		// look for an existing NodeRef; if not found, create one:
		NodeRef* ref = 0;
		if(getNodeRef(node.node, &ref) < B_OK) {
			// create one
			ref = _addRefFor(
				node,
				_userFlagsForKind(node.kind), // | NodeRef::NO_ROSTER_WATCH, // +++++ e.moon 11oct99
				_implFlagsForKind(node.kind) | NodeRef::_CREATE_NOTIFIED);

			refsCreated = true;

//			// [e.moon 7nov99] call hook
//			nodeCreated(ref);

		} else {
//			PRINT((
//				"* NodeManager::_handleNodesCreated():\n"
//				"  found existing ref for '%s' (%ld)\n",
//				ref->name(), id));


			// set _CREATE_NOTIFIED to prevent notification from being passed on
			// twice [e.moon 11oct99]
			if(!(ref->m_implFlags & NodeRef::_CREATE_NOTIFIED)) {
				ref->m_implFlags |= NodeRef::_CREATE_NOTIFIED;
				refsCreated = true;
			}

			// release the (duplicate) media_node reference
			err = roster->ReleaseNode(node);
			if(err < B_OK) {
				PRINT((
					"* NodeManager::_handleNodesCreated(): roster->ReleaseNode(%ld) failed:\n"
					"  %s\n",
					id, strerror(err)));
			}
		}

		// add to the 'initial nodes' set if necessary
		// [e.moon 13oct99] PPC-friendly
		if(initialNodes)
			initialNodes->insert(
				port_ref_map::value_type(
					node.port, ref));
	}

	if(initialNodes) {
		// populate current connections from each node in the set
//		PRINT((
//			"* NodeManager::_handleNodesCreated(): POPULATING CONNECTIONS (%ld)\n",
//			initialNodes->size()));

		for(port_ref_map::const_iterator itDest = initialNodes->begin();
			itDest != initialNodes->end(); ++itDest) {

			// walk each connected input for this node; find corresponding
			// output & fill in a new Connection instance.

			NodeRef* destRef = (*itDest).second;
			ASSERT(destRef);
			if(!(destRef->kind() & B_BUFFER_CONSUMER))
				// no inputs
				continue;

			vector<media_input> inputs;
			err = destRef->getConnectedInputs(inputs);

			// +++++ FAILED ON STARTUP [e.moon 28sep99]; haven't reproduced yet
			//       [21oct99] failed again
			//ASSERT(err == B_OK);
			if(err < B_OK) {
				PRINT((
					"!!! NodeManager::_handleNodesCreated():\n"
					"    NodeRef('%s')::getConnectedInputs() failed:\n"
					"    %s\n",
					destRef->name(), strerror(err)));

				continue;
			}


//			PRINT((" - %s: %ld inputs\n", destRef->name(), inputs.size()));

			for(vector<media_input>::const_iterator itInput = inputs.begin();
				itInput != inputs.end(); ++itInput) {

				// look for a matching source node by port ID:
				const media_input& input = *itInput;
				port_ref_map::const_iterator itSource = initialNodes->find(
					input.source.port);

				if(itSource == initialNodes->end()) {
					// source not found!
					PRINT((
						"* NodeManager::_handleNodesCreated():\n"
						"  Building initial Connection set: couldn't find source node\n"
						"  connected to input '%s' of '%s' (source port %d).\n",
						input.name, destRef->name(), input.source.port));
					continue;
				}

				// found it; fetch matching output
				NodeRef* sourceRef = (*itSource).second;
				ASSERT(sourceRef);
				media_output output;
				err = sourceRef->findOutput(input.source, &output);
				if(err < B_OK) {
					PRINT((
						"* NodeManager::_handleNodesCreated():\n"
						"  Building initial Connection set: couldn't find output\n"
						"  in node '%s' connected to input '%s' of '%s'.\n",
						sourceRef->name(),
						input.name, destRef->name()));
					continue;
				}

				// sanity check
//				ASSERT(input.source == output.source);
//				ASSERT(input.destination == output.destination);
				// diagnostics [e.moon 11jan00]
				if(input.source != output.source ||
					input.destination != output.destination) {
					PRINT((
						"!!! NodeManager::_handleNodesCreated():\n"
						"    input/output mismatch for connection\n"
						"    '%s' (%s) -> '%s' (%s)\n"
						"    input.source:        port %ld, ID %ld\n"
						"    output.source:       port %ld, ID %ld\n"
						"    input.destination:   port %ld, ID %ld\n"
						"    output.destination:  port %ld, ID %ld\n\n",
						sourceRef->name(), output.name,
						destRef->name(), input.name,
						input.source.port, input.source.id,
						output.source.port, output.source.id,
						input.destination.port, input.destination.id,
						output.destination.port, output.destination.id));
					continue;
				}

				// instantiate & add connection

				Connection* con = new Connection(
					m_nextConID++,
					output.node,
					output.source,
					output.name,
					input.node,
					input.destination,
					input.name,
					input.format,
					0);

				_addConnection(con);

//				// [e.moon 7nov99] call hook
//				connectionMade(con);

//				PRINT((
//					"* NodeManager::_handleNodesCreated(): Found initial connection:\n"
//					"  %s:%s -> %s:%s\n",
//					sourceRef->name(), con->outputName(),
//					destRef->name(), con->inputName()));

			} // for(vector<media_input> ...

		} // for(port_ref_map ...

		// mark the ordeal as over & done with
		m_existingNodesInit = true;
		// clean up
		delete initialNodes;
	}

	// don't relay message if no new create notifications were received [e.moon 11oct99]
	return refsCreated ? B_OK : B_ERROR;
}

inline void NodeManager::_handleNodesDeleted(
	BMessage*											message) {
	ASSERT(IsLocked());

	D_METHOD((
		"NodeManager::_handleNodesDeleted()\n"));

	// walk the list of deleted nodes, removing & cleaning up refs
	// (and any straggler connections)

	type_code type;
	int32 count;
	status_t err = message->GetInfo("media_node_id", &type, &count);
	if(err < B_OK) {
		PRINT((
			"* NodeManager::_handleNodesDeleted(): GetInfo() failed:\n"
			"  %s\n",
			strerror(err)));
		return;
	}
	if(!count)
		return;

	for(int32 n = 0; n < count; n++) {

		int32 id;
		err = message->FindInt32("media_node_id", n, &id);
		if(err < B_OK) {
			PRINT((
				"* NodeManager::_handleNodesDeleted(): FindInt32() failed\n"
				"  %s\n",
				strerror(err)));
			continue;
		}

		// fetch ref
		NodeRef* ref;
		err = getNodeRef(id, &ref);
		if(err < B_OK) {
			PRINT((
				"* NodeManager::_handleNodesDeleted(): getNodeRef(%ld) failed\n"
				"  %s\n",
				id, strerror(err)));
			continue;
		}

		// find & remove any 'stuck' connections; notify any observers
		// that the connections have been removed
		vector<Connection> stuckConnections;
		ref->getInputConnections(stuckConnections);
		ref->getOutputConnections(stuckConnections);

		BMessage message(B_MEDIA_CONNECTION_BROKEN);

		for(uint32 n = 0; n < stuckConnections.size(); ++n) {
			Connection& c = stuckConnections[n];

			// generate a complete B_MEDIA_CONNECTION_BROKEN message
			// +++++ the message format may be extended in the future -- ick

			message.AddData("source", B_RAW_TYPE, &c.source(), sizeof(media_source));
			message.AddData("destination", B_RAW_TYPE, &c.destination(), sizeof(media_destination));
			message.AddInt32(_connectionField, c.id());
			message.AddInt32(_sourceNodeField, c.sourceNode());
			message.AddInt32(_destNodeField, c.destinationNode());

			_removeConnection(c);
		}

		// +++++ don't notify if no stuck connections were found
		notify(&message);

		// ungroup if necessary
		if(ref->m_group) {
			ASSERT(!ref->isReleased());
			ref->m_group->removeNode(ref);
		}

		// [e.moon 23oct99] mark the node released!
		ref->m_nodeReleased = true;

//		// [e.moon 7nov99] call hook
//		nodeDeleted(ref);

		// release it
		ref->release();

	} // for(int32 n ...
}

inline void NodeManager::_handleConnectionMade(
	BMessage*											message) {
	ASSERT(IsLocked());
	D_METHOD((
		"NodeManager::_handleConnectionMade()\n"));
	status_t err;

	for(int32 n = 0;;++n) {
		media_input input;
		media_output output;
		const void* data;
		ssize_t dataSize;

		// fetch output
		err = message->FindData("output", B_RAW_TYPE, n, &data, &dataSize);
		if(err < B_OK) {
			if(!n) {
				PRINT((
					"* NodeManager::_handleConnectionMade(): no entries in message.\n"));
			}
			break;
		}
		if(dataSize < ssize_t(sizeof(media_output))) {
			PRINT((
				"* NodeManager::_handleConnectionMade(): not enough data for output.\n"));
			break;
		}
		output = *(media_output*)data;

		// fetch input
		err = message->FindData("input", B_RAW_TYPE, n, &data, &dataSize);
		if(err < B_OK) {
			if(!n) {
				PRINT((
					"* NodeManager::_handleConnectionMade(): no complete entries in message.\n"));
			}
			break;
		}
		if(dataSize < ssize_t(sizeof(media_input))) {
			PRINT((
				"* NodeManager::_handleConnectionMade(): not enough data for input.\n"));
			break;
		}
		input = *(media_input*)data;

		// look for existing Connection instance
		Connection found;
		err = findConnection(
			output.node.node,
			output.source,
			&found);
		if(err == B_OK) {
			PRINT((
				"  - existing connection for %s -> %s found\n",
				found.outputName(), found.inputName()));
			continue;
		}

		// instantiate & add Connection
		Connection* con = new Connection(
			m_nextConID++,
			output.node,
			output.source,
			output.name,
			input.node,
			input.destination,
			input.name,
			input.format,
			0);

		_addConnection(con);
	}
}

// augments message with source and destination node ID's
inline void NodeManager::_handleConnectionBroken(
	BMessage*											message) {

	D_METHOD((
		"NodeManager::_handleConnectionBroken()\n"));
	status_t err;

	// walk the listed connections
	for(int32 n=0;;n++) {
		media_source source;

		const void* data;
		ssize_t dataSize;

		// fetch source
		err = message->FindData("source", B_RAW_TYPE, n, &data, &dataSize);
		if(err < B_OK) {
			if(!n) {
				PRINT((
					"* NodeManager::_handleConnectionBroken(): incomplete entry in message.\n"));
			}
			break;
		}
		if(dataSize < ssize_t(sizeof(media_source))) {
			PRINT((
				"* NodeManager::_handleConnectionBroken(): not enough data for source.\n"));
			continue;
		}
		source = *(media_source*)data;

		// look up the connection +++++ SLOW +++++
		Connection con;
		err = findConnection(source, &con);
		if(err < B_OK) {
			PRINT((
				"* NodeManager::_handleConnectionBroken(): connection not found:\n"
				"  %ld:%ld\n",
				source.port, source.id));

			// add empty entry to message
			message->AddInt32(_connectionField, 0);
			message->AddInt32(_sourceNodeField, 0);
			message->AddInt32(_destNodeField, 0);
			continue;
		}

		// add entry to the message
		message->AddInt32(_connectionField, con.id());
		message->AddInt32(_sourceNodeField, con.sourceNode());
		message->AddInt32(_destNodeField, con.destinationNode());

//		// [e.moon 7nov99] call hook
//		connectionBroken(&con);

		// home free; delete the connection
		_removeConnection(con);

	} // for(int32 n ...
}

inline void
NodeManager::_handleFormatChanged(BMessage *message)
{
	D_METHOD((
		"NodeManager::_handleFormatChanged()\n"));
	status_t err;

	ssize_t dataSize;

	// fetch source
	media_source* source;
	err = message->FindData("be:source", B_RAW_TYPE, (const void**)&source, &dataSize);
	if(err < B_OK) {
		PRINT((
			"* NodeManager::_handleFormatChanged(): incomplete entry in message.\n"));
		return;
	}

	// fetch destination
	media_destination* destination;
	err = message->FindData("be:destination", B_RAW_TYPE, (const void**)&destination, &dataSize);
	if(err < B_OK) {
		PRINT((
			"* NodeManager::_handleFormatChanged(): incomplete entry in message.\n"));
		return;
	}

	// fetch format
	media_format* format;
	err = message->FindData("be:format", B_RAW_TYPE, (const void**)&format, &dataSize);
	if(err < B_OK) {
		PRINT((
			"* NodeManager::_handleFormatChanged(): incomplete entry in message.\n"));
		return;
	}

	// find matching connection
	for(con_map::const_iterator it = m_conSourceMap.begin();
		it != m_conSourceMap.end(); ++it) {
		if((*it).second->source() == *source) {
			if((*it).second->destination() != *destination) {
				// connection defunct
				return;
			}

			// found
			(*it).second->m_format = *format;

			// attach node IDs to message
			message->AddInt32(_connectionField, (*it).second->id());
			message->AddInt32(_sourceNodeField, (*it).second->sourceNode());
			message->AddInt32(_destNodeField, (*it).second->destinationNode());

			break;
		}
	}
}


// return flags appropriate for an external
// node with the given 'kind'

inline uint32 NodeManager::_userFlagsForKind(
	uint32												kind) {

	uint32 f = 0;
	if(
//		kind & B_TIME_SOURCE ||
		kind & B_PHYSICAL_OUTPUT
		// || kind & B_SYSTEM_MIXER [now in initCommonNodes() e.moon 17nov99]
		)
		f |= (NodeRef::NO_START_STOP | NodeRef::NO_SEEK | NodeRef::NO_PREROLL);

//	// [28sep99 e.moon] physical inputs may not be stopped for now; at
//	// least one sound input node (for emu10k) stops working when requested
//	// to stop.
//	// +++++ should this logic be in initCommonNodes()?
//	if(
//		kind & B_PHYSICAL_INPUT)
//		f |= NodeRef::NO_STOP;

	return f;
}

inline uint32 NodeManager::_implFlagsForKind(
	uint32												kind) {

	return 0;
}

// [e.moon 28sep99] latency updating
// These methods must set the recording-mode delay for
// any B_RECORDING nodes they handle.

// +++++ abstract to 'for each' and 'for each from'
//       methods (template or callback?)


// refresh cached latency for every node in the given group
// (or all nodes if no group given.)

inline void NodeManager::_updateLatencies(
	NodeGroup*										group) {

	ASSERT(IsLocked());
	if(group) {
		ASSERT(group->isLocked());
	}

	if(group) {
		for(NodeGroup::node_set::iterator it = group->m_nodes.begin();
			it != group->m_nodes.end(); ++it) {

			(*it)->_updateLatency();
		}
	}
	else {
		for(node_ref_map::iterator it = m_nodeRefMap.begin();
			it != m_nodeRefMap.end(); ++it) {

			(*it).second->_updateLatency();
		}
	}
}

// refresh cached latency for every node attached to
// AND INCLUDING the given origin node.
// if 'recurse' is true, affects indirectly attached
// nodes as well.


inline void NodeManager::_updateLatenciesFrom(
	NodeRef*											origin,
	bool													recurse) {

	ASSERT(IsLocked());

//	PRINT(("### NodeManager::_updateLatenciesFrom()\n"));

	origin->lock();
	origin->_updateLatency();
	origin->unlock();

	_lockAllGroups(); // [e.moon 13oct99]

	_for_each_state st;
	_do_for_each_connected(
		this,
		origin,
		0, // all groups
		recurse,
		mem_fun(&NodeRef::_updateLatency),
		&st);

	_unlockAllGroups(); // [e.moon 13oct99]
}

// a bit of unpleasantness [e.moon 13oct99]
void NodeManager::_lockAllGroups() {

	ASSERT(IsLocked());
	for(node_group_set::iterator it = m_nodeGroupSet.begin();
		it != m_nodeGroupSet.end(); ++it) {
		(*it)->lock();
	}
}

void NodeManager::_unlockAllGroups() {
	ASSERT(IsLocked());
	for(node_group_set::iterator it = m_nodeGroupSet.begin();
		it != m_nodeGroupSet.end(); ++it) {
		(*it)->unlock();
	}
}


// END -- NodeManager.cpp --
