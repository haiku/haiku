/*
 * Copyright 1999, Be Incorporated.
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
	ToneProducer.cpp

	NOTE:  to compile this code under Genki beta releases, do a search-
	and-replace to change "B_PARAMETER" to "B_USER_EVENT+1"
*/

#include "ToneProducer.h"
#include <support/ByteOrder.h>
#include <media/BufferGroup.h>
#include <media/Buffer.h>
#include <media/TimeSource.h>
#include <media/ParameterWeb.h>
#include <media/MediaDefs.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <Messenger.h>

#include <Debug.h>
#if DEBUG
	#define FPRINTF fprintf
#else
	static inline void FPRINTF(FILE*, const char*, ...) { }
#endif

// parameter web handling
static BParameterWeb* make_parameter_web();
const int32 FREQUENCY_NULL_PARAM = 1;
const int32 FREQUENCY_PARAM = 2;
const int32 GAIN_NULL_PARAM = 11;
const int32 GAIN_PARAM = 12;
const int32 WAVEFORM_NULL_PARAM = 21;
const int32  WAVEFORM_PARAM = 22;
const int32 SINE_WAVE = 90;
const int32 TRIANGLE_WAVE = 91;
const int32 SAWTOOTH_WAVE = 92;

// ----------------
// ToneProducer implementation

ToneProducer::ToneProducer(BMediaAddOn* pAddOn)
	:	BMediaNode("ToneProducer"),
		BBufferProducer(B_MEDIA_RAW_AUDIO),
		BControllable(),
		BMediaEventLooper(),
		mWeb(NULL),
		mBufferGroup(NULL),
		mLatency(0),
		mInternalLatency(0),
		mOutputEnabled(true),
		mTheta(0.0),
		mWaveAscending(true),
		mFrequency(440),
		mGain(0.25),
		mWaveform(SINE_WAVE),
		mFramesSent(0),
		mStartTime(0),
		mGainLastChanged(0),
		mFreqLastChanged(0),
		mWaveLastChanged(0),
		m_pAddOn(pAddOn)
{
	// initialize our preferred format object
	mPreferredFormat.type = B_MEDIA_RAW_AUDIO;
	mPreferredFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	mPreferredFormat.u.raw_audio.frame_rate = 44100;		// measured in Hertz
	mPreferredFormat.u.raw_audio.byte_order = (B_HOST_IS_BENDIAN) ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;

	// we'll use the consumer's preferred buffer size, if any
	mPreferredFormat.u.raw_audio.buffer_size = media_raw_audio_format::wildcard.buffer_size;

	// 20sep99: multiple-channel support
	mPreferredFormat.u.raw_audio.channel_count = media_raw_audio_format::wildcard.channel_count;


	// we're not connected yet
	mOutput.destination = media_destination::null;

	// [e.moon 1dec99]
	mOutput.format = mPreferredFormat;

	// set up as much information about our output as we can
	// +++++ wrong; can't call Node() until the node is registered!
	mOutput.source.port = ControlPort();
	mOutput.source.id = 0;
	mOutput.node = Node();
	::strcpy(mOutput.name, "ToneProducer Output");
}

ToneProducer::~ToneProducer()
{
	// Stop the BMediaEventLooper thread
	Quit();

	// the BControllable destructor deletes our parameter web for us; we just use
	// a little defensive programming here and set our web pointer to be NULL.
	mWeb = NULL;
}

//#pragma mark -

// BMediaNode methods
BMediaAddOn *
ToneProducer::AddOn(int32 *internal_id) const
{
	// e.moon [8jun99]
	if(m_pAddOn) {
		*internal_id = 0;
		return m_pAddOn;
	} else
		return NULL;
}

//#pragma mark -

// BControllable methods
status_t
ToneProducer::GetParameterValue(int32 id, bigtime_t* last_change, void* value, size_t* ioSize)
{
	FPRINTF(stderr, "ToneProducer::GetParameterValue\n");

	// floats & int32s are the same size, so this one test of the size of the
	// output buffer is sufficient for all of our parameters
	if (*ioSize < sizeof(float)) return B_ERROR;

	// fill in the value of the requested parameter
	switch (id)
	{
	case FREQUENCY_PARAM:
		*last_change = mFreqLastChanged;
		*((float*) value) = mFrequency;
		*ioSize = sizeof(float);
		break;

	case GAIN_PARAM:
		*last_change = mGainLastChanged;
		*((float*) value) = mGain;
		*ioSize = sizeof(float);
		break;

	case WAVEFORM_PARAM:
		*last_change = mWaveLastChanged;
		*((int32*) value) = mWaveform;
		*ioSize = sizeof(int32);
		break;

	default:
		// Hmmm, we were asked for a parameter that we don't actually
		// support.  Report an error back to the caller.
		FPRINTF(stderr, "\terror - asked for illegal parameter %ld\n", id);
		return B_ERROR;
		break;
	}

	return B_OK;
}

