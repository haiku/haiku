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


// AudioFilterNode.cpp

#include "AudioFilterNode.h"

#include "AudioBuffer.h"
#include "IParameterSet.h"
#include "IAudioOpFactory.h"
#include "IAudioOp.h"
#include "SoundUtils.h"

#include <Buffer.h>
#include <BufferGroup.h>
#include <ByteOrder.h>
#include <ParameterWeb.h>
#include <String.h>
#include <TimeSource.h>


#include <cstdio>
#include <cstdlib>
#include <cstring>
//#include <cmath>

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
	
// -------------------------------------------------------- //
// *** HOOKS
// -------------------------------------------------------- //

// *** FORMAT NEGOTIATION

// requests the required format for the given type (ioFormat.type must be
// filled in!)
// upon returning, all fields must be filled in.
// Default:
// - raw_audio format:
//   float
//   44100hz
//   host-endian
//   1 channel
//   1k buffers

status_t AudioFilterNode::getPreferredInputFormat(
	media_format&								ioFormat) {
	return getPreferredOutputFormat(ioFormat);
}

status_t AudioFilterNode::getPreferredOutputFormat(
	media_format&								ioFormat) {

	if(ioFormat.type != B_MEDIA_RAW_AUDIO)
		return B_MEDIA_BAD_FORMAT;
	
	media_raw_audio_format& f = ioFormat.u.raw_audio;
	f.format = media_raw_audio_format::B_AUDIO_FLOAT;
	f.frame_rate = 44100.0;
	f.channel_count = 1;
	f.byte_order = B_MEDIA_HOST_ENDIAN; //(B_HOST_IS_BENDIAN) ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;
	f.buffer_size = 1024;

	return B_OK;	
}

// test the given template format against a proposed format.
// specialize wildcards for fields where the template contains
// non-wildcard data; write required fields into proposed format
// if they mismatch.
// Returns B_OK if the proposed format doesn't conflict with the
// template, or B_MEDIA_BAD_FORMAT otherwise.

status_t AudioFilterNode::_validate_raw_audio_format(
	const media_format&					preferredFormat,
	media_format&								ioProposedFormat) {

	char formatStr[256];
	PRINT(("AudioFilterNode::_validate_raw_audio_format()\n"));

	ASSERT(preferredFormat.type == B_MEDIA_RAW_AUDIO);
	
	string_for_format(preferredFormat, formatStr, 255);
	PRINT(("\ttemplate format: %s\n", formatStr));

	string_for_format(ioProposedFormat, formatStr, 255);
	PRINT(("\tincoming proposed format: %s\n", formatStr));
	
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
	else {
		string_for_format(ioProposedFormat, formatStr, 255);
		PRINT(("\toutbound proposed format: %s\n", formatStr));
	}
	
	return err;	
}

status_t AudioFilterNode::validateProposedInputFormat(
	const media_format&					preferredFormat,
	media_format&								ioProposedFormat) {
	
	return _validate_raw_audio_format(
		preferredFormat, ioProposedFormat);
}
	
status_t AudioFilterNode::validateProposedOutputFormat(
	const media_format&					preferredFormat,
	media_format&								ioProposedFormat) {
	
	return _validate_raw_audio_format(
		preferredFormat, ioProposedFormat);
}


void AudioFilterNode::_specialize_raw_audio_format(
	const media_format&					templateFormat,
	media_format&								ioFormat) {
	
	ASSERT(templateFormat.type == B_MEDIA_RAW_AUDIO);
	ASSERT(ioFormat.type == B_MEDIA_RAW_AUDIO);
	
	media_raw_audio_format& f = ioFormat.u.raw_audio;
	const media_raw_audio_format& p = templateFormat.u.raw_audio;
	const media_raw_audio_format& w = media_raw_audio_format::wildcard;

	if(f.format == w.format) {
		ASSERT(p.format);
		f.format = p.format;
	}

	if(f.channel_count == w.channel_count) {
		ASSERT(p.channel_count);
		f.channel_count = p.channel_count;
	}

	if(f.frame_rate == w.frame_rate) {
		ASSERT(p.frame_rate);
		f.frame_rate = p.frame_rate;
	}

	if(f.byte_order == w.byte_order) {
		ASSERT(p.byte_order);
		f.byte_order = p.byte_order;
	}

	if(f.buffer_size == w.buffer_size) {
		ASSERT(p.buffer_size);
		f.buffer_size = p.buffer_size;
	}
}

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

AudioFilterNode::~AudioFilterNode() {
	// shut down
	Quit();
	
	// clean up
	if(m_parameterSet) delete m_parameterSet;
	if(m_opFactory) delete m_opFactory;
	if(m_op) delete m_op;
}
	
