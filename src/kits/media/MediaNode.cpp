/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: MediaNode.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaRoster.h>
#include <MediaNode.h>
#include <TimeSource.h>
#include <BufferConsumer.h>
#include <BufferProducer.h>
#include <Controllable.h>
#include <FileInterface.h>
#include <string.h>
#define DEBUG 3
#include <Debug.h>
#include "debug.h"
#include "DataExchange.h"
#include "SystemTimeSource.h"
#include "ServerInterface.h"
#include "Notifications.h"

// don't rename this one, it's used and exported for binary compatibility
int32 BMediaNode::_m_changeTag = 0;

/*************************************************************
 * media_node 
 *************************************************************/

// final & verified
media_node::media_node()
	: node(-1),
	port(-1),
	kind(0)
{
}

// final & verified
media_node::~media_node()
{
}

/*************************************************************
 * static media_node variables
 *************************************************************/

// final & verified
media_node media_node::null;

/*************************************************************
 * media_input 
 *************************************************************/

// final
media_input::media_input()
{
	name[0] = '\0';
}

// final
media_input::~media_input()
{
}

/*************************************************************
 * media_output 
 *************************************************************/

// final
media_output::media_output()
{
	name[0] = '\0';
}

// final
media_output::~media_output()
{
}

/*************************************************************
 * live_node_info 
 *************************************************************/

// final & verified
live_node_info::live_node_info()
	: hint_point(0.0f,0.0f)
{
	name[0] = '\0';
}

// final & verified
live_node_info::~live_node_info()
{
}

/*************************************************************
 * protected BMediaNode
 *************************************************************/

/* virtual */
BMediaNode::~BMediaNode()
{
	CALLED();
	
	// BeBook: UnregisterNode() unregisters a node from the Media Server. It's called automatically 
	// BeBook: by the BMediaNode destructor, but it might be convenient to call it sometime before 
	// BeBook: you delete your node instance, depending on your implementation and circumstances. 
	(BMediaRoster::Roster())->UnregisterNode(this);
	
	if (fControlPort != -1)
		delete_port(fControlPort);
	if (fTimeSource)
		fTimeSource->Release();
}

/*************************************************************
 * public BMediaNode
 *************************************************************/

BMediaNode *
BMediaNode::Acquire()
{
	CALLED();
	atomic_add(&fRefCount,1);
	return this;
}


BMediaNode *
BMediaNode::Release()
{
	CALLED();
	if (atomic_add(&fRefCount,-1) == 1) {
		if (DeleteHook(this) != B_OK) {
			TRACE("BMediaNode::Release(): DeleteHook failed\n");
			return Acquire();
		}
		return NULL;
	}
	return this;
}


const char *
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
	return fKinds;
}


