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


// NodeRef.cpp

#include "NodeRef.h"

#include "Connection.h"
#include "NodeGroup.h"
//#include "NodeGroup_transport_thread.h"
#include "NodeManager.h"
#include "NodeSyncThread.h"

#include "AddOnHost.h"

#include <Entry.h>
#include <MediaRoster.h>
#include <TimeSource.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <stdexcept>

#include "functional_tools.h"
#include "node_manager_impl.h"
#include "SoundUtils.h"

// -------------------------------------------------------- //

using namespace std;

__USE_CORTEX_NAMESPACE

#define D_METHOD(x) //PRINT (x)
#define D_MESSAGE(x) //PRINT (x)
#define D_ROSTER(x) //PRINT (x)
#define D_LOCK(x) //PRINT (x)

// -------------------------------------------------------- //
// addon_hint
//
// Contains information that can be used to reconstruct this
// node later on.
// -------------------------------------------------------- //

// [e.moon 29sep99] added 'file'

struct NodeRef::addon_hint {

	addon_hint(
		const dormant_node_info*			_dormantInfo,
		const entry_ref*							_file=0) :
		dormantInfo(*_dormantInfo),
		file(_file ? new entry_ref(*_file) : 0) {}
	
	~addon_hint() {
		if(file) delete file;
	}
	
	void setFile(
		const entry_ref*						_file) {
		ASSERT(_file);
		if(file) delete file;
		file = new entry_ref(*_file);
	}
		
	dormant_node_info							dormantInfo;
	entry_ref*										file;
};

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

// free the node (this call will result in the eventual
// deletion of the object.)

status_t NodeRef::release() {

	// release the node, if possible:
	status_t err = releaseNode();

	// hand off to parent release() implementation
	status_t parentErr = _inherited::release();
	return (parentErr < B_OK) ? parentErr : err;
}

NodeRef::~NodeRef() {
	D_METHOD(("~NodeRef[%s]\n", name()));
	Autolock _l(m_manager);
	
	// remove from NodeManager
	m_manager->_removeRef(id());
	
	// [14oct99 e.moon] kill position-report thread if necessary
	if(m_positionThread)
		_stopPositionThread();

	if(m_addonHint) {
		delete m_addonHint;
		m_addonHint = 0;
	}

	// close Media Roster connection [e.moon 11oct99]
	BMediaRoster* r = BMediaRoster::Roster();
	if(m_watching) {
		r->StopWatching(
			BMessenger(this),
			node(),
			B_MEDIA_WILDCARD);
	}
}

// -------------------------------------------------------- //
// *** const accessors
// -------------------------------------------------------- //

// [e.moon 13oct99] moved to header

//inline const media_node& NodeRef::node() const { return m_info.node; }
//inline uint32 NodeRef::kind() const { return m_info.node.kind; }
//inline const live_node_info& NodeRef::nodeInfo() const { return m_info; }
//inline media_node_id NodeRef::id() const { return m_info.node.node; }
//inline const char* NodeRef::name() const { return m_info.name; }

// -------------------------------------------------------- //
// *** member access
// -------------------------------------------------------- //

// turn cycle mode (looping) on or off
	
void NodeRef::setCycling(
	bool												cycle) {
	Autolock _l(this);

	D_METHOD((
		"NodeRef::setCycling(%s)\n",
		cycle ? "true" : "false"));

	if(cycle == m_cycle)
		return;

	m_cycle = cycle;

	if(m_group) {
		m_group->_refCycleChanged(this);
		
		// +++++ if running, get started...
	}
	
	// notify
	BMessage m(M_CYCLING_CHANGED);
	m.AddBool("cycling", cycle);
	notify(&m);
}

bool NodeRef::isCycling() const {
	return m_cycle;
}


// is the node running?
	
bool NodeRef::isRunning() const {
	return m_running;
}

// was the node created via NodeManager::instantiate()?
bool NodeRef::isInternal() const {
	return m_implFlags & _INTERNAL;
}


// fetch the group; may return 0 if the node has no connections	
NodeGroup* NodeRef::group() const {
	Autolock _l(this);
	return m_group;
}
	
// flag access
uint32 NodeRef::flags() const {
	Autolock _l(this);
	return m_flags;
}

void NodeRef::setFlags(
	uint32											flags) {
	Autolock _l(this);
	m_flags = flags;
}

//// has this reference been released?
//bool NodeRef::isReleased() const {
//	// no lock necessary for bool access -- right?  +++++
//	return m_released;
//}
//

// [e.moon 29sep99]
// access addon-hint info
// - returns B_BAD_VALUE if not an add-on node created by this NodeManager

status_t NodeRef::getDormantNodeInfo(
	dormant_node_info*						outInfo) {
	
	if(!m_addonHint)
		return B_BAD_VALUE;
		
	*outInfo = m_addonHint->dormantInfo;
	return B_OK;
}

// [e.moon 29sep99]
// access file being played
// - returns B_BAD_VALUE if not an add-on node created by this NodeManager,
//   or if the node has no associated file

status_t NodeRef::getFile(
	entry_ref*										outFile) {

	Autolock _l(this);
		
	if(!m_addonHint || !m_addonHint->file)
		return B_BAD_VALUE;
		
	*outFile = *(m_addonHint->file);
	return B_OK;
}

// [e.moon 8dec99]
// set file to play

status_t NodeRef::setFile(
	const entry_ref&						file,
	bigtime_t*									outDuration) {

	Autolock _l(this);

	bigtime_t dur;
	status_t err = m_manager->roster->SetRefFor(
		node(),
		file,
		false,
		&dur);
	if(err == B_OK) {
		if(outDuration)
			*outDuration = dur;
		if(m_addonHint)
			m_addonHint->setFile(&file);
	} 
	
	return err;
}

// [e.moon 23oct99]
// returns true if the media_node has been released (call releaseNode() to
// make this happen.)

bool NodeRef::isNodeReleased() const {
	return m_nodeReleased;
}

// -------------------------------------------------------- //
// *** run-mode operations
// -------------------------------------------------------- //

void NodeRef::setRunMode(
	uint32											runMode,
	bigtime_t										delay) {
	Autolock _l(this);
	
	ASSERT(runMode <= BMediaNode::B_RECORDING);
	m_runMode = runMode;
	if(runMode == BMediaNode::B_RECORDING)
		m_recordingDelay = delay;
		
	// send notification to all observers	
	BMessage m(M_RUN_MODE_CHANGED);
	m.AddInt32("nodeID", id());
	m.AddInt32("runMode", runMode);
	
	if(runMode == BMediaNode::B_RECORDING && m_recordingDelay != 0)
		m.AddInt64("delay", m_recordingDelay);
	
	notify(&m);

	// update real run mode
	if(m_group)
		_setRunMode(m_group->runMode(), m_recordingDelay);
}

uint32 NodeRef::runMode() const {
	Autolock _l(this);
	return m_runMode;
}

bigtime_t NodeRef::recordingDelay() const {
	Autolock _l(this);
	return m_recordingDelay;
}

// calculates the minimum amount of delay needed for
// B_RECORDING mode
// +++++ 15sep99: returns biggest_output_buffer_duration * 2
// +++++ 28sep99: adds downstream latency

