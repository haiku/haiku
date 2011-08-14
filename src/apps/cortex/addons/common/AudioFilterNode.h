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


// AudioFilterNode.h
// * PURPOSE
//   A framework class designed to make it easy to develop simple
//   audio filters in the BeOS Media Kit.  The actual DSP work
//   is done externally by an IAudioOp object.
//
// * NOT SUPPORTED YET... 
//   - multiple inputs or outputs
//   - chaining multiple operations (yum)
//
// * HISTORY
//   e.moon		7sep99		Begun: abstracting from FlangerNode and
//                      implementing IAudioOp.

#ifndef __AudioFilterNode_H__
#define __AudioFilterNode_H__

#include <BufferProducer.h>
#include <BufferConsumer.h>
#include <Controllable.h>
#include <Debug.h>
#include <MediaEventLooper.h>

#include "IAudioOpHost.h"

// forwards
class BBufferGroup;
class BMediaAddOn;

class AudioBuffer;
class IAudioOpFactory;
class IAudioOp;
class IParameterSet;

class AudioFilterNode :
	public		BBufferConsumer,
	public		BBufferProducer,
	public		BControllable,
	public		BMediaEventLooper,
	public		IAudioOpHost {

public:													// *** HOOKS

	// *** FORMAT NEGOTIATION
	
	// +++++ 8sep99: there's currently no differentiation made between input
	//               and output formats.  there probably should be, though for
	//               now you can do extra format restriction in your factory's
	//               createOp() implementation... which is too late.
	// +++++
	// +++++ okay, time to split out into getRequiredInputFormat(), ... etc
	//       subclasses allowing format conversion will need to restrict buffer
	//       sizes based on current connection (input or output.)

	// requests the required format for the given type (ioFormat.type must be
	// filled in!)
	// upon returning, any fields left as wildcards are negotiable.
	// Default implementation hands off to getPreferredFormat(),
	// yielding a rather strict node.
	
	virtual status_t getRequiredInputFormat(
		media_format&								ioFormat) { return getPreferredInputFormat(ioFormat); }
		
	virtual status_t getRequiredOutputFormat(
		media_format&								ioFormat) { return getPreferredOutputFormat(ioFormat); }
		
	// requests the required format for the given type (ioFormat.type must be
	// filled in!)
	// upon returning, all fields must be filled in.
	//
	// Default raw audio format:
	//   44100hz
	//   host-endian
	//   1 channel
	//   float
	//   1k buffers
	//
	virtual status_t getPreferredInputFormat(
		media_format&								ioFormat);
	virtual status_t getPreferredOutputFormat(
		media_format&								ioFormat);

	// test the given template format against a proposed format.
	// specialize wildcards for fields where the template contains
	// non-wildcard data; write required fields into proposed format
	// if they mismatch.
	// Returns B_OK if the proposed format doesn't conflict with the
	// template, or B_MEDIA_BAD_FORMAT otherwise.
	
	virtual status_t validateProposedInputFormat(
		const media_format&					preferredFormat,
		media_format&								ioProposedFormat);

	virtual status_t validateProposedOutputFormat(
		const media_format&					preferredFormat,
		media_format&								ioProposedFormat);
		
	// fill in wildcards in the given format.
	// (assumes the format passes validateProposedFormat().)
	// Default implementation: specializes all wildcards to format returned by
	//                         getPreferredXXXFormat()
	
	virtual void specializeInputFormat(
		media_format&								ioFormat) {

		media_format preferred;
		preferred.type = B_MEDIA_RAW_AUDIO;
#ifdef DEBUG
		status_t err =
#endif
		getPreferredInputFormat(preferred);
		ASSERT(err == B_OK);
		_specialize_raw_audio_format(preferred, ioFormat);
	}

	virtual void specializeOutputFormat(
		media_format&								ioFormat) {

		char fmt_buffer[256];
		string_for_format(ioFormat, fmt_buffer, 255);
		PRINT((
			"### specializeOutputFormat:\n"
			"    given '%s'\n", fmt_buffer));

		media_format preferred;
		preferred.type = B_MEDIA_RAW_AUDIO;
#ifdef DEBUG
		status_t err =
#endif
		getPreferredOutputFormat(preferred);
		ASSERT(err == B_OK);

		string_for_format(preferred, fmt_buffer, 255);
		PRINT((
			"    pref '%s'\n", fmt_buffer));

//		ioFormat.SpecializeTo(&preferred);
		_specialize_raw_audio_format(preferred, ioFormat);

		string_for_format(ioFormat, fmt_buffer, 255);
		PRINT((
			"    writing '%s'\n", fmt_buffer));
	}

public:													// *** ctor/dtor

	virtual ~AudioFilterNode();
	
	// the node acquires ownership of opFactory
	AudioFilterNode(
		const char*									name,
		IAudioOpFactory*						opFactory,
		BMediaAddOn*								addOn=0);
		
public:													// *** accessors
	const media_input& input() const { return m_input; }
	const media_output& output() const { return m_output; }

public:													// *** BMediaNode

	virtual status_t HandleMessage(
		int32												code,
		const void*									data,
		size_t											size);

	virtual BMediaAddOn* AddOn(
		int32*											outID) const;

	virtual void SetRunMode(
		run_mode										mode);
	
protected:											// *** BMediaEventLooper

	virtual void HandleEvent(
		const media_timed_event*		event,
		bigtime_t										howLate,
		bool												realTimeEvent=false);

	// "The Media Server calls this hook function after the node has
	//  been registered.  This is derived from BMediaNode; BMediaEventLooper
	//  implements it to call Run() automatically when the node is registered;
	//  if you implement NodeRegistered() you should call through to
	//  BMediaEventLooper::NodeRegistered() after you've done your custom 
	//  operations."

	virtual void NodeRegistered();
	
	// "Augment OfflineTime() to compute the node's current time; it's called
	//  by the Media Kit when it's in offline mode. Update any appropriate
	//  internal information as well, then call through to the BMediaEventLooper
	//  implementation."

	virtual bigtime_t OfflineTime(); //nyi

public:													// *** BBufferConsumer

	virtual status_t AcceptFormat(
		const media_destination&		destination,
		media_format*								ioFormat);
	
	// "If you're writing a node, and receive a buffer with the B_SMALL_BUFFER
	//  flag set, you must recycle the buffer before returning."	

	virtual void BufferReceived(
		BBuffer*										buffer);
	
	// * make sure to fill in poInput->format with the contents of
	//   pFormat; as of R4.5 the Media Kit passes poInput->format to
	//   the producer in BBufferProducer::Connect().

	virtual status_t Connected(
		const media_source&					source,
		const media_destination&		destination,
		const media_format&					format,
		media_input*								outInput);

	virtual void Disconnected(
		const media_source&					source,
		const media_destination&		destination);
		
	virtual void DisposeInputCookie(
		int32												cookie);
	
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

	virtual status_t FormatChanged(
		const media_source&					source,
		const media_destination&		destination,
		int32												changeTag,
		const media_format&					newFormat);
		
	virtual status_t GetLatencyFor(
		const media_destination&		destination,
		bigtime_t*									outLatency,
		media_node_id*							outTimeSource);
		
	virtual status_t GetNextInput(
		int32*											ioCookie,
		media_input*								outInput);

	virtual void ProducerDataStatus(
		const media_destination&		destination,
		int32												status,
		bigtime_t										tpWhen);
	
	// "This function is provided to aid in supporting media formats in which the
	//  outer encapsulation layer doesn't supply timing information. Producers will
	//  tag the buffers they generate with seek tags; these tags can be used to
	//  locate key frames in the media data."

	virtual status_t SeekTagRequested(
		const media_destination&		destination,
		bigtime_t										targetTime,
		uint32											flags,
		media_seek_tag*							outSeekTag,
		bigtime_t*									outTaggedTime,
		uint32*											outFlags);
	
public:													// *** BBufferProducer

	// "When a consumer calls BBufferConsumer::RequestAdditionalBuffer(), this
	//  function is called as a result. Its job is to call SendBuffer() to
	//  immediately send the next buffer to the consumer. The previousBufferID,
	//  previousTime, and previousTag arguments identify the last buffer the
	//  consumer received. Your node should respond by sending the next buffer
	//  after the one described. 
	//
	//  The previousTag may be NULL.
	//  Return B_OK if all is well; otherwise return an appropriate error code."
	virtual void AdditionalBufferRequested(
		const media_source&					source,
		media_buffer_id							previousBufferID,
		bigtime_t										previousTime,
		const media_seek_tag*				previousTag); //nyi
		
	virtual void Connect(
		status_t										status,
		const media_source&					source,
		const media_destination&		destination,
		const media_format&					format,
		char*												ioName);
		
	virtual void Disconnect(
		const media_source&					source,
		const media_destination&		destination);
		
	virtual status_t DisposeOutputCookie(
		int32												cookie);
		
	virtual void EnableOutput(
		const media_source&					source,
		bool												enabled,
		int32* _deprecated_);
		
	virtual status_t FormatChangeRequested(
		const media_source&					source,
		const media_destination&		destination,
		media_format*								ioFormat,
		int32* _deprecated_); //nyi
		
	virtual status_t FormatProposal(
		const media_source&					source,
		media_format*								ioFormat);
		
	virtual status_t FormatSuggestionRequested(
		media_type									type,
		int32												quality,
		media_format*								outFormat);
		
	virtual status_t GetLatency(
		bigtime_t*									outLatency);
		
	virtual status_t GetNextOutput(
		int32*											ioCookie,
		media_output*								outOutput);
	
	// "This hook function is called when a BBufferConsumer that's receiving data
	//  from you determines that its latency has changed. It will call its
	//  BBufferConsumer::SendLatencyChange() function, and in response, the Media
	//  Server will call your LatencyChanged() function.  The source argument
	//  indicates your output that's involved in the connection, and destination
	//  specifies the input on the consumer to which the connection is linked.
	//  newLatency is the consumer's new latency. The flags are currently unused."
	virtual void LatencyChanged(
		const media_source&					source,
		const media_destination&		destination,
		bigtime_t										newLatency,
		uint32											flags);

	virtual void LateNoticeReceived(
		const media_source&					source,
		bigtime_t										howLate,
		bigtime_t										tpWhen);
	
	// PrepareToConnect() is the second stage of format negotiations that happens
	// inside BMediaRoster::Connect().  At this point, the consumer's AcceptFormat()
	// method has been called, and that node has potentially changed the proposed
	// format.  It may also have left wildcards in the format.  PrepareToConnect()
	// *must* fully specialize the format before returning!

	virtual status_t PrepareToConnect(
		const media_source&					source,
		const media_destination&		destination,
		media_format*								ioFormat,
		media_source*								outSource,
		char*												outName);
		
	virtual status_t SetBufferGroup(
		const media_source&					source,
		BBufferGroup*								group);
	
	virtual status_t SetPlayRate(
		int32												numerator,
		int32												denominator);
	
	virtual status_t VideoClippingChanged(
		const media_source&					source,
		int16												numShorts,
		int16*											clipData,
		const media_video_display_info& display,
		int32*											outFromChangeTag);

public:													// *** BControllable

	virtual status_t GetParameterValue(
		int32												id,
		bigtime_t*									outLastChangeTime,
		void*												outValue,
		size_t*											ioSize);
		
	virtual void SetParameterValue(
		int32												id,
		bigtime_t										changeTime,
		const void*									value,
		size_t											size);

public:													// *** IAudioOpHost
	virtual IParameterSet* parameterSet() const;

protected:											// HandleEvent() impl.
	void handleParameterEvent(
		const media_timed_event*		event);
		
	void handleStartEvent(
		const media_timed_event*		event);
		
	void handleStopEvent(
		const media_timed_event*		event);
		
	void ignoreEvent(
		const media_timed_event*		event);

protected:											// *** internal operations

	status_t prepareFormatChange(
		const media_format&         newFormat);
	void doFormatChange(
		const media_format&         newFormat);

	// create and register a parameter web
	// +++++ 7sep99: hand off to IParameterSet::makeGroup()
	virtual void initParameterWeb();
	
	// [re-]initialize operation if necessary
	virtual void updateOperation();
	
	// create or discard buffer group if necessary
	virtual void updateBufferGroup();
	
//	// construct delay line if necessary, reset filter state
//	virtual void initFilter();
//	
//	virtual void startFilter();
//	virtual void stopFilter();

	// figure processing latency by doing 'dry runs' of processBuffer()
	virtual bigtime_t calcProcessingLatency();
	
	// filter buffer data; inputBuffer and outputBuffer may be the same
	virtual void processBuffer(
		BBuffer*										inputBuffer,
		BBuffer*										outputBuffer);

	
	status_t _validate_raw_audio_format(
		const media_format&					preferredFormat,
		media_format&								ioProposedFormat);
	
	void _specialize_raw_audio_format(
		const media_format&					templateFormat,
		media_format&								ioFormat); 

private:												// *** connection/format members

	// +++++ expose input & output to subclasses [8sep99]

	// Connections & associated state variables	
	media_input				m_input;

	media_output			m_output;
	bool							m_outputEnabled;

	// Time required by downstream consumer(s) to properly deliver a buffer
	bigtime_t					m_downstreamLatency;

	// Worst-case time needed to fill a buffer
	bigtime_t					m_processingLatency;

	// buffer group for on-the-fly conversion [8sep99]
	// (only created when necessary, ie. more data going out than coming in)
	BBufferGroup*			m_bufferGroup;
	
private:												// *** parameters
	IParameterSet*								m_parameterSet;

private:												// *** operations
	IAudioOpFactory*	m_opFactory;
	
	// the current operation
	IAudioOp*					m_op;

private:												// *** add-on stuff

	// host add-on
	BMediaAddOn*	m_addOn;
};

#endif /*__AudioFilterNode_H__*/
