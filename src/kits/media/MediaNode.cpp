/*
 * Copyright (c) 2015, Dario Casalinuovo
 * Copyright (c) 2002, 2003 Marcus Overhagen <Marcus@Overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */


#include <BufferConsumer.h>
#include <BufferProducer.h>
#include <Controllable.h>
#include <FileInterface.h>
#include <MediaRoster.h>
#include <MediaNode.h>
#include <SupportDefs.h>
#include <TimeSource.h>

#include <string.h>

#include "DataExchange.h"
#include "MediaDebug.h"
#include "MediaMisc.h"
#include "MediaRosterEx.h"
#include "Notifications.h"
#include "ServerInterface.h"
#include "TimeSourceObject.h"
#include "TimeSourceObjectManager.h"

using std::nothrow;
using std::nothrow_t;

#undef TRACE
//#define TRACE_MEDIA_NODE
#ifdef TRACE_MEDIA_NODE
  #define TRACE printf
#else
  #define TRACE(x...)
#endif

// Don't rename this one, it's used and exported for binary compatibility
int32 BMediaNode::_m_changeTag = 0;

// media_node

const media_node media_node::null;

media_node::media_node()
	:
	node(-1),
	port(-1),
	kind(0)
{
}


media_node::~media_node()
{
}

// media_input

media_input::media_input()
{
	name[0] = '\0';
}


media_input::~media_input()
{
}

// media_output

media_output::media_output()
{
	name[0] = '\0';
}


media_output::~media_output()
{
}

// live_node_info

live_node_info::live_node_info()
	:
	hint_point(0.0f, 0.0f)
{
	name[0] = '\0';
}


live_node_info::~live_node_info()
{
}

// BMediaNode

BMediaNode::~BMediaNode()
{
	CALLED();
	// BeBook: UnregisterNode() unregisters a node from the Media Server.
	// It's called automatically by the BMediaNode destructor, but it might
	// be convenient to call it sometime before you delete your node instance,
	// depending on your implementation and circumstances.

	// First we remove the time source
	if (fTimeSource) {
		fTimeSource->RemoveMe(this);
		fTimeSource->Release();
		fTimeSource = NULL;
	}

	// Attention! We do not delete their control ports, since they are only a
	// shadow object, and the real one still exists
	if ((fKinds & NODE_KIND_SHADOW_TIMESOURCE) == 0) {
		if (fControlPort > 0)
			delete_port(fControlPort);
	} else {
		TRACE("BMediaNode::~BMediaNode: shadow timesource,"
			" not unregistering\n");
	}
}


BMediaNode*
BMediaNode::Acquire()
{
	CALLED();
	if (atomic_add(&fRefCount,1) == 0) {
		status_t status = B_ERROR;
		BMediaRoster* roster = BMediaRoster::Roster(&status);
		if (roster != NULL && status == B_OK)
			MediaRosterEx(roster)->RegisterLocalNode(this);
	}
	return this;
}


BMediaNode*
BMediaNode::Release()
{
	CALLED();
	if (atomic_add(&fRefCount, -1) == 1) {
		status_t status = B_ERROR;
		BMediaRoster* roster = BMediaRoster::Roster(&status);
		if (roster != NULL && status == B_OK) {
			MediaRosterEx(roster)->UnregisterLocalNode(this);

			// Only addons needs the configuration to be saved.
			int32 id;
			if (AddOn(&id) != NULL) {
				TRACE("BMediaNode::Release() saving node %" B_PRId32
					" configuration\n", fNodeID);
				MediaRosterEx(roster)->SaveNodeConfiguration(this);
			}

			if (DeleteHook(this) != B_OK) {
				ERROR("BMediaNode::Release(): DeleteHook failed\n");
				return Acquire();
			}
			return NULL;
		}
		TRACE("BMediaRoster::Release() the media roster is NULL!");
	}
	return this;
}


const char*
BMediaNode::Name() const
{
	CALLED();
	return fName;
}


media_node_id
BMediaNode::ID() const
{
	CALLED();
	return fNodeID;
}


uint64
BMediaNode::Kinds() const
{
	CALLED();
	return fKinds & NODE_KIND_USER_MASK;
}


