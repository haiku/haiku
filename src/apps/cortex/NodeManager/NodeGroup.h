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


// NodeGroup.h (Cortex/NodeManager)
//
// * PURPOSE
//   Represents a logical group of media kit nodes.
//   Provides group-level operations (transport control
//   and serialization, for example.)
//
// * NODE SYNC/LOOPING +++++
//
//   22jul99
//   ----------------------------
//   +++++
//   - cycling support needs to be thoroughly defined; ie. what
//     can it do, and what can it NOT do.
//
//     For example, a change in node latency will likely confuse
//     the cycling mechanism.  Is there any possible way to avoid
//     this without explicit help from the node?  The roster sends
//     no notification, and even if it did the message might not
//     arrive in time.  Polling is possible, but ...ick.
//
//     [this is assuming the 'just-in-time' cycling mechanism:
//      seeks are queued as late as possible; if the latency increases
//      significantly, the seek will be queued TOO late and the
//      node will get out of sync.]
//
//   ----------------------------
//   14jul99
//
//   How about handling addition of a node to a group while it's
//   playing?  The "effects insert" scenario:
//
//     1) you have a producer node, followed by (0..*) filters in
//        series, followed by a consumer
//     2) the transport is started
//     3) you wish to connect a new filter in the chain with minimal
//        interruption -- preferably a pause in output, resuming
//        with media time & perf. time still in sync.
//
//     Process:
//     - instantiate the filter & do any setup needed
//     - [tmStart = current media time; tpStart = current perf. time]
//     - stop the transport; break the connection at the insert
//       point and connect the new filter
//     - calculate the new latency
//     - cue a seek for all nodes:
//       to tmStart+latency+pad,
//       at tpStart+latency+pad - 1
//     - cue a start for all nodes:
//       at tpStart+latency+pad
//
//     (pad is the estimated amount of time taken to stop, break
//      & make connections, etc.  It can probably be determined in
//      a test loop.)
//
//   With the current NodeManager grouping behavior, this operation
//   would split the NodeGroup, then (provided the filter insertion
//   works) join it again.  This is such a common procedure, though,
//   that an 'insert' operation at the NodeManager level would be
//   pretty damn handy.  So would a 'remove insert' operation (given
//   a node with a single input and output, connect its input's source
//   to its output's destination, with the original format.)
//
// * HISTORY
//   e.moon		29sep99		Made thread control variables 'volatile'.
//   e.moon		6jul99		Begun

#ifndef __NodeGroup_H__
#define __NodeGroup_H__

#include "ObservableHandler.h"
#include "observe.h"
#include "ILockable.h"

// +++++ [e.moon 3dec99] need to include these for calcLatencyFn impl
// +++++ YUCK
#include "NodeRef.h"
#include <MediaRoster.h>

#include <vector>

#include <Locker.h>
#include <MediaNode.h>
#include <String.h>

class BTimeSource;

#include "cortex_defs.h"

#if CORTEX_XML
	#include "IPersistent.h"
#endif

__BEGIN_CORTEX_NAMESPACE

class NodeManager;
class NodeRef;

class GroupCycleThread;

