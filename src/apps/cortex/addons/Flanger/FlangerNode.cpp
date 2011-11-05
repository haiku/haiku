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


// FlangerNode.cpp
// e.moon 16jun99

#include "FlangerNode.h"

#include "AudioBuffer.h"
#include "SoundUtils.h"

#include <Buffer.h>
#include <BufferGroup.h>
#include <ByteOrder.h>
#include <Debug.h>
#include <ParameterWeb.h>
#include <TimeSource.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

// -------------------------------------------------------- //
// local helpers
// -------------------------------------------------------- //

float calc_sweep_delta(
	const media_raw_audio_format& format,
	float fRate);

float calc_sweep_base(
	const media_raw_audio_format& format,
	float fDelay, float fDepth);

float calc_sweep_factor(
	const media_raw_audio_format& format,
	float fDepth);

// -------------------------------------------------------- //
// constants
// -------------------------------------------------------- //

// input-ID symbols
enum input_id_t {
	ID_AUDIO_INPUT
};

// output-ID symbols
enum output_id_t {
	ID_AUDIO_MIX_OUTPUT
	//ID_AUDIO_WET_OUTPUT ...
};

// parameter ID
enum param_id_t {
	P_MIX_RATIO_LABEL		= 100,
	P_MIX_RATIO,

	P_SWEEP_RATE_LABEL	= 200,
	P_SWEEP_RATE,

	P_DELAY_LABEL				= 300,
	P_DELAY,

	P_DEPTH_LABEL				= 400,
	P_DEPTH,

	P_FEEDBACK_LABEL		= 500,
	P_FEEDBACK
};

const float FlangerNode::s_fMaxDelay = 100.0;
const char* const FlangerNode::s_nodeName = "FlangerNode";


// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

FlangerNode::~FlangerNode() {
	// shut down
	Quit();

	// free delay buffer
	if(m_pDelayBuffer)
		delete m_pDelayBuffer;
}

FlangerNode::FlangerNode(BMediaAddOn* pAddOn) :
	// * init base classes
	BMediaNode(s_nodeName), // (virtual base)
	BBufferConsumer(B_MEDIA_RAW_AUDIO),
	BBufferProducer(B_MEDIA_RAW_AUDIO),
	BControllable(),
	BMediaEventLooper(),

	// * init connection state
	m_outputEnabled(true),
	m_downstreamLatency(0),
	m_processingLatency(0),

	// * init filter state
	m_pDelayBuffer(0),

	// * init add-on stuff
	m_pAddOn(pAddOn) {

//	PRINT((
//		"\n"
//		"--*-- FlangerNode() [%s] --*--\n\n",
//		__BUILD_DATE));
//
	// the rest of the initialization happens in NodeRegistered().
}


// -------------------------------------------------------- //
// *** BMediaNode
// -------------------------------------------------------- //

status_t FlangerNode::HandleMessage(
	int32 code,
	const void* pData,
	size_t size) {

	// pass off to each base class
	if(
		BBufferConsumer::HandleMessage(code, pData, size) &&
		BBufferProducer::HandleMessage(code, pData, size) &&
		BControllable::HandleMessage(code, pData, size) &&
		BMediaNode::HandleMessage(code, pData, size))
		BMediaNode::HandleBadMessage(code, pData, size);

	// +++++ return error on bad message?
	return B_OK;
}

BMediaAddOn* FlangerNode::AddOn(
	int32* poID) const {

	if(m_pAddOn)
		*poID = 0;
	return m_pAddOn;
}

void FlangerNode::SetRunMode(
	run_mode mode) {

	// disallow offline mode for now
	// +++++
	if(mode == B_OFFLINE)
		ReportError(B_NODE_FAILED_SET_RUN_MODE);

	// +++++ any other work to do?

	// hand off
	BMediaEventLooper::SetRunMode(mode);
}

// -------------------------------------------------------- //
// *** BMediaEventLooper
// -------------------------------------------------------- //

void FlangerNode::HandleEvent(
	const media_timed_event* pEvent,
	bigtime_t howLate,
	bool realTimeEvent) {

	ASSERT(pEvent);

	switch(pEvent->type) {
		case BTimedEventQueue::B_PARAMETER:
			handleParameterEvent(pEvent);
			break;

		case BTimedEventQueue::B_START:
			handleStartEvent(pEvent);
			break;

		case BTimedEventQueue::B_STOP:
			handleStopEvent(pEvent);
			break;

		default:
			ignoreEvent(pEvent);
			break;
	}
}

// "The Media Server calls this hook function after the node has
//  been registered.  This is derived from BMediaNode; BMediaEventLooper
//  implements it to call Run() automatically when the node is registered;
//  if you implement NodeRegistered() you should call through to
//  BMediaEventLooper::NodeRegistered() after you've done your custom
//  operations."

void FlangerNode::NodeRegistered() {

	PRINT(("FlangerNode::NodeRegistered()\n"));

	// Start the BMediaEventLooper thread
	SetPriority(B_REAL_TIME_PRIORITY);
	Run();

	// figure preferred ('template') format
	m_preferredFormat.type = B_MEDIA_RAW_AUDIO;
	getPreferredFormat(m_preferredFormat);

	// initialize current format
	m_format.type = B_MEDIA_RAW_AUDIO;
	m_format.u.raw_audio = media_raw_audio_format::wildcard;

	// init input
	m_input.destination.port = ControlPort();
	m_input.destination.id = ID_AUDIO_INPUT;
	m_input.node = Node();
	m_input.source = media_source::null;
	m_input.format = m_format;
	strncpy(m_input.name, "Audio Input", B_MEDIA_NAME_LENGTH);

	// init output
	m_output.source.port = ControlPort();
	m_output.source.id = ID_AUDIO_MIX_OUTPUT;
	m_output.node = Node();
	m_output.destination = media_destination::null;
	m_output.format = m_format;
	strncpy(m_output.name, "Mix Output", B_MEDIA_NAME_LENGTH);

	// init parameters
	initParameterValues();
	initParameterWeb();
}

