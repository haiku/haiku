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


/*
	ToneProducer.h

	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
	
	NOTES eamoon@meadgroup.com 11june99
	- this node has some holes in it, but it's pretty useful starting
	  point for writing producers.
	- I've tried to clean up the format negotiation a bit, which didn't
	  fare too well when faced with an apathetic downstream node
	  (LoggingConsumer.)
	  
	KNOWN BUGS
	  eamoon 17jun99	  
	  * Can't handle 2 channels, but is too polite to refuse.
	    How embarrassing.

*/

#ifndef ToneProducer_H
#define ToneProducer_H 1

#include <media/BufferProducer.h>
#include <media/MediaEventLooper.h>
#include <media/Controllable.h>

class BMediaAddOn;

// ----------------
// ToneProducer - a node to generate precisely defined digital tones
class ToneProducer : public BBufferProducer,  public BControllable, public BMediaEventLooper
{
public:
	enum event_t {
		// e.moon 2jun99
		_PARAMETER_EVENT 		= BTimedEventQueue::B_USER_EVENT
	};

public:
	// add-on-friendly ctor
	// e.moon [8jun99]
	ToneProducer(BMediaAddOn* pAddOn=0);
	~ToneProducer();

// BMediaNode methods
	BMediaAddOn* AddOn(	int32* internal_id) const;

// BControllable methods
	status_t GetParameterValue(
		int32 id,
		bigtime_t* last_change,
		void* value,
		size_t* ioSize);

	void SetParameterValue(
		int32 id,
		bigtime_t when,
		const void* value,
		size_t size);
		
	status_t StartControlPanel(
		BMessenger* pMessenger);

// BBufferProducer methods
	status_t FormatSuggestionRequested(
		media_type type,
		int32 quality,
		media_format* format);

	status_t FormatProposal(
		const media_source& output,
		media_format* format);

	/* If the format isn't good, put a good format into *io_format and return error */
	/* If format has wildcard, specialize to what you can do (and change). */
	/* If you can change the format, return OK. */
	/* The request comes from your destination sychronously, so you cannot ask it */
	/* whether it likes it -- you should assume it will since it asked. */
	status_t FormatChangeRequested(
		const media_source& source,
		const media_destination& destination,
		media_format* io_format,
		int32* _deprecated_);

	status_t GetNextOutput(	/* cookie starts as 0 */
		int32* cookie,
		media_output* out_output);

	status_t DisposeOutputCookie(
		int32 cookie);

	/* In this function, you should either pass on the group to your upstream guy, */
	/* or delete your current group and hang on to this group. Deleting the previous */
	/* group (unless you passed it on with the reclaim flag set to false) is very */
	/* important, else you will 1) leak memory and 2) block someone who may want */
	/* to reclaim the buffers living in that group. */
	status_t SetBufferGroup(
		const media_source& for_source,
		BBufferGroup* group);

	/* Iterates over all outputs and maxes the latency found */
	status_t GetLatency(
		bigtime_t* out_latency);

	status_t PrepareToConnect(
		const media_source& what,
		const media_destination& where,
		media_format* format,
		media_source* out_source,
		char* out_name);

	void Connect(
		status_t error, 
		const media_source& source,
		const media_destination& destination,
		const media_format& format,
		char* io_name);

	void Disconnect(
		const media_source& what,
		const media_destination& where);

	void LateNoticeReceived(
		const media_source& what,
		bigtime_t how_much,
		bigtime_t performance_time);

	void EnableOutput(
		const media_source & what,
		bool enabled,
		int32* _deprecated_);

	status_t SetPlayRate(
		int32 numer,
		int32 denom);

	status_t HandleMessage(
		int32 message,
		const void* data,
		size_t size);

	void AdditionalBufferRequested(
		const media_source& source,
		media_buffer_id prev_buffer,
		bigtime_t prev_time,
		const media_seek_tag* prev_tag);	//	may be NULL

	void LatencyChanged(
		const media_source& source,
		const media_destination& destination,
		bigtime_t new_latency,
		uint32 flags);

// BMediaEventLooper methods
	void NodeRegistered();

	void Start(bigtime_t performance_time);

	// Workaround for a Metrowerks PPC compiler bug
	void Stop(bigtime_t performance_time, bool immediate);

	// Workaround for a Metrowerks PPC compiler bug
	void Seek(bigtime_t media_time, bigtime_t performance_time);

	// Workaround for a Metrowerks PPC compiler bug
	void TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time);

	// Workaround for a Metrowerks PPC compiler bug
	status_t AddTimer(bigtime_t at_performance_time, int32 cookie);

	void SetRunMode(run_mode mode);

	void HandleEvent(
		const media_timed_event* event,
		bigtime_t lateness,
		bool realTimeEvent = false);

/* removed e.moon 2jun99
protected:
	// Workaround for a Metrowerks PPC compiler bug
	void CleanUpEvent(const media_timed_event *event);

	// Workaround for a Metrowerks PPC compiler bug
	bigtime_t OfflineTime();

	// Workaround for a Metrowerks PPC compiler bug
	void ControlLoop();
		
	// Workaround for a Metrowerks PPC compiler bug
	status_t DeleteHook(BMediaNode* node);
*/

private:
	void AllocateBuffers();
	BBuffer* FillNextBuffer(bigtime_t event_time);

	// 20sep99 e.moon: stereo support
	void FillSineBuffer(float* data, size_t numFrames, bool stereo);
	void FillTriangleBuffer(float* data, size_t numFrames, bool stereo);
	void FillSawtoothBuffer(float* data, size_t numFrames, bool stereo);

	BParameterWeb* mWeb;
	BBufferGroup* mBufferGroup;
	bigtime_t mLatency, mInternalLatency;
	media_output mOutput;
	bool mOutputEnabled;
	media_format mPreferredFormat;

	// These next attributes are related to tone buffer generation
	double mTheta;			// generic parameter used by all generators
	bool mWaveAscending;		// used by the triangle-wave generator
	float mFrequency;		// can be anything, in Hertz
	float mGain;				// should be in the range [0,1]
	int32 mWaveform;
	uint64 mFramesSent;
	bigtime_t mStartTime;

	// more parameter handling
	bigtime_t mGainLastChanged;
	bigtime_t mFreqLastChanged;
	bigtime_t mWaveLastChanged;
	
	// host addon
	// [8jun99] e.moon
	BMediaAddOn*		m_pAddOn;
};

#endif
