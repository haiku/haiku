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


// NodeRef.h (Cortex/NodeManager)
//
// * PURPOSE
//   Represents a NodeManager reference to a live Media Kit Node.
//
// * FEATURES UNDER CONSIDERATION +++++
//   - lazy referencing: m_released becomes m_referenced; only reference
//     externally created nodes once they've been grouped or connected.
//     +++++ or disconnected? keep a'thinkin...
//   - expose dormant_node_info
//
// * HISTORY
//   e.moon		11aug99		Added kind().
//   e.moon		9aug99		Moved position & cycle threads into NodeRef;
//											no more 'group transport thread'.
//   e.moon		25jun99		Begun

#ifndef __NodeRef_H__
#define __NodeRef_H__

#include <MediaAddOn.h>
#include <MediaNode.h>
#include <String.h>

#include <vector>

#include "ILockable.h"
#include "MultiInvoker.h"
#include "ObservableHandler.h"
#include "observe.h"

#include "cortex_defs.h"

__BEGIN_CORTEX_NAMESPACE

class Connection;
class NodeManager;
class NodeGroup;
class NodeSyncThread;

class NodeRef :
	public	ObservableHandler,
	protected	ILockable {

	typedef	ObservableHandler _inherited;

	friend class NodeManager;
	friend class NodeGroup;

public:				// *** messages
	enum message_t {
		// OUTBOUND
		//  nodeID: int32
		//  target: BMessenger
		M_OBSERVER_ADDED			=NodeRef_message_base,
		M_OBSERVER_REMOVED,
		M_RELEASED,
		
		// OUTBOUND
		// nodeID: int32
		// groupID: int32
		M_GROUP_CHANGED,		
		
		// nodeID: int32
		// runMode: int32
		M_RUN_MODE_CHANGED,

		// +++++
		M_INPUTS_CHANGED, //nyi
		M_OUTPUTS_CHANGED,  //nyi
		
		// OUTBOUND
		//  * only sent to position listeners
		//  nodeID:	         int32
		//  when:      		   int64
		//  position:        int64
		M_POSITION,
		
		// INBOUND
		// runMode: int32
		// delay: int64 (bigtime_t; applies only to B_RECORDING mode)
		M_SET_RUN_MODE,
		
		// INBOUND
		M_PREROLL,
		
		// INBOUND
		// "cycling"; bool (OR "be:value"; int32)
		M_SET_CYCLING,
		
		// OUTBOUND
		// "cycling"; bool
		M_CYCLING_CHANGED
	};

public:				// *** flags
	enum flag_t {
		// node-level transport restrictions
		NO_START_STOP						= 1<<1,
		NO_SEEK									= 1<<2,
		NO_PREROLL							= 1<<3,
		
		// [e.moon 28sep99] useful for misbehaved nodes
		NO_STOP									= 1<<4,
		
		// [e.moon 11oct99]
		// Disables media-roster connection (which currently
		// only makes use of B_MEDIA_NODE_STOPPED.)
		// This flag may become deprecated as the Media Kit
		// evolves (ie. more node-level notifications come
		// along.)
		NO_ROSTER_WATCH					= 1<<5,
		
		// [e.moon 14oct99]
		// Disables position reporting (via BMediaRoster::SyncToNode().)
		// Some system-provided nodes tend to explode when SyncToNode() is
		// called on them (like the video-file player in BeOS R4.5.2).
		NO_POSITION_REPORTING		= 1<<6
	};

public:				// *** dtor

	// free the node (this call will result in the eventual
	// deletion of the object.)
	// returns B_OK on success; B_NOT_ALLOWED if release() has
	// already been called; other error codes if the Media Roster
	// call fails.
	
	status_t release();

	// call release() rather than deleting NodeRef objects
	virtual ~NodeRef();

public:				// *** const accessors
	// [e.moon 13oct99] moved method definitions here to keep inline
	// in the face of a balky PPC compiler
	inline const media_node& node() const { return m_info.node; }
	inline uint32 kind() const { return m_info.node.kind; }
	inline const live_node_info& nodeInfo() const { return m_info; }
	inline const char* name() const { return m_info.name; }
	inline media_node_id id() const { return m_info.node.node; }

public:				// *** member access

	// turn cycle mode (looping) on or off	
	void setCycling(
		bool												cycle);
	bool isCycling() const;
	
	// is the node running?
	bool isRunning() const;
	
	// was the node created via NodeManager::instantiate()?
	bool isInternal() const;

	// fetch the group, or 0 if the node is ungrouped.
	NodeGroup* group() const;
	
	// flag access
	uint32 flags() const;
	void setFlags(
		uint32											flags);
	
	// [e.moon 29sep99]
	// access addon-hint info
	// - returns B_BAD_VALUE if not an add-on node created by this NodeManager

	status_t getDormantNodeInfo(
		dormant_node_info*					outInfo);

	// [e.moon 29sep99]
	// access file being played
	// - returns B_BAD_VALUE if not an add-on node created by this NodeManager,
	//   or if the node has no associated file

	status_t getFile(
		entry_ref*									outFile); 
		
	// [e.moon 8dec99]
	// set file to play
	
	status_t setFile(
		const entry_ref&						file,
		bigtime_t*									outDuration=0); //nyi

	// [e.moon 23oct99]
	// returns true if the media_node has been released (call releaseNode() to
	// make this happen.)
	
	bool isNodeReleased() const;
	
//		now implemented by ObservableHandler [20aug99]
//	// has this reference been released?
//	bool isReleased() const;

public:				// *** run-mode operations
	void setRunMode(
		uint32											runMode,
		bigtime_t										delay=0LL);
	uint32 runMode() const;
	
	bigtime_t recordingDelay() const;
	
	// calculates the minimum amount of delay needed for
	// B_RECORDING mode
	// +++++ 15sep99: returns biggest_output_buffer_duration * 2
	// +++++ 28sep99: adds downstream latency
	bigtime_t calculateRecordingModeDelay(); //nyi

public:				// *** connection access

	// connection access: vector versions
	
	status_t getInputConnections(
		std::vector<Connection>&	ioConnections,
		media_type									filterType=B_MEDIA_UNKNOWN_TYPE) const;

	status_t getOutputConnections(
		std::vector<Connection>&	ioConnections,
		media_type									filterType=B_MEDIA_UNKNOWN_TYPE) const;
	
	// connection access: flat array versions
	
	status_t getInputConnections(
		Connection*									outConnections,
		int32												maxConnections,
		int32*											outNumConnections,
		media_type									filterType=B_MEDIA_UNKNOWN_TYPE) const;

	status_t getOutputConnections(
		Connection*									outConnections,
		int32												maxConnections,
		int32*											outNumConnections,
		media_type									filterType=B_MEDIA_UNKNOWN_TYPE) const;

	// +++++ connection matching by source/destination +++++

public:				// *** position reporting/listening

	bool positionReportsEnabled() const;
	
	void enablePositionReports();
	void disablePositionReports();
	
	// Fetch the approximate current position:
	//   Returns the last reported position, and the
	//   performance time at which that position was reached.  If the
	//   transport has never been started, the start position and
	//   a performance time of 0 will be returned.  If position updating
	//   isn't currently enabled, returns B_NOT_ALLOWED.
	
	status_t getLastPosition(
		bigtime_t*									outPosition,
		bigtime_t*									outPerfTime) const;

	// Subscribe to regular position reports:
	//   Position reporting isn't rolled into the regular observer
	//   interface because a large number of messages are generated
	//   (the frequency can be changed; see below.)
	
	status_t addPositionObserver(
		BHandler*										handler);
		
	status_t removePositionObserver(
		BHandler*										handler);
	
	// Set how often position updates will be sent:
	//   Realistically, period should be > 10000 or so.
	
	status_t setPositionUpdatePeriod(
		bigtime_t										period);
		
	bigtime_t positionUpdatePeriod() const;

public:				// *** BMediaRoster wrappers & convenience methods

	// release the media node
	// (if allowed, will trigger the release/deletion of this object)
	status_t releaseNode();

	// calculate total (internal + downstream) latency for this node
	
	status_t totalLatency(
		bigtime_t*									outLatency) const;


	// retrieve input/output matching the given destination/source.
	// returns B_MEDIA_BAD_[SOURCE | DESTINATION] if the destination
	// or source don't correspond to this node.
	
	status_t findInput(
		const media_destination&		forDestination,
		media_input*								outInput) const;
		
	status_t findOutput(
		const media_source&					forSource,
		media_output*								outOutput) const;


	// endpoint matching (given name and/or format as 'hints').
	// If no hints are given, returns the first free endpoint, if
	// any exist.
	// returns B_ERROR if no matching endpoint is found.
	
	status_t findFreeInput(
		media_input*								outInput,
		media_type									type=B_MEDIA_UNKNOWN_TYPE,
		const char*									name=0) const;

	status_t findFreeInput(
		media_input*								outInput,
		const media_format*					format,
		const char*									name=0) const;

	status_t findFreeOutput(
		media_output*								outOutput,
		media_type									type=B_MEDIA_UNKNOWN_TYPE,
		const char*									name=0) const;

	status_t findFreeOutput(
		media_output*								outOutput,
		const media_format*					format,
		const char*									name=0) const;

	// node endpoint access: vector versions (wrappers for BMediaRoster
	// calls.)
	
	status_t getFreeInputs(
		std::vector<media_input>&	ioInputs,
		media_type									filterType=B_MEDIA_UNKNOWN_TYPE) const;

	status_t getConnectedInputs(
		std::vector<media_input>&	ioInputs,
		media_type									filterType=B_MEDIA_UNKNOWN_TYPE) const;

	status_t getFreeOutputs(
		std::vector<media_output>&	ioOutputs,
		media_type									filterType=B_MEDIA_UNKNOWN_TYPE) const;

	status_t getConnectedOutputs(
		std::vector<media_output>&	ioOutputs,
		media_type									filterType=B_MEDIA_UNKNOWN_TYPE) const;
		
		
	// node endpoint access: array versions (wrappers for BMediaRoster
	// calls.)
	
	status_t getFreeInputs(
		media_input*								outInputs,
		int32												maxInputs,
		int32*											outNumInputs,
		media_type									filterType=B_MEDIA_UNKNOWN_TYPE) const;
	
	status_t getConnectedInputs(
		media_input*								outInputs,
		int32												maxInputs,
		int32*											outNumInputs) const;
	
	status_t getFreeOutputs(
		media_output*								outOutputs,
		int32												maxOutputs,
		int32*											outNumOutputs,
		media_type									filterType=B_MEDIA_UNKNOWN_TYPE) const;
	
	status_t getConnectedOutputs(
		media_output*								outOutputs,
		int32												maxOutputs,
		int32*											outNumOutputs) const;
	
public:													// *** BHandler:
	virtual void MessageReceived(
		BMessage*								message);
		
public:				// *** IObservable:		[20aug99]
	virtual void observerAdded(
		const BMessenger&				observer);
		
	virtual void observerRemoved(
		const BMessenger&				observer);

	virtual void notifyRelease();

	virtual void releaseComplete();

protected:		// *** ILockable
							//     Only WRITE locking is allowed!

	bool lock(
		lock_t type=WRITE,
		bigtime_t timeout=B_INFINITE_TIMEOUT);	
	bool unlock(
		lock_t type=WRITE);	
	bool isLocked(
		lock_t type=WRITE) const;

protected:			// *** ctor (accessible to NodeManager)
	// throws runtime_error
	NodeRef(
		const media_node&		node,
		NodeManager*				manager,
		uint32							userFlags,
		uint32							implFlags);

protected:			// *** endpoint-fixing operations (no lock required)

	// 'fix' (fill in node if needed) sets of inputs/outputs
	void _fixInputs(
		media_input*									inputs,
		int32													count) const;
	void _fixInputs(
		std::vector<media_input>&		inputs) const;

	void _fixOutputs(
		media_output*									outputs,
		int32													count) const;
	void _fixOutputs(
		std::vector<media_output>&		outputs) const;

protected:			// *** internal/NodeManager operations (LOCK REQUIRED)

	// call after instantiation to register info used to select and
	// create this add-on node

	void _setAddonHint(
		const dormant_node_info*			info,
		const entry_ref*							file=0);
		
	// call to set a new group; if 0, the node must have no
	// connections
	void _setGroup(
		NodeGroup*										group);
		
	// *** NodeGroup API ***
	//     9aug99: moved from NodeGroup
	//     [e.moon 13oct99] removed inlines for the sake of PPC sanity
		
	// initialize the given node's transport-state members
	// (this may be called from the transport thread or from
	//  an API-implementation method.)
	
	status_t _initTransportState();
		
	status_t _setTimeSource(
		media_node_id					timeSourceID);
		
	status_t _setRunMode(
		const uint32					runMode,
		bigtime_t							delay=0LL);
		
	status_t _setRunModeAuto(
		const uint32					runMode);
	
	// seek and preroll the given node.
	// *** this method should not be called from the transport thread
	// (since preroll operations can block for a relatively long time.)
	//
	// returns B_NOT_ALLOWED if the node is running, or if its NO_PREROLL
	// flag is set; otherwise, returns B_OK on success or a Media Roster
	// error.
	
	status_t _preroll(
		bigtime_t							position);
		
	// seek the given node if possible
	// (this may be called from the transport thread or from
	//  an API-implementation method.)

	status_t _seek(
		bigtime_t							position,
		bigtime_t							when);

	// seek the given (stopped) node
	// (this may be called from the transport thread or from
	//  an API-implementation method.)

	status_t _seekStopped(
		bigtime_t							position);

	// start the given node, if possible & necessary, at
	// the given time
	// (this may be called from the transport thread or from
	//  an API-implementation method.)
		
	status_t _start(
		bigtime_t							when);

	// stop the given node (which may or may not still be
	// a member of this group.)
	// (this may be called from the transport thread or from
	//  an API-implementation method.)

	status_t _stop();
	
	// roll the given node, if possible.
	// (this may be called from the transport thread or from
	//  an API-implementation method.)

	status_t _roll(
		bigtime_t							start,
		bigtime_t							stop,
		bigtime_t							position);
	
	// refresh the node's current latency; if I reference
	// a B_RECORDING node, update its 'producer delay'.
	
	status_t _updateLatency();
	
	// +++++ this method may not be needed 10aug99 +++++
	// Figure the earliest time at which the given node can be started.
	// Also calculates the position at which it should start from to
	// play in sync with other nodes in the group, if the transport is
	// running; if stopped, *outPosition will be set to the current
	// start position.
	// Pass the estimated amount of time needed to prepare the
	// node for playback (ie. preroll & a little fudge factor) in
	// startDelay.
	//
	// (this may be called from the transport thread or from
	//  an API-implementation method.)

	status_t _calcStartTime(
		bigtime_t							startDelay,
		bigtime_t*						outTime,
		bigtime_t*						outPosition); //nyi

	// *** Position reporting ***
	
	// callers: _start(), _roll(), enablePositionReports()
	status_t _startPositionThread();
	
	// callers: _stop(), disablePositionReports(), dtor
	status_t _stopPositionThread();
	
	// [e.moon 14oct99] handle a report
	status_t _handlePositionUpdate(
		bigtime_t							perfTime,
		bigtime_t							position);
				
	// callers: _handlePositionUpdate
	// (schedules a position update for the given time and position;
	//  if the position overlaps a cycle, adjusts time & position
	//  so that the notification is sent at the cycle point.)
	status_t _schedulePositionUpdate(
		bigtime_t							when,
		bigtime_t							position);
		
	// callers: _handlePositionUpdate, _initPositionThread()
	// Send a message to all position observers
	status_t _notifyPosition(
		bigtime_t							when,
		bigtime_t							position);

private:										// *** members

	// the node manager
	NodeManager*							m_manager;
	
	// owning (parent) group; may be 0 if node is not connected.
	// A connected node always has a group, since groups allow transport
	// operations.  New groups are created as necessary.
	NodeGroup*								m_group;

	// the node's state
	live_node_info						m_info;

	// user-definable transport behavior
	uint32										m_flags;
	
	// private/implementation flags
	
	enum impl_flag_t {
		// the node was created by NodeManager
		_INTERNAL					= 1<<1,
		// the node should NOT be released when this instance is destroyed
		_NO_RELEASE				= 1<<2,
		// notification of the node's instantiation has been received
		// [e.moon 11oct99]
		_CREATE_NOTIFIED	= 1<<3
	};
	
	uint32										m_implFlags;
	
	// takes BMediaNode::run_mode values or 0 (wildcard:
	// group run mode used instead)
	// May not be B_OFFLINE; that must be specified at the group level.
	
	uint32										m_runMode;

	// only valid if m_runMode is BMediaNode::B_RECORDING
	bigtime_t									m_recordingDelay; 

	// Media Roster connection [e.moon 11oct99]
	bool											m_watching;

	// hint information: this info is serialized with the object
	// to provide 'hints' towards properly finding the same add-on
	// node later on.  If no hint is provided, the node is assumed
	// to be external to (not created by) the NodeManager.
	
	struct addon_hint;
	addon_hint*								m_addonHint;
	
	// * position listening:
	//   - moved from NodeGroup 9aug99

	bool											m_positionReportsEnabled;
	bool											m_positionReportsStarted;
	bigtime_t									m_positionUpdatePeriod;
	static const bigtime_t		s_defaultPositionUpdatePeriod			= 50000LL;
	
	::MultiInvoker							m_positionInvoker;
	
	bigtime_t									m_tpLastPositionUpdate;
	bigtime_t									m_lastPosition;
	
	// * synchronization threads
	
	// only active if position listening has been enabled
	NodeSyncThread*						m_positionThread;
	
//	// only active if this node is cycling (looping)
//	CycleSyncThread*				m_cycleSyncThread;
//	+++++ 10aug99: moved back to NodeGroup

private:										// *** transport state members

	// is this node running?
	bool											m_running;
	
	// has the media_node (NOT this object) been released?
	bool											m_nodeReleased;

	// is this node supposed to loop?
	bool											m_cycle;

	// has the node been prepared to start?
	bool											m_prerolled;
	
	// when was the node started?
	bigtime_t									m_tpStart;
	
	// if the node has been prerolled, m_tpLastSeek will be 0 but
	// m_lastSeekPos will reflect the node's prerolled position
	
	bigtime_t									m_tpLastSeek;
	bigtime_t									m_lastSeekPos;

	// has a stop event been queued? [e.moon 11oct99]
	bigtime_t									m_stopQueued;
	
	// last known latency for this node
	bigtime_t									m_latency;
};

__END_CORTEX_NAMESPACE
#endif /*__NodeRef_H__*/