bigtime_t NodeRef::calculateRecordingModeDelay() {
	PRINT((
		"NodeRef::calculateRecordingModeDelay()\n"));
	status_t err;
				
	bigtime_t maxBufferDur = 0LL;
	
	vector<Connection> outputConnections;
	err = getOutputConnections(outputConnections);
	for(
		vector<Connection>::iterator it = outputConnections.begin();
		it != outputConnections.end(); ++it) {
		
		bigtime_t bufferDur = buffer_duration((*it).format().u.raw_audio);
		if(bufferDur > maxBufferDur)
			maxBufferDur = bufferDur;
	}

	bigtime_t latency = 0LL;
	err = m_manager->roster->GetLatencyFor(
		node(), &latency);

	PRINT((
		"  %Ld\n", latency));
	
	return latency; // +++++ stab in the dark 28sep99
//	return maxBufferDur + latency;
}

// -------------------------------------------------------- //
// *** connection access
// -------------------------------------------------------- //

// connection access: vector versions
	
status_t NodeRef::getInputConnections(
	vector<Connection>&					ioConnections,
	media_type									filterType) const {
	Autolock _l(this);

	NodeManager::con_map::iterator it, itEnd;
	it = m_manager->m_conDestinationMap.lower_bound(m_info.node.node);
	itEnd = m_manager->m_conDestinationMap.upper_bound(m_info.node.node);

	for(; it != itEnd; ++it) {
		if(filterType == B_MEDIA_UNKNOWN_TYPE ||
			(*it).second->format().type == filterType) {

			ioConnections.push_back(*((*it).second));
		}
	}

	return B_OK;
}


status_t NodeRef::getOutputConnections(
	vector<Connection>&					ioConnections,
	media_type									filterType) const {
	Autolock _l(this);

	NodeManager::con_map::iterator it, itEnd;
	it = m_manager->m_conSourceMap.lower_bound(m_info.node.node);
	itEnd = m_manager->m_conSourceMap.upper_bound(m_info.node.node);

	for(; it != itEnd; ++it) {
		if(filterType == B_MEDIA_UNKNOWN_TYPE ||
			(*it).second->format().type == filterType) {

			ioConnections.push_back(*((*it).second));
		}
	}

	return B_OK;
}
	
// connection access: flat array versions
	
status_t NodeRef::getInputConnections(
	Connection*									outConnections,
	int32												maxConnections,
	int32*											outNumConnections,
	media_type									filterType) const {
	Autolock _l(this);

	NodeManager::con_map::iterator it, itEnd;
	it = m_manager->m_conDestinationMap.lower_bound(m_info.node.node);
	itEnd = m_manager->m_conDestinationMap.upper_bound(m_info.node.node);

	int32 count = 0;

	for(; it != itEnd && count < maxConnections; ++it) {
		if(filterType == B_MEDIA_UNKNOWN_TYPE ||
			(*it).second->format().type == filterType) {

			outConnections[count++] = *((*it).second);
		}
	}

	*outNumConnections = count;

	return B_OK;
}

status_t NodeRef::getOutputConnections(
	Connection*									outConnections,
	int32												maxConnections,
	int32*											outNumConnections,
	media_type									filterType) const {
	Autolock _l(this);

	NodeManager::con_map::iterator it, itEnd;
	it = m_manager->m_conSourceMap.lower_bound(m_info.node.node);
	itEnd = m_manager->m_conSourceMap.upper_bound(m_info.node.node);

	int32 count = 0;

	for(; it != itEnd && count < maxConnections; ++it) {
		if(filterType == B_MEDIA_UNKNOWN_TYPE ||
			(*it).second->format().type == filterType) {

			outConnections[count++] = *((*it).second);
		}
	}

	*outNumConnections = count;

	return B_OK;
}

// -------------------------------------------------------- //
// *** position reporting/listening
// -------------------------------------------------------- //

bool NodeRef::positionReportsEnabled() const {
	return m_positionReportsEnabled;
}

// start thread if necessary
void NodeRef::enablePositionReports() {
	Autolock _l(this);
	
	if(m_flags & NO_POSITION_REPORTING)
		return;
		
	if(m_positionReportsEnabled)
		return;
		
	m_positionReportsEnabled = true;
	if(m_running) {
		// start the thread
		_startPositionThread();
	}
}

// stop thread if necessary
void NodeRef::disablePositionReports() {
	Autolock _l(this);
	
	if(!m_positionReportsEnabled)
		return;
		
	m_positionReportsEnabled = false;
	if(m_running) {
		// shut down the thread
		_stopPositionThread();
	}
}

// Fetch the approximate current position:
//   Returns the last reported position, and the
//   performance time at which that position was reached.  If the
//   transport has never been started, the start position and
//   a performance time of 0 will be returned.  If position updating
//   isn't currently enabled, returns B_NOT_ALLOWED.
	
status_t NodeRef::getLastPosition(
	bigtime_t*									outPosition,
	bigtime_t*									outPerfTime) const {

	Autolock _l(this);
	
	if(!m_positionReportsEnabled)
		return B_NOT_ALLOWED;

	*outPosition = m_lastPosition;
	*outPerfTime = m_tpLastPositionUpdate;
	return B_OK;
}

// Subscribe to regular position reports:
//   Position reporting isn't rolled into the regular IObservable
//   interface because a large number of messages are generated
//   (the frequency can be changed; see below.)
	
status_t NodeRef::addPositionObserver(
	BHandler*										handler) {
	ASSERT(handler);

	// try to create messenger
	status_t error;
	BMessenger m(handler, NULL, &error);
	if(error < B_OK) {
		PRINT((
			"* NodeRef::addPositionListener(): BMessenger() failed:\n"
			"  %s\n"
			"  handler %p\n",
			strerror(error), handler));
		return error;
	}

	// add to the invoker
	Autolock _l(this);
	m_positionInvoker.AddTarget(handler);
	
	// enable position updates:
	if(!m_positionReportsEnabled)
		enablePositionReports();
	
	return B_OK;	
}
		
status_t NodeRef::removePositionObserver(
	BHandler*										handler) {
	ASSERT(handler);

	Autolock _l(this);

	// look for listener
	int32 index = m_positionInvoker.IndexOfTarget(handler);
	if(index == -1)
		return B_ERROR;

	// remove it
	m_positionInvoker.RemoveTarget(index);
	
	// last observer removed?  kill thread. [e.moon 12oct99]
	if(m_positionReportsEnabled && !m_positionInvoker.CountTargets())
		disablePositionReports();

	return B_OK;	
}
	
// Set how often position updates will be sent:
//   Realistically, period should be > 10000 or so.
	
status_t NodeRef::setPositionUpdatePeriod(
	bigtime_t										period) {
	
	Autolock _l(this);
	if(period < 1000LL)
		return B_BAD_VALUE;
	m_positionUpdatePeriod = period;
	return B_OK;
}
		
bigtime_t NodeRef::positionUpdatePeriod() const{
	Autolock _l(this);
	return m_positionUpdatePeriod;
}

// -------------------------------------------------------- //
// *** BMediaRoster wrappers & convenience methods
// -------------------------------------------------------- //

