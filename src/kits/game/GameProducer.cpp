/*
 * Copyright 2002-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christopher ML Zumwalt May (zummy@users.sf.net)
 */


/*!	A MediaKit producer node which mixes sound from the GameKit
	and sends them to the audio mixer
*/


#include <string.h>
#include <stdio.h>

#include <Buffer.h>
#include <BufferGroup.h>
#include <ByteOrder.h>
#include <List.h>
#include <MediaDefs.h>
#include <TimeSource.h>

#include "GameSoundBuffer.h"
#include "GameSoundDevice.h"
#include "GSUtility.h"

#include "GameProducer.h"


struct _gs_play  {
	gs_id		sound;
	bool*		hook;
	
	_gs_play*	next;
	_gs_play*	previous;
};


GameProducer::GameProducer(GameSoundBuffer* object,
		const gs_audio_format* format)
	:
	BMediaNode("GameProducer.h"),
	BBufferProducer(B_MEDIA_RAW_AUDIO),
	BMediaEventLooper(),
	fBufferGroup(NULL),
	fLatency(0),
	fInternalLatency(0),
	fOutputEnabled(true)
{
	// initialize our preferred format object
	fPreferredFormat.type = B_MEDIA_RAW_AUDIO;
	fPreferredFormat.u.raw_audio.format = format->format;
	fPreferredFormat.u.raw_audio.channel_count = format->channel_count;
	fPreferredFormat.u.raw_audio.frame_rate = format->frame_rate;		// measured in Hertz
	fPreferredFormat.u.raw_audio.byte_order = format->byte_order;
//	fPreferredFormat.u.raw_audio.channel_mask = B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
//	fPreferredFormat.u.raw_audio.valid_bits = 32;
//	fPreferredFormat.u.raw_audio.matrix_mask = B_MATRIX_AMBISONIC_WXYZ;
	
	// we'll use the consumer's preferred buffer size, if any
	fPreferredFormat.u.raw_audio.buffer_size = media_raw_audio_format::wildcard.buffer_size;

	// we're not connected yet
	fOutput.destination = media_destination::null;
	fOutput.format = fPreferredFormat;
	
	fFrameSize = get_sample_size(format->format) * format->channel_count;
	fObject = object;
}


GameProducer::~GameProducer()
{
	// Stop the BMediaEventLooper thread
	Quit();
}


// BMediaNode methods
BMediaAddOn *
GameProducer::AddOn(int32 *internal_id) const
{
	return NULL;
}


// BBufferProducer methods
status_t 
GameProducer::GetNextOutput(int32* cookie, media_output* out_output)
{
	// we currently support only one output
	if (0 != *cookie) return B_BAD_INDEX;
	
	*out_output = fOutput;
	*cookie += 1;
	return B_OK;
}


status_t 
GameProducer::DisposeOutputCookie(int32 cookie)
{
	// do nothing because our cookie is only an integer
	return B_OK;
}


void 
GameProducer::EnableOutput(const media_source& what, bool enabled, int32* _deprecated_)
{
	// If I had more than one output, I'd have to walk my list of output records to see
	// which one matched the given source, and then enable/disable that one.  But this
	// node only has one output, so I just make sure the given source matches, then set
	// the enable state accordingly.
	if (what == fOutput.source)
	{
		fOutputEnabled = enabled;
	}
}


status_t 
GameProducer::FormatSuggestionRequested(media_type type, int32 /*quality*/, media_format* format)
{
	// insure that we received a format 
	if (!format)
		return B_BAD_VALUE;

	// returning our preferred format
	*format = fPreferredFormat;

	// our format is supported
	if (type == B_MEDIA_UNKNOWN_TYPE) return B_OK;

	// we only support raw audo
	return (type != B_MEDIA_RAW_AUDIO) ? B_MEDIA_BAD_FORMAT : B_OK;
}


status_t 
GameProducer::FormatProposal(const media_source& output, media_format* format)
{
	// doest the proposed output match our output?
	if (output != fOutput.source) return B_MEDIA_BAD_SOURCE;

	// return our preferred format
	*format = fPreferredFormat;
	
	// we will reject the proposal if the format is not audio
	media_type requestedType = format->type;
	if ((requestedType != B_MEDIA_UNKNOWN_TYPE) && (requestedType != B_MEDIA_RAW_AUDIO))
		return B_MEDIA_BAD_FORMAT;
	
	return B_OK;		// raw audio or wildcard type, either is okay by us
}