// "Augment OfflineTime() to compute the node's current time; it's called
//  by the Media Kit when it's in offline mode. Update any appropriate
//  internal information as well, then call through to the BMediaEventLooper
//  implementation."

bigtime_t FlangerNode::OfflineTime() {
	// +++++
	return 0LL;
}

// -------------------------------------------------------- //
// *** BBufferConsumer
// -------------------------------------------------------- //

status_t FlangerNode::AcceptFormat(
	const media_destination& destination,
	media_format* pioFormat) {

	PRINT(("FlangerNode::AcceptFormat()\n"));

	// sanity checks
	if(destination != m_input.destination) {
		PRINT(("\tbad destination\n"));
		return B_MEDIA_BAD_DESTINATION;
	}
	if(pioFormat->type != B_MEDIA_RAW_AUDIO) {
		PRINT(("\tnot B_MEDIA_RAW_AUDIO\n"));
		return B_MEDIA_BAD_FORMAT;
	}

	validateProposedFormat(
		(m_format.u.raw_audio.format != media_raw_audio_format::wildcard.format) ?
			m_format : m_preferredFormat,
		*pioFormat);
	return B_OK;
}

// "If you're writing a node, and receive a buffer with the B_SMALL_BUFFER
//  flag set, you must recycle the buffer before returning."

void FlangerNode::BufferReceived(
	BBuffer* pBuffer) {
	ASSERT(pBuffer);

	// check buffer destination
	if(pBuffer->Header()->destination !=
		m_input.destination.id) {
		PRINT(("FlangerNode::BufferReceived():\n"
			"\tBad destination.\n"));
		pBuffer->Recycle();
		return;
	}

	if(pBuffer->Header()->time_source != TimeSource()->ID()) {
		PRINT(("* timesource mismatch\n"));
	}

	// check output
	if(m_output.destination == media_destination::null ||
		!m_outputEnabled) {
		pBuffer->Recycle();
		return;
	}

	// process and retransmit buffer
	filterBuffer(pBuffer);

	status_t err = SendBuffer(pBuffer, m_output.source, m_output.destination);
	if (err < B_OK) {
		PRINT(("FlangerNode::BufferReceived():\n"
			"\tSendBuffer() failed: %s\n", strerror(err)));
		pBuffer->Recycle();
	}
	// sent!
}

// * make sure to fill in poInput->format with the contents of
//   pFormat; as of R4.5 the Media Kit passes poInput->format to
//   the producer in BBufferProducer::Connect().

status_t
FlangerNode::Connected(const media_source& source,
	const media_destination& destination, const media_format& format,
	media_input* poInput)
{
	PRINT(("FlangerNode::Connected()\n"
		"\tto source %ld\n", source.id));

	// sanity check
	if(destination != m_input.destination) {
		PRINT(("\tbad destination\n"));
		return B_MEDIA_BAD_DESTINATION;
	}
	if(m_input.source != media_source::null) {
		PRINT(("\talready connected\n"));
		return B_MEDIA_ALREADY_CONNECTED;
	}

	// initialize input
	m_input.source = source;
	m_input.format = format;
	*poInput = m_input;

	// store format (this now constrains the output format)
	m_format = format;

	return B_OK;
}

void FlangerNode::Disconnected(
	const media_source& source,
	const media_destination& destination) {

	PRINT(("FlangerNode::Disconnected()\n"));

	// sanity checks
	if(m_input.source != source) {
		PRINT(("\tsource mismatch: expected ID %ld, got %ld\n",
			m_input.source.id, source.id));
		return;
	}
	if(destination != m_input.destination) {
		PRINT(("\tdestination mismatch: expected ID %ld, got %ld\n",
			m_input.destination.id, destination.id));
		return;
	}

	// mark disconnected
	m_input.source = media_source::null;

	// no output? clear format:
	if(m_output.destination == media_destination::null) {
		m_format.u.raw_audio = media_raw_audio_format::wildcard;
	}

	m_input.format = m_format;

}

void FlangerNode::DisposeInputCookie(
	int32 cookie) {}

// "You should implement this function so your node will know that the data
//  format is going to change. Note that this may be called in response to
//  your AcceptFormat() call, if your AcceptFormat() call alters any wildcard
//  fields in the specified format.
//
//  Because FormatChanged() is called by the producer, you don't need to (and
//  shouldn't) ask it if the new format is acceptable.
//
//  If the format change isn't possible, return an appropriate error from
//  FormatChanged(); this error will be passed back to the producer that
//  initiated the new format negotiation in the first place."

status_t FlangerNode::FormatChanged(
	const media_source& source,
	const media_destination& destination,
	int32 changeTag,
	const media_format& newFormat) {

	// flat-out deny format changes
	return B_MEDIA_BAD_FORMAT;
}

status_t FlangerNode::GetLatencyFor(
	const media_destination& destination,
	bigtime_t* poLatency,
	media_node_id* poTimeSource) {

	PRINT(("FlangerNode::GetLatencyFor()\n"));

	// sanity check
	if(destination != m_input.destination) {
		PRINT(("\tbad destination\n"));
		return B_MEDIA_BAD_DESTINATION;
	}

	*poLatency = m_downstreamLatency + m_processingLatency;
	PRINT(("\treturning %Ld\n", *poLatency));
	*poTimeSource = TimeSource()->ID();
	return B_OK;
}