// the node acquires ownership of opFactory and
AudioFilterNode::AudioFilterNode(
	const char*									name,
	IAudioOpFactory*						opFactory,
	BMediaAddOn*								addOn) :

	// * init base classes
	BMediaNode(name), // (virtual base)
	BBufferConsumer(B_MEDIA_RAW_AUDIO),
	BBufferProducer(B_MEDIA_RAW_AUDIO),
	BControllable(),
	BMediaEventLooper(),
	
	// * init connection state
	m_outputEnabled(true),
	m_downstreamLatency(0),
	m_processingLatency(0),
	m_bufferGroup(0),
	
	// * init parameter/operation components
	m_parameterSet(opFactory->createParameterSet()),
	m_opFactory(opFactory),
	m_op(0),
	
	// * init add-on if any
	m_addOn(addOn) {

	ASSERT(m_opFactory);
	ASSERT(m_parameterSet);
	
	PRINT((
		"AudioFilterNode::AudioFilterNode()\n"));

	// the rest of the initialization happens in NodeRegistered().
}

// -------------------------------------------------------- //
// *** BMediaNode
// -------------------------------------------------------- //

status_t AudioFilterNode::HandleMessage(
	int32												code,
	const void*									data,
	size_t											size) {

	// pass off to each base class
	if(
		BBufferConsumer::HandleMessage(code, data, size) &&
		BBufferProducer::HandleMessage(code, data, size) &&
		BControllable::HandleMessage(code, data, size) &&
		BMediaNode::HandleMessage(code, data, size))
		BMediaNode::HandleBadMessage(code, data, size);
	
	// +++++ return error on bad message?
	return B_OK;
}

BMediaAddOn* AudioFilterNode::AddOn(
	int32*											outID) const {

	if(m_addOn)
		*outID = 0;
	return m_addOn;
}

