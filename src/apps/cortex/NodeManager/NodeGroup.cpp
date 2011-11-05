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


// NodeGroup.cpp

#include "NodeGroup.h"
//#include "NodeGroup_transport_thread.h"

#include "NodeManager.h"
#include "NodeRef.h"

#include <MediaRoster.h>
#include <OS.h>
#include <TimeSource.h>

#include <algorithm>
#include <functional>

#include "array_delete.h"
#include "BasicThread.h"
#include "node_manager_impl.h"
#include "functional_tools.h"

using namespace std;

__USE_CORTEX_NAMESPACE
#define D_METHOD(x) //PRINT (x)
#define D_ROSTER(x) //PRINT (x)
#define D_LOCK(x) //PRINT (x)



// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

// free the group, including all nodes within it
// (this call will result in the eventual deletion of the object.)
// returns B_OK on success; B_NOT_ALLOWED if release() has
// already been called; other error codes if the Media Roster
// call fails.
// * THE MANAGER MUST BE LOCKED

status_t NodeGroup::release() {

	D_METHOD((
		"NodeGroup::release()\n"));
	
	if(isReleased())
		return B_NOT_ALLOWED;

	// clean up		
	lock();

	// halt all nodes
	_stop();
	
	// remove & release all nodes
	// +++++ causes triply-nested lock: eww!
	while(m_nodes.size()) {
		NodeRef* last = m_nodes.back();
		removeNode(m_nodes.size()-1);
		last->release();
	}

	unlock();

// [e.moon 7nov99]
// removing the released group is now NodeManager's responsibility
//
	// remove from NodeManager
	if(!m_manager->lock()) {
		ASSERT(!"* m_manager->lock() failed.\n");
	}
	m_manager->_removeGroup(this);
	m_manager->unlock();
	
	// hand off to IObservable
	return _inherited::release();	
}

// call release() rather than deleting NodeGroup objects
NodeGroup::~NodeGroup() {

	Autolock _l(this);
	D_METHOD((
		"~NodeGroup()\n"));

	ASSERT(!m_nodes.size());	

	if(m_timeSourceObj) {
		m_timeSourceObj->Release();
		m_timeSourceObj = 0;
	}
}


// -------------------------------------------------------- //
// *** accessors
// -------------------------------------------------------- //

// [e.moon 13oct99] moved to header
//inline uint32 NodeGroup::id() const { return m_id; }

// -------------------------------------------------------- //
// *** operations
// -------------------------------------------------------- //

// name access
const char* NodeGroup::name() const {
	Autolock _l(this);
	return m_name.String();
}

status_t NodeGroup::setName(const char* name) {
	Autolock _l(this);
	m_name = name;
	return B_OK;
}

// content access
uint32 NodeGroup::countNodes() const {
	Autolock _l(this);
	return m_nodes.size();
}

NodeRef* NodeGroup::nodeAt(
	uint32											index) const {
	Autolock _l(this);
	return (index < m_nodes.size()) ?
		m_nodes[index] :
		0;
}

// add/remove nodes:
// - you may only add a node with no current group.
// - nodes added during playback will be started;
//   nodes removed during playback will be stopped (unless
//   the NO_START_STOP transport restriction flag is set
//   for a given node.)

status_t NodeGroup::addNode(
	NodeRef*										node) {
	
	D_METHOD((
		"NodeGroup::addNode()\n"));
		
	// lock the manager first; if the node has no current group,
	// this locks the node.
	m_manager->lock();
	
	Autolock _l(this);

	// precondition: GROUP_LOCKED not set
	if(m_flags & GROUP_LOCKED)
		return B_NOT_ALLOWED;

	// precondition: no current group
	if(node->m_group) {
		// [e.moon 28sep99] whoops, forgot one
		PRINT((
			"!!! node already in group '%s'\n", node->m_group->name()));

		m_manager->unlock(); 
		return B_NOT_ALLOWED;
	}

	// add it
	m_nodes.push_back(node);	
	node->_setGroup(this);

	// release the manager
	m_manager->unlock();
	
	// first node? the transport is now ready to start
	if(m_nodes.size() == 1) {
		_changeState(TRANSPORT_INVALID, TRANSPORT_STOPPED);
	}
//
//	if(m_syncNode == media_node::null) {
//		// assign as sync node
//		setSyncNode(node->node());
//	}
//	
	// initialize the new node
	status_t err = node->_initTransportState();
	if(err < B_OK)
		return err;

	// set time source
	node->_setTimeSource(m_timeSource.node);

	// set run mode
	node->_setRunMode(m_runMode);	

	// add to cycle set if need be
	// +++++ should I call _cycleAddRef() instead?
	if(node->m_cycle)
		_refCycleChanged(node);
		
	if(m_transportState == TRANSPORT_RUNNING) {
		// +++++ start if necessary!
	}
	// +++++ not started if TRANSPORT_ROLLING: is that proper? [e.moon 11oct99]
	
	// send notification
	if(!LockLooper()) {
		ASSERT(!"LockLooper() failed.");
	}
	BMessage m(M_NODE_ADDED);
	m.AddInt32("groupID", id());
	m.AddInt32("nodeID", node->id());
	notify(&m);
	UnlockLooper();
	
	// success
	return B_OK;
}
		

