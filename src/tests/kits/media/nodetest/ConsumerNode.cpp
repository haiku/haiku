#include "ConsumerNode.h"

#include <stdlib.h>
#include <string.h>

#include <Buffer.h>
#include <TimeSource.h>

#include "misc.h"


ConsumerNode::ConsumerNode()
	:
	BBufferConsumer(B_MEDIA_RAW_AUDIO),
	BMediaEventLooper(),
	BMediaNode("ConsumerNode")
{
	out("ConsumerNode::ConsumerNode\n");
}


ConsumerNode::~ConsumerNode()
{
	out("ConsumerNode::~ConsumerNode\n");
	Quit();
}


void
ConsumerNode::NodeRegistered()
{
	out("ConsumerNode::NodeRegistered\n");
	InitializeInput();
	SetPriority(108);
	Run();
}


status_t
ConsumerNode::AcceptFormat(const media_destination& dest, media_format* format)
{
	out("ConsumerNode::AcceptFormat\n");

	if (dest != fInput.destination)
		return B_MEDIA_BAD_DESTINATION;

	if (format == NULL)
		return B_BAD_VALUE;

	if (format->type != B_MEDIA_RAW_AUDIO)
		return B_MEDIA_BAD_FORMAT;

	return B_OK;
}


status_t
ConsumerNode::GetNextInput(int32* cookie, media_input* _input)
{
	out("ConsumerNode::GetNextInput\n");

	if (_input == NULL)
		return B_BAD_VALUE;

	if (++(*cookie) > 1)
		return B_BAD_INDEX;

	*_input = fInput;
	return B_OK;
}


void
ConsumerNode::DisposeInputCookie(int32 cookie)
{
	out("ConsumerNode::DisposeInputCookie\n");
	return;
}


void
ConsumerNode::BufferReceived(BBuffer* buffer)
{
	out("ConsumerNode::BufferReceived, sheduled time = %5.4f\n",
		buffer->Header()->start_time / 1E6);

	media_timed_event event(buffer->Header()->start_time,
		BTimedEventQueue::B_HANDLE_BUFFER, buffer,
		BTimedEventQueue::B_RECYCLE_BUFFER);
	EventQueue()->AddEvent(event);
	return;
}


void
ConsumerNode::ProducerDataStatus(const media_destination& forWhom, int32 status,
	bigtime_t atPerformanceTime)
{
	out("ConsumerNode::ProducerDataStatus\n");

	if (forWhom == fInput.destination) {
		media_timed_event event(atPerformanceTime,
			BTimedEventQueue::B_DATA_STATUS, &fInput,
			BTimedEventQueue::B_NO_CLEANUP, status, 0, NULL);
		EventQueue()->AddEvent(event);
	}
}


status_t
ConsumerNode::GetLatencyFor(const media_destination& forWhom,
	bigtime_t* _latency, media_node_id* _timesource)
{
	out("ConsumerNode::GetLatencyFor\n");
	// make sure this is one of my valid inputs
	if (forWhom != fInput.destination)
		return B_MEDIA_BAD_DESTINATION;

	*_latency = 23000;
	*_timesource = TimeSource()->ID();
	return B_OK;
}


status_t
ConsumerNode::Connected(const media_source& producer,
	const media_destination& where, const media_format& withFormat,
	media_input* _input)
{
	out("ConsumerNode::Connected\n");
	if (where != fInput.destination)
		return B_MEDIA_BAD_DESTINATION;

	// calculate my latency here, because it may depend on buffer
	// sizes/durations, then tell the BMediaEventLooper how early
	// we need to get the buffers
	SetEventLatency(10 * 1000); // TODO: fix me

	/* reserve the connection */
	fInput.source = producer;
	fInput.format = withFormat;

	/* and publish it's name and connection info */
	*_input = fInput;

#if 0
	/* create the buffer group */
	if (mBufferGroup == NULL) {
		create_own_buffer_group();
		mBufferGroup = mOwnBufferGroup;
	}

	/* set the duration of the node's buffers */
	int32 numBuffers;
	mBufferGroup->CountBuffers(&numBuffers);
	SetBufferDuration((1000000LL * numBuffers) / mOutput.format.u.raw_video.field_rate);
#endif

	return B_OK;
}