media_node
BMediaNode::Node() const
{
	CALLED();
	media_node temp;
	temp.node = ID();
	// We *must* call ControlPort(), some derived nodes
	// use it to start the port read thread!
	temp.port = ControlPort();
	temp.kind = Kinds();
	return temp;
}


BMediaNode::run_mode
BMediaNode::RunMode() const
{
	CALLED();
	return fRunMode;
}


BTimeSource*
BMediaNode::TimeSource() const
{
	PRINT(7, "CALLED BMediaNode::TimeSource()\n");

	// Return the currently assigned time source
	if (fTimeSource != 0)
		return fTimeSource;

	TRACE("BMediaNode::TimeSource node %" B_PRId32 " enter\n", ID());

	// If the node doesn't have a time source object, we need to create one.
	// If the node is still unregistered, we can't call MakeTimeSourceFor(),
	// but since the node does still have the default system time source, we
	// can use GetSystemTimeSource

	BMediaNode* self = const_cast<BMediaNode*>(this);
	self->fTimeSource = MediaRosterEx(
		BMediaRoster::Roster())->MakeTimeSourceObject(fTimeSourceID);

	ASSERT(fTimeSource == self->fTimeSource);

	if (fTimeSource == 0) {
		ERROR("BMediaNode::TimeSource: MakeTimeSourceFor failed\n");
	} else {
		ASSERT(fTimeSourceID == fTimeSource->ID());
		fTimeSource->AddMe(self);
	}

	TRACE("BMediaNode::TimeSource node %" B_PRId32 " leave\n", ID());

	return fTimeSource;
}


port_id
BMediaNode::ControlPort() const
{
	PRINT(7, "CALLED BMediaNode::ControlPort()\n");
	return fControlPort;
}


status_t
BMediaNode::ReportError(node_error what, const BMessage* info)
{
	CALLED();

	// Sanity check the what value
	switch (what) {
		case BMediaNode::B_NODE_FAILED_START:
		case BMediaNode::B_NODE_FAILED_STOP:
		case BMediaNode::B_NODE_FAILED_SEEK:
		case BMediaNode::B_NODE_FAILED_SET_RUN_MODE:
		case BMediaNode::B_NODE_FAILED_TIME_WARP:
		case BMediaNode::B_NODE_FAILED_PREROLL:
		case BMediaNode::B_NODE_FAILED_SET_TIME_SOURCE_FOR:
		case BMediaNode::B_NODE_IN_DISTRESS:
			break;
		default:
			ERROR("BMediaNode::ReportError: invalid what!\n");
			return B_BAD_VALUE;
	}

	// Transmits the error code specified by what to anyone
	// that's receiving notifications from this node
	return BPrivate::media::notifications::ReportError(Node(), what, info);
}


status_t
BMediaNode::NodeStopped(bigtime_t whenPerformance)
{
	UNIMPLEMENTED();
	// Called by derived classes when they have
	// finished handling a stop request.

	// Notify anyone who is listening for stop notifications!
	BPrivate::media::notifications::NodeStopped(Node(), whenPerformance);

	// NOTE: If your node is a BBufferProducer, downstream consumers
	// will be notified that your node stopped (automatically, no less) through
	// the BBufferConsumer::ProducerDataStatus(B_PRODUCER_STOPPED) call.

	return B_OK;
}


/*
 * Used in couple with AddTimer, this will cause the BMediaRoster::SyncToNode()
 * call that requested the timer to return to the caller with an appropriate
 * value.
 */
void
BMediaNode::TimerExpired(bigtime_t notifyPoint, int32 cookie, status_t error)
{
	CALLED();
	if (write_port((port_id)cookie, 0, &error, sizeof(error)) < 0) {
		TRACE("BMediaNode::TimerExpired: error writing port %" B_PRId32
			", at notifyPoint %" B_PRId64 "\n", cookie, notifyPoint);
	}
}


BMediaNode::BMediaNode(const char* name)
{
	TRACE("BMediaNode::BMediaNode: name '%s'\n", name);
	_InitObject(name, NODE_JUST_CREATED_ID, 0);
}