void
ToneProducer::SetParameterValue(int32 id, bigtime_t performance_time, const void* value, size_t size)
{
	switch (id)
	{
	case FREQUENCY_PARAM:
	case GAIN_PARAM:
	case WAVEFORM_PARAM:
		{
			// floats and int32s are the same size, so we need only check the block's size once
			if (size > sizeof(float)) size = sizeof(float);

			// submit the parameter change as a performance event, to be handled at the
			// appropriate time
			media_timed_event event(performance_time, _PARAMETER_EVENT,
				NULL, BTimedEventQueue::B_NO_CLEANUP, size, id, (char*) value, size);
			EventQueue()->AddEvent(event);
		}
		break;

	default:
		break;
	}
}

// e.moon [17jun99]
status_t ToneProducer::StartControlPanel(
	BMessenger* pMessenger) {
	PRINT(("ToneProducer::StartControlPanel(%p)\n", pMessenger));
	status_t err = BControllable::StartControlPanel(pMessenger);
	if(pMessenger && pMessenger->IsValid()) {
		PRINT(("\tgot valid control panel\n"));
	}

	return err;
}

//#pragma mark -

// BBufferProducer methods
status_t
ToneProducer::FormatSuggestionRequested(media_type type, int32 /*quality*/, media_format* format)
{
	// FormatSuggestionRequested() is not necessarily part of the format negotiation
	// process; it's simply an interrogation -- the caller wants to see what the node's
	// preferred data format is, given a suggestion by the caller.
	FPRINTF(stderr, "ToneProducer::FormatSuggestionRequested\n");

	if (!format)
	{
		FPRINTF(stderr, "\tERROR - NULL format pointer passed in!\n");
		return B_BAD_VALUE;
	}

	// this is the format we'll be returning (our preferred format)
	*format = mPreferredFormat;

	// a wildcard type is okay; we can specialize it
	if (type == B_MEDIA_UNKNOWN_TYPE) type = B_MEDIA_RAW_AUDIO;

	// we only support raw audio
	if (type != B_MEDIA_RAW_AUDIO) return B_MEDIA_BAD_FORMAT;
	else return B_OK;
}