status_t NodeGroup::removeNode(
	NodeRef*										node) {

	D_METHOD((
		"NodeGroup::removeNode()\n"));
		
	// lock the manager first; once the node is ungrouped,
	// the manager lock applies to it
	m_manager->lock();
	
	Autolock _l(this);

	// precondition: this must be the node's group
	if(node->m_group != this) {
		// [e.moon 28sep99] whoops, forgot one
		PRINT((
			"!!! node not in group '%s'\n", node->m_group->name()));

		m_manager->unlock(); 
		return B_NOT_ALLOWED;
	}

	// remove from the cycle set
	if(node->m_cycle)
		_cycleRemoveRef(node);

	// remove it
	ASSERT(m_nodes.size());
	remove(
		m_nodes.begin(),
		m_nodes.end(),
		node);

	// should have removed one and only one entry
	m_nodes.resize(m_nodes.size()-1);

//	// 6aug99: the timesource is now the sync node...
//	// is this the sync node? reassign if so
//
//	if(node->node() == m_syncNode) {
//
//		// look for another sync-capable node
//		bool found = false;
//		for(int n = 0; !found && n < m_nodes.size(); ++n)
//			if(setSyncNode(m_nodes[n]->node()) == B_OK)
//				found = true;
//
//		// no luck? admit defeat:		
//		if(!found) {
//			PRINT((
//				"* NodeGroup::removeNode(): no sync-capable nodes left!\n"));
//				
//			// +++++ stop & set to invalid state?
//			
//			setSyncNode(media_node::null);
//		}
//	}

	// stop the node if necessary	
	status_t err = node->_stop();
	if(err < B_OK) {
		PRINT((
			"*** NodeGroup::removeNode('%s'): error from node->_stop():\n"
			"    %s\n",
			node->name(),
			strerror(err)));
	}

	// clear the node's group pointer
	node->_setGroup(0);

	// release the manager lock; the node is now ungrouped and
	// unlocked
	m_manager->unlock();
	
	// was that the last node? stop/disable the transport if so
	if(!m_nodes.size()) {

		// +++++ kill sync thread(s)
		
		_changeState(TRANSPORT_INVALID);
	}
		
	// send notification
	if(!LockLooper()) {
		ASSERT(!"LockLooper() failed.");
	}
	BMessage m(M_NODE_REMOVED);
	m.AddInt32("groupID", id());
	m.AddInt32("nodeID", node->id());
	notify(&m);
	UnlockLooper();
		
	// success
	return B_OK;
}
	
status_t NodeGroup::removeNode(
	uint32											index) {

	D_METHOD((
		"NodeGroup::removeNode(by index)\n"));
		
	// +++++ icky nested lock
	Autolock _l(this);

	ASSERT(m_nodes.size() > index);
	return removeNode(m_nodes[index]);	
}

uint32 NodeGroup::groupFlags() const {
	Autolock _l(this);
	return m_flags;
}

status_t NodeGroup::setGroupFlags(
	uint32											flags) {
	Autolock _l(this);
	m_flags = flags;
	return B_OK;
}


// returns true if one or more nodes in the group have cycling
// enabled, and the start- and end-positions are valid
bool NodeGroup::canCycle() const {
	Autolock _l(this);
	
	return
		m_cycleNodes.size() > 0 &&
		m_endPosition - m_startPosition > s_minCyclePeriod;
}

// -------------------------------------------------------- //
// *** TRANSPORT POSITIONING (LOCK REQUIRED)
// -------------------------------------------------------- //

// Fetch the current transport state
	
NodeGroup::transport_state_t NodeGroup::transportState() const {
	Autolock _l(this);
	return m_transportState;
}
	
// Set the starting media time:
//   This is the point at which playback will begin in any media
//   files/documents being played by the nodes in this group.
//   When cycle mode is enabled, this is the point to which each
//   node will be seek'd at the end of each cycle (loop).
//
//   The starting time can't be changed in the B_OFFLINE run mode
//   (this call will return an error.)

status_t NodeGroup::setStartPosition(
	bigtime_t										start) {
	Autolock _l(this);
	
	D_METHOD((
		"NodeGroup::setStartPosition(%Ld)\n", start));
		
	if(
		m_transportState == TRANSPORT_RUNNING ||
		m_transportState == TRANSPORT_ROLLING ||
		m_transportState == TRANSPORT_STARTING) {
	
		if(m_runMode == BMediaNode::B_OFFLINE)
			return B_NOT_ALLOWED;
		
		ASSERT(m_timeSourceObj);
	
		if(_cycleValid()) {
			if(m_timeSourceObj->Now() >= m_cycleDeadline) {
				// too late to change start position; defer
//				PRINT((" - deferred\n"));
				m_newStartPosition = start;
				m_newStart = true;
				return B_OK;
			}
			
			// not at deadline yet; fall through to set start position
		}
	}
	
	m_startPosition = start;
	
	// +++++ notify [e.moon 11oct99]

	return B_OK;
}
	
// Fetch the starting position:

// +++++ if a previously-set start position was deferred, it won't be
//       returned yet

bigtime_t NodeGroup::startPosition() const {
	Autolock _l(this);
	
	return m_startPosition;
}
		
// Set the ending media time:
//   This is the point at which playback will end relative to
//   media documents begin played by the nodes in this group; 
//   in cycle mode, this specifies the loop point.  If the
//   ending time is less than or equal to the starting time,
//   the transport will continue until stopped manually.
//   If the end position is changed while the transport is playing,
//   it must take effect retroactively (if it's before the current
//   position and looping is enabled, all nodes must 'warp' to
//   the proper post-loop position.)
//
//   The ending time can't be changed if run mode is B_OFFLINE and
//   the transport is running (this call will return an error.)
	