class NodeGroup :
	public		ObservableHandler,
	public		ILockable {

	typedef	ObservableHandler _inherited;

	friend class NodeManager;
	friend class NodeRef;
	
public:				// *** messages
	enum message_t {
		//  groupID: int32
		//  target: BMessenger
		M_OBSERVER_ADDED			=NodeGroup_message_base,
		M_OBSERVER_REMOVED,
		M_RELEASED,
		
		//  groupID:         int32
		//  nodeID:          int32
		M_NODE_ADDED,
		M_NODE_REMOVED,

		//  groupID:         int32
		//  transportState:  int32
		// ++++++ include the following? 
		//  runMode:         int32
		// [mediaStart:      bigtime_t] only sent if changed
		// [mediaEnd:        bigtime_t] only sent if changed		
		M_TRANSPORT_STATE_CHANGED,

		//  groupID:         int32
		//  timeSourceID:    int32 +++++ should be a node?
		M_TIME_SOURCE_CHANGED,

		//  groupID:				 int32
		//  runMode:				 int32
		M_RUN_MODE_CHANGED,
		
		// Set a new time source:
		//  timeSourceNode:   media_node
		M_SET_TIME_SOURCE,
		
		// Set a run mode
		//  runMode:				 int32 (must be a valid run mode -- not 0!)
		M_SET_RUN_MODE,
		
		// Set new start/end position:
		//  position:        bigtime_t (int64)
		M_SET_START_POSITION, //K
		M_SET_END_POSITION,   //L
		
		// Transport controls:
		M_PREROLL,
		M_START,
		M_STOP,
		M_ROLL 	// [e.moon 11oct99]
	};
		
public:				// *** types

	// transport state
	enum transport_state_t {
		TRANSPORT_INVALID,
		TRANSPORT_STOPPED,
		TRANSPORT_STARTING,
		TRANSPORT_RUNNING,
		TRANSPORT_ROLLING,		// [e.moon 11oct99]
		TRANSPORT_STOPPING
	};

	// [em 1feb00] flags
	enum flag_t {
		// no new nodes may be added
		GROUP_LOCKED     = 1
	};

public:				// *** ctor/dtor

	// free the group, including all nodes within it
	// (this call will result in the eventual deletion of the object.)
	// returns B_OK on success; B_NOT_ALLOWED if release() has
	// already been called; other error codes if the Media Roster
	// call fails.
	
	status_t release();

	// call release() rather than deleting NodeGroup objects
	virtual ~NodeGroup();

public:				// *** const accessors
	// [e.moon 13oct99] moved method definition here to keep inline
	// in the face of a balky PPC compiler
	inline uint32 id() const { return m_id; }

public:				// *** content accessors

	// name access
	const char* name() const;
	status_t setName(const char* name);

	// node access
	// - you can write-lock the group during sets of calls to these methods;
	//   this ensures that the node set won't change.  The methods do lock
	//   the group internally, so locking isn't explicitly required.
	uint32 countNodes() const;
	NodeRef* nodeAt(
		uint32											index) const;

	// add/remove nodes:
	// - you may only add a node with no current group.
	// - nodes added during playback will be started;
	//   nodes removed during playback will be stopped (unless
	//   the NO_START_STOP transport restriction flag is set
	//   for a given node.)	

	status_t addNode(
		NodeRef*										node);
		
	status_t removeNode(
		NodeRef*										node);
	
	status_t removeNode(
		uint32											index);
	
	// group flag access
	
	uint32 groupFlags() const;

	status_t setGroupFlags(
		uint32											flags);

	// returns true if one or more nodes in the group have cycling
	// enabled, and the start- and end-positions are valid
	bool canCycle() const;
	
public:				// *** TRANSPORT POSITIONING

	// Fetch the current transport state
	 
	transport_state_t transportState() const;

	// Set the starting media time:
	//   This is the point at which playback will begin in any media
	//   files/documents being played by the nodes in this group.
	//   When cycle mode is enabled, this is the point to which each
	//   node will be seek'd at the end of each cycle (loop).
	//
	//   The starting time can't be changed in the B_OFFLINE run mode
	//   (this call will return an error.)
	
	status_t setStartPosition(
		bigtime_t										start); //nyi
	
	// Fetch the starting position:
	
	bigtime_t startPosition() const; //nyi
		
	// Set the ending media time:
	//   This is the point at which playback will end relative to
	//   media documents begin played by the nodes in this group; 
	//   in cycle mode, this specifies the loop point.  If the
	//   ending time is less than or equal to the starting time,
	//   the transport will continue until stopped manually.
	//   If the end position is changed while the transport is playing,
	//   it must take effect retroactively (if it's before the current
	//   position and looping is enabled, all nodes must 'warp' to
	//   the proper post-loop position.) +++++ echk!
	//
	//   The ending time can't be changed if run mode is B_OFFLINE and
	//   the transport is running (this call will return an error.)
	
	status_t setEndPosition(
		bigtime_t										end); //nyi

	// Fetch the end position:
	//   Note that if the end position is less than or equal to the start
	//   position, it's ignored.
		
	bigtime_t endPosition() const; //nyi

public:				// *** TRANSPORT OPERATIONS
	
	// Preroll the group:
	//   Seeks, then prerolls, each node in the group (honoring the
	//   NO_SEEK and NO_PREROLL flags.)  This ensures that the group
	//   can start as quickly as possible.
	//
	//   Returns B_NOT_ALLOWED if the transport is running.

	status_t preroll();

	// Start all nodes in the group:
	//   Nodes with the NO_START_STOP flag aren't molested.
	
	status_t start();

	// Stop all nodes in the group:
	//   Nodes with the NO_START_STOP flag aren't molested.
	
	status_t stop();

	// Roll all nodes in the group:
	//   Queues a start and stop atomically (via BMediaRoster::RollNode()).
	//   Returns B_NOT_ALLOWED if endPosition <= startPosition.
	
	status_t roll();
	
public:				// *** TIME SOURCE & RUN-MODE OPERATIONS

	// getTimeSource():
	//   returns B_ERROR if no time source has been set; otherwise,
	//   returns the node ID of the current time source for all
	//   nodes in the group.
	//
	// setTimeSource():
	//   Calls SetTimeSourceFor() on every node in the group.
	//   The group must be stopped; B_NOT_ALLOWED will be returned
	//   if the state is TRANSPORT_RUNNING or TRANSPORT_ROLLING.
	
	status_t getTimeSource(
		media_node*									outTimeSource) const;
	
	status_t setTimeSource(
		const media_node&						timeSource);
		
	// run mode access:
	//   Sets the default run mode for the group.  This will be
	//   applied to every node with a wildcard (0) run mode.
	//
	//   Special case: if the run mode is B_OFFLINE, it will be
	//   applied to all nodes in the group.
	
	status_t setRunMode(
		BMediaNode::run_mode				mode); //nyi
	
	BMediaNode::run_mode runMode() const; //nyi

public:				// *** BHandler
	virtual void MessageReceived(
		BMessage*										message);
		
#if CORTEX_XML
public:				// *** IPersistent
	// +++++
	
	// Default constructor
	NodeGroup();
	
#endif /*CORTEX_XML*/

public:				// *** IObservable:		[19aug99]
	virtual void observerAdded(
		const BMessenger&				observer);
		
	virtual void observerRemoved(
		const BMessenger&				observer);

	virtual void notifyRelease();

	virtual void releaseComplete();

public:				// *** ILockable:     [21jul99]
							//     Each NodeGroup has a semaphore (BLocker).
							//     Only WRITE locking is allowed!

	bool lock(
		lock_t type=WRITE,
		bigtime_t timeout=B_INFINITE_TIMEOUT);	
	bool unlock(
		lock_t type=WRITE);	
	bool isLocked(
		lock_t type=WRITE) const;
		
protected:			// *** ctor (accessible to NodeManager)
	NodeGroup(
		const char*									name,
		NodeManager*								manager,
		BMediaNode::run_mode				runMode=BMediaNode::B_INCREASE_LATENCY);

protected:			// *** internal operations
		
	static uint32 NextID();
	
protected:			// *** ref->group communication (LOCK REQUIRED)

	// When a NodeRef's cycle state (ie. looping or not looping)
	// changes, it must pass that information on via this method.
	// +++++ group cycle thread	
	void _refCycleChanged(
		NodeRef*										ref);

	// when a cycling node's latency changes, call this method.
	// +++++ shouldn't there be a general latency-change hook?
	void _refLatencyChanged(
		NodeRef*										ref);
	
	// when a NodeRef receives notification that it has been stopped,
	// but is labeled as still running, it must call this method.
	// [e.moon 11oct99: roll/B_OFFLINE support]
	void _refStopped(
		NodeRef*										ref);

private:				// *** transport helpers (LOCK REQUIRED)
	
	// Preroll all nodes in the group; this is the implementation
	// of preroll().
	// *** this method should not be called from the transport thread
	// (since preroll operations can block for a relatively long time.)
	
	status_t _preroll();
	
	// Start all nodes in the group; this is the implementation of
	// start().
	//
	// (this may be called from the transport thread or from
	//  an API-implementation method.)
		
	status_t _start();
	
	// Stop all nodes in the group; this is the implementation of
	// stop().  Fails if the run mode is B_OFFLINE; use _roll() instead
	// in that case.
	//
	// (this may be called from the transport thread or from
	//  an API-implementation method.)
		
	status_t _stop();
	
	// Roll all nodes in the group; this is the implementation of
	// roll().
	//
	// (this may be called from the transport thread or from
	//  an API-implementation method.)

	status_t _roll(); //nyi [11oct99 e.moon]
	
	// State transition; notify listeners	
	inline void _changeState(
		transport_state_t			to);

	// Enforce a state transition, and notify listeners
	inline void _changeState(
		transport_state_t			from,
		transport_state_t			to);
	
	
private:				// *** transport thread guts
//	void _initPort();
//	void _initThread();
//
//	static status_t _TransportThread(void* user);
//	void _transportThread();

	// functor: calculates latency of each node it's handed, caching
	// the largest one found; includes initial latency if nodes report it.
	class calcLatencyFn { public:
		bigtime_t& maxLatency;
		calcLatencyFn(bigtime_t& _m) : maxLatency(_m) {}	
		void operator()(NodeRef* r) {
			ASSERT(r);
			if(!(r->node().kind & B_BUFFER_PRODUCER)) {
				// node can't incur latency
				return;
			}
			
			bigtime_t latency;
			status_t err =
				BMediaRoster::Roster()->GetLatencyFor(
					r->node(),
					&latency);
			if(err < B_OK) {
				PRINT((
					"* calcLatencyFn: GetLatencyFor() failed: %s\n",
					strerror(err)));
				return;
			}
			bigtime_t add;
			err = BMediaRoster::Roster()->GetInitialLatencyFor(
				r->node(),
				&add);
			if(err < B_OK) {
				PRINT((
					"* calcLatencyFn: GetInitialLatencyFor() failed: %s\n",
					strerror(err)));
			}
			else
				latency += add;			
			if(latency > maxLatency)
				maxLatency = latency;
		}
	};

	friend class 		calcLatencyFn;

protected:			// *** cycle thread & helpers (LOCK REQUIRED)

	// set up the cycle thread (including its kernel port)
	status_t _initCycleThread();
	
	// shut down the cycle thread/port
	status_t _destroyCycleThread();

	// 1) do the current positions specify a valid cycle region?
	// 2) are any nodes in the group cycle-enabled?
	bool _cycleValid();
	
	// initialize the next cycle
	void _cycleInit(
		bigtime_t										startTime);

	// add a ref to the cycle set (in proper order, based on latency)
	void _cycleAddRef(
		NodeRef*										ref);
		
	// remove a ref from the cycle set
	void _cycleRemoveRef(
		NodeRef*										ref);
		
	// fetches the next cycle boundary (performance time of next loop)
	bigtime_t _cycleBoundary() const;

	// cycle thread impl.
	static status_t _CycleThread(void* user);
	void _cycleThread();
	
	// cycle service: seek all nodes & initiate next cycle
	void _handleCycleService();

private:				// *** members

	// lock
	mutable BLocker					m_lock;

	// parent object
	NodeManager*						m_manager;
	
	// unique group ID (non-0)
	const uint32						m_id;
	static uint32						s_nextID;
	
	// group name
	BString									m_name;
	
	// group contents
	typedef std::vector<NodeRef*> node_set;
	node_set								m_nodes;
	
	// flags & state
	uint32									m_flags;
	transport_state_t				m_transportState;
	
	// default run mode applied to all nodes with a wildcard (0)
	// run mode.
	BMediaNode::run_mode		m_runMode;

	// current time source
	media_node							m_timeSource;
	BTimeSource*						m_timeSourceObj;
	
	// slated to die?
	bool										m_released;

	// ---------------------------
	// 10aug99
	// cycle thread implementation
	// ---------------------------
	
	enum cycle_thread_msg_t {
		_CYCLE_STOP,
		_CYCLE_END_CHANGED,
		_CYCLE_LATENCY_CHANGED
	};
	
	thread_id								m_cycleThread;
	port_id									m_cyclePort;
	bool										m_cycleThreadDone;

	// when did the current cycle begin?
	bigtime_t								m_cycleStart;
	
	// performance time at which the current cycle settings will
	// be applied (ie. the first seek should be queued)
	bigtime_t								m_cycleDeadline;
	
	// performance time at which the next cycle begins
	bigtime_t								m_cycleBoundary;
	
	// the set of nodes currently in cycle mode
	// ordered by latency (largest first)
	node_set								m_cycleNodes;
	
	// count of nodes that have completed this cycle
	// (once complete, deadline and boundary are reset, and any
	//  deferred start/end positions are applied.)
	uint32									m_cycleNodesComplete;
	
	// max latency of any cycling node
	bigtime_t								m_cycleMaxLatency;

	// the minimum allowed loop time
	static const bigtime_t	s_minCyclePeriod = 1000LL;
	
	// the amount of time to allow for Media Roster calls
	// (StartNode, SeekNode, etc) to be handled
	static const bigtime_t	s_rosterLatency = 1000LL; // +++++ probably high
	
	// position state
	volatile bigtime_t			m_startPosition;
	volatile bigtime_t			m_endPosition;
	
	// changed positions deferred in cycle mode
	volatile bool						m_newStart;
	volatile bigtime_t			m_newStartPosition;
	volatile bool						m_newEnd;
	volatile bigtime_t			m_newEndPosition;
};


__END_CORTEX_NAMESPACE
#endif /*__NodeGroup_H__*/