// release the media node
// (if allowed, will trigger the release/deletion of this object)
status_t NodeRef::releaseNode() {

	D_METHOD((
		"NodeRef[%s]::releaseNode()\n", name()));
	status_t err;

	Autolock _l(m_manager);

	if(isReleased() || m_nodeReleased)
		return B_NOT_ALLOWED;
	
	if(m_group)
		m_group->removeNode(this);

	// kill off sync thread
	if(m_positionThread) {
		delete m_positionThread;
		m_positionThread = 0;
	}

	if(m_implFlags & _INTERNAL) {
		
		// tear down all connections if the node was created by
		// NodeManager
		vector<Connection> c_set;
		getInputConnections(c_set);
		getOutputConnections(c_set);

// [e.moon 13oct99] making PPC compiler happy
//		for_each(
//			c_set.begin(),
//			c_set.end(),
//			bound_method(
//				*m_manager,
//				&NodeManager::disconnect
//			)
//		);

		for(vector<Connection>::iterator it = c_set.begin();
			it != c_set.end(); ++it) {
			err = m_manager->disconnect(*it);
			if(err < B_OK) {
				PRINT((
					"! NodeRef('%s')::releaseNode():\n"
					"  NodeManager::disconnect('%s'->'%s') failed:\n"
					"  %s\n",
					name(),
					(*it).outputName(), (*it).inputName(),
					strerror(err)));
			}
		}
		
		// +++++ ensure that the connections were really broken?
	}

	err = B_OK;
	if(!(m_implFlags & _NO_RELEASE)) {
			
//		PRINT((
//			"### releasing node %ld\n",
//			id()));
//
		// free the node
		D_ROSTER(("# roster->ReleaseNode(%ld)\n", m_info.node.node));
		err = BMediaRoster::Roster()->ReleaseNode(
			m_info.node);	
		
		if(err < B_OK) {
			PRINT((
				"!!! ReleaseNode(%ld) failed:\n"
				"    %s\n",
				m_info.node.node,
				strerror(err)));
		}

		if(
			(m_implFlags & _INTERNAL) &&
			m_manager->m_useAddOnHost) {

			// ask add-on host to release the node
			err = AddOnHost::ReleaseInternalNode(m_info);
			if(err < B_OK) {
				PRINT((
					"!!! AddOnHost::ReleaseInternalNode(%ld) failed:\n"
					"    %s\n",
					m_info.node.node,
					strerror(err)));
			}
		}
	}
	else {
//		PRINT(("- not releasing node\n"));
	}

	m_nodeReleased = true;
	return err;
}


// calculate total (internal + downstream) latency for this node
	
status_t NodeRef::totalLatency(
	bigtime_t*									outLatency) const {

	return BMediaRoster::Roster()->GetLatencyFor(
		m_info.node,
		outLatency);
}

// retrieve input/output matching the given destination/source.
// returns B_MEDIA_BAD_[SOURCE | DESTINATION] if the destination
// or source don't correspond to this node.

class match_input_destination { public:
	const media_destination& dest;
	match_input_destination(const media_destination& _dest) : dest(_dest) {}
	bool operator()(const media_input& input) const {
		return input.destination == dest;
	}
};

class match_output_source { public:
	const media_source& source;
	match_output_source(const media_source& _source) : source(_source) {}
	bool operator()(const media_output& output) const {
		return output.source == source;
	}
};

status_t NodeRef::findInput(
	const media_destination&			forDestination,
	media_input*									outInput) const {

	status_t err;
	
	vector<media_input> inputs;
	vector<media_input>::const_iterator it;
	inputs.reserve(32);
	
	// check free inputs
	err = getFreeInputs(inputs);
	if(err < B_OK)	
		return err;
		
	it = find_if(
		inputs.begin(), inputs.end(),
		match_input_destination(forDestination));
		
	if(it != inputs.end()) {
		*outInput = *it;
		return B_OK;
	}
	
	// check connected inputs
	inputs.clear();
	err = getConnectedInputs(inputs);
	if(err < B_OK)	
		return err;
		
	it = find_if(
		inputs.begin(), inputs.end(),
		match_input_destination(forDestination));
		
	if(it != inputs.end()) {
		*outInput = *it;
		return B_OK;
	}
	return B_MEDIA_BAD_DESTINATION;
}
		
status_t NodeRef::findOutput(
	const media_source&					forSource,
	media_output*								outOutput) const {

	status_t err;
	
	vector<media_output> outputs;
	vector<media_output>::const_iterator it;
	outputs.reserve(32);
	
	// check free outputs
	err = getFreeOutputs(outputs);
	if(err < B_OK)	
		return err;
		
	it = find_if(
		outputs.begin(), outputs.end(),
		match_output_source(forSource));
		
	if(it != outputs.end()) {
		*outOutput = *it;
		return B_OK;
	}
	
	// check connected outputs
	outputs.clear();
	err = getConnectedOutputs(outputs);
	if(err < B_OK)	
		return err;
		
	it = find_if(
		outputs.begin(), outputs.end(),
		match_output_source(forSource));
		
	if(it != outputs.end()) {
		*outOutput = *it;
		return B_OK;
	}

	return B_MEDIA_BAD_SOURCE;
}


// endpoint matching (given name and/or format as 'hints')

template <class T>
class match_endpoint_name_format : public unary_function<T, bool> {
public:
	const char* name;
	const media_format* format;

	match_endpoint_name_format(const char* _name, const media_format* _format) :
		name(_name), format(_format) {}
	bool operator()(const T& endpoint) const {
		// test name, if given
		if(name && strcmp(endpoint.name, name) != 0)
			return false;
		// test format, if given
		media_format* f1 = const_cast<media_format*>(format);
		media_format* f2 = const_cast<media_format*>(&endpoint.format);
		if(format && !f1->Matches(f2))
			return false;
		return true;
	}
};

template <class T>
class match_endpoint_name_type : public unary_function<T, bool> {
public:
	const char* name;
	media_type type;

	match_endpoint_name_type(const char* _name, media_type _type) :
		name(_name), type(_type) {}
	bool operator()(const T& endpoint) const {
		// test name, if given
		if(name && strcmp(endpoint.name, name) != 0)
			return false;
		// test type, if given
		if(type != B_MEDIA_UNKNOWN_TYPE &&
			type != endpoint.format.type)
			return false;

		return true;
	}
};

template <class T>
class match_endpoint_type : public unary_function<T, bool> {
public:
	media_type type;

	match_endpoint_type(media_type _type) :
		type(_type) {}
	bool operator()(const T& endpoint) const {
		// test type, if given
		if(type != B_MEDIA_UNKNOWN_TYPE &&
			type != endpoint.format.type)
			return false;

		return true;
	}
};

status_t NodeRef::findFreeInput(
	media_input*								outInput,
	const media_format*					format /*=0*/,
	const char*									name /*=0*/) const {

	status_t err;
	
	vector<media_input> inputs;
	vector<media_input>::const_iterator it;
	inputs.reserve(32);
	
	err = getFreeInputs(inputs);
	if(err < B_OK)	
		return err;
		
	it = find_if(
		inputs.begin(),
		inputs.end(),
		match_endpoint_name_format<media_input>(name, format));
		
	if(it != inputs.end()) {
		*outInput = *it;
		return B_OK;
	}
	return B_ERROR;
}