status_t NodeGroup::setEndPosition(
	bigtime_t										end) {
	Autolock _l(this);
	
	D_METHOD((
		"NodeGroup::setEndPosition(%Ld)\n", end));

	if(
		m_transportState == TRANSPORT_RUNNING ||
		m_transportState == TRANSPORT_ROLLING ||
		m_transportState == TRANSPORT_STARTING) {
	
		if(m_runMode == BMediaNode::B_OFFLINE)
			return B_NOT_ALLOWED;
		
		ASSERT(m_timeSourceObj);
	
		bigtime_t endDelta = end - m_endPosition;
	
		if(_cycleValid()) {
			if(m_timeSourceObj->Now() >= m_cycleDeadline + endDelta) {
				// too late to change end position; defer
//				PRINT((" - deferred\n"));
				m_newEndPosition = end;
				m_newEnd = true;
				return B_OK;
			}
			else {
				// set new end position
				m_endPosition = end;
				
				// inform thread
				ASSERT(m_cyclePort);
				write_port(
					m_cyclePort,
					_CYCLE_END_CHANGED,
					0,
					0);
//			
//				// restart nodes' cycle threads with new end position
//				_cycleInit(m_cycleStart);
//				for(node_set::iterator it = m_cycleNodes.begin();
//					it != m_cycleNodes.end(); ++it) {
//					(*it)->_scheduleCycle(m_cycleBoundary);
//				}
//				return B_OK;
			}
		}
	}
		
	m_endPosition = end;

	// +++++ notify [e.moon 11oct99]

	return B_OK;
}


// Fetch the end position:
//   Note that if the end position is less than or equal to the start
//   position, it's ignored.

// +++++ if a previously-set end position was deferred, it won't be
//       returned yet
		
bigtime_t NodeGroup::endPosition() const {
	Autolock _l(this);
	return m_endPosition;
}
		
// -------------------------------------------------------- //
// *** TRANSPORT OPERATIONS (LOCK REQUIRED)
// -------------------------------------------------------- //

// Preroll the group:
//   Seeks, then prerolls, each node in the group (honoring the
//   NO_SEEK and NO_PREROLL flags.)  This ensures that the group
//   can start as quickly as possible.
//
//   Does not return until all nodes in the group have been
//   prepared.

status_t NodeGroup::preroll() {
	D_METHOD((
		"NodeGroup::preroll()\n"));
		
	Autolock _l(this);
	return _preroll();
}

// Start all nodes in the group:
//   Nodes with the NO_START_STOP flag aren't molested.
	
status_t NodeGroup::start() {
	D_METHOD((
		"NodeGroup::start()\n"));
		
	Autolock _l(this);
	return _start();
}

// Stop all nodes in the group:
//   Nodes with the NO_START_STOP flag aren't molested.
	
status_t NodeGroup::stop() {
	D_METHOD((
		"NodeGroup::stop()\n"));
		
	Autolock _l(this);
	return _stop();
}

// Roll all nodes in the group:
//   Queues a start and stop atomically (via BMediaRoster::RollNode()).
//   Returns B_NOT_ALLOWED if endPosition <= startPosition;

status_t NodeGroup::roll() {
	D_METHOD((
		"NodeGroup::roll()\n"));
		
	Autolock _l(this);
	return _roll();
}

// -------------------------------------------------------- //
// *** TIME SOURCE & RUN-MODE OPERATIONS (LOCK REQUIRED)
// -------------------------------------------------------- //

// time source control:
//   getTimeSource():
//   returns B_ERROR if no time source has been set; otherwise,
//   returns the node ID of the current time source for all
//   nodes in the group.
//
//   setTimeSource():
//   Calls SetTimeSourceFor() on every node in the group.
//   The group must be stopped; B_NOT_ALLOWED will be returned
//   if the state is TRANSPORT_RUNNING or TRANSPORT_ROLLING.
	
status_t NodeGroup::getTimeSource(
	media_node*									outTimeSource) const {
	Autolock _l(this);

	if(m_timeSource != media_node::null) {
		*outTimeSource = m_timeSource;	
		return B_OK;
	}
	return B_ERROR;
}
			
status_t NodeGroup::setTimeSource(
	const media_node&						timeSource) {

	Autolock _l(this);
	
	if(m_transportState == TRANSPORT_RUNNING || m_transportState == TRANSPORT_ROLLING)
		return B_NOT_ALLOWED;
	
	if(m_timeSourceObj)
		m_timeSourceObj->Release();
	
	m_timeSource = timeSource;
	
	// cache a BTimeSource*
	m_timeSourceObj = m_manager->roster->MakeTimeSourceFor(timeSource);
	ASSERT(m_timeSourceObj);
	
	// apply new time source to all nodes
	for_each(
		m_nodes.begin(),
		m_nodes.end(),
		bind2nd(
			mem_fun(&NodeRef::_setTimeSource),
			m_timeSource.node
		)
	);
	
//	// try to set as sync node
//	err = setSyncNode(timeSource);
//	if(err < B_OK) {
//		PRINT((
//			"* NodeGroup::setTimeSource(): setSyncNode() failed: %s\n",
//			strerror(err)));
//	}
	
	// notify
	if(!LockLooper()) {
		ASSERT(!"LockLooper() failed.");
	}
	BMessage m(M_TIME_SOURCE_CHANGED);
	m.AddInt32("groupID", id());
	m.AddInt32("timeSourceID", timeSource.node);
	notify(&m);
	UnlockLooper();
	
	return B_OK;
}
		
