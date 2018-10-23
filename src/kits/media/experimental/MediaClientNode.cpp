/*
 * Copyright 2015, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "MediaClientNode.h"

#include <MediaClient.h>
#include <MediaConnection.h>
#include <MediaRoster.h>
#include <scheduler.h>
#include <TimeSource.h>

#include <string.h>

#include "MediaDebug.h"

#define B_NEW_BUFFER (BTimedEventQueue::B_USER_EVENT + 1)


BMediaClientNode::BMediaClientNode(const char* name,
	BMediaClient* owner, media_type type)
	:
	BMediaNode(name),
	BBufferConsumer(type),
	BBufferProducer(type),
	BMediaEventLooper(),
	fOwner(owner)
{
	CALLED();

	// Configure the node to do the requested jobs
	if (fOwner->Kinds() & B_MEDIA_PLAYER)
		AddNodeKind(B_BUFFER_PRODUCER);
	if (fOwner->Kinds() & B_MEDIA_RECORDER)
		AddNodeKind(B_BUFFER_CONSUMER);
	if (fOwner->Kinds() & B_MEDIA_CONTROLLABLE)
		AddNodeKind(B_CONTROLLABLE);
}


status_t
BMediaClientNode::SendBuffer(BBuffer* buffer, BMediaConnection* conn)
{
	return BBufferProducer::SendBuffer(buffer, conn->_Source(), conn->_Destination());
}


BMediaAddOn*
BMediaClientNode::AddOn(int32* id) const
{
	CALLED();

	return fOwner->AddOn(id);
}


void
BMediaClientNode::NodeRegistered()
{
	CALLED();

	fOwner->ClientRegistered();

	Run();
}


void
BMediaClientNode::SetRunMode(run_mode mode)
{
	CALLED();

	int32 priority;
	if (mode == BMediaNode::B_OFFLINE)
		priority = B_OFFLINE_PROCESSING;
	else {
		switch(ConsumerType()) {
			case B_MEDIA_RAW_AUDIO:
			case B_MEDIA_ENCODED_AUDIO:
				priority = B_AUDIO_RECORDING;
				break;

			case B_MEDIA_RAW_VIDEO:
			case B_MEDIA_ENCODED_VIDEO:
				priority = B_VIDEO_RECORDING;
				break;

			default:
				priority = B_DEFAULT_MEDIA_PRIORITY;
		}
	}

	SetPriority(suggest_thread_priority(priority));
	BMediaNode::SetRunMode(mode);
}


void
BMediaClientNode::Start(bigtime_t performanceTime)
{
	CALLED();

	BMediaEventLooper::Start(performanceTime);
}


void
BMediaClientNode::Stop(bigtime_t performanceTime, bool immediate)
{
	CALLED();

	BMediaEventLooper::Stop(performanceTime, immediate);
}


void
BMediaClientNode::Seek(bigtime_t mediaTime, bigtime_t performanceTime)
{
	CALLED();

	BMediaEventLooper::Seek(mediaTime, performanceTime);
}


void
BMediaClientNode::TimeWarp(bigtime_t realTime, bigtime_t performanceTime)
{
	CALLED();

	BMediaEventLooper::TimeWarp(realTime, performanceTime);
}


status_t
BMediaClientNode::HandleMessage(int32 message,
	const void* data, size_t size)
{
	CALLED();

	return B_ERROR;
}


status_t
BMediaClientNode::AcceptFormat(const media_destination& dest,
	media_format* format)
{
	CALLED();

	BMediaInput* conn = fOwner->_FindInput(dest);
	if (conn == NULL)
		return B_MEDIA_BAD_DESTINATION;

	return conn->AcceptFormat(format);
}


status_t
BMediaClientNode::GetNextInput(int32* cookie,
	media_input* input)
{
	CALLED();

	if (fOwner->CountInputs() == 0)
		return B_BAD_INDEX;

	if (*cookie < 0 || *cookie >= fOwner->CountInputs()) {
		*cookie = -1;
		input = NULL;
	} else {
		BMediaInput* conn = fOwner->InputAt(*cookie);
		if (conn != NULL) {
			*input = conn->fConnection._BuildMediaInput();
			*cookie += 1;
			return B_OK;
		}
	}
	return B_BAD_INDEX;
}


void
BMediaClientNode::DisposeInputCookie(int32 cookie)
{
	CALLED();
}


void
BMediaClientNode::BufferReceived(BBuffer* buffer)
{
	CALLED();

	EventQueue()->AddEvent(media_timed_event(buffer->Header()->start_time,
		BTimedEventQueue::B_HANDLE_BUFFER, buffer,
		BTimedEventQueue::B_RECYCLE_BUFFER));
}


status_t
BMediaClientNode::GetLatencyFor(const media_destination& dest,
	bigtime_t* latency, media_node_id* timesource)
{
	CALLED();

	BMediaInput* conn = fOwner->_FindInput(dest);
	if (conn == NULL)
		return B_MEDIA_BAD_DESTINATION;

	//*latency = conn->fLatency;
	*timesource = TimeSource()->ID();
	return B_OK;
}


status_t
BMediaClientNode::Connected(const media_source& source,
	const media_destination& dest, const media_format& format,
	media_input* outInput)
{
	CALLED();

	BMediaInput* conn = fOwner->_FindInput(dest);
	if (conn == NULL)
		return B_MEDIA_BAD_DESTINATION;

	conn->fConnection.source = source;
	conn->fConnection.format = format;

	// Retrieve the node without using GetNodeFor that's pretty inefficient.
	// Unfortunately we don't have an alternative which doesn't require us
	// to release the cloned node.
	// However, our node will not have flags set. Keep in mind this.
	conn->fConnection.remote_node.node
		= BMediaRoster::CurrentRoster()->NodeIDFor(source.port);
	conn->fConnection.remote_node.port = source.port;

	conn->Connected(format);

	*outInput = conn->fConnection._BuildMediaInput();
	return B_OK;
}


void
BMediaClientNode::Disconnected(const media_source& source,
	const media_destination& dest)
{
	CALLED();

	BMediaInput* conn = fOwner->_FindInput(dest);
	if (conn == NULL)
		return;

	if (conn->_Source() == source) {
		// Cleanup the connection
		conn->fConnection.source = media_source::null;
		conn->fConnection.format = media_format();

		conn->fConnection.remote_node.node = -1;
		conn->fConnection.remote_node.port = -1;

		conn->Disconnected();
	}
}


status_t
BMediaClientNode::FormatChanged(const media_source& source,
	const media_destination& dest,
	int32 tag, const media_format& format)
{
	CALLED();
	return B_ERROR;
}


status_t
BMediaClientNode::FormatSuggestionRequested(media_type type,
	int32 quality, media_format* format)
{
	CALLED();

	if (type != ConsumerType()
			&& type != ProducerType()) {
		return B_MEDIA_BAD_FORMAT;
	}

	status_t ret = fOwner->FormatSuggestion(type, quality, format);
	if (ret != B_OK) {
		// In that case we return just a very generic format.
		media_format outFormat;
		outFormat.type = fOwner->MediaType();
		*format = outFormat;
		return B_OK;
	}

	return ret;
}


status_t
BMediaClientNode::FormatProposal(const media_source& source,
	media_format* format)
{
	CALLED();

	BMediaOutput* conn = fOwner->_FindOutput(source);
	if (conn == NULL)
		return B_MEDIA_BAD_DESTINATION;

	return conn->FormatProposal(format);
}


status_t
BMediaClientNode::FormatChangeRequested(const media_source& source,
	const media_destination& dest, media_format* format,
	int32* _deprecated_)
{
	CALLED();

	return B_ERROR;
}


void
BMediaClientNode::LateNoticeReceived(const media_source& source,
	bigtime_t late, bigtime_t when)
{
	CALLED();

}


status_t
BMediaClientNode::GetNextOutput(int32* cookie, media_output* output)
{
	CALLED();

	if (fOwner->CountOutputs() == 0)
		return B_BAD_INDEX;

	if (*cookie < 0 || *cookie >= fOwner->CountOutputs()) {
		*cookie = -1;
		output = NULL;
	} else {
		BMediaOutput* conn = fOwner->OutputAt(*cookie);
		if (conn != NULL) {
			*output = conn->fConnection._BuildMediaOutput();
			*cookie += 1;
			return B_OK;
		}
	}
	return B_BAD_INDEX;
}


status_t
BMediaClientNode::DisposeOutputCookie(int32 cookie)
{
	CALLED();

	return B_OK;
}


status_t
BMediaClientNode::SetBufferGroup(const media_source& source, BBufferGroup* group)
{
	CALLED();

	BMediaOutput* conn = fOwner->_FindOutput(source);
	if (conn == NULL)
		return B_MEDIA_BAD_SOURCE;

	if (group == conn->fBufferGroup)
		return B_OK;

	delete conn->fBufferGroup;

	if (group != NULL) {
		conn->fBufferGroup = group;
		return B_OK;
	}

	conn->fBufferGroup = new BBufferGroup(conn->BufferSize(), 3);
	if (conn->fBufferGroup == NULL)
		return B_NO_MEMORY;

	return conn->fBufferGroup->InitCheck();
}


status_t
BMediaClientNode::PrepareToConnect(const media_source& source,
	const media_destination& dest, media_format* format,
	media_source* out_source, char *name)
{
	CALLED();

	BMediaOutput* conn = fOwner->_FindOutput(source);
	if (conn == NULL)
		return B_MEDIA_BAD_SOURCE;

	if (conn->_Destination() != media_destination::null)
		return B_MEDIA_ALREADY_CONNECTED;

	if (fOwner->MediaType() != B_MEDIA_UNKNOWN_TYPE
			&& format->type != fOwner->MediaType()) {
		return B_MEDIA_BAD_FORMAT;
	}

	conn->fConnection.destination = dest;

	status_t err = conn->PrepareToConnect(format);
	if (err != B_OK)
		return err;

	*out_source = conn->_Source();
	strcpy(name, conn->Name());

	return B_OK;
}


void
BMediaClientNode::Connect(status_t status, const media_source& source,
	const media_destination& dest, const media_format& format,
	char* name)
{
	CALLED();

	BMediaOutput* conn = fOwner->_FindOutput(source);
	if (conn == NULL)
		return;

	// Connection failed, return.
	if (status != B_OK)
		return;

	conn->fConnection.destination = dest;
	conn->fConnection.format = format;

	// Retrieve the node without using GetNodeFor that's pretty inefficient.
	// Unfortunately we don't have an alternative which doesn't require us
	// to release the cloned node.
	// However, our node will not have flags set. Keep in mind this.
	conn->fConnection.remote_node.node
		= BMediaRoster::CurrentRoster()->NodeIDFor(dest.port);
	conn->fConnection.remote_node.port = dest.port;

	strcpy(name, conn->Name());

	// TODO: add correct latency estimate
	SetEventLatency(1000);

	conn->fBufferGroup = new BBufferGroup(conn->BufferSize(), 3);
	if (conn->fBufferGroup == NULL)
		TRACE("Can't allocate the buffer group\n");

	conn->Connected(format);
}


void
BMediaClientNode::Disconnect(const media_source& source,
	const media_destination& dest)
{
	CALLED();

	BMediaOutput* conn = fOwner->_FindOutput(source);
	if (conn == NULL)
		return;

	if (conn->_Destination() == dest) {
		// Cleanup the connection
		delete conn->fBufferGroup;
		conn->fBufferGroup = NULL;

		conn->fConnection.destination = media_destination::null;
		conn->fConnection.format = media_format();

		conn->fConnection.remote_node.node = -1;
		conn->fConnection.remote_node.port = -1;

		conn->Disconnected();
	}
}


void
BMediaClientNode::EnableOutput(const media_source& source,
	bool enabled, int32* _deprecated_)
{
	CALLED();

	BMediaOutput* conn = fOwner->_FindOutput(source);
	if (conn != NULL)
		conn->_SetEnabled(enabled);
}


status_t
BMediaClientNode::GetLatency(bigtime_t* outLatency)
{
	CALLED();

	return BBufferProducer::GetLatency(outLatency);
}


void
BMediaClientNode::LatencyChanged(const media_source& source,
	const media_destination& dest, bigtime_t latency, uint32 flags)
{
	CALLED();
}


void
BMediaClientNode::ProducerDataStatus(const media_destination& dest,
	int32 status, bigtime_t when)
{
	CALLED();
}


void
BMediaClientNode::HandleEvent(const media_timed_event* event,
	bigtime_t late, bool realTimeEvent)
{
	CALLED();

	switch (event->type) {
		// This event is used for inputs which consumes buffers
		// or binded connections which also send them to an output.
		case BTimedEventQueue::B_HANDLE_BUFFER:
			_HandleBuffer((BBuffer*)event->pointer);
			break;

		// This is used for connections which produce buffers only.
		case B_NEW_BUFFER:
			_ProduceNewBuffer(event, late);
			break;

		case BTimedEventQueue::B_START:
		{
			if (RunState() != B_STARTED)
				fOwner->HandleStart(event->event_time);

			fStartTime = event->event_time;

			_ScheduleConnections(event->event_time);
			break;
		}

		case BTimedEventQueue::B_STOP:
		{
			fOwner->HandleStop(event->event_time);

			EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true,
				BTimedEventQueue::B_HANDLE_BUFFER);
			break;
		}

		case BTimedEventQueue::B_SEEK:
			fOwner->HandleSeek(event->event_time, event->bigdata);
			break;

		case BTimedEventQueue::B_WARP:
			// NOTE: We have no need to handle it
			break;
	}
}


BMediaClientNode::~BMediaClientNode()
{
	CALLED();

	Quit();
}


void
BMediaClientNode::_ScheduleConnections(bigtime_t eventTime)
{
	for (int32 i = 0; i < fOwner->CountOutputs(); i++) {
		BMediaOutput* output = fOwner->OutputAt(i);

		if (output->HasBinding())
			continue;

		media_timed_event firstBufferEvent(eventTime,
			B_NEW_BUFFER);

		output->fFramesSent = 0;

		firstBufferEvent.pointer = (void*) output;
		EventQueue()->AddEvent(firstBufferEvent);
	}
}


void
BMediaClientNode::_HandleBuffer(BBuffer* buffer)
{
	CALLED();

	media_destination dest;
	dest.id = buffer->Header()->destination;
	BMediaInput* conn = fOwner->_FindInput(dest);

	if (conn != NULL)
		conn->HandleBuffer(buffer);

	// TODO: Investigate system level latency logging

	if (conn->HasBinding()) {
		BMediaOutput* output = dynamic_cast<BMediaOutput*>(conn->Binding());
		output->SendBuffer(buffer);
	}
}


void
BMediaClientNode::_ProduceNewBuffer(const media_timed_event* event,
	bigtime_t late)
{
	CALLED();

	if (RunState() != BMediaEventLooper::B_STARTED)
		return;

	// The connection is get through the event
	BMediaOutput* output
		= dynamic_cast<BMediaOutput*>((BMediaConnection*)event->pointer);
	if (output == NULL)
		return;

	if (output->_IsEnabled()) {
		BBuffer* buffer = _GetNextBuffer(output, event->event_time);

		if (buffer != NULL) {
			if (output->SendBuffer(buffer) != B_OK) {
				TRACE("BMediaClientNode: Failed to send buffer\n");
				// The output failed, let's recycle the buffer
				buffer->Recycle();
			}
		}
	}

	bigtime_t time = 0;
	media_format format = output->fConnection.format;
	if (format.IsAudio()) {
		size_t nFrames = format.u.raw_audio.buffer_size
			/ ((format.u.raw_audio.format
				& media_raw_audio_format::B_AUDIO_SIZE_MASK)
			* format.u.raw_audio.channel_count);
		output->fFramesSent += nFrames;

		time = fStartTime + bigtime_t((1000000LL * output->fFramesSent)
			/ (int32)format.u.raw_audio.frame_rate);
	}

	media_timed_event nextEvent(time, B_NEW_BUFFER);
	EventQueue()->AddEvent(nextEvent);
}


BBuffer*
BMediaClientNode::_GetNextBuffer(BMediaOutput* output, bigtime_t eventTime)
{
	CALLED();

	BBuffer* buffer = NULL;
	if (output->fBufferGroup->RequestBuffer(buffer, 0) != B_OK) {
		TRACE("MediaClientNode:::_GetNextBuffer: Failed to get the buffer\n");
		return NULL;
	}

	media_header* header = buffer->Header();
	header->type = output->fConnection.format.type;
	header->size_used = output->BufferSize();
	header->time_source = TimeSource()->ID();
	header->start_time = eventTime;

	return buffer;
}