status_t NodeRef::findFreeInput(
	media_input*								outInput,
	media_type									type /*=B_MEDIA_UNKNOWN_TYPE*/,
	const char*										name /*=0*/) const {

	status_t err;
	
	vector<media_input> inputs;
	vector<media_input>::const_iterator it;
	inputs.reserve(32);
	
	err = getFreeInputs(inputs);
	if(err < B_OK)	
		return err;
		
	it = find_if(
		inputs.begin(),
		inputs.end(),
		match_endpoint_name_type<media_input>(name, type));
	if(it != inputs.end()) {
		*outInput = *it;
		return B_OK;
	}
	return B_ERROR;
}

status_t NodeRef::findFreeOutput(
	media_output*								outOutput,
	const media_format*					format /*=0*/,
	const char*										name /*=0*/) const {

	status_t err;
	
	vector<media_output> outputs;
	vector<media_output>::const_iterator it;
	outputs.reserve(32);
	
	err = getFreeOutputs(outputs);
	if(err < B_OK)	
		return err;
		
	it = find_if(
		outputs.begin(),
		outputs.end(),
		match_endpoint_name_format<media_output>(name, format));
	if(it != outputs.end()) {
		*outOutput = *it;
		return B_OK;
	}
	return B_ERROR;
}

status_t NodeRef::findFreeOutput(
	media_output*								outOutput,
	media_type									type /*=B_MEDIA_UNKNOWN_TYPE*/,
	const char*										name /*=0*/) const {

	status_t err;
	
	vector<media_output> outputs;
	vector<media_output>::const_iterator it;
	outputs.reserve(32);
	
	err = getFreeOutputs(outputs);
	if(err < B_OK)	
		return err;
		
	it = find_if(
		outputs.begin(),
		outputs.end(),
		match_endpoint_name_type<media_output>(name, type));
	if(it != outputs.end()) {
		*outOutput = *it;
		return B_OK;
	}
	return B_ERROR;
}


// node endpoint access: vector versions (wrappers for BMediaRoster
// calls.)
	
status_t NodeRef::getFreeInputs(
	vector<media_input>&		ioInputs,
	media_type							filterType) const {
	
	BMediaRoster* r = BMediaRoster::Roster();
	status_t err;

	int32 count;
	int32 bufferInc = 16;
	int32 inputBufferSize = 16;
	media_input* inputBuffer = new media_input[inputBufferSize];

	while(true) {
		err = r->GetFreeInputsFor(
			m_info.node, inputBuffer, inputBufferSize, &count, filterType);
		if(err < B_OK)
			return err;
			
		if(count == inputBufferSize) {
			// buffer too small; increase & try again
			inputBufferSize += bufferInc;
			delete [] inputBuffer;
			inputBuffer = new media_input[inputBufferSize];
			continue;
		}
		
		if(count)
			// copy found inputs into vector
			copy(inputBuffer, inputBuffer + count,
				back_inserter(ioInputs));
			
		break;
	}

	// fix missing node info
	_fixInputs(ioInputs);

	delete [] inputBuffer;
	return B_OK;
}

// +++++ broken?
status_t NodeRef::getConnectedInputs(
	vector<media_input>&		ioInputs,
	media_type							filterType) const {
	
	BMediaRoster* r = BMediaRoster::Roster();
	status_t err;

	int32 count;
	int32 bufferInc = 16;
	int32 inputBufferSize = 16;
	media_input* inputBuffer = new media_input[inputBufferSize];

	while(true) {
		err = r->GetConnectedInputsFor(
			m_info.node, inputBuffer, inputBufferSize, &count);
		if(err < B_OK)
			return err;
			
		if(count == inputBufferSize) {
			// buffer too small; increase & try again
			inputBufferSize += bufferInc;
			delete [] inputBuffer;
			inputBuffer = new media_input[inputBufferSize];
			continue;
		}
		
		if(count)
			// copy found inputs matching the given type into vector
			remove_copy_if(inputBuffer, inputBuffer + count,
				back_inserter(ioInputs),
				not1(match_endpoint_type<media_input>(filterType)));
			
		break;
	}

	// fix missing node info
	_fixInputs(ioInputs);

	delete [] inputBuffer;
	return B_OK;
}

status_t NodeRef::getFreeOutputs(
	vector<media_output>&		ioOutputs,
	media_type							filterType) const {
	
	BMediaRoster* r = BMediaRoster::Roster();
	status_t err;

	int32 count;
	int32 bufferInc = 16;
	int32 outputBufferSize = 16;
	media_output* outputBuffer = new media_output[outputBufferSize];

	while(true) {
		err = r->GetFreeOutputsFor(
			m_info.node, outputBuffer, outputBufferSize, &count, filterType);
		if(err < B_OK)
			return err;
			
		if(count == outputBufferSize) {
			// buffer too small; increase & try again
			outputBufferSize += bufferInc;
			delete [] outputBuffer;
			outputBuffer = new media_output[outputBufferSize];
			continue;
		}
		
		if(count)
			// copy found outputs into vector
			copy(outputBuffer, outputBuffer + count,
				back_inserter(ioOutputs));
			
		break;
	}

	// fix missing node info
	_fixOutputs(ioOutputs);

	delete [] outputBuffer;
	return B_OK;
}

status_t NodeRef::getConnectedOutputs(
	vector<media_output>&		ioOutputs,
	media_type							filterType) const {
	
	BMediaRoster* r = BMediaRoster::Roster();
	status_t err;

	int32 count;
	int32 bufferInc = 16;
	int32 outputBufferSize = 16;
	media_output* outputBuffer = new media_output[outputBufferSize];

	while(true) {
		err = r->GetConnectedOutputsFor(
			m_info.node, outputBuffer, outputBufferSize, &count);
		if(err < B_OK)
			return err;
			
		if(count == outputBufferSize) {
			// buffer too small; increase & try again
			outputBufferSize += bufferInc;
			delete [] outputBuffer;
			outputBuffer = new media_output[outputBufferSize];
			continue;
		}
		
		if(count)
			// copy found outputs matching the given type into vector
			remove_copy_if(outputBuffer, outputBuffer + count,
				back_inserter(ioOutputs),
				not1(match_endpoint_type<media_output>(filterType)));
			
		break;
	}

	// fix missing node info
	_fixOutputs(ioOutputs);

	delete [] outputBuffer;
	return B_OK;
}
		
		
// node endpoint access: array versions (wrappers for BMediaRoster
// calls.)
	
status_t NodeRef::getFreeInputs(
	media_input*								outInputs,
	int32												maxInputs,
	int32*											outNumInputs,
	media_type									filterType) const {
	
	status_t err = BMediaRoster::Roster()->GetFreeInputsFor(
		m_info.node, outInputs, maxInputs, outNumInputs, filterType);
		
	if(err < B_OK)
		return err;
		
	// fix missing node info
	_fixInputs(outInputs, *outNumInputs);
	return err;
}

	
status_t NodeRef::getConnectedInputs(
	media_input*								outInputs,
	int32												maxInputs,
	int32*											outNumInputs) const {
	
	status_t err = BMediaRoster::Roster()->GetConnectedInputsFor(
		m_info.node, outInputs, maxInputs, outNumInputs);
		
	if(err < B_OK)
		return err;
		
	// fix missing node info
	_fixInputs(outInputs, *outNumInputs);
	return err;
}
	
