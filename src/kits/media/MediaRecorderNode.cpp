/*
 * Copyright 2014, Dario Casalinuovo
 * Copyright 1999, Be Incorporated
 * All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */


#include "MediaRecorderNode.h"

#include <Buffer.h>
#include <scheduler.h>
#include <TimedEventQueue.h>
#include <TimeSource.h>

#include <MediaDebug.h>
#include <MediaRecorder.h>


BMediaRecorderNode::BMediaRecorderNode(const char* name,
	BMediaRecorder* recorder, media_type type)
	:
	BMediaNode(name),
	BMediaEventLooper(),
	BBufferConsumer(type),
	fRecorder(recorder)
{
	CALLED();

	fInput.destination.id = 1;
	fInput.destination.port = ControlPort();

	fName.SetTo(name);

	BString str(name);
	str << " Input";
	strcpy(fInput.name, str.String());
}


BMediaRecorderNode::~BMediaRecorderNode()
{
	CALLED();
}


BMediaAddOn*
BMediaRecorderNode::AddOn(int32* id) const
{
	CALLED();

	if (id)
		*id = -1;

	return NULL;
}


void
BMediaRecorderNode::NodeRegistered()
{
	CALLED();
	Run();
}


void
BMediaRecorderNode::SetRunMode(run_mode mode)
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
BMediaRecorderNode::SetAcceptedFormat(const media_format& format)
{
	CALLED();

	fOKFormat = format;
}


status_t
BMediaRecorderNode::GetInput(media_input* outInput)
{
	CALLED();

	fInput.node = Node();
	*outInput = fInput;

	return B_OK;
}


void
BMediaRecorderNode::SetDataEnabled(bool enabled)
{
	CALLED();

	int32 tag;

	SetOutputEnabled(fInput.source,
		fInput.destination, enabled, NULL, &tag);
}


void
BMediaRecorderNode::HandleEvent(const media_timed_event* event,
	bigtime_t lateness, bool realTimeEvent)
{
	CALLED();

	// we ignore them all!
}


void
BMediaRecorderNode::Start(bigtime_t performanceTime)
{
	CALLED();

	if (fRecorder->fNotifyHook)
		(*fRecorder->fNotifyHook)(fRecorder->fBufferCookie,
			BMediaRecorder::B_WILL_START, performanceTime);

	fRecorder->fRunning = true;
}


void
BMediaRecorderNode::Stop(bigtime_t performanceTime, bool immediate)
{
	CALLED();

	if (fRecorder->fNotifyHook)
		(*fRecorder->fNotifyHook)(fRecorder->fBufferCookie,
			BMediaRecorder::B_WILL_STOP, performanceTime, immediate);

	fRecorder->fRunning = false;
}


void
BMediaRecorderNode::Seek(bigtime_t mediaTime, bigtime_t performanceTime)
{
	CALLED();

	if (fRecorder->fNotifyHook)
		(*fRecorder->fNotifyHook)(fRecorder->fBufferCookie,
			BMediaRecorder::B_WILL_SEEK, performanceTime, mediaTime);
}


void
BMediaRecorderNode::TimeWarp(bigtime_t realTime, bigtime_t performanceTime)
{
	CALLED();

	// Since buffers will come pre-time-stamped, we only need to look
	// at them, so we can ignore the time warp as a consumer.
	if (fRecorder->fNotifyHook)
		(*fRecorder->fNotifyHook)(fRecorder->fBufferCookie,
			BMediaRecorder::B_WILL_TIMEWARP, realTime, performanceTime);
}


status_t
BMediaRecorderNode::HandleMessage(int32 message,
	const void* data, size_t size)
{
	CALLED();

	if (BBufferConsumer::HandleMessage(message, data, size) < 0
		&& BMediaEventLooper::HandleMessage(message, data, size) < 0
		&& BMediaNode::HandleMessage(message, data, size) < 0) {
		HandleBadMessage(message, data, size);
		return B_ERROR;
	}
	return B_OK;
}


status_t
BMediaRecorderNode::AcceptFormat(const media_destination& dest,
	media_format* format)
{
	CALLED();

	if (format_is_compatible(*format, fOKFormat))
		return B_OK;

	*format = fOKFormat;

	return B_MEDIA_BAD_FORMAT;
}


status_t
BMediaRecorderNode::GetNextInput(int32* cookie, media_input* outInput)
{
	CALLED();

	if (*cookie == 0) {
		*cookie = -1;
		*outInput = fInput;
		return B_OK;
	}

	return B_BAD_INDEX;
}


void
BMediaRecorderNode::DisposeInputCookie(int32 cookie)
{
	CALLED();
}


void
BMediaRecorderNode::BufferReceived(BBuffer* buffer)
{
	CALLED();

	fRecorder->BufferReceived(buffer->Data(), buffer->SizeUsed(),
		*buffer->Header());

	buffer->Recycle();
}


void
BMediaRecorderNode::ProducerDataStatus(
	const media_destination& forWhom, int32 status,
	bigtime_t performanceTime)
{
	CALLED();
}


status_t
BMediaRecorderNode::GetLatencyFor(const media_destination& forWhom,
	bigtime_t* outLatency, media_node_id* outTimesource)
{
	CALLED();

	*outLatency = 0;
	*outTimesource = TimeSource()->ID();

	return B_OK;
}


status_t
BMediaRecorderNode::Connected(const media_source &producer,
	const media_destination &where, const media_format &withFormat,
	media_input* outInput)
{
	CALLED();

	fInput.source = producer;
	fInput.format = withFormat;
	*outInput = fInput;

	fRecorder->fConnected = true;
	fRecorder->fInput = fInput;

	return B_OK;
}


void
BMediaRecorderNode::Disconnected(const media_source& producer,
	const media_destination& where)
{
	CALLED();

	fInput.source = media_source::null;

	fRecorder->fConnected = false;

	fRecorder->fInput.format = fOKFormat;
}


status_t
BMediaRecorderNode::FormatChanged(const media_source& producer,
	const media_destination& consumer, int32 tag,
	const media_format& format)
{
	CALLED();

	if (!format_is_compatible(format, fOKFormat))
		return B_MEDIA_BAD_FORMAT;

	fInput.format = format;

	return B_OK;
}
