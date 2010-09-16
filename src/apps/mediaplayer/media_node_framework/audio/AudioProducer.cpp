/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2000-2010, Stephan Aßmus <superstippi@gmx.de>,
 * Copyright 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>,
 * All Rights Reserved. Distributed under the terms of the MIT license.
 *
 * Copyright (c) 1998-99, Be Incorporated, All Rights Reserved.
 * Distributed under the terms of the Be Sample Code license.
 */


#include "AudioProducer.h"

#include <math.h>
#include <string.h>
#include <stdio.h>

#include <BufferGroup.h>
#include <Buffer.h>
#include <MediaDefs.h>
#include <ParameterWeb.h>
#include <TimeSource.h>

#include "AudioSupplier.h"
#include "EventQueue.h"
#include "MessageEvent.h"

#define DEBUG_TO_FILE 0

#if DEBUG_TO_FILE
# include <Entry.h>
# include <MediaFormats.h>
# include <MediaFile.h>
# include <MediaTrack.h>
#endif // DEBUG_TO_FILE


// debugging
//#define TRACE_AUDIO_PRODUCER
#ifdef TRACE_AUDIO_PRODUCER
#	define TRACE(x...)		printf(x)
#	define TRACE_BUFFER(x...)
#	define ERROR(x...)		fprintf(stderr, x)
#else
#	define TRACE(x...)
#	define TRACE_BUFFER(x...)
#	define ERROR(x...)		fprintf(stderr, x)
#endif


const static bigtime_t kMaxLatency = 150000;
	// 150 ms is the maximum latency we publish


#if DEBUG_TO_FILE
static BMediaFile*
init_media_file(media_format format, BMediaTrack** _track)
{
	static BMediaFile* file = NULL;
	static BMediaTrack* track = NULL;
	if (file == NULL) {
		entry_ref ref;
		get_ref_for_path("/boot/home/Desktop/test.wav", &ref);

		media_file_format fileFormat;
		int32 cookie = 0;
		while (get_next_file_format(&cookie, &fileFormat) == B_OK) {
			if (strcmp(fileFormat.short_name, "wav") == 0)
				break;
		}
		file = new BMediaFile(&ref, &fileFormat);

		media_codec_info info;
		cookie = 0;
		while (get_next_encoder(&cookie, &info) == B_OK) {
			if (strcmp(info.short_name, "raw-audio") == 0
				|| strcmp(info.short_name, "pcm") == 0) {
				break;
			}
		}

		track = file->CreateTrack(&format, &info);
		if (track == NULL)
			printf("failed to create track\n");

		file->CommitHeader();
	}
	*_track = track;
	return track != NULL ? file : NULL;
}
#endif // DEBUG_TO_FILE


static bigtime_t
estimate_internal_latency(const media_format& format)
{
	bigtime_t startTime = system_time();
	// calculate the number of samples per buffer
	int32 sampleSize = format.u.raw_audio.format
		& media_raw_audio_format::B_AUDIO_SIZE_MASK;
	int32 sampleCount = format.u.raw_audio.buffer_size / sampleSize;
	// alloc float buffers of this size
	const int bufferCount = 10;	// number of input buffers
	float* buffers[bufferCount + 1];
	for (int32 i = 0; i < bufferCount + 1; i++)
		buffers[i] = new float[sampleCount];
	float* outBuffer = buffers[bufferCount];
	// fill all buffers save the last one with arbitrary data and merge them
	// into the last one
	for (int32 i = 0; i < bufferCount; i++) {
		for (int32 k = 0; k < sampleCount; k++) {
			buffers[i][k] = ((float)i * (float)k)
				/ float(bufferCount * sampleCount);
		}
	}
	for (int32 k = 0; k < sampleCount; k++) {
		outBuffer[k] = 0;
		for (int32 i = 0; i < bufferCount; i++)
			outBuffer[k] += buffers[i][k];
		outBuffer[k] /= bufferCount;
	}
	// cleanup
	for (int32 i = 0; i < bufferCount + 1; i++)
		delete[] buffers[i];
	return system_time() - startTime;
}


// #pragma mark -