media_node
BMediaNode::Node() const
{
	CALLED();
	media_node temp;
	temp.node = ID();
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


BTimeSource *
BMediaNode::TimeSource() const
{
	CALLED();
	if (fTimeSource == 0)
		const_cast<BMediaNode *>(this)->fTimeSource = new _SysTimeSource;
	return fTimeSource;
}


/* virtual */ port_id
BMediaNode::ControlPort() const
{
	CALLED();
	return fControlPort;
}


/*************************************************************
 * protected BMediaNode
 *************************************************************/

status_t
BMediaNode::ReportError(node_error what,
						const BMessage *info)
{
	CALLED();

	// sanity check the what value
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
			TRACE("BMediaNode::ReportError: invalid what!\n");
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
	// called by derived classes when they have
	// finished handling a stop request.
	
	// notify anyone who is listening for stop notifications!
	BPrivate::media::notifications::NodeStopped(Node(), whenPerformance);
	
	// XXX If your node is a BBufferProducer, downstream consumers 
	// XXX will be notified that your node stopped (automatically, no less) 
	// XXX through the BBufferConsumer::ProducerDataStatus(B_PRODUCER_STOPPED) call.
	
	return B_OK;
}


void
BMediaNode::TimerExpired(bigtime_t notifyPoint,
						 int32 cookie,
						 status_t error)
{
	UNIMPLEMENTED();
	// Used with AddTimer
	// This will, in turn, cause the BMediaRoster::SyncToNode() call 
	// that instigated the timer to return to the caller. 
	// Probably only important to classes derived from BTimeSource.
}


/* explicit */
BMediaNode::BMediaNode(const char *name)
{
	TRACE("BMediaNode::BMediaNode: name '%s'\n", name);
	_InitObject(name, -1, 0);
}


status_t
BMediaNode::WaitForMessage(bigtime_t waitUntil,
						   uint32 flags,
						   void *_reserved_)
{
	CALLED();
	ASSERT(this != 0);
	// This function waits until either real time specified by 
	// waitUntil or a message is received on the control port.
	// The flags are currently unused and should be 0. 

	char data[B_MEDIA_MESSAGE_SIZE]; // about 16 KByte stack used
	int32 message;
	ssize_t size;
	
	size = read_port_etc(fControlPort, &message, data, sizeof(data), B_ABSOLUTE_TIMEOUT, waitUntil);
	if (size <= 0) {
		if (size != B_TIMED_OUT)
			TRACE("BMediaNode::WaitForMessage: read_port_etc error 0x%08lx\n",size);
		return size; // returns the error code
	}

	TRACE("BMediaNode::WaitForMessage %#lx, node %ld, this %p\n", message, fNodeID, this);

	if (message > NODE_MESSAGE_START && message < NODE_MESSAGE_END) {
		TRACE("BMediaNode::WaitForMessage calling BMediaNode\n");
		if (B_OK == BMediaNode::HandleMessage(message, data, size))
			return B_OK;
	}
	
	if (message > PRODUCER_MESSAGE_START && message < PRODUCER_MESSAGE_END) {
		if (!fProducerThis)
			fProducerThis = dynamic_cast<BBufferProducer *>(this);
		TRACE("BMediaNode::WaitForMessage calling BBufferProducer %p\n", fProducerThis);
		if (fProducerThis && B_OK == fProducerThis->BBufferProducer::HandleMessage(message, data, size))
			return B_OK;
	}

	if (message > CONSUMER_MESSAGE_START && message < CONSUMER_MESSAGE_END) {
		if (!fConsumerThis)
			fConsumerThis = dynamic_cast<BBufferConsumer *>(this);
		TRACE("BMediaNode::WaitForMessage calling BBufferConsumer %p\n", fConsumerThis);
		if (fConsumerThis && B_OK == fConsumerThis->BBufferConsumer::HandleMessage(message, data, size))
			return B_OK;
	}

	if (message > FILEINTERFACE_MESSAGE_START && message < FILEINTERFACE_MESSAGE_END) {
		if (!fFileInterfaceThis)
			fFileInterfaceThis = dynamic_cast<BFileInterface *>(this);
		TRACE("BMediaNode::WaitForMessage calling BFileInterface %p\n", fFileInterfaceThis);
		if (fFileInterfaceThis && B_OK == fFileInterfaceThis->BFileInterface::HandleMessage(message, data, size))
			return B_OK;
	}

	if (message > CONTROLLABLE_MESSAGE_START && message < CONTROLLABLE_MESSAGE_END) {
		if (!fControllableThis)
			fControllableThis = dynamic_cast<BControllable *>(this);
		TRACE("BMediaNode::WaitForMessage calling BControllable %p\n", fControllableThis);
		if (fControllableThis && B_OK == fControllableThis->BControllable::HandleMessage(message, data, size))
			return B_OK;
	}

	if (message > TIMESOURECE_MESSAGE_START && message < TIMESOURECE_MESSAGE_END) {
		if (!fTimeSourceThis)
			fTimeSourceThis = dynamic_cast<BTimeSource *>(this);
		TRACE("BMediaNode::WaitForMessage calling BTimeSource %p\n", fTimeSourceThis);
		if (fTimeSourceThis && B_OK == fTimeSourceThis->BTimeSource::HandleMessage(message, data, size))
			return B_OK;
	}

	TRACE("BMediaNode::WaitForMessage calling default\n");
	if (B_OK == HandleMessage(message, data, size))
		return B_OK;

	HandleBadMessage(message, data, size);
	
	return B_ERROR;
}


/* virtual */ void
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


/* virtual */ void
BMediaNode::Stop(bigtime_t performance_time,
				 bool immediate)
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


/* virtual */ void
BMediaNode::Seek(bigtime_t media_time,
				 bigtime_t performance_time)
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


/* virtual */ void
BMediaNode::SetRunMode(run_mode mode)
{
	CALLED();

	// this is a hook function, and 
	// may be overriden by derived classes.
	
	// the functionality here is only to
	// support those people that don't
	// use the roster to set the run mode
	fRunMode = mode;
}


/* virtual */ void
BMediaNode::TimeWarp(bigtime_t at_real_time,
					 bigtime_t to_performance_time)
{
	CALLED();
	// May be overriden by derived classes.
}


/* virtual */ void
BMediaNode::Preroll()
{
	CALLED();
	// May be overriden by derived classes.
}


/* virtual */ void
BMediaNode::SetTimeSource(BTimeSource *time_source)
{
	CALLED();
	return;// XXX
	
	// this is a hook function, and 
	// may be overriden by derived classes.
	
	// the functionality here is only to
	// support those people that don't
	// use the roster to set a time source
	if (time_source == fTimeSource)
		return;
	if (time_source == NULL)
		return;
	if (fTimeSource)
		fTimeSource->Release();
	fTimeSource = dynamic_cast<BTimeSource *>(time_source->Acquire());
	fTimeSourceID = fTimeSource->ID();
}

/*************************************************************
 * public BMediaNode
 *************************************************************/

/* virtual */ status_t
BMediaNode::HandleMessage(int32 message,
						  const void *data,
						  size_t size)
{
//	CALLED();
	TRACE("BMediaNode::HandleMessage %#lx, node %ld\n", message, fNodeID);
	switch (message) {
		case NODE_START:
		{
			const node_start_command *command = static_cast<const node_start_command *>(data);
			Start(command->performance_time);
			return B_OK;
		}

		case NODE_STOP:
		{
			const node_stop_command *command = static_cast<const node_stop_command *>(data);
			Stop(command->performance_time, command->immediate);
			return B_OK;
		}

		case NODE_SEEK:
		{
			const node_seek_command *command = static_cast<const node_seek_command *>(data);
			Seek(command->media_time, command->performance_time);
			return B_OK;
		}

		case NODE_SET_RUN_MODE:
		{
			const node_set_run_mode_command *command = static_cast<const node_set_run_mode_command *>(data);
			fRunMode = command->mode;
			SetRunMode(fRunMode);
			return B_OK;
		}

		case NODE_TIME_WARP:
		{
			const node_time_warp_command *command = static_cast<const node_time_warp_command *>(data);
			TimeWarp(command->at_real_time, command->to_performance_time);
			return B_OK;
		}

		case NODE_PREROLL:
		{
			Preroll();
			return B_OK;
		}
		
		case NODE_SET_TIMESOURCE:
		{
			const node_set_timesource_command *command = static_cast<const node_set_timesource_command *>(data);
			bool first = (fTimeSourceID == 0);
			if (fTimeSource)
				fTimeSource->Release();
			fTimeSourceID = command->timesource_id;
			fTimeSource = 0; // XXX create timesource object here
			fTimeSource = new _SysTimeSource;
			if (!first)
				SetTimeSource(fTimeSource);
			return B_OK;
		}

		case NODE_REQUEST_COMPLETED:
		{
			const node_request_completed_command *command = static_cast<const node_request_completed_command *>(data);
			RequestCompleted(command->info);
			return B_OK;
		}
		
	};
	return B_ERROR;
}


void
BMediaNode::HandleBadMessage(int32 code,
							 const void *buffer,
							 size_t size)
{
	CALLED();

	TRACE("BMediaNode::HandleBadMessage: code %#08lx, buffer %p, size %ld\n", code, buffer, size);
	if (code < NODE_MESSAGE_START || code > TIMESOURECE_MESSAGE_END) {
		TRACE("BMediaNode::HandleBadMessage: unknown code!\n");
	} else {
		/* All messages targeted to nodes should be handled here,
		 * messages targetted to the wrong node should be handled
		 * by returning an error, not by stalling the sender.
		 */
		const request_data *request = static_cast<const request_data *>(buffer);
		reply_data reply;
		request->SendReply(B_ERROR, &reply, sizeof(reply));
	}
}


void
BMediaNode::AddNodeKind(uint64 kind)
{
	TRACE("BMediaNode::AddNodeKind: node %ld, this %p\n", fNodeID, this);

	fKinds |= kind;
}


void *
BMediaNode::operator new(size_t size)
{
	CALLED();
	return ::operator new(size);
}

void *
BMediaNode::operator new(size_t size,
						 const nothrow_t &) throw()
{
	CALLED();
	return ::operator new(size, nothrow);
}

void
BMediaNode::operator delete(void *ptr)
{
	CALLED();
	::operator delete(ptr);
}

void 
BMediaNode::operator delete(void * ptr, 
							const nothrow_t &) throw()
{
	CALLED();
	::operator delete(ptr, nothrow);
}

/*************************************************************
 * protected BMediaNode
 *************************************************************/

/* virtual */ status_t
BMediaNode::RequestCompleted(const media_request_info &info)
{
	CALLED();
	// This function is called whenever a request issued by the node is completed.
	// May be overriden by derived classes.
	// info.change_tag can be used to match up requests against 
	// the accompaning calles from
	// BBufferConsumer::RequestFormatChange()
	// BBufferConsumer::SetOutputBuffersFor()
	// BBufferConsumer::SetOutputEnabled()
	// BBufferConsumer::SetVideoClippingFor()
	return B_OK;
}

/*************************************************************
 * private BMediaNode
 *************************************************************/

int32
BMediaNode::IncrementChangeTag()
{
	CALLED();
	// Only present in BeOS R4
	// Obsoleted in BeOS R4.5 and later
	// "updates the change tag, so that downstream consumers know that the node is in a new state."
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

/*************************************************************
 * protected BMediaNode
 *************************************************************/

/* virtual */ status_t
BMediaNode::DeleteHook(BMediaNode *node)
{
	CALLED();
	delete this; // delete "this" or "node" ???
	return B_OK;
}


/* virtual */ void
BMediaNode::NodeRegistered()
{
	CALLED();
	// The Media Server calls this hook function after the node has been registered. 
	// May be overriden by derived classes.
}

/*************************************************************
 * public BMediaNode
 *************************************************************/

/* virtual */ status_t
BMediaNode::GetNodeAttributes(media_node_attribute *outAttributes,
							  size_t inMaxCount)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


/* virtual */ status_t
BMediaNode::AddTimer(bigtime_t at_performance_time,
					 int32 cookie)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


status_t BMediaNode::_Reserved_MediaNode_0(void *) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_1(void *) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_2(void *) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_3(void *) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_4(void *) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_5(void *) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_6(void *) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_7(void *) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_8(void *) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_9(void *) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_10(void *) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_11(void *) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_12(void *) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_13(void *) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_14(void *) { return B_ERROR; }
status_t BMediaNode::_Reserved_MediaNode_15(void *) { return B_ERROR; }

/*
private unimplemented
BMediaNode::BMediaNode()
BMediaNode::BMediaNode(const BMediaNode &clone)
BMediaNode &BMediaNode::operator=(const BMediaNode &clone)
*/

void
BMediaNode::_InitObject(const char *name, media_node_id id, uint64 kinds)
{
	TRACE("BMediaNode::_InitObject: nodeid %ld, this %p\n", id, this);

	fNodeID = id;
	fTimeSource = 0;
	fRefCount = 1;
	fName[0] = 0;
	if (name) {
		strncpy(fName, name, B_MEDIA_NAME_LENGTH - 1);
		fName[B_MEDIA_NAME_LENGTH - 1] = 0;
	}
	fRunMode = B_INCREASE_LATENCY;
	_mChangeCount = 0;			//	deprecated
	_mChangeCountReserved = 0;	//	deprecated
	fKinds = kinds;
	fTimeSourceID = -1;
	fProducerThis = 0;
	fConsumerThis = 0;
	fFileInterfaceThis = 0;
	fControllableThis = 0;
	fTimeSourceThis = 0;

	// create control port
	fControlPort = create_port(64,fName);
}


BMediaNode::BMediaNode(const char *name,
					   media_node_id id,
					   uint32 kinds)
{
	TRACE("BMediaNode::BMediaNode: name '%s', nodeid %ld, kinds %#lx\n", name, id, kinds);
	_InitObject(name, id, kinds);
}


/*************************************************************
 * protected BMediaNode
 *************************************************************/

/* static */ int32
BMediaNode::NewChangeTag()
{
	CALLED();
	// change tags have been used in BeOS R4 to match up 
	// format change requests between producer and consumer,
	// This has changed starting with R4.5
	// now "change tags" are used with
	// BMediaNode::RequestCompleted()
	// and
	// BBufferConsumer::RequestFormatChange()
	// BBufferConsumer::SetOutputBuffersFor()
	// BBufferConsumer::SetOutputEnabled()
	// BBufferConsumer::SetVideoClippingFor()
	return atomic_add(&BMediaNode::_m_changeTag,1);
}