status_t NodeRef::getFreeOutputs(
	media_output*								outOutputs,
	int32												maxOutputs,
	int32*											outNumOutputs,
	media_type									filterType) const {
	
	status_t err = BMediaRoster::Roster()->GetFreeOutputsFor(
		m_info.node, outOutputs, maxOutputs, outNumOutputs, filterType);
		
	if(err < B_OK)
		return err;
		
	// fix missing node info
	_fixOutputs(outOutputs, *outNumOutputs);
	return err;
}
	
status_t NodeRef::getConnectedOutputs(
	media_output*								outOutputs,
	int32												maxOutputs,
	int32*											outNumOutputs) const {
	
	status_t err = BMediaRoster::Roster()->GetConnectedOutputsFor(
		m_info.node, outOutputs, maxOutputs, outNumOutputs);
		
	if(err < B_OK)
		return err;
		
	// fix missing node info
	_fixOutputs(outOutputs, *outNumOutputs);
	return err;
}
	

// -------------------------------------------------------- //
// *** IPersistent
// -------------------------------------------------------- //

// !
#if CORTEX_XML
// !

// +++++

// !
#endif /*CORTEX_XML*/
// !

// -------------------------------------------------------- //
// *** BHandler:
// -------------------------------------------------------- //

void NodeRef::MessageReceived(
	BMessage*								message) {

	D_MESSAGE((
		"NodeRef['%s']::MessageReceived(): %c%c%c%c\n",
		name(),
		 message->what >> 24,
		(message->what >> 16)	& 0xff,
		(message->what >> 8)	& 0xff,
		(message->what) 			& 0xff));
	status_t err;
		
	switch(message->what) {
		case M_SET_RUN_MODE:
			{
				// set run mode & delay (if given)
				int32 runMode;
				bigtime_t delay = 0LL;
				err = message->FindInt32("runMode", &runMode);
				if(err < B_OK) {
					PRINT((
						"! NodeRef::MessageReceived(M_SET_RUN_MODE): no value found.\n"));
					break;
				}
				if(runMode == BMediaNode::B_RECORDING)
					message->FindInt64("delay", &delay); // optional
				
				setRunMode(runMode, delay);
			}
			break;
			

		case M_PREROLL:
			// +++++
			break;

		case M_SET_CYCLING:
			{
				bool cycling;
				err = message->FindBool("cycling", &cycling);
				if(err < B_OK) {
					int32 val;
					err = message->FindInt32("be:value", &val);
					if(err < B_OK) {
						PRINT((
							"! NodeRef::MessageReceived(M_SET_CYCLING): no value found.\n"));
						break;
					}
					cycling = val;
				}
				
				setCycling(cycling);
			}
			break;
		
		case B_MEDIA_NODE_STOPPED:
//			PRINT(("### B_MEDIA_NODE_STOPPED\n"));
//			
			// if still marked running, let the group know [e.moon 11oct99]
			if(m_running) {
				m_running = false;
				m_stopQueued = false;

				if(m_group) {
					Autolock _l(m_group);
					m_group->_refStopped(this);
				}
			}

			break;
			
		case NodeSyncThread::M_SYNC_COMPLETE: {
			// [e.moon 14oct99] position-report messages are now sent
			// by the NodeSyncThread.
			
			Autolock _l(this);
			
			// unpack message
			bigtime_t when, position;
			err = message->FindInt64("perfTime", &when);
			ASSERT(err == B_OK);
			err = message->FindInt64("position", &position);
			ASSERT(err == B_OK);

			_handlePositionUpdate(when, position);
			break;
		}
			
		default:
			_inherited::MessageReceived(message);
	}
}

// -------------------------------------------------------- //
// *** IObservable:		[20aug99]
// -------------------------------------------------------- //

void NodeRef::observerAdded(
	const BMessenger&				observer) {

	BMessage m(M_OBSERVER_ADDED);
	m.AddInt32("nodeID", id());
	m.AddMessenger("target", BMessenger(this));
	observer.SendMessage(&m);
}
		
void NodeRef::observerRemoved(
	const BMessenger&				observer) {

	BMessage m(M_OBSERVER_REMOVED);
	m.AddInt32("nodeID", id());
	m.AddMessenger("target", BMessenger(this));
	observer.SendMessage(&m);
}

void NodeRef::notifyRelease() {

	BMessage m(M_RELEASED);
	m.AddInt32("nodeID", id());
	m.AddMessenger("target", BMessenger(this));
	notify(&m);
}

void NodeRef::releaseComplete() {
	// +++++
}

// -------------------------------------------------------- //
// *** ILockable: pass lock requests to parent group,
//                or manager if no group found
// -------------------------------------------------------- //

// this is hideous. [24aug99]
// it must die soon.

// The two-stage lock appears safe UNLESS it's multiply acquired
// (and the NodeManager is locked somewhere in between.)  Then
// it's deadlocks all around...

// safe two-stage lock (only WRITE locking is supported)
// Notes:
// a) a NodeRef either belongs to a group (m_group != 0) or
//    is free.  If the ref is free, the NodeManager is the
//    target for locking; otherwise the group is the 'lockee'.
// b) operations which affect a NodeRef's group affiliation
//    (ie. adding or removing a node to/from a group) must
//    lock first the NodeManager, then the NodeGroup.  The
//    locks should be released in the opposite order.

bool NodeRef::lock(
	lock_t type,
	bigtime_t timeout) {
		
	D_LOCK(("*** NodeRef::lock(): %ld\n", find_thread(0)));
	
	ASSERT(type == WRITE);
	ASSERT(m_manager);

	// lock manager
	if(!m_manager->lock(type, timeout))
		return false;
	
	// transfer lock to group, if any	
	NodeGroup* group = m_group;
	if(!group)
		return true;
		
	bool ret = m_group->lock(type, timeout);

	m_manager->unlock();

	D_LOCK(("*** NodeRef::lock() ACQUIRED: %ld\n", find_thread(0)));
	
	return ret;
}
	
bool NodeRef::unlock(
		lock_t type) {

	D_LOCK(("*** NodeRef::unlock(): %ld\n", find_thread(0)));
	
	ASSERT(type == WRITE);
	ASSERT(m_manager);

	NodeGroup* group = m_group;
	if(group) {
		bool ret = m_group->unlock(type);
		D_LOCK(("*** NodeRef::unlock() RELEASED: %ld\n", find_thread(0)));
		return ret;
	}
	
	bool ret = m_manager->unlock(type);
	
	D_LOCK(("*** NodeRef::unlock() RELEASED: %ld\n", find_thread(0)));
	return ret;
}
	
bool NodeRef::isLocked(
	lock_t type) const {

	ASSERT(type == WRITE);
	ASSERT(m_manager);

	NodeGroup* group = m_group;
	if(group)
		return m_group->isLocked(type);

	return m_manager->isLocked(type);
}
		
// -------------------------------------------------------- //
// *** ctor
// -------------------------------------------------------- //