// run mode access:
//   Sets the default run mode for the group.  This will be
//   applied to every node with a wildcard (0) run mode.
//
//   Special case: if the run mode is B_OFFLINE, it will be
//   applied to all nodes in the group.
	
BMediaNode::run_mode NodeGroup::runMode() const {
	Autolock _l(this);
	return m_runMode;
}

status_t NodeGroup::setRunMode(BMediaNode::run_mode mode) {
	Autolock _l(this);

	m_runMode = mode;
	
	// apply to all nodes
	for_each(
		m_nodes.begin(),
		m_nodes.end(),
		bind2nd(
			mem_fun(&NodeRef::_setRunModeAuto),
			m_runMode
		)
//		bound_method(
//			*this,
//			&NodeGroup::setRunModeFor)
	);
	

	return B_OK;
}

// -------------------------------------------------------- //
// *** BHandler
// -------------------------------------------------------- //

void NodeGroup::MessageReceived(
	BMessage*										message) {

//	PRINT((
//		"NodeGroup::MessageReceived():\n"));
//	message->PrintToStream();
	status_t err;
	
	switch(message->what) {
		case M_SET_TIME_SOURCE:
			{
				media_node timeSource;
				void* data;
				ssize_t dataSize;
				err = message->FindData(
					"timeSourceNode",
					B_RAW_TYPE,
					(const void**)&data,
					&dataSize);
				if(err < B_OK) {
					PRINT((
						"* NodeGroup::MessageReceived(M_SET_TIME_SOURCE):\n"
						"  no timeSourceNode!\n"));
					break;
				}
				timeSource = *(media_node*)data;

				setTimeSource(timeSource);
			}
			break;
			
		case M_SET_RUN_MODE:
			{
				uint32 runMode;
				err = message->FindInt32("runMode", (int32*)&runMode);
				if(err < B_OK) {
					PRINT((
						"* NodeGroup::MessageReceived(M_SET_RUN_MODE):\n"
						"  no runMode!\n"));
					break;
				}
				
				if(runMode < BMediaNode::B_OFFLINE ||
					runMode > BMediaNode::B_RECORDING) {
					PRINT((
						"* NodeGroup::MessageReceived(M_SET_RUN_MODE):\n"
						"  invalid run mode (%ld)\n", runMode));
					break;
				}
				
				setRunMode((BMediaNode::run_mode)runMode);
			}
			break;
			
		case M_SET_START_POSITION:
			{
				bigtime_t position;
				err = message->FindInt64("position", (int64*)&position);
				if(err < B_OK) {
					PRINT((	
						"* NodeGroup::MessageReceived(M_SET_START_POSITION):\n"
						"  no position!\n"));
					break;
				}
				setStartPosition(position);
			}
			break;
	
		case M_SET_END_POSITION:
			{
				bigtime_t position;
				err = message->FindInt64("position", (int64*)&position);
				if(err < B_OK) {
					PRINT((	
						"* NodeGroup::MessageReceived(M_SET_END_POSITION):\n"
						"  no position!\n"));
					break;
				}
				setEndPosition(position);
			}
			break;
	
		case M_PREROLL:
			preroll();
			break;

		case M_START:
			start();
			break;
	
		case M_STOP:
			stop();
			break;
			
		case M_ROLL:
			roll();
			break;
	
		default:
			_inherited::MessageReceived(message);
			break;
	}
}
		

// -------------------------------------------------------- //
// *** IPersistent
// -------------------------------------------------------- //

// !
#if CORTEX_XML
// !

// +++++

// Default constructor
NodeGroup::NodeGroup() :
	m_manager(0) {} // +++++ finish initialization
	

// !
#endif /*CORTEX_XML*/
// !

// -------------------------------------------------------- //
// *** IObservable:		[19aug99]
// -------------------------------------------------------- //

void NodeGroup::observerAdded(
	const BMessenger&				observer) {

	BMessage m(M_OBSERVER_ADDED);
	m.AddInt32("groupID", id());
	m.AddMessenger("target", BMessenger(this));
	observer.SendMessage(&m);
}
		
void NodeGroup::observerRemoved(
	const BMessenger&				observer) {

	BMessage m(M_OBSERVER_REMOVED);
	m.AddInt32("groupID", id());
	m.AddMessenger("target", BMessenger(this));
	observer.SendMessage(&m);
}

void NodeGroup::notifyRelease() {

	BMessage m(M_RELEASED);
	m.AddInt32("groupID", id());
	m.AddMessenger("target", BMessenger(this));
	notify(&m);
}

void NodeGroup::releaseComplete() {
	// +++++
}

// -------------------------------------------------------- //
// *** ILockable: pass lock requests to m_lock
// -------------------------------------------------------- //

bool NodeGroup::lock(
	lock_t type,
	bigtime_t timeout) {
	
	D_LOCK(("*** NodeGroup::lock(): %ld\n", find_thread(0)));
	
	ASSERT(type == WRITE);
	status_t err = m_lock.LockWithTimeout(timeout);
	
	D_LOCK(("*** NodeGroup::lock() ACQUIRED: %ld\n", find_thread(0)));

	return err == B_OK;
}
	
bool NodeGroup::unlock(
	lock_t type) {

	D_LOCK(("*** NodeGroup::unlock(): %ld\n", find_thread(0)));
	
	ASSERT(type == WRITE);
	m_lock.Unlock();

	D_LOCK(("*** NodeGroup::unlock() RELEASED: %ld\n", find_thread(0)));

	return true;
}
	
bool NodeGroup::isLocked(
	lock_t type) const {

	ASSERT(type == WRITE);
	return m_lock.IsLocked();
}
		