status_t
BMediaNode::WaitForMessage(bigtime_t waitUntil, uint32 flags,
	void* _reserved_)
{
	TRACE("entering: BMediaNode::WaitForMessage()\n");

	// This function waits until either real time specified by
	// waitUntil or a message is received on the control port.
	// The flags are currently unused and should be 0.
	// Note: about 16 KByte stack used
	char data[B_MEDIA_MESSAGE_SIZE];
	int32 message;
	ssize_t size;

	while (true) {
		size = read_port_etc(ControlPort(), &message, data,
			sizeof(data), B_ABSOLUTE_TIMEOUT, waitUntil);

		if (size >= 0)
			break;

		status_t error = (status_t)size;
		if (error == B_INTERRUPTED)
			continue;

		if (error != B_TIMED_OUT && error != B_BAD_PORT_ID) {
			ERROR("BMediaNode::WaitForMessage: read_port_etc error: %s\n",
				strerror(error));
		}

		return error;
	}

	TRACE("BMediaNode::WaitForMessage request is: %#" B_PRIx32 ", node %"
		B_PRId32 ", this %p\n", message, fNodeID, this);

	if (message == GENERAL_PURPOSE_WAKEUP)
		return B_OK;

	if (message > NODE_MESSAGE_START && message < NODE_MESSAGE_END) {
		TRACE("BMediaNode::WaitForMessage calling BMediaNode\n");

		if (B_OK == BMediaNode::HandleMessage(message, data, size))
			return B_OK;
	}

	if (message > PRODUCER_MESSAGE_START && message < PRODUCER_MESSAGE_END) {
		if (!fProducerThis)
			fProducerThis = dynamic_cast<BBufferProducer*>(this);

		TRACE("BMediaNode::WaitForMessage calling BBufferProducer %p\n",
			fProducerThis);

		if (fProducerThis && fProducerThis->BBufferProducer::HandleMessage(
				message, data, size) == B_OK) {
			return B_OK;
		}
	}

	if (message > CONSUMER_MESSAGE_START && message < CONSUMER_MESSAGE_END) {
		if (!fConsumerThis)
			fConsumerThis = dynamic_cast<BBufferConsumer*>(this);

		TRACE("BMediaNode::WaitForMessage calling BBufferConsumer %p\n",
			fConsumerThis);

		if (fConsumerThis && fConsumerThis->BBufferConsumer::HandleMessage(
			message, data, size) == B_OK) {
			return B_OK;
		}
	}

	if (message > FILEINTERFACE_MESSAGE_START
		&& message < FILEINTERFACE_MESSAGE_END) {
		if (!fFileInterfaceThis)
			fFileInterfaceThis = dynamic_cast<BFileInterface*>(this);

		TRACE("BMediaNode::WaitForMessage calling BFileInterface %p\n",
			fFileInterfaceThis);

		if (fFileInterfaceThis
			&& fFileInterfaceThis->BFileInterface::HandleMessage(
				message, data, size) == B_OK) {
			return B_OK;
		}
	}

	if (message > CONTROLLABLE_MESSAGE_START
		&& message < CONTROLLABLE_MESSAGE_END) {
		if (!fControllableThis)
			fControllableThis = dynamic_cast<BControllable*>(this);

		TRACE("BMediaNode::WaitForMessage calling BControllable %p\n",
			fControllableThis);

		if (fControllableThis
			&& fControllableThis->BControllable::HandleMessage(
				message, data, size) == B_OK) {
			return B_OK;
		}
	}

	if (message > TIMESOURCE_MESSAGE_START
		&& message < TIMESOURCE_MESSAGE_END) {
		if (!fTimeSourceThis)
			fTimeSourceThis = dynamic_cast<BTimeSource*>(this);

		TRACE("BMediaNode::WaitForMessage calling BTimeSource %p\n",
			fTimeSourceThis);

		if (fTimeSourceThis && fTimeSourceThis->BTimeSource::HandleMessage(
				message, data, size) == B_OK) {
			return B_OK;
		}
	}

	TRACE("BMediaNode::WaitForMessage calling default HandleMessage\n");
	if (HandleMessage(message, data, size) == B_OK)
		return B_OK;

	HandleBadMessage(message, data, size);

	return B_ERROR;
}