status_t 
GameProducer::PrepareToConnect(const media_source& what, const media_destination& where, media_format* format, media_source* out_source, char* out_name)
{
	// The format has been processed by the consumer at this point. We need
	// to insure the format is still acceptable and any wild care are filled in.

	// trying to connect something that isn't our source?
	if (what != fOutput.source) return B_MEDIA_BAD_SOURCE;

	// are we already connected?
	if (fOutput.destination != media_destination::null) return B_MEDIA_ALREADY_CONNECTED;

	// the format may not yet be fully specialized (the consumer might have
	// passed back some wildcards).  Finish specializing it now, and return an
	// error if we don't support the requested format.
	if (format->type != B_MEDIA_RAW_AUDIO) 
		return B_MEDIA_BAD_FORMAT;
	
	if (format->u.raw_audio.format != fPreferredFormat.u.raw_audio.format)
		return B_MEDIA_BAD_FORMAT;

	// check the buffer size, which may still be wildcarded
	if (format->u.raw_audio.buffer_size == media_raw_audio_format::wildcard.buffer_size)
		format->u.raw_audio.buffer_size = 4096;		// pick something comfortable to suggest

	// Now reserve the connection, and return information about it
	fOutput.destination = where;
	fOutput.format = *format;
	*out_source = fOutput.source;
	strlcpy(out_name, fOutput.name, B_MEDIA_NAME_LENGTH);
	return B_OK;
}


void 
GameProducer::Connect(status_t error, const media_source& source, const media_destination& destination, const media_format& format, char* io_name)
{
	// If something earlier failed, Connect() might still be called, but with a non-zero
	// error code.  When that happens we simply unreserve the connection and do
	// nothing else.
	if (error) {
		fOutput.destination = media_destination::null;
		fOutput.format = fPreferredFormat;
		return;
	}

	// Okay, the connection has been confirmed.  Record the destination and format
	// that we agreed on, and report our connection name again.
	fOutput.destination = destination;
	fOutput.format = format;
	strlcpy(io_name, fOutput.name, B_MEDIA_NAME_LENGTH);

	// Now that we're connected, we can determine our downstream latency.
	// Do so, then make sure we get our events early enough.
	media_node_id id;
	FindLatencyFor(fOutput.destination, &fLatency, &id);

	// Use a dry run to see how long it takes me to fill a buffer of data
		
	// The first step to setup the buffer
	bigtime_t start, produceLatency;
	int32 frames = int32(fOutput.format.u.raw_audio.buffer_size / fFrameSize);
	float* data = new float[frames * 2];
	
	// Second, fill the buffer
	start = ::system_time();
	for (int32 i = 0; i < frames; i++) {
		data[i*2] = 0.8 * float(i/frames);
		data[i*2+1] = 0.8 * float(i/frames);
	}
	produceLatency = ::system_time();
		
	// Third, calculate the latency
	fInternalLatency = produceLatency - start;
	SetEventLatency(fLatency + fInternalLatency);
			
	// Finaily, clean up
	delete [] data;
		
	// reset our buffer duration, etc. to avoid later calculations
	bigtime_t duration = bigtime_t(1000000) * frames / bigtime_t(fOutput.format.u.raw_audio.frame_rate);
	SetBufferDuration(duration);

	// Set up the buffer group for our connection, as long as nobody handed us a
	// buffer group (via SetBufferGroup()) prior to this.  
	if (!fBufferGroup) {
		size_t size = fOutput.format.u.raw_audio.buffer_size;
		int32 count = int32(fLatency / BufferDuration() + 2);
		fBufferGroup = new BBufferGroup(size, count);
	}
}


void 
GameProducer::Disconnect(const media_source& what, const media_destination& where)
{
	// Make sure that our connection is the one being disconnected
	if ((where == fOutput.destination) && (what == fOutput.source)) {
		fOutput.destination = media_destination::null;
		fOutput.format = fPreferredFormat;
		delete fBufferGroup;
		fBufferGroup = NULL;
	}
}


status_t 
GameProducer::FormatChangeRequested(const media_source& source, const media_destination& destination, media_format* io_format, int32* _deprecated_)
{
	// we don't support any other formats, so we just reject any format changes.
	return B_ERROR;
}