status_t FlangerNode::GetNextInput(
	int32* pioCookie,
	media_input* poInput) {

	if(*pioCookie)
		return B_BAD_INDEX;

	++*pioCookie;
	*poInput = m_input;
	return B_OK;
}

void FlangerNode::ProducerDataStatus(
	const media_destination& destination,
	int32 status,
	bigtime_t tpWhen) {

	PRINT(("FlangerNode::ProducerDataStatus()\n"));

	// sanity check
	if(destination != m_input.destination) {
		PRINT(("\tbad destination\n"));
	}

	if(m_output.destination != media_destination::null) {
		// pass status downstream
		status_t err = SendDataStatus(
			status,
			m_output.destination,
			tpWhen);
		if(err < B_OK) {
			PRINT(("\tSendDataStatus(): %s\n", strerror(err)));
		}
	}
}

// "This function is provided to aid in supporting media formats in which the
//  outer encapsulation layer doesn't supply timing information. Producers will
//  tag the buffers they generate with seek tags; these tags can be used to
//  locate key frames in the media data."

status_t FlangerNode::SeekTagRequested(
	const media_destination& destination,
	bigtime_t targetTime,
	uint32 flags,
	media_seek_tag* poSeekTag,
	bigtime_t* poTaggedTime,
	uint32* poFlags) {

	PRINT(("FlangerNode::SeekTagRequested()\n"
		"\tNot implemented.\n"));
	return B_ERROR;
}

// -------------------------------------------------------- //
// *** BBufferProducer
// -------------------------------------------------------- //

// "When a consumer calls BBufferConsumer::RequestAdditionalBuffer(), this
//  function is called as a result. Its job is to call SendBuffer() to
//  immediately send the next buffer to the consumer. The previousBufferID,
//  previousTime, and previousTag arguments identify the last buffer the
//  consumer received. Your node should respond by sending the next buffer
//  after the one described.
//
//  The previousTag may be NULL.
//  Return B_OK if all is well; otherwise return an appropriate error code."

void FlangerNode::AdditionalBufferRequested(
	const media_source& source,
	media_buffer_id previousBufferID,
	bigtime_t previousTime,
	const media_seek_tag* pPreviousTag) {

	PRINT(("FlangerNode::AdditionalBufferRequested\n"
		"\tOffline mode not implemented."));
}

void FlangerNode::Connect(
	status_t status,
	const media_source& source,
	const media_destination& destination,
	const media_format& format,
	char* pioName) {

	PRINT(("FlangerNode::Connect()\n"));
	status_t err;

	// connection failed?
	if(status < B_OK) {
		PRINT(("\tStatus: %s\n", strerror(status)));
		// 'unreserve' the output
		m_output.destination = media_destination::null;
		return;
	}

	// connection established:
	strncpy(pioName, m_output.name, B_MEDIA_NAME_LENGTH);
	m_output.destination = destination;
	m_format = format;

	// figure downstream latency
	media_node_id timeSource;
	err = FindLatencyFor(m_output.destination, &m_downstreamLatency, &timeSource);
	if(err < B_OK) {
		PRINT(("\t!!! FindLatencyFor(): %s\n", strerror(err)));
	}
	PRINT(("\tdownstream latency = %Ld\n", m_downstreamLatency));

	// prepare the filter
	initFilter();

	// figure processing time
	m_processingLatency = calcProcessingLatency();
	PRINT(("\tprocessing latency = %Ld\n", m_processingLatency));

	// store summed latency
	SetEventLatency(m_downstreamLatency + m_processingLatency);

	if(m_input.source != media_source::null) {
		// pass new latency upstream
		err = SendLatencyChange(
			m_input.source,
			m_input.destination,
			EventLatency() + SchedulingLatency());
		if(err < B_OK)
			PRINT(("\t!!! SendLatencyChange(): %s\n", strerror(err)));
	}

	// cache buffer duration
	SetBufferDuration(
		buffer_duration(
			m_format.u.raw_audio));
}

void FlangerNode::Disconnect(
	const media_source& source,
	const media_destination& destination) {

	PRINT(("FlangerNode::Disconnect()\n"));

	// sanity checks
	if(source != m_output.source) {
		PRINT(("\tbad source\n"));
		return;
	}
	if(destination != m_output.destination) {
		PRINT(("\tbad destination\n"));
		return;
	}

	// clean up
	m_output.destination = media_destination::null;

	// no input? clear format:
	if(m_input.source == media_source::null) {
		m_format.u.raw_audio = media_raw_audio_format::wildcard;
	}

	m_output.format = m_format;

	// +++++ other cleanup goes here
}

status_t FlangerNode::DisposeOutputCookie(
	int32 cookie) {
	return B_OK;
}

void FlangerNode::EnableOutput(
	const media_source& source,
	bool enabled,
	int32* _deprecated_) {
	PRINT(("FlangerNode::EnableOutput()\n"));
	if(source != m_output.source) {
		PRINT(("\tbad source\n"));
		return;
	}

	m_outputEnabled = enabled;
}

status_t FlangerNode::FormatChangeRequested(
	const media_source& source,
	const media_destination& destination,
	media_format* pioFormat,
	int32* _deprecated_) {

	// deny
	PRINT(("FlangerNode::FormatChangeRequested()\n"
		"\tNot supported.\n"));

	return B_MEDIA_BAD_FORMAT;
}