AudioProducer::AudioProducer(const char* name, AudioSupplier* supplier,
		bool lowLatency)
	:
	BMediaNode(name),
	BBufferProducer(B_MEDIA_RAW_AUDIO),
	BMediaEventLooper(),

	fBufferGroup(NULL),
	fLatency(0),
	fInternalLatency(0),
	fLastLateNotice(0),
	fNextScheduledBuffer(0),
	fLowLatency(lowLatency),
	fOutputEnabled(true),
	fFramesSent(0),
	fStartTime(0),
	fSupplier(supplier),

	fPeakListener(NULL)
{
	TRACE("%p->AudioProducer::AudioProducer(%s, %p, %d)\n", this, name,
		supplier, lowLatency);

	// initialize our preferred format object
	fPreferredFormat.type = B_MEDIA_RAW_AUDIO;
	fPreferredFormat.u.raw_audio.format
		= media_raw_audio_format::B_AUDIO_FLOAT;
//		= media_raw_audio_format::B_AUDIO_SHORT;
	fPreferredFormat.u.raw_audio.byte_order
		= (B_HOST_IS_BENDIAN) ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;
#if 0
	fPreferredFormat.u.raw_audio.channel_count = 2;
	fPreferredFormat.u.raw_audio.frame_rate = 44100.0;

	// NOTE: the (buffer_size * 1000000) needs to be dividable by
	// fPreferredFormat.u.raw_audio.frame_rate!
	fPreferredFormat.u.raw_audio.buffer_size = 441 * 4
		* (fPreferredFormat.u.raw_audio.format
			& media_raw_audio_format::B_AUDIO_SIZE_MASK);

	if (!fLowLatency)
		fPreferredFormat.u.raw_audio.buffer_size *= 3;
#else
	fPreferredFormat.u.raw_audio.channel_count = 0;
	fPreferredFormat.u.raw_audio.frame_rate = 0.0;
	fPreferredFormat.u.raw_audio.buffer_size = 0;
#endif

	// we're not connected yet
	fOutput.destination = media_destination::null;
	fOutput.format = fPreferredFormat;

	// init the audio supplier
	if (fSupplier != NULL) {
		fSupplier->SetAudioProducer(this);
		SetInitialLatency(fSupplier->InitialLatency());
	}
}


AudioProducer::~AudioProducer()
{
	TRACE("%p->AudioProducer::~AudioProducer()\n", this);

#if DEBUG_TO_FILE
	BMediaTrack* track;
	if (BMediaFile* file = init_media_file(fPreferredFormat, &track)) {
		printf("deleting file...\n");
		track->Flush();
		file->ReleaseTrack(track);
		file->CloseFile();
		delete file;
	}
#endif // DEBUG_TO_FILE

	// Stop the BMediaEventLooper thread
	Quit();
	TRACE("AudioProducer::~AudioProducer() done\n");
}


BMediaAddOn*
AudioProducer::AddOn(int32* internalId) const
{
	return NULL;
}


status_t
AudioProducer::FormatSuggestionRequested(media_type type, int32 quality,
	media_format* _format)
{
	TRACE("%p->AudioProducer::FormatSuggestionRequested()\n", this);

	if (!_format)
		return B_BAD_VALUE;

	// This is the format we'll be returning (our preferred format)
	*_format = fPreferredFormat;

	// A wildcard type is okay; we can specialize it, otherwise only raw audio
	// is supported
	if (type != B_MEDIA_UNKNOWN_TYPE && type != B_MEDIA_RAW_AUDIO)
		return B_MEDIA_BAD_FORMAT;

	return B_OK;
}