void
BMediaNode::Start(bigtime_t performance_time)
{
	CALLED();
	// This hook function is called when a node is started
	// by a call to the BMediaRoster. The specified
	// performanceTime, the time at which the node
	// should start running, may be in the future.
	// It may be overriden by derived classes.
	// The BMediaEventLooper class handles this event!
	// The BMediaNode class does nothing here.
}


void
BMediaNode::Stop(bigtime_t performance_time, bool immediate)
{
	CALLED();
	// This hook function is called when a node is stopped
	// by a call to the BMediaRoster. The specified
	// performanceTime, the time at which the node
	// should stop running, may be in the future.
	// It may be overriden by derived classes.
	// The BMediaEventLooper class handles this event!
	// The BMediaNode class does nothing here.
}


void
BMediaNode::Seek(bigtime_t media_time, bigtime_t performance_time)
{
	CALLED();
	// This hook function is called when a node is asked
	// to seek to the specified mediaTime by a call to
	// the BMediaRoster. The specified performanceTime,
	// the time at which the node should begin the seek
	// operation, may be in the future.
	// It may be overriden by derived classes.
	// The BMediaEventLooper class handles this event!
	// The BMediaNode class does nothing here.
}


void
BMediaNode::SetRunMode(run_mode mode)
{
	CALLED();

	// This is a hook function, and
	// may be overriden by derived classes.

	// The functionality here is only to
	// support those people that don't
	// use the roster to set the run mode
	fRunMode = mode;
}


void
BMediaNode::TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time)
{
	CALLED();
	// May be overriden by derived classes.
}


void
BMediaNode::Preroll()
{
	CALLED();
	// May be overriden by derived classes.
}


void
BMediaNode::SetTimeSource(BTimeSource* time_source)
{
	CALLED();
	// This is a hook function, and
	// may be overriden by derived classes.

	if (time_source == NULL || time_source == fTimeSource)
		return;

	// We just trip into debugger, code that tries to do this is broken.
	debugger("BMediaNode::SetTimeSource() can't be used to set a timesource, "
		"use BMediaRoster::SetTimeSourceFor()!\n");
}