status_t FlangerNode::FormatProposal(
	const media_source& source,
	media_format* pioFormat) {

	PRINT(("FlangerNode::FormatProposal()\n"));

	if(source != m_output.source) {
		PRINT(("\tbad source\n"));
		return B_MEDIA_BAD_SOURCE;
	}

	if(pioFormat->type != B_MEDIA_RAW_AUDIO) {
		PRINT(("\tbad type\n"));
		return B_MEDIA_BAD_FORMAT;
	}

	validateProposedFormat(
		(m_format.u.raw_audio.format != media_raw_audio_format::wildcard.format) ?
			m_format :
			m_preferredFormat,
		*pioFormat);
	return B_OK;
}

status_t FlangerNode::FormatSuggestionRequested(
	media_type type,
	int32 quality,
	media_format* poFormat) {

	PRINT(("FlangerNode::FormatSuggestionRequested()\n"));
	if(type != B_MEDIA_RAW_AUDIO) {
		PRINT(("\tbad type\n"));
		return B_MEDIA_BAD_FORMAT;
	}

	if(m_format.u.raw_audio.format != media_raw_audio_format::wildcard.format)
		*poFormat = m_format;
	else
		*poFormat = m_preferredFormat;
	return B_OK;
}

status_t FlangerNode::GetLatency(
	bigtime_t* poLatency) {

	PRINT(("FlangerNode::GetLatency()\n"));
	*poLatency = EventLatency() + SchedulingLatency();
	PRINT(("\treturning %Ld\n", *poLatency));

	return B_OK;
}

status_t FlangerNode::GetNextOutput(
	int32* pioCookie,
	media_output* poOutput) {

	if(*pioCookie)
		return B_BAD_INDEX;

	++*pioCookie;
	*poOutput = m_output;

	return B_OK;
}

// "This hook function is called when a BBufferConsumer that's receiving data
//  from you determines that its latency has changed. It will call its
//  BBufferConsumer::SendLatencyChange() function, and in response, the Media
//  Server will call your LatencyChanged() function.  The source argument
//  indicates your output that's involved in the connection, and destination
//  specifies the input on the consumer to which the connection is linked.
//  newLatency is the consumer's new latency. The flags are currently unused."
void FlangerNode::LatencyChanged(
	const media_source& source,
	const media_destination& destination,
	bigtime_t newLatency,
	uint32 flags) {

	PRINT(("FlangerNode::LatencyChanged()\n"));

	if(source != m_output.source) {
		PRINT(("\tBad source.\n"));
		return;
	}
	if(destination != m_output.destination) {
		PRINT(("\tBad destination.\n"));
		return;
	}

	m_downstreamLatency = newLatency;
	SetEventLatency(m_downstreamLatency + m_processingLatency);

	if(m_input.source != media_source::null) {
		// pass new latency upstream
		status_t err = SendLatencyChange(
			m_input.source,
			m_input.destination,
			EventLatency() + SchedulingLatency());
		if(err < B_OK)
			PRINT(("\t!!! SendLatencyChange(): %s\n", strerror(err)));
	}
}

void FlangerNode::LateNoticeReceived(
	const media_source& source,
	bigtime_t howLate,
	bigtime_t tpWhen) {

	PRINT(("FlangerNode::LateNoticeReceived()\n"
		"\thowLate == %Ld\n"
		"\twhen    == %Ld\n", howLate, tpWhen));

	if(source != m_output.source) {
		PRINT(("\tBad source.\n"));
		return;
	}

	if(m_input.source == media_source::null) {
		PRINT(("\t!!! No input to blame.\n"));
		return;
	}

	// +++++ check run mode?

	// pass the buck, since this node doesn't schedule buffer
	// production
	NotifyLateProducer(
		m_input.source,
		howLate,
		tpWhen);
}

// PrepareToConnect() is the second stage of format negotiations that happens
// inside BMediaRoster::Connect().  At this point, the consumer's AcceptFormat()
// method has been called, and that node has potentially changed the proposed
// format.  It may also have left wildcards in the format.  PrepareToConnect()
// *must* fully specialize the format before returning!

status_t FlangerNode::PrepareToConnect(
	const media_source& source,
	const media_destination& destination,
	media_format* pioFormat,
	media_source* poSource,
	char* poName) {

	char formatStr[256];
	string_for_format(*pioFormat, formatStr, 255);
	PRINT(("FlangerNode::PrepareToConnect()\n"
		"\tproposed format: %s\n", formatStr));

	if(source != m_output.source) {
		PRINT(("\tBad source.\n"));
		return B_MEDIA_BAD_SOURCE;
	}
	if(m_output.destination != media_destination::null) {
		PRINT(("\tAlready connected.\n"));
		return B_MEDIA_ALREADY_CONNECTED;
	}

	if(pioFormat->type != B_MEDIA_RAW_AUDIO) {
		PRINT(("\tBad format type.\n"));
		return B_MEDIA_BAD_FORMAT;
	}

	// do a final validity check:
	status_t err = validateProposedFormat(
		(m_format.u.raw_audio.format != media_raw_audio_format::wildcard.format) ?
			m_format :
			m_preferredFormat,
		*pioFormat);

	if(err < B_OK) {
		// no go
		return err;
	}

	// fill in wildcards
	specializeOutputFormat(*pioFormat);

	// reserve the output
	m_output.destination = destination;
	m_output.format = *pioFormat;

	// pass back source & output name
	*poSource = m_output.source;
	strncpy(poName, m_output.name, B_MEDIA_NAME_LENGTH);

	return B_OK;
}