NodeRef::NodeRef(
	const media_node&		node,
	NodeManager*				manager,
	uint32							userFlags,
	uint32							implFlags) :
	
	m_manager(manager),
	m_group(0),
	m_flags(userFlags),
	m_implFlags(implFlags),
	m_runMode(0),
	m_recordingDelay(0LL),
	m_watching(false),
	m_addonHint(0),
	m_positionReportsEnabled(false),
	m_positionReportsStarted(false),
	m_positionUpdatePeriod(s_defaultPositionUpdatePeriod),
	m_tpLastPositionUpdate(0LL),
	m_lastPosition(0LL),
	m_positionThread(0),
	m_running(false),
	m_nodeReleased(false),
	m_cycle(false),
	m_prerolled(false),
//	m_cycleSyncThread(0),
	m_stopQueued(false),
	m_latency(0LL) {

	ASSERT(manager);

	if(!m_manager->Lock()) {
		ASSERT(!"m_manager->Lock() failed");
	}
	m_manager->AddHandler(this);
	m_manager->Unlock();

	// fetch node details
	BMediaRoster* r = BMediaRoster::Roster();
	status_t err = r->GetLiveNodeInfo(
		node,
		&m_info);

	if(err < B_OK) {
		PRINT((
			"!!! NodeRef(): BMediaRoster::GetLiveNodeInfo(%ld) failed:\n"
			"    %s\n",
			node.node,
			strerror(err)));
		// at least store node info
		m_info.node = node;
	}

	// name self after node
	SetName(m_info.name);

	// init Media Roster connection [e.moon 11oct99]
	if(!(m_flags & NO_ROSTER_WATCH)) {
		r->StartWatching(
			BMessenger(this),
			m_info.node,
			B_MEDIA_NODE_STOPPED);
		m_watching = true;
	}
}
// -------------------------------------------------------- //
// *** endpoint-fixing operations (no lock required)
// -------------------------------------------------------- //

template <class T>
class fixEndpointFn : public unary_function<T&, void> {
	const media_node&		node;
public:
	fixEndpointFn(const media_node& _n) : node(_n) {}
	void operator()(T& endpoint) {
//		PRINT((
//			"fixEndpointFn(): endpoint '%s', node ID %ld\n",
//				endpoint.name, endpoint.node.node));
		if(endpoint.node != node) {
			PRINT((
				"  fixing '%s'\n", endpoint.name));
			endpoint.node = node;
		}
	}
};

// 'fix' (fill in node if needed) sets of inputs/outputs
void NodeRef::_fixInputs(
	media_input*									inputs,
	int32													count) const {

	D_METHOD((
		"NodeRef[%s]::fixInputs()\n", m_info.name));

	for_each(
		inputs,
		inputs+count,
		fixEndpointFn<media_input>(node()));
}

void NodeRef::_fixInputs(
	vector<media_input>&			inputs) const {

	D_METHOD((
		"NodeRef[%s]::fixInputs()\n", m_info.name));
		
	for_each(
		inputs.begin(),
		inputs.end(),
		fixEndpointFn<media_input>(node()));
}

void NodeRef::_fixOutputs(
	media_output*									outputs,
	int32													count) const {

	D_METHOD((
		"NodeRef[%s]::fixOutputs()\n", m_info.name));

	for_each(
		outputs,
		outputs+count,
		fixEndpointFn<media_output>(node()));
}

void NodeRef::_fixOutputs(
	vector<media_output>&		outputs) const {

	D_METHOD((
		"NodeRef[%s]::fixOutputs()\n", m_info.name));
		
	for_each(
		outputs.begin(),
		outputs.end(),
		fixEndpointFn<media_output>(node()));
}

// -------------------------------------------------------- //
// *** internal/NodeManager operations (LOCK REQUIRED)
// -------------------------------------------------------- //

// call after instantiation to register the dormant_node_info
// used to select this add-on node

void NodeRef::_setAddonHint(
	const dormant_node_info*				info,
	const entry_ref*								file) {

	assert_locked(this);
	
	if(m_addonHint)
		delete m_addonHint;

	m_addonHint = new addon_hint(info, file);
}

// call to set a new group; if 0, the node must have no
// connections
void NodeRef::_setGroup(
	NodeGroup*										group) {
	assert_locked(this);
	
	m_group = group;
	
	if(!LockLooper()) {
		ASSERT(!"LockLooper() failed.");
	}
	BMessage m(M_GROUP_CHANGED);
	m.AddInt32("nodeID", (int32)m_info.node.node);
	m.AddInt32("groupID", m_group ? (int32)m_group->id() : 0);
	notify(&m);
	UnlockLooper();
}

// *** NodeGroup API ***
//     9aug99: moved from NodeGroup
		
// initialize the given node's transport-state members
// (this may be called from the transport thread or from
//  an API-implementation method.)
	
status_t NodeRef::_initTransportState() {
	assert_locked(this);

	D_METHOD((
		"NodeRef('%s')::_initTransportState()\n",
		name()));
	
	// init transport state	for this node
	m_prerolled = false;	
	m_tpStart = 0LL;
	m_tpLastSeek = 0LL;
	m_lastSeekPos = 0LL;
	
	// +++++ init position reporting stuff here?

	return B_OK;
}
		
status_t NodeRef::_setTimeSource(
	media_node_id					timeSourceID) {
	assert_locked(this);
	
	D_METHOD((
		"NodeRef('%s')::_setTimeSource(%ld)\n",
		name(), timeSourceID));
	status_t err;

	// set time source
	ASSERT(timeSourceID != media_node::null.node);
	D_ROSTER(("# roster->SetTimeSourceFor()\n"));
	err = m_manager->roster->SetTimeSourceFor(
		id(), timeSourceID);
		
	if(err < B_OK) {
		PRINT((
			"* NodeRef('%s')::_setTimeSource(%ld):\n"
			"  SetTimeSourceFor() failed: %s\n",
			name(), timeSourceID, strerror(err)));
	}
	
	return err;
}
		
status_t NodeRef::_setRunMode(
	const uint32					runMode,
	bigtime_t							delay) {
	assert_locked(this);

	D_METHOD((
		"NodeRef('%s')::_setRunMode(%ld : %Ld)\n",
		name(), runMode, delay));
	status_t err;

	
	BMediaNode::run_mode m = 
		// if group is in offline mode, so are all its nodes
		(runMode == BMediaNode::B_OFFLINE) ?
			(BMediaNode::run_mode)runMode :
			// if non-0, the node's setting is used
			(m_runMode > 0) ?
				(BMediaNode::run_mode)m_runMode :
				(BMediaNode::run_mode)runMode;
	ASSERT(m > 0);

	// +++++ additional producer run-mode delay support here?
	
	if(
		kind() & B_BUFFER_PRODUCER &&
		runMode == BMediaNode::B_RECORDING) {

		D_ROSTER(("# roster->SetProducerRunModeDelay()\n"));
		err = m_manager->roster->SetProducerRunModeDelay(
			node(), delay, m);
		if(err < B_OK) {
			PRINT((
				"NodeRef('%s')::_setRunMode(): SetProducerRunModeDelay(%Ld) failed: %s\n",
				name(), delay, strerror(err)));
		}
	} else {

		D_ROSTER(("# roster->SetRunModeNode()\n"));
		err = m_manager->roster->SetRunModeNode(
			node(), m);
		if(err < B_OK) {
			PRINT((
				"NodeRef('%s')::_setRunMode(): SetRunModeNode(%ld) failed: %s\n",
				name(), m, strerror(err)));
		}
	}
	
	return err;
}