status_t 
GameProducer::SetBufferGroup(const media_source& for_source, BBufferGroup* newGroup)
{
	// verify that we didn't get bogus arguments before we proceed
	if (for_source != fOutput.source)
		return B_MEDIA_BAD_SOURCE;

	// Are we being passed the buffer group we're already using?
	if (newGroup == fBufferGroup)
		return B_OK;

	// Ahh, someone wants us to use a different buffer group.  At this point we delete
	// the one we are using and use the specified one instead.  If the specified group is
	// NULL, we need to recreate one ourselves, and use *that*.  Note that if we're
	// caching a BBuffer that we requested earlier, we have to Recycle() that buffer
	// *before* deleting the buffer group, otherwise we'll deadlock waiting for that
	// buffer to be recycled!
	delete fBufferGroup;		// waits for all buffers to recycle
	if (newGroup != NULL) {
		// we were given a valid group; just use that one from now on
		fBufferGroup = newGroup;
	} else {
		// we were passed a NULL group pointer; that means we construct
		// our own buffer group to use from now on
		size_t size = fOutput.format.u.raw_audio.buffer_size;
		int32 count = int32(fLatency / BufferDuration() + 2);
		fBufferGroup = new BBufferGroup(size, count);
	}

	return B_OK;
}


status_t 
GameProducer::GetLatency(bigtime_t* out_latency)
{
	// report our *total* latency:  internal plus downstream plus scheduling
	*out_latency = EventLatency() + SchedulingLatency();
	return B_OK;
}


void 
GameProducer::LateNoticeReceived(const media_source& what, bigtime_t how_much, bigtime_t performance_time)
{
	// If we're late, we need to catch up.  Respond in a manner appropriate to our
	// current run mode.
	if (what == fOutput.source) {
		if (RunMode() == B_RECORDING) {
			// A hardware capture node can't adjust; it simply emits buffers at
			// appropriate points.  We (partially) simulate this by not adjusting
			// our behavior upon receiving late notices -- after all, the hardware
			// can't choose to capture "sooner"....
		} else if (RunMode() == B_INCREASE_LATENCY) {
			// We're late, and our run mode dictates that we try to produce buffers
			// earlier in order to catch up.  This argues that the downstream nodes are
			// not properly reporting their latency, but there's not much we can do about
			// that at the moment, so we try to start producing buffers earlier to
			// compensate.
			fInternalLatency += how_much;
			SetEventLatency(fLatency + fInternalLatency);
		} else {
			// The other run modes dictate various strategies for sacrificing data quality
			// in the interests of timely data delivery.  The way *we* do this is to skip
			// a buffer, which catches us up in time by one buffer duration.
			size_t nSamples = fOutput.format.u.raw_audio.buffer_size / fFrameSize;
			fFramesSent += nSamples;
		}
	}
}


void 
GameProducer::LatencyChanged(const media_source& source, const media_destination& destination, bigtime_t new_latency, uint32 flags)
{
	// something downstream changed latency, so we need to start producing
	// buffers earlier (or later) than we were previously.  Make sure that the
	// connection that changed is ours, and adjust to the new downstream
	// latency if so.
	if ((source == fOutput.source) && (destination == fOutput.destination)) {
		fLatency = new_latency;
		SetEventLatency(fLatency + fInternalLatency);
	}
}


status_t 
GameProducer::SetPlayRate(int32 numer, int32 denom)
{
	// Play rates are weird.  We don't support them
	return B_ERROR;
}


status_t 
GameProducer::HandleMessage(int32 message, const void* data, size_t size)
{
	// We currently do not handle private messages
	return B_ERROR;
}


void 
GameProducer::AdditionalBufferRequested(const media_source& source, media_buffer_id prev_buffer, bigtime_t prev_time, const media_seek_tag* prev_tag)
{
	// we don't support offline mode (yet...)
	return;
}


// BMediaEventLooper methods
void 
GameProducer::NodeRegistered()
{
	// Start the BMediaEventLooper thread
	SetPriority(B_REAL_TIME_PRIORITY);
	Run();

	// set up as much information about our output as we can
	fOutput.source.port = ControlPort();
	fOutput.source.id = 0;
	fOutput.node = Node();
	strlcpy(fOutput.name, "GameProducer Output", B_MEDIA_NAME_LENGTH);
}


void 
GameProducer::SetRunMode(run_mode mode)
{
	// We don't support offline run mode, so broadcast an error if we're set to
	// B_OFFLINE.  Unfortunately, we can't actually reject the mode change...
	if (B_OFFLINE == mode) {
		ReportError(B_NODE_FAILED_SET_RUN_MODE);
	}
}