status_t
BMediaNode::HandleMessage(int32 message, const void* data, size_t size)
{
	TRACE("BMediaNode::HandleMessage %#" B_PRIx32", node %" B_PRId32 "\n",
		message, fNodeID);
	switch (message) {
		case NODE_FINAL_RELEASE:
		{
			TRACE("BMediaNode::HandleMessage NODE_FINAL_RELEASE, this %p\n",
				this);

			// This is called by the media server to delete the object
			// after is has been released by all nodes that are using it.
			// We forward the function to the BMediaRoster, since the
			// deletion must be done from a different thread, or the
			// outermost destructor that will exit the thread that is
			// reading messages from the port (this thread contex) will
			// quit, and ~BMediaNode destructor won't be called ever.
			BMessage msg(NODE_FINAL_RELEASE);
			msg.AddPointer("node", this);
			BMediaRoster::Roster()->PostMessage(&msg);
			return B_OK;
		}

		case NODE_START:
		{
			const node_start_command* command
				= static_cast<const node_start_command*>(data);
			TRACE("BMediaNode::HandleMessage NODE_START, node %" B_PRId32 "\n",
				fNodeID);
			Start(command->performance_time);
			return B_OK;
		}

		case NODE_STOP:
		{
			const node_stop_command* command
				= static_cast<const node_stop_command*>(data);
			TRACE("BMediaNode::HandleMessage NODE_STOP, node %" B_PRId32 "\n",
				fNodeID);
			Stop(command->performance_time, command->immediate);
			return B_OK;
		}

		case NODE_SEEK:
		{
			const node_seek_command* command
				= static_cast<const node_seek_command*>(data);
			TRACE("BMediaNode::HandleMessage NODE_SEEK, node %" B_PRId32 "\n",
				fNodeID);
			Seek(command->media_time, command->performance_time);
			return B_OK;
		}

		case NODE_SET_RUN_MODE:
		{
			const node_set_run_mode_command* command
				= static_cast<const node_set_run_mode_command*>(data);
			TRACE("BMediaNode::HandleMessage NODE_SET_RUN_MODE,"
				" node %" B_PRId32 "\n", fNodeID);
			// Need to change PRODUCER_SET_RUN_MODE_DELAY
			fRunMode = command->mode;
			SetRunMode(fRunMode);
			return B_OK;
		}

		case NODE_TIME_WARP:
		{
			const node_time_warp_command* command
				= static_cast<const node_time_warp_command*>(data);
			TRACE("BMediaNode::HandleMessage NODE_TIME_WARP,"
				" node %" B_PRId32 "\n", fNodeID);
			TimeWarp(command->at_real_time, command->to_performance_time);
			return B_OK;
		}

		case NODE_PREROLL:
		{
			TRACE("BMediaNode::HandleMessage NODE_PREROLL, "
				" node %" B_PRId32 "\n", fNodeID);
			Preroll();
			return B_OK;
		}

		case NODE_ROLL:
		{
			const node_roll_command* command
				= static_cast<const node_roll_command*>(data);

			TRACE("BMediaNode::HandleMessage NODE_ROLL, node %" B_PRId32 "\n",
				fNodeID);

			if (command->seek_media_time != B_INFINITE_TIMEOUT)
				Seek(command->seek_media_time,
					command->start_performance_time);

			Start(command->start_performance_time);
			Stop(command->stop_performance_time, false);
			return B_OK;
		}

		case NODE_SYNC_TO:
		{
			const node_sync_to_request* request
				= static_cast<const node_sync_to_request*>(data);
			node_sync_to_reply reply;

			TRACE("BMediaNode::HandleMessage NODE_SYNC_TO, node %" B_PRId32
				"\n", fNodeID);

			// If AddTimer return an error the caller will know that the node
			// doesn't support this feature or there was a problem when adding
			// it, this will result in SyncToNode returning immediately
			// to the caller with an error.
			status_t status = AddTimer(request->performance_time,
				request->port);

			request->SendReply(status, &reply, sizeof(reply));
			return B_OK;
		}

		case NODE_SET_TIMESOURCE:
		{
			const node_set_timesource_command* command
				= static_cast<const node_set_timesource_command*>(data);

			TRACE("BMediaNode::HandleMessage NODE_SET_TIMESOURCE,"
				" node %" B_PRId32 ", timesource %" B_PRId32 " enter\n",
				fNodeID, command->timesource_id);

			fTimeSourceID = command->timesource_id;

			if (fTimeSource) {
				// As this node already had a timesource, to remove this node
				// from time source control
				fTimeSource->RemoveMe(this);
				// Release the time source
				fTimeSource->Release();
				// Force next call to TimeSource() to create a new object
				fTimeSource = 0;
			}

			// Create new time source object and call the SetTimeSource
			// hook function to notify any derived class
			fTimeSource = TimeSource();
			SetTimeSource(fTimeSource);

			TRACE("BMediaNode::HandleMessage NODE_SET_TIMESOURCE, node %"
				B_PRId32 ", timesource %" B_PRId32 " leave\n", fNodeID,
				command->timesource_id);

			return B_OK;
		}

		case NODE_GET_TIMESOURCE:
		{
			const node_get_timesource_request* request
				= static_cast<const node_get_timesource_request*>(data);

			TRACE("BMediaNode::HandleMessage NODE_GET_TIMESOURCE,"
				" node %" B_PRId32 "\n", fNodeID);

			node_get_timesource_reply reply;
			reply.timesource_id = fTimeSourceID;
			request->SendReply(B_OK, &reply, sizeof(reply));
			return B_OK;
		}

		case NODE_GET_ATTRIBUTES_FOR:
		{
			const node_get_attributes_for_request *request =
				(const node_get_attributes_for_request*) data;

			TRACE("BMediaNode::HandleMessage NODE_GET_ATTRIBUTES_FOR,"
				"node %" B_PRId32 "\n", fNodeID);

			node_get_attributes_for_reply reply;

			media_node_attribute* addr;
			area_id dataArea = clone_area("client attributes area",
				(void**)&addr, B_ANY_ADDRESS, B_WRITE_AREA,
				request->area);

			if (dataArea < 0) {
				ERROR("NODE_GET_ATTRIBUTES_FOR can't clone area\n");
				return B_NO_MEMORY;
			}

			status_t status = GetNodeAttributes(addr, request->count);
			if (status == B_OK) {
				// NOTE: we do it because there's not an easy way
				// to guess the number of attributes filled.
				size_t i;
				for (i = 0; i < request->count; i++) {
					if (addr[i].what <= 0)
						break;
				}
				reply.filled_count = i;
			}
			request->SendReply(status, &reply, sizeof(reply));
			delete_area(dataArea);
			return B_OK;
		}

		case NODE_REQUEST_COMPLETED:
		{
			const node_request_completed_command* command
				= static_cast<const node_request_completed_command*>(data);
			TRACE("BMediaNode::HandleMessage NODE_REQUEST_COMPLETED,"
				" node %" B_PRId32 "\n", fNodeID);
			RequestCompleted(command->info);
			return B_OK;
		}

		default:
			return B_ERROR;

	}
	return B_ERROR;
}