status_t
ToneProducer::FormatProposal(const media_source& output, media_format* format)
{
	// FormatProposal() is the first stage in the BMediaRoster::Connect() process.  We hand
	// out a suggested format, with wildcards for any variations we support.
	FPRINTF(stderr, "ToneProducer::FormatProposal\n");

	// is this a proposal for our one output?
	if (output != mOutput.source)
	{
		FPRINTF(stderr, "ToneProducer::FormatProposal returning B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// we only support floating-point raw audio, so we always return that, but we
	// supply an error code depending on whether we found the proposal acceptable.

	media_type requestedType = format->type;
	*format = mPreferredFormat;
	if ((requestedType != B_MEDIA_UNKNOWN_TYPE) && (requestedType != B_MEDIA_RAW_AUDIO))
	{
		FPRINTF(stderr, "ToneProducer::FormatProposal returning B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}
	else return B_OK;		// raw audio or wildcard type, either is okay by us
}

status_t
ToneProducer::FormatChangeRequested(const media_source& source, const media_destination& destination, media_format* io_format, int32* _deprecated_)
{
	FPRINTF(stderr, "ToneProducer::FormatChangeRequested\n");

	// we don't support any other formats, so we just reject any format changes.
	return B_ERROR;
}

status_t
ToneProducer::GetNextOutput(int32* cookie, media_output* out_output)
{
	FPRINTF(stderr, "ToneProducer::GetNextOutput\n");

	// we have only a single output; if we supported multiple outputs, we'd
	// iterate over whatever data structure we were using to keep track of
	// them.
	if (0 == *cookie)
	{
		*out_output = mOutput;
		*cookie += 1;
		return B_OK;
	}
	else return B_BAD_INDEX;
}

status_t
ToneProducer::DisposeOutputCookie(int32 cookie)
{
	FPRINTF(stderr, "ToneProducer::DisposeOutputCookie\n");

	// do nothing because we don't use the cookie for anything special
	return B_OK;
}

status_t
ToneProducer::SetBufferGroup(const media_source& for_source, BBufferGroup* newGroup)
{
	FPRINTF(stderr, "ToneProducer::SetBufferGroup\n");

	// verify that we didn't get bogus arguments before we proceed
	if (for_source != mOutput.source) return B_MEDIA_BAD_SOURCE;

	// Are we being passed the buffer group we're already using?
	if (newGroup == mBufferGroup) return B_OK;

	// Ahh, someone wants us to use a different buffer group.  At this point we delete
	// the one we are using and use the specified one instead.  If the specified group is
	// NULL, we need to recreate one ourselves, and use *that*.  Note that if we're
	// caching a BBuffer that we requested earlier, we have to Recycle() that buffer
	// *before* deleting the buffer group, otherwise we'll deadlock waiting for that
	// buffer to be recycled!
	delete mBufferGroup;		// waits for all buffers to recycle
	if (newGroup != NULL)
	{
		// we were given a valid group; just use that one from now on
		mBufferGroup = newGroup;
	}
	else
	{
		// we were passed a NULL group pointer; that means we construct
		// our own buffer group to use from now on
		size_t size = mOutput.format.u.raw_audio.buffer_size;
		int32 count = int32(mLatency / BufferDuration() + 1 + 1);
		mBufferGroup = new BBufferGroup(size, count);
	}

	return B_OK;
}

status_t
ToneProducer::GetLatency(bigtime_t* out_latency)
{
	FPRINTF(stderr, "ToneProducer::GetLatency\n");

	// report our *total* latency:  internal plus downstream plus scheduling
	*out_latency = EventLatency() + SchedulingLatency();
	return B_OK;
}

status_t
ToneProducer::PrepareToConnect(const media_source& what, const media_destination& where, media_format* format, media_source* out_source, char* out_name)
{
	// PrepareToConnect() is the second stage of format negotiations that happens
	// inside BMediaRoster::Connect().  At this point, the consumer's AcceptFormat()
	// method has been called, and that node has potentially changed the proposed
	// format.  It may also have left wildcards in the format.  PrepareToConnect()
	// *must* fully specialize the format before returning!
	FPRINTF(stderr, "ToneProducer::PrepareToConnect\n");

	// trying to connect something that isn't our source?
	if (what != mOutput.source) return B_MEDIA_BAD_SOURCE;

	// are we already connected?
	if (mOutput.destination != media_destination::null) return B_MEDIA_ALREADY_CONNECTED;

	// the format may not yet be fully specialized (the consumer might have
	// passed back some wildcards).  Finish specializing it now, and return an
	// error if we don't support the requested format.
	if (format->type != B_MEDIA_RAW_AUDIO)
	{
		FPRINTF(stderr, "\tnon-raw-audio format?!\n");
		return B_MEDIA_BAD_FORMAT;
	}
	else if (format->u.raw_audio.format != media_raw_audio_format::B_AUDIO_FLOAT)
	{
		FPRINTF(stderr, "\tnon-float-audio format?!\n");
		return B_MEDIA_BAD_FORMAT;
	}
	else if(format->u.raw_audio.channel_count > 2) {
		format->u.raw_audio.channel_count = 2;
		return B_MEDIA_BAD_FORMAT;
	}


	 // !!! validate all other fields except for buffer_size here, because the consumer might have
	// supplied different values from AcceptFormat()?

	// ***   e.moon [11jun99]: filling in sensible field values.
	//       Connect() doesn't take kindly to a frame_rate of 0.

	if(format->u.raw_audio.frame_rate == media_raw_audio_format::wildcard.frame_rate) {
		format->u.raw_audio.frame_rate = mPreferredFormat.u.raw_audio.frame_rate;
		FPRINTF(stderr, "\tno frame rate provided, suggesting %.1f\n", format->u.raw_audio.frame_rate);
	}
	if(format->u.raw_audio.channel_count == media_raw_audio_format::wildcard.channel_count) {
		//format->u.raw_audio.channel_count = mPreferredFormat.u.raw_audio.channel_count;
		format->u.raw_audio.channel_count = 1;
		FPRINTF(stderr, "\tno channel count provided, suggesting %lu\n", format->u.raw_audio.channel_count);
	}
	if(format->u.raw_audio.byte_order == media_raw_audio_format::wildcard.byte_order) {
		format->u.raw_audio.byte_order = mPreferredFormat.u.raw_audio.byte_order;
		FPRINTF(stderr, "\tno channel count provided, suggesting %s\n",
			(format->u.raw_audio.byte_order == B_MEDIA_BIG_ENDIAN) ? "B_MEDIA_BIG_ENDIAN" : "B_MEDIA_LITTLE_ENDIAN");
	}

	// check the buffer size, which may still be wildcarded
	if (format->u.raw_audio.buffer_size == media_raw_audio_format::wildcard.buffer_size)
	{
		format->u.raw_audio.buffer_size = 2048;		// pick something comfortable to suggest
		FPRINTF(stderr, "\tno buffer size provided, suggesting %lu\n", format->u.raw_audio.buffer_size);
	}
	else
	{
		FPRINTF(stderr, "\tconsumer suggested buffer_size %lu\n", format->u.raw_audio.buffer_size);
	}

	// Now reserve the connection, and return information about it
	mOutput.destination = where;
	mOutput.format = *format;
	*out_source = mOutput.source;
	strncpy(out_name, mOutput.name, B_MEDIA_NAME_LENGTH);

	char formatStr[256];
	string_for_format(*format, formatStr, 255);
	FPRINTF(stderr, "\treturning format: %s\n", formatStr);

	return B_OK;
}

void
ToneProducer::Connect(status_t error, const media_source& source, const media_destination& destination, const media_format& format, char* io_name)
{
	FPRINTF(stderr, "ToneProducer::Connect\n");

	// If something earlier failed, Connect() might still be called, but with a non-zero
	// error code.  When that happens we simply unreserve the connection and do
	// nothing else.
	if (error)
	{
		mOutput.destination = media_destination::null;
		mOutput.format = mPreferredFormat;
		return;
	}

// old workaround for format bug: Connect() receives the format data from the
// input returned from BBufferConsumer::Connected().
//
//	char formatStr[256];
//	string_for_format(format, formatStr, 255);
//	FPRINTF(stderr, "\trequested format: %s\n", formatStr);
//	if(format.type != B_MEDIA_RAW_AUDIO) {
//		// +++++ this is NOT proper behavior
//		//       but it works
//		FPRINTF(stderr, "\tcorrupted format; falling back to last suggested format\n");
//		format = mOutput.format;
//	}
//

	// Okay, the connection has been confirmed.  Record the destination and format
	// that we agreed on, and report our connection name again.
	mOutput.destination = destination;
	mOutput.format = format;
	strncpy(io_name, mOutput.name, B_MEDIA_NAME_LENGTH);

	// Now that we're connected, we can determine our downstream latency.
	// Do so, then make sure we get our events early enough.
	media_node_id id;
	FindLatencyFor(mOutput.destination, &mLatency, &id);
	FPRINTF(stderr, "\tdownstream latency = %Ld\n", mLatency);

	// Use a dry run to see how long it takes me to fill a buffer of data
	bigtime_t start, produceLatency;
	size_t samplesPerBuffer = mOutput.format.u.raw_audio.buffer_size / sizeof(float);
	size_t framesPerBuffer = samplesPerBuffer / mOutput.format.u.raw_audio.channel_count;
	float* data = new float[samplesPerBuffer];
	mTheta = 0;
	start = ::system_time();
	FillSineBuffer(data, framesPerBuffer, mOutput.format.u.raw_audio.channel_count==2);
	produceLatency = ::system_time();
	mInternalLatency = produceLatency - start;

	// +++++ e.moon [13jun99]: fiddling with latency, ick
	mInternalLatency += 20000LL;

	delete [] data;
	FPRINTF(stderr, "\tbuffer-filling took %Ld usec on this machine\n", mInternalLatency);
	SetEventLatency(mLatency + mInternalLatency);

	// reset our buffer duration, etc. to avoid later calculations
	// +++++ e.moon 11jun99: crashes w/ divide-by-zero when connecting to LoggingConsumer
	ASSERT(mOutput.format.u.raw_audio.frame_rate);

	bigtime_t duration = bigtime_t(1000000) * samplesPerBuffer / bigtime_t(mOutput.format.u.raw_audio.frame_rate);
	SetBufferDuration(duration);

	// Set up the buffer group for our connection, as long as nobody handed us a
	// buffer group (via SetBufferGroup()) prior to this.  That can happen, for example,
	// if the consumer calls SetOutputBuffersFor() on us from within its Connected()
	// method.
	if (!mBufferGroup) AllocateBuffers();
}

void
ToneProducer::Disconnect(const media_source& what, const media_destination& where)
{
	FPRINTF(stderr, "ToneProducer::Disconnect\n");

	// Make sure that our connection is the one being disconnected
	if ((where == mOutput.destination) && (what == mOutput.source))
	{
		mOutput.destination = media_destination::null;
		mOutput.format = mPreferredFormat;
		delete mBufferGroup;
		mBufferGroup = NULL;
	}
	else
	{
		FPRINTF(stderr, "\tDisconnect() called with wrong source/destination (%ld/%ld), ours is (%ld/%ld)\n",
			what.id, where.id, mOutput.source.id, mOutput.destination.id);
	}
}

void
ToneProducer::LateNoticeReceived(const media_source& what, bigtime_t how_much, bigtime_t performance_time)
{
	FPRINTF(stderr, "ToneProducer::LateNoticeReceived\n");

	// If we're late, we need to catch up.  Respond in a manner appropriate to our
	// current run mode.
	if (what == mOutput.source)
	{
		if (RunMode() == B_RECORDING)
		{
			// A hardware capture node can't adjust; it simply emits buffers at
			// appropriate points.  We (partially) simulate this by not adjusting
			// our behavior upon receiving late notices -- after all, the hardware
			// can't choose to capture "sooner"....
		}
		else if (RunMode() == B_INCREASE_LATENCY)
		{
			// We're late, and our run mode dictates that we try to produce buffers
			// earlier in order to catch up.  This argues that the downstream nodes are
			// not properly reporting their latency, but there's not much we can do about
			// that at the moment, so we try to start producing buffers earlier to
			// compensate.
			mInternalLatency += how_much;
			SetEventLatency(mLatency + mInternalLatency);

			FPRINTF(stderr, "\tincreasing latency to %Ld\n", mLatency + mInternalLatency);
		}
		else
		{
			// The other run modes dictate various strategies for sacrificing data quality
			// in the interests of timely data delivery.  The way *we* do this is to skip
			// a buffer, which catches us up in time by one buffer duration.
			size_t nSamples = mOutput.format.u.raw_audio.buffer_size / sizeof(float);
			mFramesSent += nSamples;

			FPRINTF(stderr, "\tskipping a buffer to try to catch up\n");
		}
	}
}

void
ToneProducer::EnableOutput(const media_source& what, bool enabled, int32* _deprecated_)
{
	FPRINTF(stderr, "ToneProducer::EnableOutput\n");

	// If I had more than one output, I'd have to walk my list of output records to see
	// which one matched the given source, and then enable/disable that one.  But this
	// node only has one output, so I just make sure the given source matches, then set
	// the enable state accordingly.
	if (what == mOutput.source)
	{
		mOutputEnabled = enabled;
	}
}

status_t
ToneProducer::SetPlayRate(int32 numer, int32 denom)
{
	FPRINTF(stderr, "ToneProducer::SetPlayRate\n");

	// Play rates are weird.  We don't support them.  Maybe we will in a
	// later newsletter article....
	return B_ERROR;
}

status_t
ToneProducer::HandleMessage(int32 message, const void* data, size_t size)
{
	FPRINTF(stderr, "ToneProducer::HandleMessage(%ld = 0x%lx)\n", message, message);
	// HandleMessage() is where you implement any private message protocols
	// that you want to use.  When messages are written to your node's control
	// port that are not recognized by any of the node superclasses, they'll be
	// passed to this method in your node's implementation for handling.  The
	// ToneProducer node doesn't support any private messages, so we just
	// return an error, indicating that the message wasn't handled.
	return B_ERROR;
}

void
ToneProducer::AdditionalBufferRequested(const media_source& source, media_buffer_id prev_buffer, bigtime_t prev_time, const media_seek_tag* prev_tag)
{
	FPRINTF(stderr, "ToneProducer::AdditionalBufferRequested\n");

	// we don't support offline mode (yet...)
	return;
}

void
ToneProducer::LatencyChanged(
	const media_source& source,
	const media_destination& destination,
	bigtime_t new_latency,
	uint32 flags)
{
	PRINT(("ToneProducer::LatencyChanged(): %Ld\n", new_latency));

	// something downstream changed latency, so we need to start producing
	// buffers earlier (or later) than we were previously.  Make sure that the
	// connection that changed is ours, and adjust to the new downstream
	// latency if so.
	if ((source == mOutput.source) && (destination == mOutput.destination))
	{
		mLatency = new_latency;
		SetEventLatency(mLatency + mInternalLatency);
	}
}
/*
// Workaround for a Metrowerks PPC compiler bug
status_t
ToneProducer::DeleteHook(BMediaNode* node)
{
	return BMediaEventLooper::DeleteHook(node);
}

//#pragma mark -
*/

// BMediaEventLooper methods
void
ToneProducer::NodeRegistered()
{
	FPRINTF(stderr, "ToneProducer::NodeRegistered\n");

	// Start the BMediaEventLooper thread
	SetPriority(B_REAL_TIME_PRIORITY);
	Run();

	// output init moved to ctor
	// e.moon [4jun99]

	// Set up our parameter web
	mWeb = make_parameter_web();
	SetParameterWeb(mWeb);
}

void
ToneProducer::Start(bigtime_t performance_time)
{
	PRINT(("ToneProducer::Start(%Ld): now %Ld\n", performance_time, TimeSource()->Now()));

	// send 'data available' message
	if(mOutput.destination != media_destination::null)
		SendDataStatus(B_DATA_AVAILABLE, mOutput.destination, performance_time);

	// A bug in the current PowerPC compiler demands that we implement
	// this, even though it just calls up to the inherited implementation.
	BMediaEventLooper::Start(performance_time);
}

void
ToneProducer::Stop(bigtime_t performance_time, bool immediate)
{
	// send 'data not available' message
	if(mOutput.destination != media_destination::null) {
		printf("ToneProducer: B_PRODUCER_STOPPED at %Ld\n", performance_time);
		SendDataStatus(B_PRODUCER_STOPPED, mOutput.destination, performance_time);
	}

	// A bug in the current PowerPC compiler demands that we implement
	// this, even though it just calls up to the inherited implementation.
	BMediaEventLooper::Stop(performance_time, immediate);
}

void
ToneProducer::Seek(bigtime_t media_time, bigtime_t performance_time)
{
	// A bug in the current PowerPC compiler demands that we implement
	// this, even though it just calls up to the inherited implementation.
	BMediaEventLooper::Seek(media_time, performance_time);
}

void
ToneProducer::TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time)
{
	// A bug in the current PowerPC compiler demands that we implement
	// this, even though it just calls up to the inherited implementation.
	BMediaEventLooper::TimeWarp(at_real_time, to_performance_time);
}

status_t
ToneProducer::AddTimer(bigtime_t at_performance_time, int32 cookie)
{
	// A bug in the current PowerPC compiler demands that we implement
	// this, even though it just calls up to the inherited implementation.
	return BMediaEventLooper::AddTimer(at_performance_time, cookie);
}

void
ToneProducer::SetRunMode(run_mode mode)
{
	FPRINTF(stderr, "ToneProducer::SetRunMode\n");

	// We don't support offline run mode, so broadcast an error if we're set to
	// B_OFFLINE.  Unfortunately, we can't actually reject the mode change...
	if (B_OFFLINE == mode)
	{
		ReportError(B_NODE_FAILED_SET_RUN_MODE);
	}
}

void
ToneProducer::HandleEvent(const media_timed_event* event, bigtime_t lateness, bool realTimeEvent)
{
//	FPRINTF(stderr, "ToneProducer::HandleEvent\n");
	switch (event->type)
	{
	case BTimedEventQueue::B_START:
		// don't do anything if we're already running
		if (RunState() != B_STARTED)
		{
			// We want to start sending buffers now, so we set up the buffer-sending bookkeeping
			// and fire off the first "produce a buffer" event.
			mFramesSent = 0;
			mTheta = 0;
			mStartTime = event->event_time;
			media_timed_event firstBufferEvent(mStartTime, BTimedEventQueue::B_HANDLE_BUFFER);

			// Alternatively, we could call HandleEvent() directly with this event, to avoid a trip through
			// the event queue, like this:
			//
			//		this->HandleEvent(&firstBufferEvent, 0, false);
			//
			EventQueue()->AddEvent(firstBufferEvent);
		}
		break;

	case BTimedEventQueue::B_STOP:
		FPRINTF(stderr, "Handling B_STOP event\n");

		// When we handle a stop, we must ensure that downstream consumers don't
		// get any more buffers from us.  This means we have to flush any pending
		// buffer-producing events from the queue.
		EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
		break;

	case _PARAMETER_EVENT:
		{
			size_t dataSize = size_t(event->data);
			int32 param = int32(event->bigdata);
			if (dataSize >= sizeof(float)) switch (param)
			{
			case FREQUENCY_PARAM:
				{
					float newValue = *((float*) event->user_data);
					if (mFrequency != newValue)		// an actual change in the value?
					{
						mFrequency = newValue;
						mFreqLastChanged = TimeSource()->Now();
						BroadcastNewParameterValue(mFreqLastChanged, param, &mFrequency, sizeof(mFrequency));
					}
				}
				break;

			case GAIN_PARAM:
				{
					float newValue = *((float*) event->user_data);
					if (mGain != newValue)
					{
						mGain = newValue;
						mGainLastChanged = TimeSource()->Now();
						BroadcastNewParameterValue(mGainLastChanged, param, &mGain, sizeof(mGain));
					}
				}
				break;

			case WAVEFORM_PARAM:
				{
					int32 newValue = *((int32*) event->user_data);
					if (mWaveform != newValue)
					{
						mWaveform = newValue;
						mTheta = 0;			// reset the generator parameters when we change waveforms
						mWaveAscending = true;
						mWaveLastChanged = TimeSource()->Now();
						BroadcastNewParameterValue(mWaveLastChanged, param, &mWaveform, sizeof(mWaveform));
					}
				}
				break;

			default:
				FPRINTF(stderr, "Hmmm... got a B_PARAMETER event for a parameter we don't have? (%ld)\n", param);
				break;
			}
		}
		break;

	case BTimedEventQueue::B_HANDLE_BUFFER:
		{
			// make sure we're both started *and* connected before delivering a buffer
			if ((RunState() == BMediaEventLooper::B_STARTED) && (mOutput.destination != media_destination::null))
			{
				// Get the next buffer of data
				BBuffer* buffer = FillNextBuffer(event->event_time);
				if (buffer)
				{
					// send the buffer downstream if and only if output is enabled
					status_t err = B_ERROR;
					if (mOutputEnabled) err = SendBuffer(buffer, mOutput.destination);
					if (err)
					{
						// we need to recycle the buffer ourselves if output is disabled or
						// if the call to SendBuffer() fails
						buffer->Recycle();
					}
				}

				// track how much media we've delivered so far
				size_t nFrames = mOutput.format.u.raw_audio.buffer_size /
					(sizeof(float) * mOutput.format.u.raw_audio.channel_count);
				mFramesSent += nFrames;

				// The buffer is on its way; now schedule the next one to go
				bigtime_t nextEvent = mStartTime + bigtime_t(double(mFramesSent) / double(mOutput.format.u.raw_audio.frame_rate) * 1000000.0);
				media_timed_event nextBufferEvent(nextEvent, BTimedEventQueue::B_HANDLE_BUFFER);
				EventQueue()->AddEvent(nextBufferEvent);
			}
		}
		break;

	default:
		break;
	}
}

/*
void
ToneProducer::CleanUpEvent(const media_timed_event *event)
{
	// A bug in the current PowerPC compiler demands that we implement
	// this, even though it just calls up to the inherited implementation.
	BMediaEventLooper::CleanUpEvent(event);
}

bigtime_t
ToneProducer::OfflineTime()
{
	// A bug in the current PowerPC compiler demands that we implement
	// this, even though it just calls up to the inherited implementation.
	return BMediaEventLooper::OfflineTime();
}

void
ToneProducer::ControlLoop()
{
	// A bug in the current PowerPC compiler demands that we implement
	// this, even though it just calls up to the inherited implementation.
	BMediaEventLooper::ControlLoop();
}

//#pragma mark -
*/

void
ToneProducer::AllocateBuffers()
{
	FPRINTF(stderr, "ToneProducer::AllocateBuffers\n");

	// allocate enough buffers to span our downstream latency, plus one
	size_t size = mOutput.format.u.raw_audio.buffer_size;
	int32 count = int32(mLatency / BufferDuration() + 1 + 1);

	FPRINTF(stderr, "\tlatency = %Ld, buffer duration = %Ld\n", mLatency, BufferDuration());
	FPRINTF(stderr, "\tcreating group of %ld buffers, size = %lx\n", count, size);
	mBufferGroup = new BBufferGroup(size, count);
}

BBuffer*
ToneProducer::FillNextBuffer(bigtime_t event_time)
{
	// get a buffer from our buffer group
	BBuffer* buf = mBufferGroup->RequestBuffer(mOutput.format.u.raw_audio.buffer_size, BufferDuration());

	// if we fail to get a buffer (for example, if the request times out), we skip this
	// buffer and go on to the next, to avoid locking up the control thread
	if (!buf)
	{
		return NULL;
	}

	// now fill it with data, continuing where the last buffer left off
	// 20sep99: multichannel support

	size_t numFrames =
		mOutput.format.u.raw_audio.buffer_size /
		(sizeof(float)*mOutput.format.u.raw_audio.channel_count);
	bool stereo = (mOutput.format.u.raw_audio.channel_count == 2);
	if(!stereo) {
		ASSERT(mOutput.format.u.raw_audio.channel_count == 1);
	}
//	PRINT(("buffer: %ld, %ld frames, %s\n", mOutput.format.u.raw_audio.buffer_size, numFrames, stereo ? "stereo" : "mono"));

	float* data = (float*) buf->Data();

	switch (mWaveform)
	{
	case SINE_WAVE:
		FillSineBuffer(data, numFrames, stereo);
		break;

	case TRIANGLE_WAVE:
		FillTriangleBuffer(data, numFrames, stereo);
		break;

	case SAWTOOTH_WAVE:
		FillSawtoothBuffer(data, numFrames, stereo);
		break;
	}

	// fill in the buffer header
	media_header* hdr = buf->Header();
	hdr->type = B_MEDIA_RAW_AUDIO;
	hdr->size_used = mOutput.format.u.raw_audio.buffer_size;
	hdr->time_source = TimeSource()->ID();

	bigtime_t stamp;
	if (RunMode() == B_RECORDING)
	{
		// In B_RECORDING mode, we stamp with the capture time.  We're not
		// really a hardware capture node, but we simulate it by using the (precalculated)
		// time at which this buffer "should" have been created.
		stamp = event_time;
	}
	else
	{
		// okay, we're in one of the "live" performance run modes.  in these modes, we
		// stamp the buffer with the time at which the buffer should be rendered to the
		// output, not with the capture time.  mStartTime is the cached value of the
		// first buffer's performance time; we calculate this buffer's performance time as
		// an offset from that time, based on the amount of media we've created so far.
		// Recalculating every buffer like this avoids accumulation of error.
		stamp = mStartTime + bigtime_t(double(mFramesSent) / double(mOutput.format.u.raw_audio.frame_rate) * 1000000.0);
	}
	hdr->start_time = stamp;

	return buf;
}

// waveform generators - fill buffers with various waveforms
void
ToneProducer::FillSineBuffer(float *data, size_t numFrames, bool stereo)
{


	// cover 2pi radians in one period
	double dTheta = 2*M_PI * double(mFrequency) / mOutput.format.u.raw_audio.frame_rate;

	// Fill the buffer!
	for (size_t i = 0; i < numFrames; i++, data++)
	{
		float val = mGain * float(sin(mTheta));
		*data = val;
		if(stereo) {
			++data;
			*data = val;
		}

		mTheta += dTheta;
		if (mTheta > 2*M_PI)
		{
			mTheta -= 2*M_PI;
		}
	}
}

void
ToneProducer::FillTriangleBuffer(float *data, size_t numFrames, bool stereo)
{
	// ramp from -1 to 1 and back in one period
	double dTheta = 4.0 * double(mFrequency) / mOutput.format.u.raw_audio.frame_rate;
	if (!mWaveAscending) dTheta = -dTheta;

	// fill the buffer!
	for (size_t i = 0; i < numFrames; i++, data++)
	{
		float val = mGain * mTheta;
		*data = val;
		if(stereo) {
			++data;
			*data = val;
		}

		mTheta += dTheta;
		if (mTheta >= 1)
		{
			mTheta = 2 - mTheta;		// reflect across the mTheta=1 line to preserve drift
			mWaveAscending = false;
			dTheta = -dTheta;
		}
		else if (mTheta <= -1)
		{
			mTheta = -2 - mTheta;		// reflect across mTheta=-1
			mWaveAscending = true;
			dTheta = -dTheta;
		}
	}
}

void
ToneProducer::FillSawtoothBuffer(float *data, size_t numFrames, bool stereo)
{
	// ramp from -1 to 1 in one period
	double dTheta = 2 * double(mFrequency) / mOutput.format.u.raw_audio.frame_rate;
	mWaveAscending = true;

	// fill the buffer!
	for (size_t i = 0; i < numFrames; i++, data++)
	{
		float val = mGain * mTheta;
		*data = val;
		if(stereo) {
			++data;
			*data = val;
		}

		mTheta += dTheta;
		if (mTheta > 1)
		{
			mTheta -= 2;		// back to the base of the sawtooth, including cumulative drift
		}
	}
}

// utility - build the ToneProducer's parameter web
static BParameterWeb* make_parameter_web()
{
	FPRINTF(stderr, "make_parameter_web() called\n");

	BParameterWeb* web = new BParameterWeb;
	BParameterGroup* mainGroup = web->MakeGroup("Tone Generator Parameters");

	BParameterGroup* group = mainGroup->MakeGroup("Frequency");
	BParameter* nullParam = group->MakeNullParameter(FREQUENCY_NULL_PARAM, B_MEDIA_NO_TYPE, "Frequency", B_GENERIC);
	BContinuousParameter* param = group->MakeContinuousParameter(FREQUENCY_PARAM, B_MEDIA_NO_TYPE, "", B_GAIN, "Hz", 0, 2500, 0.1);
	nullParam->AddOutput(param);
	param->AddInput(nullParam);

	group = mainGroup->MakeGroup("Amplitude");
	nullParam = group->MakeNullParameter(GAIN_NULL_PARAM, B_MEDIA_NO_TYPE, "Amplitude", B_GENERIC);
	param = group->MakeContinuousParameter(GAIN_PARAM, B_MEDIA_NO_TYPE, "", B_GAIN, "", 0, 1, 0.01);
	nullParam->AddOutput(param);
	param->AddInput(nullParam);

	group = mainGroup->MakeGroup("Waveform");
	nullParam = group->MakeNullParameter(WAVEFORM_NULL_PARAM, B_MEDIA_NO_TYPE, "Waveform", B_GENERIC);
	BDiscreteParameter* waveParam = group->MakeDiscreteParameter(WAVEFORM_PARAM, B_MEDIA_NO_TYPE, "", B_GENERIC);
	waveParam->AddItem(SINE_WAVE, "Sine wave");
	waveParam->AddItem(TRIANGLE_WAVE, "Triangle");
	waveParam->AddItem(SAWTOOTH_WAVE, "Sawtooth");
	nullParam->AddOutput(waveParam);
	waveParam->AddInput(nullParam);

	return web;
}