status_t FlangerNode::SetBufferGroup(
	const media_source& source,
	BBufferGroup* pGroup) {

	PRINT(("FlangerNode::SetBufferGroup()\n"));
	if(source != m_output.source) {
		PRINT(("\tBad source.\n"));
		return B_MEDIA_BAD_SOURCE;
	}

	if(m_input.source == media_source::null) {
		PRINT(("\tNo producer to send buffers to.\n"));
		return B_ERROR;
	}

	// +++++ is this right?  buffer-group selection gets
	//       all asynchronous and weird...
	int32 changeTag;
	return SetOutputBuffersFor(
		m_input.source,
		m_input.destination,
		pGroup,
		0, &changeTag);
}

status_t FlangerNode::SetPlayRate(
	int32 numerator,
	int32 denominator) {
	// not supported
	return B_ERROR;
}

status_t FlangerNode::VideoClippingChanged(
	const media_source& source,
	int16 numShorts,
	int16* pClipData,
	const media_video_display_info& display,
	int32* poFromChangeTag) {
	// not sane
	return B_ERROR;
}

// -------------------------------------------------------- //
// *** BControllable
// -------------------------------------------------------- //

status_t FlangerNode::GetParameterValue(
	int32 id,
	bigtime_t* poLastChangeTime,
	void* poValue,
	size_t* pioSize) {

//	PRINT(("FlangerNode::GetParameterValue()\n"));

	// all parameters are floats
	if(*pioSize < sizeof(float)) {
		return B_NO_MEMORY;
	}

	*pioSize = sizeof(float);
	switch(id) {
		case P_MIX_RATIO:
			*(float*)poValue = m_fMixRatio;
			*poLastChangeTime = m_tpMixRatioChanged;
			break;

		case P_SWEEP_RATE:
			*(float*)poValue = m_fSweepRate;
			*poLastChangeTime = m_tpSweepRateChanged;
			break;

		case P_DELAY:
			*(float*)poValue = m_fDelay;
			*poLastChangeTime = m_tpDelayChanged;
			break;

		case P_DEPTH:
			*(float*)poValue = m_fDepth;
			*poLastChangeTime = m_tpDepthChanged;
			break;

		case P_FEEDBACK:
			*(float*)poValue = m_fFeedback;
			*poLastChangeTime = m_tpFeedbackChanged;
			break;

		default:
			return B_ERROR;
	}

	return B_OK;
}

void FlangerNode::SetParameterValue(
	int32 id,
	bigtime_t changeTime,
	const void* pValue,
	size_t size) {

	switch(id) {
		case P_MIX_RATIO:
		case P_SWEEP_RATE:
		case P_DELAY:
		case P_DEPTH:
		case P_FEEDBACK: {
			if(size < sizeof(float))
				break;

//      this is from ToneProducer.  it's fishy.
//			if(size > sizeof(float))
//				size = sizeof(float);

			media_timed_event ev(
				changeTime,
				BTimedEventQueue::B_PARAMETER,
				0,
				BTimedEventQueue::B_NO_CLEANUP,
				size,
				id,
				(char*)pValue, size);
			EventQueue()->AddEvent(ev);
			break;
		}
	}
}

// -------------------------------------------------------- //
// *** HandleEvent() impl
// -------------------------------------------------------- //

void FlangerNode::handleParameterEvent(
	const media_timed_event* pEvent) {

	float value = *(float*)pEvent->user_data;
	int32 id = pEvent->bigdata;
	size_t size = pEvent->data;
	bigtime_t now = TimeSource()->Now();

	switch(id) {
		case P_MIX_RATIO:
			if(value == m_fMixRatio)
				break;

			// set
			m_fMixRatio = value;
			m_tpMixRatioChanged = now;
			// broadcast
			BroadcastNewParameterValue(
				now,
				id,
				&m_fMixRatio,
				size);
			break;

		case P_SWEEP_RATE:
			if(value == m_fSweepRate)
				break;

			// set
			m_fSweepRate = value;
			m_tpSweepRateChanged = now;

			if(m_output.destination != media_destination::null) {
				m_fThetaInc = calc_sweep_delta(
					m_format.u.raw_audio,
					m_fSweepRate);
			}

			// broadcast
			BroadcastNewParameterValue(
				now,
				id,
				&m_fSweepRate,
				size);
			break;

		case P_DELAY:
			if(value == m_fDelay)
				break;

			// set
			m_fDelay = value;
			m_tpDelayChanged = now;

			if(m_output.destination != media_destination::null) {
				m_fSweepBase = calc_sweep_base(
					m_format.u.raw_audio,
					m_fDelay, m_fDepth);
			}

			// broadcast
			BroadcastNewParameterValue(
				now,
				id,
				&m_fDelay,
				size);
			break;

		case P_DEPTH:
			if(value == m_fDepth)
				break;

			// set
			m_fDepth = value;
			m_tpDepthChanged = now;

			if(m_output.destination != media_destination::null) {
				m_fSweepBase = calc_sweep_base(
					m_format.u.raw_audio,
					m_fDelay, m_fDepth);
				m_fSweepFactor = calc_sweep_factor(
					m_format.u.raw_audio,
					m_fDepth);
			}

			// broadcast
			BroadcastNewParameterValue(
				now,
				id,
				&m_fDepth,
				size);
			break;

		case P_FEEDBACK:
			if(value == m_fFeedback)
				break;

			// set
			m_fFeedback = value;
			m_tpFeedbackChanged = now;
			// broadcast
			BroadcastNewParameterValue(
				now,
				id,
				&m_fFeedback,
				size);
			break;
	}
}