status_t NodeRef::_setRunModeAuto(
	const uint32					runMode) {

	if(
		kind() && B_BUFFER_PRODUCER &&
		runMode == BMediaNode::B_RECORDING) {

		return _setRunMode(
			runMode,
			calculateRecordingModeDelay());

	} else
		return _setRunMode(runMode);
}

// seek and preroll the given node.
// *** this method should not be called from the transport thread
// (since preroll operations can block for a relatively long time.)
//
// returns B_NOT_ALLOWED if the node is running, or if its NO_PREROLL
// flag is set; otherwise, returns B_OK on success or a Media Roster
// error.
	
status_t NodeRef::_preroll(
	bigtime_t							position) {
	assert_locked(this);

	D_METHOD((
		"NodeRef('%s')::_preroll(%Ld)\n",
		name(), position));
	status_t err;
		
	// make sure the node can be and wants to be prerolled
	if(m_running ||
		m_flags & NO_PREROLL)
		return B_NOT_ALLOWED;
			
	if(!(m_flags & NO_SEEK)) {
		// seek the node first
		err = BMediaRoster::Roster()->SeekNode(
			node(),
			position,
			0LL);

		if(err < B_OK) {
			PRINT((
				"*** NodeRef('%s')::_preroll(%Ld): BMediaRoster::SeekNode():\n"
				"    %s\n",
				name(), position, strerror(err)));
			return err;
		}
	}
		
	// preroll the node (*** this blocks until the node's Preroll()
	//                       implementation returns ***)

	err = BMediaRoster::Roster()->PrerollNode(
		node());
	
	if(err < B_OK) {
		PRINT((
			"*** NodeRef('%s')::_preroll(%Ld): BMediaRoster::PrerollNode():\n"
			"    %s\n",
			name(), position, strerror(err)));
		return err;
	}
			
	m_prerolled = true;
	m_tpLastSeek = 0LL;
	m_lastSeekPos = position;
	
	return B_OK;
}
		
// seek the given node if possible
// (this may be called from the transport thread or from
//  an API-implementation method.)

status_t NodeRef::_seek(
	bigtime_t							position,
	bigtime_t							when) {
	assert_locked(this);
	
	D_METHOD((
		"NodeRef('%s')::_seek(to %Ld, at %Ld)\n",
		name(), position, when));
	
	if(m_flags & NO_SEEK)
		// the node should not be seek'd
		return B_OK;
	
	if(m_prerolled && m_lastSeekPos == position)
		// the node has already been advanced to the proper position
		return B_OK;

	// do it
	status_t err = BMediaRoster::Roster()->SeekNode(
		node(), position, when);			

	if(err < B_OK) {
		PRINT((
			"*** NodeRef('%s')::_seek(to %Ld, at %Ld): BMediaRoster::SeekNode():\n"
			"    %s\n",
			name(), position, when, strerror(err)));
		return err;
	}

	// update node state
	m_tpLastSeek = when;
	m_lastSeekPos = position;
	
	// node can't be considered prerolled after a seek
	m_prerolled = false;

	return B_OK;
}

// seek the given (stopped) node
// (this may be called from the transport thread or from
//  an API-implementation method.)

status_t NodeRef::_seekStopped(
	bigtime_t							position) {
	assert_locked(this);
	
	D_METHOD((
		"NodeRef('%s')::_seekStopped(to %Ld)\n",
		name(), position));	

	if(m_running)
		return B_NOT_ALLOWED;
		
	return _seek(position, 0LL);	
}

		
// start the given node, if possible & necessary, at
// the given time
// (this may be called from the transport thread or from
//  an API-implementation method.)
		
status_t NodeRef::_start(
	bigtime_t							when) {
	assert_locked(this);

	D_METHOD((
		"NodeRef('%s')::_start(at %Ld)\n",
		name(), when));
		
	if(isRunning()) {
		D_METHOD((
			"  * node already running; aborting\n"));
		return B_OK; // +++++ is this technically an error?
	}

	// +++++ is this proper?
	ASSERT(m_group);
	ASSERT(
		m_group->m_transportState == NodeGroup::TRANSPORT_RUNNING ||
		m_group->m_transportState == NodeGroup::TRANSPORT_STARTING);

	if(m_flags & NO_START_STOP) {
		D_METHOD((
			"  * NO_START_STOP; aborting\n"));
		return B_OK;
	}
		
	D_ROSTER(("# roster->StartNode(%ld)\n", id()));
	status_t err = BMediaRoster::Roster()->StartNode(
		node(),	when);
		
	if(err < B_OK) {
		PRINT((
			"  * StartNode(%ld) failed: '%s'\n",
			id(), strerror(err)));
		return err;
	}
		
	// update state
	m_running = true;
	m_tpStart = when;

	// fetch new node latency
	_updateLatency();
	
	// start position tracking thread if needed
	m_positionReportsStarted = false;
	if(m_positionReportsEnabled)
		_startPositionThread();
		
	return B_OK;
}
		
// stop the given node (which may or may not still be
// a member of this group.)
// (this may be called from the transport thread or from
//  an API-implementation method.)

status_t NodeRef::_stop() {
	assert_locked(this);

	D_METHOD((
		"NodeRef('%s')::_stop()\n",
		name()));
		
	if(!isRunning())
		return B_OK; // +++++ error?
	
	if(m_flags & NO_START_STOP || m_flags & NO_STOP)
		return B_OK;

	D_ROSTER(("# roster->StopNode(%ld)\n", id()));
	status_t err = BMediaRoster::Roster()->StopNode(
		node(), 0, true);

	if(err < B_OK)
		return err;
	
	// 9aug99: refuse further position notification
	_stopPositionThread();
	
	// clear node's state
	m_running = false;
	m_stopQueued = false; // asked for immediate stop [e.moon 11oct99]
	return _initTransportState();
}