void
BMediaNode::HandleBadMessage(int32 code, const void* buffer, size_t size)
{
	CALLED();

	TRACE("BMediaNode::HandleBadMessage: code %#08" B_PRIx32 ", buffer %p, size %"
		B_PRIuSIZE "\n", code, buffer, size);
	if (code < NODE_MESSAGE_START || code > TIMESOURCE_MESSAGE_END) {
		ERROR("BMediaNode::HandleBadMessage: unknown code!\n");
	} else {
		 // All messages targeted to nodes should be handled here,
		 // messages targetted to the wrong node should be handled
		 // by returning an error, not by stalling the sender.
		const request_data* request = static_cast<const request_data* >(buffer);
		reply_data reply;
		request->SendReply(B_ERROR, &reply, sizeof(reply));
	}
}


void
BMediaNode::AddNodeKind(uint64 kind)
{
	TRACE("BMediaNode::AddNodeKind: node %" B_PRId32 ", this %p\n", fNodeID,
		this);
	fKinds |= kind;
}


void*
BMediaNode::operator new(size_t size)
{
	CALLED();
	return ::operator new(size);
}


void*
BMediaNode::operator new(size_t size, const nothrow_t&) throw()
{
	CALLED();
	return ::operator new(size, nothrow);
}


void
BMediaNode::operator delete(void* ptr)
{
	CALLED();
	::operator delete(ptr);
}


void
BMediaNode::operator delete(void* ptr, const nothrow_t&) throw()
{
	CALLED();
	::operator delete(ptr, nothrow);
}


status_t
BMediaNode::RequestCompleted(const media_request_info& info)
{
	CALLED();
	// This function is called whenever
	// a request issued by the node is completed.
	// May be overriden by derived classes.
	// info.change_tag can be used to match up requests against
	// the accompaning calles from
	// BBufferConsumer::RequestFormatChange()
	// BBufferConsumer::SetOutputBuffersFor()
	// BBufferConsumer::SetOutputEnabled()
	// BBufferConsumer::SetVideoClippingFor()
	return B_OK;
}


status_t
BMediaNode::DeleteHook(BMediaNode* node)
{
	CALLED();
	// Attention! We do not unregister TimeSourceObject nodes,
	// since they are only a shadow object, and the real one still exists
	if ((fKinds & NODE_KIND_SHADOW_TIMESOURCE) == 0)
		BMediaRoster::Roster()->UnregisterNode(this);
	delete this; // delete "this" or "node", both are the same
	return B_OK;
}


void
BMediaNode::NodeRegistered()
{
	CALLED();
	// The Media Server calls this hook function
	// after the node has been registered.
	// May be overriden by derived classes.
}


status_t
BMediaNode::GetNodeAttributes(media_node_attribute* outAttributes,
	size_t inMaxCount)
{
	CALLED();
	// This is implemented by derived classes that fills
	// it's own attributes to a max of inMaxCount elements.
	return B_ERROR;
}