void FlangerNode::handleStartEvent(
	const media_timed_event* pEvent) {
	PRINT(("FlangerNode::handleStartEvent\n"));

	startFilter();
}

void FlangerNode::handleStopEvent(
	const media_timed_event* pEvent) {
	PRINT(("FlangerNode::handleStopEvent\n"));

	stopFilter();
}

void FlangerNode::ignoreEvent(
	const media_timed_event* pEvent) {
	PRINT(("FlangerNode::ignoreEvent\n"));

}


// -------------------------------------------------------- //
// *** internal operations
// -------------------------------------------------------- //


// figure the preferred format: any fields left as wildcards
// are negotiable
void FlangerNode::getPreferredFormat(
	media_format& ioFormat) {
	ASSERT(ioFormat.type == B_MEDIA_RAW_AUDIO);

	ioFormat.u.raw_audio = media_raw_audio_format::wildcard;
	ioFormat.u.raw_audio.channel_count = 1;
	ioFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;

//	ioFormat.u.raw_audio.frame_rate = 44100.0;
//	ioFormat.u.raw_audio.buffer_size = 0x1000;
}

// test the given template format against a proposed format.
// specialize wildcards for fields where the template contains
// non-wildcard data; write required fields into proposed format
// if they mismatch.
// Returns B_OK if the proposed format doesn't conflict with the
// template, or B_MEDIA_BAD_FORMAT otherwise.

status_t FlangerNode::validateProposedFormat(
	const media_format& preferredFormat,
	media_format& ioProposedFormat) {

	char formatStr[256];
	PRINT(("FlangerNode::validateProposedFormat()\n"));

	ASSERT(preferredFormat.type == B_MEDIA_RAW_AUDIO);

	string_for_format(preferredFormat, formatStr, 255);
	PRINT(("\ttemplate format: %s\n", formatStr));

	string_for_format(ioProposedFormat, formatStr, 255);
	PRINT(("\tproposed format: %s\n", formatStr));

	status_t err = B_OK;

	if(ioProposedFormat.type != B_MEDIA_RAW_AUDIO) {
		// out of the ballpark
		ioProposedFormat = preferredFormat;
		return B_MEDIA_BAD_FORMAT;
	}

	// wildcard format
	media_raw_audio_format& wild = media_raw_audio_format::wildcard;
	// proposed format
	media_raw_audio_format& f = ioProposedFormat.u.raw_audio;
	// template format
	const media_raw_audio_format& pref = preferredFormat.u.raw_audio;

	if(pref.frame_rate != wild.frame_rate) {
		if(f.frame_rate != pref.frame_rate) {
			if(f.frame_rate != wild.frame_rate)
				err = B_MEDIA_BAD_FORMAT;
			f.frame_rate = pref.frame_rate;
		}
	}

	if(pref.channel_count != wild.channel_count) {
		if(f.channel_count != pref.channel_count) {
			if(f.channel_count != wild.channel_count)
				err = B_MEDIA_BAD_FORMAT;
			f.channel_count = pref.channel_count;
		}
	}

	if(pref.format != wild.format) {
		if(f.format != pref.format) {
			if(f.format != wild.format)
				err = B_MEDIA_BAD_FORMAT;
			f.format = pref.format;
		}
	}

	if(pref.byte_order != wild.byte_order) {
		if(f.byte_order != pref.byte_order) {
			if(f.byte_order != wild.byte_order)
				err = B_MEDIA_BAD_FORMAT;
			f.byte_order = pref.byte_order;
		}
	}

	if(pref.buffer_size != wild.buffer_size) {
		if(f.buffer_size != pref.buffer_size) {
			if(f.buffer_size != wild.buffer_size)
				err = B_MEDIA_BAD_FORMAT;
			f.buffer_size = pref.buffer_size;
		}
	}

	if(err != B_OK) {
		string_for_format(ioProposedFormat, formatStr, 255);
		PRINT((
			"\tformat conflict; suggesting:\n\tformat %s\n", formatStr));
	}

	return err;
}

// fill in wildcards in the given format.
// (assumes the format passes validateProposedFormat().)
void FlangerNode::specializeOutputFormat(
	media_format& ioFormat) {

	char formatStr[256];
	string_for_format(ioFormat, formatStr, 255);
	PRINT(("FlangerNode::specializeOutputFormat()\n"
		"\tinput format: %s\n", formatStr));

	ASSERT(ioFormat.type == B_MEDIA_RAW_AUDIO);

	// carpal_tunnel_paranoia
	media_raw_audio_format& f = ioFormat.u.raw_audio;
	media_raw_audio_format& w = media_raw_audio_format::wildcard;

	if(f.frame_rate == w.frame_rate)
		f.frame_rate = 44100.0;
	if(f.channel_count == w.channel_count) {
		//+++++ tweaked 15sep99
		if(m_input.source != media_source::null)
			f.channel_count = m_input.format.u.raw_audio.channel_count;
		else
			f.channel_count = 1;
	}
	if(f.format == w.format)
		f.format = media_raw_audio_format::B_AUDIO_FLOAT;
	if(f.byte_order == w.format)
		f.byte_order = (B_HOST_IS_BENDIAN) ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;
	if(f.buffer_size == w.buffer_size)
		f.buffer_size = 2048;

	string_for_format(ioFormat, formatStr, 255);
	PRINT(("\toutput format: %s\n", formatStr));
}

// set parameters to their default settings
void FlangerNode::initParameterValues() {
	m_fMixRatio = 0.5;
	m_tpMixRatioChanged = 0LL;

	m_fSweepRate = 0.1;
	m_tpSweepRateChanged = 0LL;

	m_fDelay = 10.0;
	m_tpDelayChanged = 0LL;

	m_fDepth = 25.0;
	m_tpDepthChanged = 0LL;

	m_fFeedback = 0.1;
	m_tpFeedbackChanged = 0LL;
}