// -------------------------------------------------------- //
// *** ctor (accessible to NodeManager)
// -------------------------------------------------------- //

NodeGroup::NodeGroup(
	const char*									name,
	NodeManager*								manager,
	BMediaNode::run_mode				runMode) :
	
	ObservableHandler(name),
	m_lock("NodeGroup::m_lock"),
	m_manager(manager),
	m_id(NextID()),
	m_name(name),
	m_flags(0),
	m_transportState(TRANSPORT_INVALID),
	m_runMode(runMode),
	m_timeSourceObj(0),
	m_released(false),
	m_cycleThread(0),
	m_cyclePort(0),
	m_startPosition(0LL),
	m_endPosition(0LL),
	m_newStart(false),
	m_newEnd(false) {

	ASSERT(m_manager);
	
	if(!m_manager->Lock()) {
		ASSERT(!"m_manager->Lock() failed");
	}
	m_manager->AddHandler(this);
	m_manager->Unlock();
	
	// set default time source
	media_node ts;
	D_ROSTER(("# roster->GetTimeSource()\n"));
	status_t err = m_manager->roster->GetTimeSource(&ts);
	if(err < B_OK) {
		PRINT((
			"*** NodeGroup(): roster->GetTimeSource() failed:\n"
			"    %s\n", strerror(err)));
	}
	setTimeSource(ts);
}

// -------------------------------------------------------- //
// *** internal operations
// -------------------------------------------------------- //

uint32 NodeGroup::s_nextID = 1;
uint32 NodeGroup::NextID() {
	return atomic_add((int32*)&s_nextID, 1);
}

// -------------------------------------------------------- //
// *** ref->group communication (LOCK REQUIRED)
// -------------------------------------------------------- //

// when a NodeRef's cycle state (ie. looping or not looping)
// changes, it must pass that information on via this method

void NodeGroup::_refCycleChanged(
	NodeRef*										ref) {
	assert_locked(this);
	D_METHOD((
		"NodeGroup::_refCycleChanged('%s')\n",
		ref->name()));

	if(ref->m_cycle) {
		_cycleAddRef(ref);
	}	else {
		_cycleRemoveRef(ref);
	}

	// +++++ if running & cycle valid, the node should be properly
	//       seek'd and start'd
}
	

// when a cycling node's latency changes, call this method.

void NodeGroup::_refLatencyChanged(
	NodeRef*										ref) {
	assert_locked(this);
	D_METHOD((
		"NodeGroup::_refLatencyChanged('%s')\n",
		ref->name()));
	
	if(!_cycleValid())
		return;

	// remove & replace ref (positions it properly)	
	_cycleRemoveRef(ref);
	_cycleAddRef(ref);
	
	// slap my thread up
	ASSERT(m_cyclePort);
	write_port(
		m_cyclePort,
		_CYCLE_LATENCY_CHANGED,
		0,
		0);

	// +++++ zat it?
}

// when a NodeRef receives notification that it has been stopped,
// but is labeled as still running, it must call this method.
// [e.moon 11oct99: roll/B_OFFLINE support]

void NodeGroup::_refStopped(
	NodeRef*										ref) {
	assert_locked(this);
	D_METHOD((
		"NodeGroup::_refStopped('%s')\n",
		ref->name()));
	
	// roll/B_OFFLINE support [e.moon 11oct99]
	// (check to see if any other nodes in the group are still running;
	//  mark group stopped if not.)
	if(m_transportState == TRANSPORT_ROLLING) {
		bool nodesRunning = false;
		for(node_set::iterator it = m_nodes.begin();
			it != m_nodes.end(); ++it) {
			if((*it)->isRunning()) {
				nodesRunning = true;
				break;
			}
		}
		if(!nodesRunning)
			// the group has stopped; update transport state
			_changeState(TRANSPORT_STOPPED);	

	}
	
}


// -------------------------------------------------------- //
// *** transport helpers (LOCK REQUIRED)
// -------------------------------------------------------- //


// Preroll all nodes in the group; this is the implementation
// of preroll().
// *** this method should not be called from the transport thread
// (since preroll operations can block for a relatively long time.)
	
status_t NodeGroup::_preroll() {
	assert_locked(this);
	
	D_METHOD((
		"NodeGroup::_preroll()\n"));
		
	if(
		m_transportState == TRANSPORT_RUNNING ||
		m_transportState == TRANSPORT_ROLLING)
		// too late
		return B_NOT_ALLOWED;
	
	// * preroll all nodes to the start position

	// +++++ currently, if an error is encountered it's ignored.
	//       should the whole operation fail if one node couldn't
	//       be prerolled?
	//
	//       My gut response is 'no', since the preroll step is
	//       optional, but the caller should have some inkling that
	//       one of its nodes didn't behave.

// [e.moon 13oct99] making PPC compiler happy
//	for_each(
//		m_nodes.begin(),
//		m_nodes.end(),
//		bind2nd(
//			mem_fun(&NodeRef::_preroll),
//			m_startPosition
//		)
//	);
	for(node_set::iterator it = m_nodes.begin();
		it != m_nodes.end(); ++it) {
		(*it)->_preroll(m_startPosition);
	}

//    replaces
//		bind2nd(
//			bound_method(*this, &NodeGroup::prerollNode),
//			m_startPosition
//		)

	return B_OK;
}