void
ConsumerNode::Disconnected(const media_source& producer,
	const media_destination& where)
{
	out("ConsumerNode::Disconnected\n");

	/* unreserve the connection */
	InitializeInput();

#if 0
	/* release buffer group */
	mBufferGroup = NULL;
	if (mOwnBufferGroup != NULL) {
		delete_own_buffer_group();
	}
#endif

	return;
}


status_t
ConsumerNode::FormatChanged(const media_source& producer,
	const media_destination& consumer, int32 changeTag,
	const media_format& format)
{
	out("ConsumerNode::FormatChanged\n");
	return B_OK;
}


status_t
ConsumerNode::SeekTagRequested(const media_destination& destination,
	bigtime_t targetTime, uint32 flags, media_seek_tag* _seekTag,
	bigtime_t* _taggedTime, uint32* _flags)
{
	out("ConsumerNode::SeekTagRequested\n");
	return B_OK;
}


BMediaAddOn*
ConsumerNode::AddOn(int32* internalID) const
{
	out("ConsumerNode::AddOn\n");
	return NULL;
}


void
ConsumerNode::HandleEvent(const media_timed_event* event, bigtime_t lateness,
	bool realTimeEvent)
{
	switch (event->type) {
		case BTimedEventQueue::B_HANDLE_BUFFER:
		{
			out("ConsumerNode::HandleEvent B_HANDLE_BUFFER\n");
			BBuffer* buffer = const_cast<BBuffer*>((BBuffer*)event->pointer);

			out("### sheduled time = %5.4f, current time = %5.4f, lateness = "
				"%5.4f\n", buffer->Header()->start_time / 1E6,
				TimeSource()->Now() / 1E6,lateness / 1E6);

			snooze((rand() * 100) % 200000);

			if (buffer)
				buffer->Recycle();
			break;
		}

		case BTimedEventQueue::B_PARAMETER:
			out("ConsumerNode::HandleEvent B_PARAMETER\n");
			break;

		case BTimedEventQueue::B_START:
			out("ConsumerNode::HandleEvent B_START\n");
			break;

		case BTimedEventQueue::B_STOP:
			out("ConsumerNode::HandleEvent B_STOP\n");
			// stopping implies not handling any more buffers.  So, we flush
			// all pending buffers out of the event queue before returning to
			// the event loop.
			EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true,
				BTimedEventQueue::B_HANDLE_BUFFER);
			break;

		case BTimedEventQueue::B_SEEK:
			out("ConsumerNode::HandleEvent B_SEEK\n");
			break;

		case BTimedEventQueue::B_WARP:
			out("ConsumerNode::HandleEvent B_WARP\n");
			// similarly, time warps aren't meaningful to the logger, so just
			// record it and return
			//mLogger->Log(LOG_WARP_HANDLED, logMsg);
			break;

		case BTimedEventQueue::B_DATA_STATUS:
			out("ConsumerNode::HandleEvent B_DATA_STATUS\n");
			break;

		default:
			out("ConsumerNode::HandleEvent default\n");
			break;
	}
}


status_t
ConsumerNode::HandleMessage(int32 message, const void* data, size_t size)
{
	out("ConsumerNode::HandleMessage %lx\n", message);

	if (BBufferConsumer::HandleMessage(message, data, size) == B_OK
		|| BMediaEventLooper::HandleMessage(message, data, size) == B_OK)
		return B_OK;

	return BMediaNode::HandleMessage(message, data, size);
}


void
ConsumerNode::InitializeInput()
{
	out("ConsumerNode::InitializeInput()\n");
	fInput.source = media_source::null;
	fInput.destination.port = ControlPort();
	fInput.destination.id = 0;
	fInput.node = Node();
	fInput.format.type = B_MEDIA_RAW_AUDIO;
	fInput.format.u.raw_audio = media_raw_audio_format::wildcard;
	fInput.format.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	fInput.format.u.raw_audio.channel_count = 1;
	fInput.format.u.raw_audio.frame_rate = 44100;
	fInput.format.u.raw_audio.byte_order = B_HOST_IS_BENDIAN
		? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;
	strcpy(fInput.name, "this way in");
}