// create and register a parameter web
void FlangerNode::initParameterWeb() {
	BParameterWeb* pWeb = new BParameterWeb();
	BParameterGroup* pTopGroup = pWeb->MakeGroup("FlangerNode Parameters");

	BNullParameter* label;
	BContinuousParameter* value;
	BParameterGroup* g;

	// mix ratio
	g = pTopGroup->MakeGroup("Mix ratio");
	label = g->MakeNullParameter(
		P_MIX_RATIO_LABEL,
		B_MEDIA_NO_TYPE,
		"Mix ratio",
		B_GENERIC);

	value = g->MakeContinuousParameter(
		P_MIX_RATIO,
		B_MEDIA_NO_TYPE,
		"",
		B_GAIN, "", 0.0, 1.0, 0.05);
	label->AddOutput(value);
	value->AddInput(label);

	// sweep rate
	g = pTopGroup->MakeGroup("Sweep rate");
	label = g->MakeNullParameter(
		P_SWEEP_RATE_LABEL,
		B_MEDIA_NO_TYPE,
		"Sweep rate",
		B_GENERIC);

	value = g->MakeContinuousParameter(
		P_SWEEP_RATE,
		B_MEDIA_NO_TYPE,
		"",
		B_GAIN, "Hz", 0.01, 10.0, 0.01);
	label->AddOutput(value);
	value->AddInput(label);

	// sweep range: minimum delay
	g = pTopGroup->MakeGroup("Delay");
	label = g->MakeNullParameter(
		P_DELAY_LABEL,
		B_MEDIA_NO_TYPE,
		"Delay",
		B_GENERIC);

	value = g->MakeContinuousParameter(
		P_DELAY,
		B_MEDIA_NO_TYPE,
		"",
		B_GAIN, "ms", 0.1, s_fMaxDelay/2.0, 0.1);
	label->AddOutput(value);
	value->AddInput(label);

	// sweep range: maximum
	g = pTopGroup->MakeGroup("Depth");
	label = g->MakeNullParameter(
		P_DEPTH_LABEL,
		B_MEDIA_NO_TYPE,
		"Depth",
		B_GENERIC);

	value = g->MakeContinuousParameter(
		P_DEPTH,
		B_MEDIA_NO_TYPE,
		"",
		B_GAIN, "ms", 1.0, s_fMaxDelay/4.0, 0.1);
	label->AddOutput(value);
	value->AddInput(label);

	// feedback
	g = pTopGroup->MakeGroup("Feedback");
	label = g->MakeNullParameter(
		P_FEEDBACK_LABEL,
		B_MEDIA_NO_TYPE,
		"Feedback",
		B_GENERIC);

	value = g->MakeContinuousParameter(
		P_FEEDBACK,
		B_MEDIA_NO_TYPE,
		"",
		B_GAIN, "", 0.0, 1.0, 0.01);
	label->AddOutput(value);
	value->AddInput(label);

	// * Install parameter web
	SetParameterWeb(pWeb);
}

// construct delay line if necessary, reset filter state
void FlangerNode::initFilter() {
	PRINT(("FlangerNode::initFilter()\n"));
	ASSERT(m_format.u.raw_audio.format != media_raw_audio_format::wildcard.format);

	if(!m_pDelayBuffer) {
		m_pDelayBuffer = new AudioBuffer(
			m_format.u.raw_audio,
			frames_for_duration(
				m_format.u.raw_audio,
				(bigtime_t)s_fMaxDelay*1000LL));
		m_pDelayBuffer->zero();
	}

	m_framesSent = 0;
	m_delayWriteFrame = 0;
	m_fTheta = 0.0;
	m_fThetaInc = calc_sweep_delta(m_format.u.raw_audio, m_fSweepRate);
	m_fSweepBase = calc_sweep_base(m_format.u.raw_audio, m_fDelay, m_fDepth);
	m_fSweepFactor = calc_sweep_factor(m_format.u.raw_audio, m_fDepth);

//
//	PRINT((
//		"\tFrames       %ld\n"
//		"\tDelay        %.2f\n"
//		"\tDepth        %.2f\n"
//		"\tSweepBase    %.2f\n"
//		"\tSweepFactor  %.2f\n",
//		m_pDelayBuffer->frames(),
//		m_fDelay, m_fDepth, m_fSweepBase, m_fSweepFactor));
}

void FlangerNode::startFilter() {
	PRINT(("FlangerNode::startFilter()\n"));
}
void FlangerNode::stopFilter() {
	PRINT(("FlangerNode::stopFilter()\n"));
}

// figure processing latency by doing 'dry runs' of filterBuffer()
bigtime_t FlangerNode::calcProcessingLatency() {
	PRINT(("FlangerNode::calcProcessingLatency()\n"));

	if(m_output.destination == media_destination::null) {
		PRINT(("\tNot connected.\n"));
		return 0LL;
	}

	// allocate a temporary buffer group
	BBufferGroup* pTestGroup = new BBufferGroup(
		m_output.format.u.raw_audio.buffer_size, 1);

	// fetch a buffer
	BBuffer* pBuffer = pTestGroup->RequestBuffer(
		m_output.format.u.raw_audio.buffer_size);
	ASSERT(pBuffer);

	pBuffer->Header()->type = B_MEDIA_RAW_AUDIO;
	pBuffer->Header()->size_used = m_output.format.u.raw_audio.buffer_size;

	// run the test
	bigtime_t preTest = system_time();
	filterBuffer(pBuffer);
	bigtime_t elapsed = system_time()-preTest;

	// clean up
	pBuffer->Recycle();
	delete pTestGroup;

	// reset filter state
	initFilter();

	return elapsed;
}