status_t
AudioProducer::FormatProposal(const media_source& output, media_format* format)
{
	TRACE("%p->AudioProducer::FormatProposal()\n", this);

	// is this a proposal for our one output?
	if (output != fOutput.source) {
		TRACE("  -> B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// Raw audio or wildcard type, either is okay by us. If the format is
	// anything else, overwrite it with our preferred format. Also, we only support
	// floating point audio in the host native byte order at the moment.
	if ((format->type != B_MEDIA_UNKNOWN_TYPE
			&& format->type != B_MEDIA_RAW_AUDIO)
		|| (format->u.raw_audio.format
				!= media_raw_audio_format::wildcard.format
			&& format->u.raw_audio.format
				!= fPreferredFormat.u.raw_audio.format)
		|| (format->u.raw_audio.byte_order
				!= media_raw_audio_format::wildcard.byte_order
			&& format->u.raw_audio.byte_order
				!= fPreferredFormat.u.raw_audio.byte_order)) {
		TRACE("  -> B_MEDIA_BAD_FORMAT\n");
		*format = fPreferredFormat;
		return B_MEDIA_BAD_FORMAT;
	}

	format->type = B_MEDIA_RAW_AUDIO;
	format->u.raw_audio.format = fPreferredFormat.u.raw_audio.format;
	format->u.raw_audio.byte_order = fPreferredFormat.u.raw_audio.byte_order;

	return B_OK;
}


status_t
AudioProducer::FormatChangeRequested(const media_source& source,
	const media_destination& destination, media_format* ioFormat,
	int32* _deprecated_)
{
	TRACE("%p->AudioProducer::FormatChangeRequested()\n", this);

	if (destination != fOutput.destination) {
		TRACE("  -> B_MEDIA_BAD_DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;
	}

	if (source != fOutput.source) {
		TRACE("  -> B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

// TODO: Maybe we are supposed to specialize here only and not actually change yet?
//	status_t ret = _SpecializeFormat(ioFormat);

	return ChangeFormat(ioFormat);
}


status_t
AudioProducer::GetNextOutput(int32* cookie, media_output* _output)
{
	TRACE("%p->AudioProducer::GetNextOutput(%ld)\n", this, *cookie);

	// we have only a single output; if we supported multiple outputs, we'd
	// iterate over whatever data structure we were using to keep track of
	// them.
	if (0 == *cookie) {
		*_output = fOutput;
		*cookie += 1;
		return B_OK;
	}

	return B_BAD_INDEX;
}


status_t
AudioProducer::DisposeOutputCookie(int32 cookie)
{
	// do nothing because we don't use the cookie for anything special
	return B_OK;
}


status_t
AudioProducer::SetBufferGroup(const media_source& forSource,
	BBufferGroup* newGroup)
{
	TRACE("%p->AudioProducer::SetBufferGroup()\n", this);

	if (forSource != fOutput.source)
		return B_MEDIA_BAD_SOURCE;

	if (newGroup == fBufferGroup)
		return B_OK;

	if (fUsingOurBuffers && fBufferGroup)
		delete fBufferGroup;	// waits for all buffers to recycle

	if (newGroup != NULL) {
		// we were given a valid group; just use that one from now on
		fBufferGroup = newGroup;
		fUsingOurBuffers = false;
	} else {
		// we were passed a NULL group pointer; that means we construct
		// our own buffer group to use from now on
		size_t size = fOutput.format.u.raw_audio.buffer_size;
		int32 count = int32(fLatency / BufferDuration() + 1 + 1);
		fBufferGroup = new BBufferGroup(size, count);
		fUsingOurBuffers = true;
	}

	return B_OK;
}


status_t
AudioProducer::GetLatency(bigtime_t* _latency)
{
	TRACE("%p->AudioProducer::GetLatency()\n", this);

	// report our *total* latency:  internal plus downstream plus scheduling
	*_latency = EventLatency() + SchedulingLatency();
	return B_OK;
}


status_t
AudioProducer::PrepareToConnect(const media_source& what,
	const media_destination& where, media_format* format,
	media_source* _source, char* _name)
{
	TRACE("%p->AudioProducer::PrepareToConnect()\n", this);

	// trying to connect something that isn't our source?
	if (what != fOutput.source) {
		TRACE("  -> B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// are we already connected?
	if (fOutput.destination != media_destination::null) {
		TRACE("  -> B_MEDIA_ALREADY_CONNECTED\n");
		return B_MEDIA_ALREADY_CONNECTED;
	}

	status_t ret = _SpecializeFormat(format);
	if (ret != B_OK) {
		TRACE("  -> format error: %s\n", strerror(ret));
		return ret;
	}

	// Now reserve the connection, and return information about it
	fOutput.destination = where;
	fOutput.format = *format;

	if (fSupplier != NULL)
		fSupplier->SetFormat(fOutput.format);

	*_source = fOutput.source;
	strncpy(_name, fOutput.name, B_MEDIA_NAME_LENGTH);
	TRACE("  -> B_OK\n");
	return B_OK;
}


void
AudioProducer::Connect(status_t error, const media_source& source,
	 const media_destination& destination, const media_format& format,
	 char* _name)
{
	TRACE("AudioProducer::Connect(%s)\n", strerror(error));

	// If something earlier failed, Connect() might still be called, but with
	// a non-zero error code.  When that happens we simply unreserve the
	// connection and do nothing else.
	if (error != B_OK) {
		fOutput.destination = media_destination::null;
		fOutput.format = fPreferredFormat;
		return;
	}

	// Okay, the connection has been confirmed.  Record the destination and
	// format that we agreed on, and report our connection name again.
	fOutput.destination = destination;
	fOutput.format = format;
	strncpy(_name, fOutput.name, B_MEDIA_NAME_LENGTH);

	// tell our audio supplier about the format
	if (fSupplier) {
		TRACE("AudioProducer::Connect() fSupplier->SetFormat()\n");
		fSupplier->SetFormat(fOutput.format);
	}

	TRACE("AudioProducer::Connect() FindLatencyFor()\n");

	// Now that we're connected, we can determine our downstream latency.
	// Do so, then make sure we get our events early enough.
	media_node_id id;
	FindLatencyFor(fOutput.destination, &fLatency, &id);

	// Use a dry run to see how long it takes me to fill a buffer of data
	size_t sampleSize = fOutput.format.u.raw_audio.format
		& media_raw_audio_format::B_AUDIO_SIZE_MASK;
	size_t samplesPerBuffer
		= fOutput.format.u.raw_audio.buffer_size / sampleSize;
	fInternalLatency = estimate_internal_latency(fOutput.format);
	if (!fLowLatency)
		fInternalLatency *= 32;
	SetEventLatency(fLatency + fInternalLatency);

	// reset our buffer duration, etc. to avoid later calculations
	bigtime_t duration = bigtime_t(1000000)
		* samplesPerBuffer / bigtime_t(fOutput.format.u.raw_audio.frame_rate
		* fOutput.format.u.raw_audio.channel_count);
	TRACE("AudioProducer::Connect() SetBufferDuration(%lld)\n", duration);
	SetBufferDuration(duration);

	TRACE("AudioProducer::Connect() _AllocateBuffers()\n");

	// Set up the buffer group for our connection, as long as nobody handed
	// us a buffer group (via SetBufferGroup()) prior to this.  That can
	// happen, for example, if the consumer calls SetOutputBuffersFor() on
	// us from within its Connected() method.
	if (fBufferGroup == NULL)
		_AllocateBuffers(fOutput.format);

	TRACE("AudioProducer::Connect() done\n");
}


void
AudioProducer::Disconnect(const media_source& what,
	const media_destination& where)
{
	TRACE("%p->AudioProducer::Disconnect()\n", this);

	// Make sure that our connection is the one being disconnected
	if (where == fOutput.destination && what == fOutput.source) {
		fOutput.destination = media_destination::null;
		fOutput.format = fPreferredFormat;
		TRACE("AudioProducer:  deleting buffer group...\n");
		// Always delete the buffer group, even if it is not ours.
		// (See BeBook::SetBufferGroup()).
		delete fBufferGroup;
		TRACE("AudioProducer:  buffer group deleted\n");
		fBufferGroup = NULL;
	}

	TRACE("%p->AudioProducer::Disconnect() done\n", this);
}


void
AudioProducer::LateNoticeReceived(const media_source& what, bigtime_t howMuch,
	bigtime_t performanceTime)
{
	TRACE("%p->AudioProducer::LateNoticeReceived(%lld, %lld)\n", this, howMuch,
		performanceTime);
	// If we're late, we need to catch up. Respond in a manner appropriate
	// to our current run mode.
	if (what == fOutput.source) {
		// Ignore the notices for buffers we already send out (or scheduled
		// their event) before we processed the last notice
		if (fLastLateNotice > performanceTime)
			return;

		fLastLateNotice = fNextScheduledBuffer;

		if (RunMode() == B_RECORDING) {
			// ...
		} else if (RunMode() == B_INCREASE_LATENCY) {
			fInternalLatency += howMuch;

			// At some point a too large latency can get annoying
			if (fInternalLatency > kMaxLatency)
				fInternalLatency = kMaxLatency;

			SetEventLatency(fLatency + fInternalLatency);
		} else {
			// Skip one buffer ahead in the audio data.
			size_t sampleSize
				= fOutput.format.u.raw_audio.format
					& media_raw_audio_format::B_AUDIO_SIZE_MASK;
			size_t samplesPerBuffer
				= fOutput.format.u.raw_audio.buffer_size / sampleSize;
			size_t framesPerBuffer
				= samplesPerBuffer / fOutput.format.u.raw_audio.channel_count;
			fFramesSent += framesPerBuffer;
		}
	}
}


void
AudioProducer::EnableOutput(const media_source& what, bool enabled,
	int32* _deprecated_)
{
	TRACE("%p->AudioProducer::EnableOutput(%d)\n", this, enabled);

	if (what == fOutput.source)
		fOutputEnabled = enabled;
}


status_t
AudioProducer::SetPlayRate(int32 numer, int32 denom)
{
	return B_ERROR;
}


status_t
AudioProducer::HandleMessage(int32 message, const void *data, size_t size)
{
	TRACE("%p->AudioProducer::HandleMessage()\n", this);
	return B_ERROR;
}


void
AudioProducer::AdditionalBufferRequested(const media_source& source,
	media_buffer_id prevBuffer, bigtime_t prevTime,
	const media_seek_tag *prevTag)
{
	TRACE("%p->AudioProducer::AdditionalBufferRequested()\n", this);
}


void
AudioProducer::LatencyChanged(const media_source& source,
	const media_destination& destination, bigtime_t newLatency, uint32 flags)
{
	TRACE("%p->AudioProducer::LatencyChanged(%lld)\n", this, newLatency);

	if (source == fOutput.source && destination == fOutput.destination) {
		fLatency = newLatency;
		SetEventLatency(fLatency + fInternalLatency);
	}
}


void
AudioProducer::NodeRegistered()
{
	TRACE("%p->AudioProducer::NodeRegistered()\n", this);

	// Start the BMediaEventLooper thread
	SetPriority(B_REAL_TIME_PRIORITY);
	Run();

	// set up as much information about our output as we can
	fOutput.source.port = ControlPort();
	fOutput.source.id = 0;
	fOutput.node = Node();
	::strcpy(fOutput.name, Name());
}


void
AudioProducer::SetRunMode(run_mode mode)
{
	TRACE("%p->AudioProducer::SetRunMode()\n", this);

	if (B_OFFLINE == mode)
		ReportError(B_NODE_FAILED_SET_RUN_MODE);
	else
		BBufferProducer::SetRunMode(mode);
}


void
AudioProducer::HandleEvent(const media_timed_event* event, bigtime_t lateness,
	bool realTimeEvent)
{
	TRACE_BUFFER("%p->AudioProducer::HandleEvent()\n", this);

	switch (event->type) {
		case BTimedEventQueue::B_START:
			TRACE("AudioProducer::HandleEvent(B_START)\n");
			if (RunState() != B_STARTED) {
				fFramesSent = 0;
				fStartTime = event->event_time + fSupplier->InitialLatency();
				media_timed_event firstBufferEvent(
					fStartTime - fSupplier->InitialLatency(),
					BTimedEventQueue::B_HANDLE_BUFFER);
				EventQueue()->AddEvent(firstBufferEvent);
			}
			TRACE("AudioProducer::HandleEvent(B_START) done\n");
			break;

		case BTimedEventQueue::B_STOP:
			TRACE("AudioProducer::HandleEvent(B_STOP)\n");
			EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true,
				BTimedEventQueue::B_HANDLE_BUFFER);
			TRACE("AudioProducer::HandleEvent(B_STOP) done\n");
			break;

		case BTimedEventQueue::B_HANDLE_BUFFER:
		{
			TRACE_BUFFER("AudioProducer::HandleEvent(B_HANDLE_BUFFER)\n");
			if (RunState() == BMediaEventLooper::B_STARTED
				&& fOutput.destination != media_destination::null) {
				BBuffer* buffer = _FillNextBuffer(event->event_time);
				if (buffer != NULL) {
					status_t err = B_ERROR;
					if (fOutputEnabled) {
						err = SendBuffer(buffer, fOutput.source,
							fOutput.destination);
					}
					if (err != B_OK)
						buffer->Recycle();
				}
				size_t sampleSize = fOutput.format.u.raw_audio.format
						& media_raw_audio_format::B_AUDIO_SIZE_MASK;

				size_t nFrames = fOutput.format.u.raw_audio.buffer_size
					/ (sampleSize * fOutput.format.u.raw_audio.channel_count);
				fFramesSent += nFrames;

				fNextScheduledBuffer = fStartTime
					+ bigtime_t(double(fFramesSent) * 1000000.0
						/ double(fOutput.format.u.raw_audio.frame_rate));
				media_timed_event nextBufferEvent(fNextScheduledBuffer,
					BTimedEventQueue::B_HANDLE_BUFFER);
				EventQueue()->AddEvent(nextBufferEvent);
			} else {
				ERROR("B_HANDLE_BUFFER, but not started!\n");
			}
			TRACE_BUFFER("AudioProducer::HandleEvent(B_HANDLE_BUFFER) done\n");
			break;
		}
		default:
			break;
	}
}


void
AudioProducer::SetPeakListener(BHandler* handler)
{
	fPeakListener = handler;
}


status_t
AudioProducer::ChangeFormat(media_format* format)
{
	TRACE("AudioProducer::ChangeFormat()\n");

	format->u.raw_audio.buffer_size
		= media_raw_audio_format::wildcard.buffer_size;

	status_t ret = _SpecializeFormat(format);
	if (ret != B_OK) {
		TRACE("  _SpecializeFormat(): %s\n", strerror(ret));
		return ret;
	}

	ret = BBufferProducer::ProposeFormatChange(format, fOutput.destination);
	if (ret != B_OK) {
		TRACE("  ProposeFormatChange(): %s\n", strerror(ret));
		return ret;
	}

	ret = BBufferProducer::ChangeFormat(fOutput.source, fOutput.destination,
		format);
	if (ret != B_OK) {
		TRACE("  ChangeFormat(): %s\n", strerror(ret));
		return ret;
	}

	return _ChangeFormat(*format);
}


// #pragma mark -


status_t
AudioProducer::_SpecializeFormat(media_format* format)
{
	// the format may not yet be fully specialized (the consumer might have
	// passed back some wildcards).  Finish specializing it now, and return an
	// error if we don't support the requested format.
	if (format->type != B_MEDIA_RAW_AUDIO) {
		TRACE("  not raw audio\n");
		return B_MEDIA_BAD_FORMAT;
// TODO: we might want to support different audio formats
	} else if (format->u.raw_audio.format
			!= fPreferredFormat.u.raw_audio.format) {
		TRACE("  format does not match\n");
		return B_MEDIA_BAD_FORMAT;
	}

	if (format->u.raw_audio.channel_count
			== media_raw_audio_format::wildcard.channel_count) {
		format->u.raw_audio.channel_count = 2;
		TRACE("  -> adjusting channel count, it was wildcard\n");
	}

	if (format->u.raw_audio.frame_rate
			== media_raw_audio_format::wildcard.frame_rate) {
		format->u.raw_audio.frame_rate = 44100.0;
		TRACE("  -> adjusting frame rate, it was wildcard\n");
	}

	// check the buffer size, which may still be wildcarded
	if (format->u.raw_audio.buffer_size
			== media_raw_audio_format::wildcard.buffer_size) {
		// pick something comfortable to suggest
		TRACE("  -> adjusting buffer size, it was wildcard\n");

		// NOTE: the (buffer_size * 1000000) needs to be dividable by
		// format->u.raw_audio.frame_rate! (We assume frame rate is a multiple of
		// 25, which it usually is.)
		format->u.raw_audio.buffer_size
			= uint32(format->u.raw_audio.frame_rate / 25.0)
				* format->u.raw_audio.channel_count
				* (format->u.raw_audio.format
					& media_raw_audio_format::B_AUDIO_SIZE_MASK);

		if (!fLowLatency)
			format->u.raw_audio.buffer_size *= 3;
	}

	return B_OK;
}


status_t
AudioProducer::_ChangeFormat(const media_format& format)
{
	fOutput.format = format;

	// notify our audio supplier of the format change
	if (fSupplier)
		fSupplier->SetFormat(format);

	return _AllocateBuffers(format);
}


status_t
AudioProducer::_AllocateBuffers(const media_format& format)
{
	TRACE("%p->AudioProducer::_AllocateBuffers()\n", this);

	if (fBufferGroup && fUsingOurBuffers) {
		delete fBufferGroup;
		fBufferGroup = NULL;
	}
	size_t size = format.u.raw_audio.buffer_size;
	int32 bufferDuration = BufferDuration();
	int32 count = 0;
	if (bufferDuration > 0) {
		count = (int32)((fLatency + fInternalLatency)
			/ bufferDuration + 2);
	}

	fBufferGroup = new BBufferGroup(size, count);
	fUsingOurBuffers = true;
	return fBufferGroup->InitCheck();
}


BBuffer*
AudioProducer::_FillNextBuffer(bigtime_t eventTime)
{
	fBufferGroup->WaitForBuffers();
	BBuffer* buffer = fBufferGroup->RequestBuffer(
		fOutput.format.u.raw_audio.buffer_size, BufferDuration());

	if (buffer == NULL) {
		static bool errorPrinted = false;
		if (!errorPrinted) {
			ERROR("AudioProducer::_FillNextBuffer() - no buffer "
				"(size: %ld, duration: %lld)\n",
				fOutput.format.u.raw_audio.buffer_size, BufferDuration());
			errorPrinted = true;
		}
		return NULL;
	}

	size_t sampleSize = fOutput.format.u.raw_audio.format
		& media_raw_audio_format::B_AUDIO_SIZE_MASK;
	size_t numSamples = fOutput.format.u.raw_audio.buffer_size / sampleSize;
		// number of sample in the buffer

	// fill in the buffer header
	media_header* header = buffer->Header();
	header->type = B_MEDIA_RAW_AUDIO;
	header->time_source = TimeSource()->ID();
	buffer->SetSizeUsed(fOutput.format.u.raw_audio.buffer_size);

	// fill in data from audio supplier
	int64 frameCount = numSamples / fOutput.format.u.raw_audio.channel_count;
	bigtime_t startTime = bigtime_t(double(fFramesSent)
		* 1000000.0 / fOutput.format.u.raw_audio.frame_rate);
	bigtime_t endTime = bigtime_t(double(fFramesSent + frameCount)
		* 1000000.0 / fOutput.format.u.raw_audio.frame_rate);

	if (fSupplier == NULL || fSupplier->InitCheck() != B_OK
		|| fSupplier->GetFrames(buffer->Data(), frameCount, startTime,
			endTime) != B_OK) {
		ERROR("AudioProducer::_FillNextBuffer() - supplier error -> silence\n");
		memset(buffer->Data(), 0, buffer->SizeUsed());
	}

	// stamp buffer
	if (RunMode() == B_RECORDING)
		header->start_time = eventTime;
	else
		header->start_time = fStartTime + startTime;

#if DEBUG_TO_FILE
	BMediaTrack* track;
	if (init_media_file(fOutput.format, &track) != NULL)
		track->WriteFrames(buffer->Data(), frameCount);
#endif // DEBUG_TO_FILE

	if (fPeakListener
		&& fOutput.format.u.raw_audio.format
			== media_raw_audio_format::B_AUDIO_FLOAT) {
		// TODO: extend the peak notifier for other sample formats
		int32 channels = fOutput.format.u.raw_audio.channel_count;
		float max[channels];
		float min[channels];
		for (int32 i = 0; i < channels; i++) {
			max[i] = -1.0;
			min[i] = 1.0;
		}
		float* sample = (float*)buffer->Data();
		for (uint32 i = 0; i < frameCount; i++) {
			for (int32 k = 0; k < channels; k++) {
				if (*sample < min[k])
					min[k] = *sample;
				if (*sample > max[k])
					max[k] = *sample;
				sample++;
			}
		}
		BMessage message(MSG_PEAK_NOTIFICATION);
		for (int32 i = 0; i < channels; i++) {
			float maxAbs = max_c(fabs(min[i]), fabs(max[i]));
			message.AddFloat("max", maxAbs);
		}
		bigtime_t realTime = TimeSource()->RealTimeFor(
			fStartTime + startTime, 0);
		MessageEvent* event = new (std::nothrow) MessageEvent(realTime,
			fPeakListener, message);
		if (event != NULL)
			EventQueue::Default().AddEvent(event);
	}

	return buffer;
}