//// functor: calculates latency of each node it's handed, caching
//// the largest one found; includes initial latency if nodes report it.
//
//class NodeGroup::calcLatencyFn { public:
//	bigtime_t& maxLatency;
//	
//	calcLatencyFn(bigtime_t& _m) : maxLatency(_m) {}
//
//	void operator()(NodeRef* r) {
//		ASSERT(r);
//		
////		PRINT((
////			"# calcLatencyFn(): '%s'\n",
////			r->name()));
//			
//		if(!(r->node().kind & B_BUFFER_PRODUCER)) {
//			// node can't incur latency
////			PRINT((
////				"-   not a producer\n"));
//			return;
//		}
//		
//		bigtime_t latency;
//		status_t err =
//			BMediaRoster::Roster()->GetLatencyFor(
//				r->node(),
//				&latency);
//		if(err < B_OK) {
//			PRINT((
//				"* calcLatencyFn: GetLatencyFor() failed: %s\n",
//				strerror(err)));
//			return;
//		}
////		PRINT(("-   %Ld\n", latency));
//		
//		bigtime_t add;
//		err = BMediaRoster::Roster()->GetInitialLatencyFor(
//			r->node(),
//			&add);
////		PRINT(("-   %Ld\n", add));
//		if(err < B_OK) {
//			PRINT((
//				"* calcLatencyFn: GetInitialLatencyFor() failed: %s\n",
//				strerror(err)));
//		}
//		else
//			latency += add;
//		
//		if(latency > maxLatency)
//			maxLatency = latency;
//
////		PRINT((
////			"-   max latency: %Ld\n",
////			maxLatency));
//	}
//};

// Start all nodes in the group; this is the implementation of
// start().  Fails if the run mode is B_OFFLINE; use _roll() instead
// in that case.
//
// (this may be called from the transport thread or from
//  an API-implementation method.)
		
status_t NodeGroup::_start() {
	assert_locked(this);
	
	D_METHOD((
		"NodeGroup::_start()\n"));
	status_t err;
		
	if(m_transportState != TRANSPORT_STOPPED)
		return B_NOT_ALLOWED;
		
	if(m_runMode == BMediaNode::B_OFFLINE)
		return B_NOT_ALLOWED;

	ASSERT(m_nodes.size());

	_changeState(TRANSPORT_STARTING);

	// * Find the highest latency in the group
	
	bigtime_t offset = 0LL;
	calcLatencyFn _f(offset);
	for_each(
		m_nodes.begin(),
		m_nodes.end(),
		_f);

	offset += s_rosterLatency;
	PRINT((
		"- offset: %Ld\n", offset));	
	
	// * Seek all nodes (in case one or more failed to preroll)

	for(node_set::iterator it = m_nodes.begin();
		it != m_nodes.end(); ++it) {
		err = (*it)->_seekStopped(m_startPosition);
		if(err < B_OK) {
			PRINT((
				"! NodeGroup('%s')::_start():\n"
				"  ref('%s')->_seekStopped(%Ld) failed:\n"
				"  %s\n",
				name(), (*it)->name(), m_startPosition,
				strerror(err)));

			// +++++ continue?
		}
	}
	
	// * Start all nodes, allowing for the max latency found

	ASSERT(m_timeSourceObj);
	bigtime_t when = m_timeSourceObj->Now() + offset;

	// 10aug99: initialize cycle (loop) settings
	if(_cycleValid()) {
		_initCycleThread();
		_cycleInit(when);
	}
	
	// start the nodes
	for(node_set::iterator it = m_nodes.begin();
		it != m_nodes.end(); ++it) {
		err = (*it)->_start(when);
		if(err < B_OK) {
			PRINT((
				"! NodeGroup('%s')::_start():\n"
				"  ref('%s')->_start(%Ld) failed:\n"
				"  %s\n",
				name(), (*it)->name(), when,
				strerror(err)));

			// +++++ continue?
		}
	}
	
	// notify observers
	_changeState(TRANSPORT_RUNNING);
	return B_OK;	
}
	
// Stop all nodes in the group; this is the implementation of
// stop().
//
// (this may be called from the transport thread or from
//  an API-implementation method.)
		
status_t NodeGroup::_stop() {

	D_METHOD((
		"NodeGroup::_stop()\n"));
		
	assert_locked(this);

	if(
		m_transportState != TRANSPORT_RUNNING &&
		m_transportState != TRANSPORT_ROLLING)
		return B_NOT_ALLOWED;

	_changeState(TRANSPORT_STOPPING);

	// * stop the cycle thread if need be
	_destroyCycleThread();

	// * stop all nodes
	//   +++++ error reports would be nice
	
	for_each(
		m_nodes.begin(),
		m_nodes.end(),
		mem_fun(&NodeRef::_stop)
	);

	// update transport state
	_changeState(TRANSPORT_STOPPED);
		
	return B_OK;
}

// Roll all nodes in the group; this is the implementation of
// roll().
//
// (this may be called from the transport thread or from
//  an API-implementation method.)