// roll the given node, if possible
// (this may be called from the transport thread or from
//  an API-implementation method.)
status_t NodeRef::_roll(
	bigtime_t							start,
	bigtime_t							stop,
	bigtime_t							position) {
	assert_locked(this);

	D_METHOD((
		"NodeRef('%s')::_roll(%Ld to %Ld, from %Ld)\n",
		name(), start, stop, position));
	status_t err;

	// roll only if the node can be started & stopped,
	// AND if this NodeRef is watching the Media Roster.
	if(
		m_flags & NO_START_STOP ||
		m_flags & NO_STOP ||
		m_flags & NO_ROSTER_WATCH)
		return B_NOT_ALLOWED;

	if(isRunning())
		return B_NOT_ALLOWED;

	ASSERT(m_group);
	ASSERT(
		m_group->m_transportState == NodeGroup::TRANSPORT_RUNNING ||
		m_group->m_transportState == NodeGroup::TRANSPORT_STARTING);

	D_ROSTER(("# roster->RollNode(%ld)\n", id()));
	if(m_flags & NO_SEEK)
		err = BMediaRoster::Roster()->RollNode(
			node(),	start, stop);
	else
		err = BMediaRoster::Roster()->RollNode(
			node(),	start, stop, position);
		
	if(err < B_OK) {
		PRINT((
			"NodeRef('%s')::_roll(%Ld to %Ld, from %Ld)\n"
			"!!! BMediaRoster::RollNode(%ld) failed: '%s'\n",
			name(), start, stop, position, id(), strerror(err)));
		return err;
	}
		
	// update state
	m_running = true;
	m_stopQueued = true; // remember that node will stop on its own [e.moon 11oct99]
	m_tpStart = start;

	// fetch new node latency
	_updateLatency();
	
	// start position tracking thread if needed	
	m_positionReportsStarted = false;
	if(m_positionReportsEnabled)
		_startPositionThread();
		
	return B_OK;
}

// [28sep99 e.moon]
// refresh the node's current latency; if I reference
// a B_RECORDING node, update its 'producer delay'.

status_t NodeRef::_updateLatency() {
	assert_locked(this);

	// [11nov99 e.moon] don't bother if it's not a producer:
	if(!(kind() & B_BUFFER_PRODUCER)) {
		m_latency = 0LL;
		return B_OK;
	}

	bigtime_t latency;
	status_t err = BMediaRoster::Roster()->GetLatencyFor(
		node(),
		&latency);
	if(err < B_OK) {
		PRINT((
			"* NodeRef('%s')::_updateLatency(): GetLatencyFor() failed:\n"
			"  %s\n",
			name(), strerror(err)));

		return err;
	}

	// success
	m_latency = latency;
	
	// update run-mode & delay if necessary
	if(
		m_runMode == BMediaNode::B_RECORDING ||
		(m_runMode == 0 && m_group && m_group->runMode() == BMediaNode::B_RECORDING))
		_setRunModeAuto(BMediaNode::B_RECORDING);
	
	return B_OK;
}

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

status_t NodeRef::_calcStartTime(
	bigtime_t							startDelay,
	bigtime_t*						outTime,
	bigtime_t*						outPosition) {
	assert_locked(this);
	
	// +++++
		
	return B_ERROR;
}


// -------------------------------------------------------- //
// *** Position and cycle thread management *** (LOCK REQUIRED)
// -------------------------------------------------------- //
	
status_t NodeRef::_startPositionThread() {
	assert_locked(this);
	ASSERT(m_group);
	status_t err;
	
	if(!m_positionReportsEnabled)
		return B_NOT_ALLOWED;
	
	if(m_positionThread)
		_stopPositionThread();
	
	m_positionThread = new NodeSyncThread(
		m_info.node,
		new BMessenger(this));
	
	// send an initial position report if necessary
	if(!m_positionReportsStarted) {
		m_positionReportsStarted = true;

		err = _handlePositionUpdate(
			m_tpStart,
			m_lastSeekPos);
			
		if(err < B_OK) {
			PRINT((
				"* NodeRef::_startPositionThread(): _handlePositionUpdate() failed:\n"
				"  %s\\n",
				strerror(err)));
			return err;
		}	
	}
	else {
	
		// figure when the last jump in position occurred
		bigtime_t tpFrom = (m_tpLastSeek > m_tpStart) ? m_tpLastSeek : m_tpStart;

		// figure the corresponding position
		bigtime_t lastPosition = m_lastSeekPos;
		
		// figure the next time for a position report
		BTimeSource* ts = m_group->m_timeSourceObj;
			
		bigtime_t tpTarget = ts->Now() + m_positionUpdatePeriod;
		bigtime_t targetPosition = lastPosition + (tpTarget-tpFrom);
		
		err = _schedulePositionUpdate(
			tpTarget,
			targetPosition);

		if(err < B_OK) {
			PRINT((
				"* NodeRef::_createPositionThread(): _schedulePositionUpdate() failed:\n"
				"  %s\\n",
				strerror(err)));
			return err;
		}
	}
	
	return B_OK;
}

status_t NodeRef::_handlePositionUpdate(
	bigtime_t							perfTime,
	bigtime_t							position) {
	assert_locked(this);
	status_t err;

	if(!m_running) {
		PRINT((
			"* NodeRef::_handlePositionUpdate(): not running.\n"));
		return B_NOT_ALLOWED;
	}	

	if(!m_positionReportsEnabled) {
		PRINT((
			"* NodeRef::_handlePositionUpdate(): position reports disabled.\n"));
		return B_NOT_ALLOWED;
	}

	// store info
	m_tpLastPositionUpdate = perfTime;
	m_lastPosition = position;

	// relay notification to all 'position listeners'
	_notifyPosition(perfTime, position);

	// schedule next update
	err = _schedulePositionUpdate(	
		perfTime + m_positionUpdatePeriod,
		position + m_positionUpdatePeriod);

	if(err < B_OK) {
		PRINT((
			"* NodeRef::_handlePositionUpdate(): _schedulePositionUpdate() failed:\n"
			"  %s\n",
			strerror(err)));
	}
	return err;
}

status_t NodeRef::_schedulePositionUpdate(	
	bigtime_t							when,
	bigtime_t							position) {
	assert_locked(this);
	status_t err;
	
	if(!m_positionReportsEnabled)
		return B_NOT_ALLOWED;
	ASSERT(m_positionThread);

	if(m_cycle && m_group->_cycleValid()) {
		if(position >= m_group->endPosition()) {
			// snap to start of next cycle
			when = m_group->_cycleBoundary();
			position = m_group->startPosition();
		}
	}

////	position_sync_msg m = {
////		id(),
////		m_group->id(),
////		when,
////		position
////	};
//
////	PRINT((
////		"NodeRef::_schedulePositionUpdate():\n"
////		"  when     = %Ld\n"
////		"  position = %Ld\n",
////		when, position));
//
//	m_positionSyncThread->setPosition(position);
//
//	if(first)	
//		err = m_positionSyncThread->go(when);
//	else
//		err = m_positionSyncThread->reschedule(when);

	err = m_positionThread->sync(when, position, B_INFINITE_TIMEOUT);

	if(err < B_OK) {
		PRINT((
			"! NodeRef::_schedulePositionUpdate(): m_positionThread->sync() failed:\n"
			"  %s\n", strerror(err)));
	}
	return err;
}

status_t NodeRef::_stopPositionThread() {
	assert_locked(this);

	if(!m_positionThread)
		return B_NOT_ALLOWED;
		
	delete m_positionThread;
	m_positionThread = 0;
	
	return B_OK;
}
	
	
// Send a message to all position listeners
status_t NodeRef::_notifyPosition(
	bigtime_t							when,
	bigtime_t							position) {
	assert_locked(this);
	status_t err = B_OK;

	if(!m_positionReportsEnabled)
		return B_NOT_ALLOWED;

	BMessage message(M_POSITION);
	message.AddInt32("nodeID", id());
	message.AddInt64("when", when);
	message.AddInt64("position", position);
	
	m_positionInvoker.Invoke(&message);
	
	return err;
}

// END -- NodeRef.cpp --