status_t
BMediaNode::AddTimer(bigtime_t at_performance_time, int32 cookie)
{
	CALLED();
	return B_ERROR;
}


status_t BMediaNode::_Reserved_MediaNode_0(void*) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_1(void*) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_2(void*) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_3(void*) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_4(void*) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_5(void*) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_6(void*) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_7(void*) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_8(void*) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_9(void*) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_10(void*) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_11(void*) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_12(void*) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_13(void*) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_14(void*) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_15(void*) { return B_ERROR; }

/*
private unimplemented
BMediaNode::BMediaNode()
BMediaNode::BMediaNode(const BMediaNode &clone)
BMediaNode &BMediaNode::operator=(const BMediaNode &clone)
*/

void
BMediaNode::_InitObject(const char* name, media_node_id id, uint64 kinds)
{
	TRACE("BMediaNode::_InitObject: nodeid %" B_PRId32 ", this %p\n", id,
		this);

	fNodeID = id;
	fRefCount = 1;
	fName[0] = 0;
	if (name)
		strlcpy(fName, name, B_MEDIA_NAME_LENGTH);
	fRunMode = B_INCREASE_LATENCY;
	fKinds = kinds;
	fProducerThis = 0;
	fConsumerThis = 0;
	fFileInterfaceThis = 0;
	fControllableThis = 0;
	fTimeSourceThis = 0;

	// Create control port
	fControlPort = create_port(64, fName);

	// Nodes are assigned the system time source by default
	fTimeSourceID = NODE_SYSTEM_TIMESOURCE_ID;

	// We can't create the timesource object here, because
	// every timesource is a BMediaNode, which would result
	// in infinite recursions
	fTimeSource = NULL;
}


BMediaNode::BMediaNode(const char* name, media_node_id id, uint32 kinds)
{
	TRACE("BMediaNode::BMediaNode: name '%s', nodeid %" B_PRId32 ", kinds %#"
		B_PRIx32 "\n", name, id, kinds);
	_InitObject(name, id, kinds);
}


int32
BMediaNode::NewChangeTag()
{
	CALLED();
	// Change tags have been used in BeOS R4 to match up
	// format change requests between producer and consumer,
	// This has changed starting with R4.5
	// now "change tags" are used with the following functions:
	// BMediaNode::RequestCompleted()
	// BBufferConsumer::RequestFormatChange()
	// BBufferConsumer::SetOutputBuffersFor()
	// BBufferConsumer::SetOutputEnabled()
	// BBufferConsumer::SetVideoClippingFor()
	return atomic_add(&BMediaNode::_m_changeTag,1);
}

// BeOS R4 deprecated API

int32
BMediaNode::IncrementChangeTag()
{
	CALLED();
	// Only present in BeOS R4
	// Obsoleted in BeOS R4.5 and later
	// "updates the change tag, so that downstream consumers
	// know that the node is in a new state."
	// not supported, only for binary compatibility
	return 0;
}


int32
BMediaNode::ChangeTag()
{
	UNIMPLEMENTED();
	// Only present in BeOS R4
	// Obsoleted in BeOS R4.5 and later
	// "returns the node's current change tag value."
	// not supported, only for binary compatibility
	return 0;
}


int32
BMediaNode::MintChangeTag()
{
	UNIMPLEMENTED();
	// Only present in BeOS R4
	// Obsoleted in BeOS R4.5 and later
	// "mints a new, reserved, change tag."
	// "Call ApplyChangeTag() to apply it to the node"
	// not supported, only for binary compatibility
	return 0;
}


status_t
BMediaNode::ApplyChangeTag(int32 previously_reserved)
{
	UNIMPLEMENTED();
	// Only present in BeOS R4
	// Obsoleted in BeOS R4.5 and later
	// "this returns B_OK if the new change tag is"
	// "successfully applied, or B_MEDIA_STALE_CHANGE_TAG if the new change"
	// "count you tried to apply is already obsolete."
	// not supported, only for binary compatibility
	return B_OK;
}