status_t NodeGroup::_roll() {

	D_METHOD((
		"NodeGroup::_roll()\n"));		
	assert_locked(this);
	status_t err;

	if(m_transportState != TRANSPORT_STOPPED)
		return B_NOT_ALLOWED;
		
	bigtime_t period = m_endPosition - m_startPosition;
	if(period <= 0LL)
		return B_NOT_ALLOWED;
	
	_changeState(TRANSPORT_STARTING);

	bigtime_t tpStart = 0LL;
	bigtime_t tpStop = period;
	
	if(m_runMode != BMediaNode::B_OFFLINE) {
	
		// * Find the highest latency in the group	
		bigtime_t offset = 0LL;
		calcLatencyFn _f(offset);
		for_each(
			m_nodes.begin(),
			m_nodes.end(),
			_f);

		offset += s_rosterLatency;
		PRINT((
			"- offset: %Ld\n", offset));	

		ASSERT(m_timeSourceObj);
		tpStart = m_timeSourceObj->Now() + offset;
		tpStop += tpStart;
	}
	
	// * Roll all nodes; watch for errors
	bool allFailed = true;
	err = B_OK;
	for(
		node_set::iterator it = m_nodes.begin();
		it != m_nodes.end(); ++it) {

		status_t e = (*it)->_roll(
			tpStart,
			tpStop,
			m_startPosition);
		if(e < B_OK)
			err = e;
		else
			allFailed = false;
	}
	
	if(!allFailed)
		// notify observers
		_changeState(TRANSPORT_ROLLING);

	return err;	
}
	

// State transition; notify listeners
// +++++ [18aug99] DANGER: should notification happen in the middle
//                         of such an operation?
inline void NodeGroup::_changeState(
	transport_state_t			to) {

	assert_locked(this);
	
	m_transportState = to;

	if(!LockLooper()) {
		ASSERT(!"LockLooper() failed.");
	}
	BMessage m(M_TRANSPORT_STATE_CHANGED);
	m.AddInt32("groupID", id());
	m.AddInt32("transportState", m_transportState);
	notify(&m);
	UnlockLooper();
}

// Enforce a state transition, and notify listeners
inline void NodeGroup::_changeState(
	transport_state_t			from,
	transport_state_t			to) {

	assert_locked(this);
	ASSERT(m_transportState == from);
	
	_changeState(to);
}


// -------------------------------------------------------- //
// *** cycle thread & helpers (LOCK REQUIRED)
// -------------------------------------------------------- //

// *** cycle port definitions

const int32					_portLength			= 32;
const char* const		_portName				= "NodeGroup::m_cyclePort";
const size_t				_portMsgMaxSize	= 256;


// set up the cycle thread (including its kernel port)
status_t NodeGroup::_initCycleThread() {
	assert_locked(this);
	status_t err;
	D_METHOD((
		"NodeGroup::_initCycleThread()\n"));

	if(m_cycleThread) {
		// thread is still alive
		err = _destroyCycleThread();
		if(err < B_OK)
			return err;
	}

	// create
	m_cycleThreadDone = false;
	m_cycleThread = spawn_thread(
		&_CycleThread,
		"NodeGroup[cycleThread]",
		B_NORMAL_PRIORITY,
		(void*)this);
	if(m_cycleThread < B_OK) {
		PRINT((
			"* NodeGroup::_initCycleThread(): spawn_thread() failed:\n"
			"  %s\n",
			strerror(m_cycleThread)));
		return m_cycleThread;
	}

	// launch
	return resume_thread(m_cycleThread);
}
	
// shut down the cycle thread/port
status_t NodeGroup::_destroyCycleThread() {
	assert_locked(this);
	status_t err;
	D_METHOD((
		"NodeGroup::_destroyCycleThread()\n"));

	if(!m_cycleThread)
		return B_OK;

	if(!m_cycleThreadDone) {
		// kill the thread		
		ASSERT(m_cyclePort);
		err = write_port_etc(
			m_cyclePort,
			_CYCLE_STOP,
			0,
			0,
			B_TIMEOUT,
			10000LL);

		if(err < B_OK) {
			// bad thread.  die, thread, die.
			PRINT((
				"* NodeGroup::_destroyCycleThread(): port write failed; killing.\n"));
			delete_port(m_cyclePort);
			m_cyclePort = 0;
			kill_thread(m_cycleThread);
			m_cycleThread = 0;
			return B_OK;
		}	
	
		// the thread got the message; wait for it to quit
		unlock();
		while(wait_for_thread(m_cycleThread, &err) == B_INTERRUPTED) {
			PRINT((
				"! wait_for_thread(m_cycleThread, &err) == B_INTERRUPTED\n"));
		}		
		lock();
	}
	
	// it's up to the thread to close its port
	ASSERT(!m_cyclePort);

	m_cycleThread = 0;
				
	return B_OK;
}


// 1) do the current positions specify a valid cycle region?
// 2) are any nodes in the group cycle-enabled?

bool NodeGroup::_cycleValid() {
	assert_locked(this);
	return
		(m_transportState == TRANSPORT_RUNNING ||
		 m_transportState == TRANSPORT_STARTING) &&
		 canCycle();
}

// initialize the cycle members (call when starting)

void NodeGroup::_cycleInit(
	bigtime_t										startTime) {
	assert_locked(this);
	ASSERT(m_cycleNodes.size() > 0);
	D_METHOD((
		"NodeGroup::_cycleInit(%Ld)\n",
		startTime));

	// +++++ rescan latencies?

	// figure new boundary & deadline from region length
	bigtime_t cyclePeriod = m_endPosition - m_startPosition;

	if(cyclePeriod <= 0) {
		// cycle region is no longer valid
		m_cycleBoundary = 0LL;
		m_cycleDeadline = 0LL;

//		no no no -- deadlocks when the thread calls this method
//		// stop the thread
//		_destroyCycleThread();
		return;
	}
	
	m_cycleStart = startTime;
	m_cycleBoundary = startTime + cyclePeriod;
	m_cycleDeadline = m_cycleBoundary - (m_cycleMaxLatency + s_rosterLatency);
}