void AudioFilterNode::SetRunMode(
	run_mode										mode) {

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

void AudioFilterNode::HandleEvent(
	const media_timed_event*		event,
	bigtime_t										howLate,
	bool												realTimeEvent) {

	ASSERT(event);
	
	switch(event->type) {
		case BTimedEventQueue::B_PARAMETER:
			handleParameterEvent(event);
			break;
			
		case BTimedEventQueue::B_START:
			handleStartEvent(event);
			break;
			
		case BTimedEventQueue::B_STOP:
			handleStopEvent(event);
			break;
			
		default:
			ignoreEvent(event);
			break;
	}
}

// "The Media Server calls this hook function after the node has
//  been registered.  This is derived from BMediaNode; BMediaEventLooper
//  implements it to call Run() automatically when the node is registered;
//  if you implement NodeRegistered() you should call through to
//  BMediaEventLooper::NodeRegistered() after you've done your custom 
//  operations."

void AudioFilterNode::NodeRegistered() {

	PRINT(("AudioFilterNode::NodeRegistered()\n"));
	status_t err;
		
	// Start the BMediaEventLooper thread
	SetPriority(B_REAL_TIME_PRIORITY);
	Run();

	// init input
	m_input.destination.port = ControlPort();
	m_input.destination.id = ID_AUDIO_INPUT;
	m_input.node = Node();
	m_input.source = media_source::null;

	m_input.format.type = B_MEDIA_RAW_AUDIO;
	err = getRequiredInputFormat(m_input.format);
	ASSERT(err == B_OK);

	strncpy(m_input.name, "Audio Input", B_MEDIA_NAME_LENGTH);
	
	// init output
	m_output.source.port = ControlPort();
	m_output.source.id = ID_AUDIO_MIX_OUTPUT;
	m_output.node = Node();
	m_output.destination = media_destination::null;

	m_output.format.type = B_MEDIA_RAW_AUDIO;
	err = getRequiredOutputFormat(m_output.format);
	ASSERT(err == B_OK);

	strncpy(m_output.name, "Audio Output", B_MEDIA_NAME_LENGTH);

	// init parameters
	initParameterWeb();
}
	
// "Augment OfflineTime() to compute the node's current time; it's called
//  by the Media Kit when it's in offline mode. Update any appropriate
//  internal information as well, then call through to the BMediaEventLooper
//  implementation."

bigtime_t AudioFilterNode::OfflineTime() {
	// +++++ offline mode not implemented +++++
	return 0LL;
}


// -------------------------------------------------------- //
// *** BBufferConsumer
// -------------------------------------------------------- //

status_t AudioFilterNode::AcceptFormat(
	const media_destination&		destination,
	media_format*								ioFormat) {

	PRINT(("AudioFilterNode::AcceptFormat()\n"));
	status_t err;
	
	// sanity checks
	if(destination != m_input.destination) {
		PRINT(("\tbad destination\n"));
		return B_MEDIA_BAD_DESTINATION;
	}
	if(ioFormat->type != B_MEDIA_RAW_AUDIO) {
		PRINT(("\tnot B_MEDIA_RAW_AUDIO\n"));
		return B_MEDIA_BAD_FORMAT;
	}

	media_format required;
	required.type = B_MEDIA_RAW_AUDIO;
	err = getRequiredInputFormat(required);
	ASSERT(err == B_OK);

//	// attempt to create op? +++++  
//
//	// validate against current input/output format for now
//	validateProposedFormat(
//		(m_format.u.raw_audio.format != media_raw_audio_format::wildcard.format) ?
//			m_format : preferred,
//		*ioFormat);

	// validate against required format
	err = validateProposedInputFormat(required, *ioFormat);
	if(err < B_OK)
		return err;
		
	// if an output connection has been made, try to create an operation
	if(m_output.destination != media_destination::null) {
		ASSERT(m_opFactory);
		IAudioOp* op = m_opFactory->createOp(
			this,
			ioFormat->u.raw_audio,
			m_output.format.u.raw_audio);

		if(!op) {
			// format passed validation, but factory failed to provide a
			// capable operation object
			char fmt[256];
			string_for_format(*ioFormat, fmt, 255);
			PRINT((
				"*** AcceptFormat(): format validated, but no operation found:\n"
				"    %s\n",
				fmt));

			return B_MEDIA_BAD_FORMAT;
		}
		// clean up
		delete op;
	}
	
	// format passed inspection
	return B_OK;
}
	
// "If you're writing a node, and receive a buffer with the B_SMALL_BUFFER
//  flag set, you must recycle the buffer before returning."	

void AudioFilterNode::BufferReceived(
	BBuffer*										buffer) {
	ASSERT(buffer);

	// check buffer destination
	if(buffer->Header()->destination !=
		m_input.destination.id) {
		PRINT(("AudioFilterNode::BufferReceived():\n"
			"\tBad destination.\n"));
		buffer->Recycle();
		return;
	}
	
	if(buffer->Header()->time_source != TimeSource()->ID()) { // +++++ no-go in offline mode
		PRINT(("* timesource mismatch\n"));
	}

	// check output
	if(m_output.destination == media_destination::null ||
		!m_outputEnabled) {
		buffer->Recycle();
		return;
	}
	
//	// +++++ [9sep99]
//	bigtime_t now = TimeSource()->Now();
//	bigtime_t delta = now - m_tpLastReceived;
//	m_tpLastReceived = now;
//	PRINT((
//		"### delta: %Ld (%Ld)\n",
//		delta, buffer->Header()->start_time - now));

	// fetch outbound buffer if needed
	BBuffer* outBuffer;
	if(m_bufferGroup) {
		outBuffer = m_bufferGroup->RequestBuffer(
			m_output.format.u.raw_audio.buffer_size, -1);
		ASSERT(outBuffer);
		
		// prepare outbound buffer
		outBuffer->Header()->type = B_MEDIA_RAW_AUDIO;

		// copy start time info from upstream node
		// +++++ is this proper, or should the next buffer-start be
		//       continuously tracked (figured from Start() or the first
		//       buffer received?)
		outBuffer->Header()->time_source = buffer->Header()->time_source;
		outBuffer->Header()->start_time = buffer->Header()->start_time;
	}
	else {
		// process inplace
		outBuffer = buffer;
	}
			
	// process and retransmit buffer
	processBuffer(buffer, outBuffer);

	status_t err = SendBuffer(outBuffer, m_output.source, m_output.destination);
	if (err < B_OK) {
		PRINT(("AudioFilterNode::BufferReceived():\n"
			"\tSendBuffer() failed: %s\n", strerror(err)));
		outBuffer->Recycle();
	}

	// free inbound buffer if data was copied	
	if(buffer != outBuffer)
		buffer->Recycle();

//	//####resend
//	SendBuffer(buffer, m_output.destination);

	// sent!
}
	
// * make sure to fill in poInput->format with the contents of
//   pFormat; as of R4.5 the Media Kit passes poInput->format to
//   the producer in BBufferProducer::Connect().

status_t AudioFilterNode::Connected(
	const media_source&					source,
	const media_destination&		destination,
	const media_format&					format,
	media_input*								outInput) {
	
	PRINT(("AudioFilterNode::Connected()\n"
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
	*outInput = m_input;
	
	// [re-]initialize operation
	updateOperation();
	
	return B_OK;
}

void AudioFilterNode::Disconnected(
	const media_source&					source,
	const media_destination&		destination) {
	
	PRINT(("AudioFilterNode::Disconnected()\n"));

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

#ifdef DEBUG
	status_t err =
#endif
	getRequiredInputFormat(m_input.format);
	ASSERT(err == B_OK);
	
	// remove operation
	if(m_op) {
		delete m_op;
		m_op = 0;
	}
	
	// +++++ further cleanup?

	// release buffer group
	updateBufferGroup();
}
		
		
void AudioFilterNode::DisposeInputCookie(
	int32												cookie) {}
	
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

status_t AudioFilterNode::FormatChanged(
	const media_source&					source,
	const media_destination&		destination,
	int32												changeTag,
	const media_format&					newFormat) {
	
	// flat-out deny format changes for now +++++
	return B_MEDIA_BAD_FORMAT;
}
		
status_t AudioFilterNode::GetLatencyFor(
	const media_destination&		destination,
	bigtime_t*									outLatency,
	media_node_id*							outTimeSource) {
	
	PRINT(("AudioFilterNode::GetLatencyFor()\n"));
	
	// sanity check
	if(destination != m_input.destination) {
		PRINT(("\tbad destination\n"));
		return B_MEDIA_BAD_DESTINATION;
	}
	
	*outLatency = m_downstreamLatency + m_processingLatency;
	PRINT(("\treturning %Ld\n", *outLatency));
	*outTimeSource = TimeSource()->ID();
	return B_OK;
}
		
status_t AudioFilterNode::GetNextInput(
	int32*											ioCookie,
	media_input*								outInput) {

	if(*ioCookie)
		return B_BAD_INDEX;
	
	++*ioCookie;
	*outInput = m_input;
	return B_OK;
}


void AudioFilterNode::ProducerDataStatus(
	const media_destination&		destination,
	int32												status,
	bigtime_t										tpWhen) {

	PRINT(("AudioFilterNode::ProducerDataStatus(%ld at %Ld)\n", status, tpWhen));
	
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

status_t AudioFilterNode::SeekTagRequested(
	const media_destination&		destination,
	bigtime_t										targetTime,
	uint32											flags,
	media_seek_tag*							outSeekTag,
	bigtime_t*									outTaggedTime,
	uint32*											outFlags) {

	// +++++
	PRINT(("AudioFilterNode::SeekTagRequested()\n"
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

void AudioFilterNode::AdditionalBufferRequested(
	const media_source&					source,
	media_buffer_id							previousBufferID,
	bigtime_t										previousTime,
	const media_seek_tag*				previousTag) {
	
	// +++++
	PRINT(("AudioFilterNode::AdditionalBufferRequested\n"
		"\tOffline mode not implemented."));
}
		
void AudioFilterNode::Connect(
	status_t										status,
	const media_source&					source,
	const media_destination&		destination,
	const media_format&					format,
	char*												ioName) {
	
	PRINT(("AudioFilterNode::Connect()\n"));
	status_t err;

#if DEBUG
	char formatStr[256];
	string_for_format(format, formatStr, 255);
	PRINT(("\tformat: %s\n", formatStr));
#endif

	// connection failed?	
	if(status < B_OK) {
		PRINT(("\tCONNECTION FAILED: Status '%s'\n", strerror(status)));
		// 'unreserve' the output
		m_output.destination = media_destination::null;
		return;
	}
	
	// connection established:
	strncpy(ioName, m_output.name, B_MEDIA_NAME_LENGTH);
	m_output.destination = destination;
	
	// figure downstream latency
	media_node_id timeSource;
	err = FindLatencyFor(m_output.destination, &m_downstreamLatency, &timeSource);
	if(err < B_OK) {
		PRINT(("\t!!! FindLatencyFor(): %s\n", strerror(err)));
	}
	PRINT(("\tdownstream latency = %Ld\n", m_downstreamLatency));
	
//	// prepare the filter
//	initFilter();
//
//	// figure processing time
//	m_processingLatency = calcProcessingLatency();
//	PRINT(("\tprocessing latency = %Ld\n", m_processingLatency));
//	
//	// store summed latency
//	SetEventLatency(m_downstreamLatency + m_processingLatency);
//	
//	if(m_input.source != media_source::null) {
//		// pass new latency upstream
//		err = SendLatencyChange(
//			m_input.source,
//			m_input.destination,
//			EventLatency() + SchedulingLatency());
//		if(err < B_OK)
//			PRINT(("\t!!! SendLatencyChange(): %s\n", strerror(err)));
//	}

	// cache buffer duration
	SetBufferDuration(
		buffer_duration(
			m_output.format.u.raw_audio));
			
	// [re-]initialize operation
	updateOperation();	

	// initialize buffer group if necessary
	updateBufferGroup();
}
	
void AudioFilterNode::Disconnect(
	const media_source&					source,
	const media_destination&		destination) {

	PRINT(("AudioFilterNode::Disconnect()\n"));
	
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

#ifdef DEBUG
	status_t err =
#endif
	getRequiredOutputFormat(m_output.format);
	ASSERT(err == B_OK);

	updateBufferGroup();
	
	if(m_op) {
		delete m_op;
		m_op = 0;
	}
}
		
status_t AudioFilterNode::DisposeOutputCookie(
	int32												cookie) {
	return B_OK;
}
		
void AudioFilterNode::EnableOutput(
	const media_source&					source,
	bool												enabled,
	int32* _deprecated_) {
	
	PRINT(("AudioFilterNode::EnableOutput()\n"));
	if(source != m_output.source) {
		PRINT(("\tbad source\n"));
		return;
	}
	
	m_outputEnabled = enabled;
}
		
status_t AudioFilterNode::FormatChangeRequested(
	const media_source&					source,
	const media_destination&		destination,
	media_format*								ioFormat,
	int32* _deprecated_) {
	
	// deny +++++ for now
	PRINT(("AudioFilterNode::FormatChangeRequested()\n"
		"\tNot supported.\n"));
		
	return B_MEDIA_BAD_FORMAT;
}
		
status_t AudioFilterNode::FormatProposal(
	const media_source&					source,
	media_format*								ioFormat) {

	PRINT(("AudioFilterNode::FormatProposal()\n"));
	status_t err;
	
	if(source != m_output.source) {
		PRINT(("\tbad source\n"));
		return B_MEDIA_BAD_SOURCE;
	}
	
	if(ioFormat->type != B_MEDIA_RAW_AUDIO) {
		PRINT(("\tbad type\n"));
		return B_MEDIA_BAD_FORMAT;
	}
	
	// validate against required format
	media_format required;
	required.type = B_MEDIA_RAW_AUDIO;
	err = getRequiredOutputFormat(required);
	ASSERT(err == B_OK);
	
	err = validateProposedOutputFormat(
		required,
		*ioFormat);
	if(err < B_OK)
		return err;
	
//	// specialize the format
//	media_format testFormat = *ioFormat;
//	specializeOutputFormat(testFormat);
//	
//	// if the input is connected, ask the factory for a matching operation
//	if(m_input.source != media_source::null) {
//		ASSERT(m_opFactory);
//		IAudioOp* op = m_opFactory->createOp(
//			this,
//			m_input.format.u.raw_audio,
//			testFormat.u.raw_audio);
//
//		if(!op) {
//			// format passed validation, but factory failed to provide a
//			// capable operation object
//			char fmt[256];
//			string_for_format(*ioFormat, fmt, 255);
//			PRINT((
//				"*** FormatProposal(): format validated, but no operation found:\n"
//				"    %s\n",
//				fmt));
//
//			return B_MEDIA_BAD_FORMAT;
//		}
//		// clean up
//		delete op;
//	}
	
	// format passed inspection
	return B_OK;
}
		
status_t AudioFilterNode::FormatSuggestionRequested(
	media_type									type,
	int32												quality,
	media_format*								outFormat) {
	
	PRINT(("AudioFilterNode::FormatSuggestionRequested()\n"));
	if(type != B_MEDIA_RAW_AUDIO) {
		PRINT(("\tbad type\n"));
		return B_MEDIA_BAD_FORMAT;
	}
	
	outFormat->type = type;
	return getPreferredOutputFormat(*outFormat);
}
		
status_t AudioFilterNode::GetLatency(
	bigtime_t*									outLatency) {
	
	PRINT(("AudioFilterNode::GetLatency()\n"));
	*outLatency = EventLatency() + SchedulingLatency();
	PRINT(("\treturning %Ld\n", *outLatency));
	
	return B_OK;
}
		
status_t AudioFilterNode::GetNextOutput(
	int32*											ioCookie,
	media_output*								outOutput) {

	if(*ioCookie)
		return B_BAD_INDEX;
	
	++*ioCookie;
	*outOutput = m_output;
	
	return B_OK;
}

	
// "This hook function is called when a BBufferConsumer that's receiving data
//  from you determines that its latency has changed. It will call its
//  BBufferConsumer::SendLatencyChange() function, and in response, the Media
//  Server will call your LatencyChanged() function.  The source argument
//  indicates your output that's involved in the connection, and destination
//  specifies the input on the consumer to which the connection is linked.
//  newLatency is the consumer's new latency. The flags are currently unused."
void AudioFilterNode::LatencyChanged(
	const media_source&					source,
	const media_destination&		destination,
	bigtime_t										newLatency,
	uint32											flags) {

	PRINT(("AudioFilterNode::LatencyChanged()\n"));
	
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

void AudioFilterNode::LateNoticeReceived(
	const media_source&					source,
	bigtime_t										howLate,
	bigtime_t										tpWhen) {

	PRINT(("AudioFilterNode::LateNoticeReceived()\n"
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

status_t AudioFilterNode::PrepareToConnect(
	const media_source&					source,
	const media_destination&		destination,
	media_format*								ioFormat,
	media_source*								outSource,
	char*												outName) {

	status_t err;
	char formatStr[256];
	string_for_format(*ioFormat, formatStr, 255);
	PRINT(("AudioFilterNode::PrepareToConnect()\n"
		"\tproposed format: %s\n", formatStr));
	
	if(source != m_output.source) {
		PRINT(("\tBad source.\n"));
		return B_MEDIA_BAD_SOURCE;
	}
	if(m_output.destination != media_destination::null) {
		PRINT(("\tAlready connected.\n"));
		return B_MEDIA_ALREADY_CONNECTED;
	}
	
	if(ioFormat->type != B_MEDIA_RAW_AUDIO) {
		PRINT(("\tBad format type.\n"));
		return B_MEDIA_BAD_FORMAT;
	}

	// do a final validity check:
	media_format required;
	required.type = B_MEDIA_RAW_AUDIO;
	err = getRequiredOutputFormat(required);
	ASSERT(err == B_OK);
	
	err = validateProposedOutputFormat(
		required,	*ioFormat);

	if(err < B_OK) {
		// no go
		return err;
	}
	
	// fill in wildcards
	specializeOutputFormat(*ioFormat);
	
	string_for_format(*ioFormat, formatStr, 255);
	PRINT(("FINAL FORMAT: %s\n", formatStr));

	// reserve the output
	m_output.destination = destination;
	m_output.format = *ioFormat;
	
	// pass back source & output name
	*outSource = m_output.source;
	strncpy(outName, m_output.name, B_MEDIA_NAME_LENGTH);
	
	return B_OK;
}
		
status_t AudioFilterNode::SetBufferGroup(
	const media_source&					source,
	BBufferGroup*								group) {

	PRINT(("AudioFilterNode::SetBufferGroup()\n"));
	if(source != m_output.source) {
		PRINT(("\tBad source.\n"));
		return B_MEDIA_BAD_SOURCE;
	}
	
//	if(m_input.source == media_source::null) {
//		PRINT(("\tNo producer to send buffers to.\n"));
//		return B_ERROR;
//	}
//	
//	// +++++ is this right?  buffer-group selection gets
//	//       all asynchronous and weird...
//	int32 changeTag;
//	return SetOutputBuffersFor(
//		m_input.source,
//		m_input.destination,
//		group,
//		0, &changeTag);

	// do it [8sep99]
	if(m_bufferGroup)
		delete m_bufferGroup;
	m_bufferGroup = group;
	
	return B_OK;
}
	
status_t AudioFilterNode::SetPlayRate(
	int32												numerator,
	int32												denominator) {
	// not supported
	return B_ERROR;
}
	
status_t AudioFilterNode::VideoClippingChanged(
	const media_source&					source,
	int16												numShorts,
	int16*											clipData,
	const media_video_display_info& display,
	int32*											outFromChangeTag) {
	// not sane
	return B_ERROR;
}

// -------------------------------------------------------- //
// *** BControllable
// -------------------------------------------------------- //

status_t AudioFilterNode::GetParameterValue(
	int32												id,
	bigtime_t*									outLastChangeTime,
	void*												outValue,
	size_t*											ioSize) {
	
	ASSERT(m_parameterSet);
	return m_parameterSet->getValue(
		id,
		outLastChangeTime,
		outValue,
		ioSize);
}
	
void AudioFilterNode::SetParameterValue(
	int32												id,
	bigtime_t										changeTime,
	const void*									value,
	size_t											size) {

	// not running? set parameter now
	if(RunState() != B_STARTED) {
		ASSERT(m_parameterSet);
		m_parameterSet->setValue(
			id,
			changeTime,
			value,
			size);
		return;
	}
	
	// queue a parameter-change event

	if(size > 64) { // +++++ hard-coded limitation in media_timed_event
		DEBUGGER((
			"!!! AudioFilterNode::SetParameterValue(): parameter data too large\n"));
	}
	
	media_timed_event ev(
		changeTime,
		BTimedEventQueue::B_PARAMETER,
		0,
		BTimedEventQueue::B_NO_CLEANUP,
		size,
		id,
		(char*)value, size);
	EventQueue()->AddEvent(ev);	
}

// -------------------------------------------------------- //
// *** IAudioOpHost
// -------------------------------------------------------- //

IParameterSet* AudioFilterNode::parameterSet() const {
	return m_parameterSet;
}

// -------------------------------------------------------- //
// HandleEvent() impl.
// -------------------------------------------------------- //

void AudioFilterNode::handleParameterEvent(
	const media_timed_event*		event) {
	
	// retrieve encoded parameter data
	void* value = (void*)event->user_data;
	int32 id = event->bigdata;
	size_t size = event->data;
	bigtime_t changeTime = event->event_time;
	status_t err;
	
	// hand to parameter set
	ASSERT(m_parameterSet);
	err = m_parameterSet->setValue(id, changeTime, value, size);
	
	if(err < B_OK) {
		PRINT((
			"* AudioFilterNode::handleParameterEvent(): m_parameterSet->SetValue() failed:\n"
			"  %s\n", strerror(err)));
	}
}
		
void AudioFilterNode::handleStartEvent(
	const media_timed_event*		event) {
	PRINT(("AudioFilterNode::handleStartEvent\n"));

	// initialize the filter
	ASSERT(m_op);
	m_op->init();
}
		
void AudioFilterNode::handleStopEvent(
	const media_timed_event*		event) {

	PRINT(("AudioFilterNode::handleStopEvent\n"));
	// +++++
}
		
void AudioFilterNode::ignoreEvent(
	const media_timed_event*		event) {

	PRINT(("AudioFilterNode::ignoreEvent\n"));
}

// -------------------------------------------------------- //
// *** internal operations
// -------------------------------------------------------- //

status_t 
AudioFilterNode::prepareFormatChange(const media_format &newFormat)
{
	media_format required;
	required.type = B_MEDIA_RAW_AUDIO;
	status_t err = getRequiredOutputFormat(required);
	ASSERT(err == B_OK);
	
	media_format proposed = newFormat;
	err = validateProposedOutputFormat(
		required,
		proposed);
	return err;
}

void 
AudioFilterNode::doFormatChange(const media_format &newFormat)
{
	m_output.format = newFormat;
	updateOperation();
}


// create and register a parameter web
void AudioFilterNode::initParameterWeb() {
	ASSERT(m_parameterSet);
	
	BParameterWeb* web = new BParameterWeb();
	BString groupName = Name();
	groupName << " Parameters";
	BParameterGroup* group = web->MakeGroup(groupName.String());
	m_parameterSet->populateGroup(group);
	
	SetParameterWeb(web);
}

// [re-]initialize operation if necessary
void AudioFilterNode::updateOperation() {

	if(m_input.source == media_source::null ||
		m_output.destination == media_destination::null)
		// not fully connected; nothing to do
		return;
	
	// ask the factory for an operation
	ASSERT(m_opFactory);
	IAudioOp* op = m_opFactory->createOp(
		this,
		m_input.format.u.raw_audio,
		m_output.format.u.raw_audio);
	if(!op) {
		PRINT((
			"!!! AudioFilterNode::updateOperation(): no operation created!\n"));
			
		// clean up existing operation
		delete m_op;
		m_op = 0;
		return;
	}
	
	// install new operation
	op->replace(m_op);
	m_op = op;
	
	// do performance tests (what if I'm running? +++++)

	m_processingLatency = calcProcessingLatency();
	PRINT(("\tprocessing latency = %Ld\n", m_processingLatency));
	
	// store summed latency
	SetEventLatency(m_downstreamLatency + m_processingLatency);
	
	// pass new latency upstream
	status_t err = SendLatencyChange(
		m_input.source,
		m_input.destination,
		EventLatency() + SchedulingLatency());
	if(err < B_OK)
		PRINT(("\t!!! SendLatencyChange(): %s\n", strerror(err)));
}


// create or discard buffer group if necessary
void AudioFilterNode::updateBufferGroup() {

	status_t err;
	
	size_t inputSize = bytes_per_frame(m_input.format.u.raw_audio);
	size_t outputSize = bytes_per_frame(m_output.format.u.raw_audio);
	
	if(m_input.source == media_source::null ||
		m_output.destination == media_destination::null ||
		inputSize >= outputSize) {

		PRINT(("###### NO BUFFER GROUP NEEDED\n"));
		
		// no internal buffer group needed
		if(m_bufferGroup) {
			// does this block? +++++
			delete m_bufferGroup;
			m_bufferGroup = 0;
		}
		return;
	}
	
	int32 bufferCount = EventLatency() / BufferDuration() + 1 + 1;
	
	// +++++
	// [e.moon 27sep99] this is a reasonable number of buffers,
	// but it fails with looped file-player node in BeOS 4.5.2.
	//
	if(bufferCount < 5)
		bufferCount = 5;
//	if(bufferCount < 3)
//		bufferCount = 3;
		
	if(m_bufferGroup) {

		// is the current group sufficient?
		int32 curBufferCount;
		err = m_bufferGroup->CountBuffers(&curBufferCount);
		if(err == B_OK && curBufferCount >= bufferCount) {		
			BBuffer* buf = m_bufferGroup->RequestBuffer(
				outputSize, -1);
		
			if(buf) {
				// yup
				buf->Recycle();
				return;
			}
		}

		// nope, delete it to make way for the new one
		delete m_bufferGroup;
		m_bufferGroup = 0;		
	}
	
	// create buffer group
	PRINT((
		"##### AudioFilterNode::updateBufferGroup():\n"
		"##### creating %ld buffers of size %ld\n",
		bufferCount, m_output.format.u.raw_audio.buffer_size));

	m_bufferGroup = new BBufferGroup(
		m_output.format.u.raw_audio.buffer_size,
		bufferCount);
}


// figure processing latency by doing 'dry runs' of processBuffer()
bigtime_t AudioFilterNode::calcProcessingLatency() {

	PRINT(("AudioFilterNode::calcProcessingLatency()\n"));
	
	ASSERT(m_input.source != media_source::null);
	ASSERT(m_output.destination != media_destination::null);
	ASSERT(m_op);

	// initialize filter
	m_op->init();

	size_t maxSize = max_c(
		m_input.format.u.raw_audio.buffer_size,
		m_output.format.u.raw_audio.buffer_size);

	// allocate a temporary buffer group
	BBufferGroup* testGroup = new BBufferGroup(
		maxSize, 1);
	
	// fetch a buffer big enough for in-place processing
	BBuffer* buffer = testGroup->RequestBuffer(
		maxSize, -1);
	ASSERT(buffer);
	
	buffer->Header()->type = B_MEDIA_RAW_AUDIO;
	buffer->Header()->size_used = m_input.format.u.raw_audio.buffer_size;
	
	// run the test
	bigtime_t preTest = system_time();
	processBuffer(buffer, buffer);
	bigtime_t elapsed = system_time()-preTest;
	
	// clean up
	buffer->Recycle();
	delete testGroup;

	// reset filter state
	m_op->init();

	return elapsed;// + 100000LL;
}
	
// filter buffer data; inputBuffer and outputBuffer may be identical!

void AudioFilterNode::processBuffer(
	BBuffer*										inputBuffer,
	BBuffer*										outputBuffer) {

	ASSERT(inputBuffer);
	ASSERT(outputBuffer);
	ASSERT(m_op);

	// create wrapper objects
	AudioBuffer input(m_input.format.u.raw_audio, inputBuffer);
	AudioBuffer output(m_output.format.u.raw_audio, outputBuffer);

	double sourceOffset = 0.0;
	uint32 destinationOffset = 0L;

	// when is the first frame due to be consumed?
	bigtime_t startTime = outputBuffer->Header()->start_time;
	// when is the next frame to be produced going to be consumed?
	bigtime_t targetTime = startTime;
	// when will the first frame of the next buffer be consumed?
	bigtime_t endTime = startTime + BufferDuration();
	
	uint32 framesRemaining = input.frames();
	while(framesRemaining) {

		// handle all events occurring before targetTime
		// +++++
		
		bigtime_t nextEventTime = endTime;
		
		// look for next event occurring before endTime
		// +++++
		
		// process up to found event, if any, or to end of buffer
		
		int64 toProcess = frames_for_duration(output.format(), nextEventTime - targetTime);

		ASSERT(toProcess > 0);

		uint32 processed = m_op->process(
			input, output, sourceOffset, destinationOffset, (uint32)toProcess, targetTime);
		if(processed < toProcess) {
			// +++++ in offline mode this will have to request additional buffer(s), right?
			PRINT((
				"*** AudioFilterNode::processBuffer(): insufficient frames filled\n"));
		}
			
		if(toProcess > framesRemaining)
			framesRemaining = 0;
		else
			framesRemaining -= toProcess;
			
		// advance target time
		targetTime = nextEventTime; // +++++ might this drift from the real frame offset?
	}
	
	outputBuffer->Header()->size_used = input.frames() * bytes_per_frame(m_output.format.u.raw_audio);
//	PRINT(("### output size: %ld\n", outputBuffer->Header()->size_used));
}

// END -- AudioFilterNode.cpp --