void 
GameProducer::HandleEvent(const media_timed_event* event, bigtime_t lateness, bool realTimeEvent)
{
//	FPRINTF(stderr, "ToneProducer::HandleEvent\n");
	switch (event->type)
	{
	case BTimedEventQueue::B_START:
		// don't do anything if we're already running
		if (RunState() != B_STARTED) {
			// We are going to start sending buffers so setup the needed bookkeeping
			fFramesSent = 0;
			fStartTime = event->event_time;
			media_timed_event firstBufferEvent(fStartTime, BTimedEventQueue::B_HANDLE_BUFFER);

			// Alternatively, we could call HandleEvent() directly with this event, to avoid a trip through
			// the event queue, like this:
			//
			//		this->HandleEvent(&firstBufferEvent, 0, false);
			//
			EventQueue()->AddEvent(firstBufferEvent);
		}
		break;

	case BTimedEventQueue::B_STOP:
		// When we handle a stop, we must ensure that downstream consumers don't
		// get any more buffers from us.  This means we have to flush any pending
		// buffer-producing events from the queue.
		EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
		break;

	case BTimedEventQueue::B_HANDLE_BUFFER:
		{
			// make sure we're both started *and* connected before delivering a buffer
			if ((RunState() == BMediaEventLooper::B_STARTED) 
				&& (fOutput.destination != media_destination::null)) {
				// Get the next buffer of data
				BBuffer* buffer = FillNextBuffer(event->event_time);
				if (buffer) {
					// send the buffer downstream if and only if output is enabled
					status_t err = B_ERROR;
					if (fOutputEnabled) {
						err = SendBuffer(buffer, fOutput.source,
							fOutput.destination);
					}
					if (err) {
						// we need to recycle the buffer ourselves if output is disabled or
						// if the call to SendBuffer() fails
						buffer->Recycle();
					}
				}
				
				// track how much media we've delivered so far
				size_t nFrames = fOutput.format.u.raw_audio.buffer_size / fFrameSize;
				fFramesSent += nFrames;

				// The buffer is on its way; now schedule the next one to go
				bigtime_t nextEvent = fStartTime + bigtime_t(double(fFramesSent) / double(fOutput.format.u.raw_audio.frame_rate) * 1000000.0);
				media_timed_event nextBufferEvent(nextEvent, BTimedEventQueue::B_HANDLE_BUFFER);
				EventQueue()->AddEvent(nextBufferEvent);
			}
		}
		break;

	default:
		break;
	}
}


BBuffer* 
GameProducer::FillNextBuffer(bigtime_t event_time)
{	
	// get a buffer from our buffer group
	BBuffer* buf = fBufferGroup->RequestBuffer(fOutput.format.u.raw_audio.buffer_size, BufferDuration());

	// if we fail to get a buffer (for example, if the request times out), we skip this
	// buffer and go on to the next, to avoid locking up the control thread
	if (!buf) 
		return NULL;

	// we need to discribe the buffer
	int64 frames = int64(fOutput.format.u.raw_audio.buffer_size / fFrameSize);
	memset(buf->Data(), 0, fOutput.format.u.raw_audio.buffer_size);
		
	// now fill the buffer with data, continuing where the last buffer left off
	fObject->Play(buf->Data(), frames);
	 		
	// fill in the buffer header
	media_header* hdr = buf->Header();
	hdr->type = B_MEDIA_RAW_AUDIO;
	hdr->size_used = fOutput.format.u.raw_audio.buffer_size;
	hdr->time_source = TimeSource()->ID();
	
	bigtime_t stamp;
	if (RunMode() == B_RECORDING) {
		// In B_RECORDING mode, we stamp with the capture time.  We're not
		// really a hardware capture node, but we simulate it by using the (precalculated)
		// time at which this buffer "should" have been created.
		stamp = event_time;
	} else {
		// okay, we're in one of the "live" performance run modes.  in these modes, we
		// stamp the buffer with the time at which the buffer should be rendered to the
		// output, not with the capture time.  fStartTime is the cached value of the
		// first buffer's performance time; we calculate this buffer's performance time as
		// an offset from that time, based on the amount of media we've created so far.
		// Recalculating every buffer like this avoids accumulation of error.
		stamp = fStartTime + bigtime_t(double(fFramesSent) / double(fOutput.format.u.raw_audio.frame_rate) * 1000000.0);
	}
	hdr->start_time = stamp;

	return buf;
}