// filter buffer data in place
//
// +++++ add 2-channel support 15sep991
//

const size_t MAX_CHANNELS = 2;

struct _frame {
	float channel[MAX_CHANNELS];
};

void FlangerNode::filterBuffer(
	BBuffer* pBuffer) {

	if(!m_pDelayBuffer)
		return;

	// for each input frame:
	// - fetch
	// - write delay line(writeFrame)
	// - read delay line(writeFrame-readOffset)  [interpolate]
	// - mix (replace)
	// - advance writeFrame
	// - update readOffset

	AudioBuffer input(m_format.u.raw_audio, pBuffer);

	ASSERT(
		m_format.u.raw_audio.channel_count == 1 ||
		m_format.u.raw_audio.channel_count == 2);
	uint32 channels = m_format.u.raw_audio.channel_count;
	bool stereo = m_format.u.raw_audio.channel_count == 2;

	uint32 samples = input.frames() * channels;
	for(uint32 inPos = 0; inPos < samples; ++inPos) {

		// read from input buffer
		_frame inFrame;
		inFrame.channel[0] = ((float*)input.data())[inPos];
		if(stereo)
			inFrame.channel[1] = ((float*)input.data())[inPos + 1];

		// interpolate from delay buffer
		float readOffset = m_fSweepBase + (m_fSweepFactor * sin(m_fTheta));
		float fReadFrame = (float)m_delayWriteFrame - readOffset;
		if(fReadFrame < 0.0)
			fReadFrame += m_pDelayBuffer->frames();

//		float delayed;


		// read low-index (possibly only) frame
		_frame delayedFrame;

		int32 readFrameLo = (int32)floor(fReadFrame);
		uint32 pos = readFrameLo * channels;
		delayedFrame.channel[0] = ((float*)m_pDelayBuffer->data())[pos];
		if(stereo)
			delayedFrame.channel[1] = ((float*)m_pDelayBuffer->data())[pos+1];

		if(readFrameLo != (int32)fReadFrame) {

			// interpolate (A)
			uint32 readFrameHi = (int32)ceil(fReadFrame);
			delayedFrame.channel[0] *= ((float)readFrameHi - fReadFrame);
			if(stereo)
				delayedFrame.channel[1] *= ((float)readFrameHi - fReadFrame);

			// read high-index frame
			int32 hiWrap = (readFrameHi == m_pDelayBuffer->frames()) ? 0 : readFrameHi;
			ASSERT(hiWrap >= 0);
			pos = (uint32)hiWrap * channels;
			_frame hiFrame;
			hiFrame.channel[0] = ((float*)m_pDelayBuffer->data())[pos];
			if(stereo)
				hiFrame.channel[1] = ((float*)m_pDelayBuffer->data())[pos+1];

			// interpolate (B)
			delayedFrame.channel[0] +=
				hiFrame.channel[0] * (fReadFrame - (float)readFrameLo);
			if(stereo)
				delayedFrame.channel[1] +=
					hiFrame.channel[1] * (fReadFrame - (float)readFrameLo);
		}

		// mix back to output buffer
		((float*)input.data())[inPos] =
			(inFrame.channel[0] * (1.0-m_fMixRatio)) +
			(delayedFrame.channel[0] * m_fMixRatio);
		if(stereo)
			((float*)input.data())[inPos+1] =
				(inFrame.channel[1] * (1.0-m_fMixRatio)) +
				(delayedFrame.channel[1] * m_fMixRatio);

		// write to delay buffer
		uint32 delayWritePos = m_delayWriteFrame * channels;
		((float*)m_pDelayBuffer->data())[delayWritePos] =
			inFrame.channel[0] +
			(delayedFrame.channel[0] * m_fFeedback);
		if(stereo)
			((float*)m_pDelayBuffer->data())[delayWritePos+1] =
				inFrame.channel[1] +
				(delayedFrame.channel[1] * m_fFeedback);

		// advance write position
		if(++m_delayWriteFrame >= m_pDelayBuffer->frames())
			m_delayWriteFrame = 0;

		// advance read offset ('LFO')
		m_fTheta += m_fThetaInc;
		if(m_fTheta > 2 * M_PI)
			m_fTheta -= 2 * M_PI;

//		if(m_fDelayReadDelta < 0.0) {
//			if(m_fDelayReadOffset < m_fDelay)
//				m_fDelayReadDelta = -m_fDelayReadDelta;
//		} else {
//			if(m_fDelayReadOffset > m_fDepth)
//				m_fDelayReadDelta = -m_fDelayReadDelta;
//		}
//		m_fDelayReadOffset += m_fDelayReadDelta;
	}
}


/*!	Figure the rate at which the (radial) read offset changes,
	based on the given sweep rate (in Hz)
*/
float
calc_sweep_delta(const media_raw_audio_format& format, float fRate)
{
	return 2 * M_PI * fRate / format.frame_rate;
}

/*!	Figure the base delay (in frames) based on the given
	sweep delay/depth (in msec)
*/
float
calc_sweep_base(const media_raw_audio_format& format, float delay, float depth)
{
	return (format.frame_rate * (delay + depth)) / 1000.0;
}


float
calc_sweep_factor(const media_raw_audio_format& format, float depth)
{
	return (format.frame_rate * depth) / 1000.0;
}


// END -- FlangerNode.cpp --