// add a ref to the cycle set (in proper order, based on latency)
void NodeGroup::_cycleAddRef(
	NodeRef*										ref) {
	assert_locked(this);
	
	// make sure it's not already there
	ASSERT(find(
		m_cycleNodes.begin(),
		m_cycleNodes.end(),
		ref) == m_cycleNodes.end());

	// [re]calc latency if 0
	if(!ref->m_latency)
		ref->_updateLatency();

	node_set::iterator it;
	for(it = m_cycleNodes.begin();
		it != m_cycleNodes.end(); ++it) {
		if(ref->m_latency > (*it)->m_latency) {
			m_cycleNodes.insert(it, ref);
			break;
		}
	}
	
	// not inserted? new ref belongs at the end
	if(it == m_cycleNodes.end())
		m_cycleNodes.insert(it, ref);
}
		
// remove a ref from the cycle set
void NodeGroup::_cycleRemoveRef(
	NodeRef*										ref) {
	assert_locked(this);
	
	node_set::iterator it = find(
		m_cycleNodes.begin(),
		m_cycleNodes.end(),
		ref);
	ASSERT(it != m_cycleNodes.end());
	m_cycleNodes.erase(it);
}

bigtime_t NodeGroup::_cycleBoundary() const {
	Autolock _l(this);
	return m_cycleBoundary;
}

// cycle thread impl.
/*static*/
status_t NodeGroup::_CycleThread(void* user) {
	((NodeGroup*)user)->_cycleThread();
	return B_OK;
}

void NodeGroup::_cycleThread() {

	status_t err;
	int32 code;
	int32 errorCount = 0;

	// +++++ liability -- if the thread has to be killed, this buffer
	//       won't be reclaimed
	char* msgBuffer = new char[_portMsgMaxSize];
	array_delete<char> _d(msgBuffer);
	
	// create port
	ASSERT(!m_cyclePort);
	m_cyclePort = create_port(
		_portLength,
		_portName);
	ASSERT(m_cyclePort >= B_OK);
	
	// the message-handling loop
	bool done = false;	
	while(!done) {

		// *** wait until it's time to queue the next cycle, or until
		// *** a message arrives

		lock();				// **** BEGIN LOCKED SECTION ****
		if(!_cycleValid()) {
			unlock();
			break;
		}

		ASSERT(m_cycleNodes.size() > 0);
		ASSERT(m_timeSourceObj);

		bigtime_t maxLatency = m_cycleNodes.front()->m_latency;
		bigtime_t wakeUpAt = m_timeSourceObj->RealTimeFor(
			m_cycleBoundary, maxLatency + s_rosterLatency);
		bigtime_t timeout = wakeUpAt - m_timeSourceObj->RealTime();

		if(timeout <= 0) {
			// +++++ whoops, I'm late.
			// +++++ adjust to compensate !!!
			PRINT((
				"*** NodeGroup::_cycleThread(): LATE\n"
				"    by %Ld\n", -timeout));
		}
		
		// +++++ if timeout is very short, spin until the target time arrives
		
		unlock();			// **** END LOCKED SECTION ****
		
		// block until message arrives or it's time to wake up
		err = read_port_etc(
			m_cyclePort,
			&code,
			msgBuffer,
			_portMsgMaxSize,
			B_TIMEOUT,
			timeout);

		if(err == B_TIMED_OUT) {
			// the time has come to seek my nodes
			_handleCycleService();
			continue;
		}		
		else if(err < B_OK) {
			// any other error is bad news
			PRINT((
				"* NodeGroup::_cycleThread(): read_port error:\n"
				"  %s\n"
				"  ABORTING\n\n", strerror(err)));
			if(++errorCount > 10) {
				PRINT((
					"*** Too many errors; aborting.\n"));
				break;
			}
			continue;
		}
		
		errorCount = 0;
		
		// process the message
		switch(code) {
			case _CYCLE_STOP:
				// bail
				done = true;
				break;
				
			case _CYCLE_END_CHANGED:
			case _CYCLE_LATENCY_CHANGED:
				// fall through to next loop; for now, these messages
				// serve only to slap me out of my stupor and reassess
				// the timing situation...
				break;
				
			default:
				PRINT((
					"* NodeGroup::_cycleThread(): unknown message code '%ld'\n", code));
				break;
		}
	} // while(!done)

	
	// delete port
	delete_port(m_cyclePort);
	m_cyclePort = 0;

	// done
	m_cycleThreadDone = true;
}

// cycle service: seek all nodes & initiate next cycle
void NodeGroup::_handleCycleService() {
	Autolock _l(this);
//	D_METHOD((
//		"NodeGroup::_handleCycleService()\n"));
	status_t err;
		
	if(!_cycleValid()) {
//		PRINT((
//			"- _handleCycleService(): cycle not valid; quitting.\n"));
		return;
	}
	
	// seek
	for(node_set::iterator it = m_cycleNodes.begin();
		it != m_cycleNodes.end(); ++it) {
		err = (*it)->_seek(
			m_startPosition,
			m_cycleBoundary);
		if(err < B_OK) {
			PRINT((
				"- _handleCycleService(): node('%s')::_seek() failed:\n"
				"  %s\n",
				(*it)->name(), strerror(err)));
		}
	}

	// update cycle settings
	if(m_newStart) {
		m_newStart = false;
		m_startPosition = m_newStartPosition;
	}
	if(m_newEnd) {
		m_newEnd = false;
		m_endPosition = m_newEndPosition;
	}

	// prepare next cycle
	_cycleInit(m_cycleBoundary);
}

// END -- NodeGroup.cpp --
