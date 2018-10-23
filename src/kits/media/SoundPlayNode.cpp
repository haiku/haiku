/*
 * Copyright 2002-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marcus Overhagen
 *		Jérôme Duval
 */


/*!	This is the BBufferProducer used internally by BSoundPlayer.
*/


#include "SoundPlayNode.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <TimeSource.h>
#include <MediaRoster.h>
#include "MediaDebug.h"


#define SEND_NEW_BUFFER_EVENT (BTimedEventQueue::B_USER_EVENT + 1)


namespace BPrivate {


SoundPlayNode::SoundPlayNode(const char* name, BSoundPlayer* player)
	:
	BMediaNode(name),
	BBufferProducer(B_MEDIA_RAW_AUDIO),
	BMediaEventLooper(),
	fPlayer(player),
	fInitStatus(B_OK),
	fOutputEnabled(true),
	fBufferGroup(NULL),
	fFramesSent(0),
	fTooEarlyCount(0)
{
	CALLED();
	fOutput.format.type = B_MEDIA_RAW_AUDIO;
	fOutput.format.u.raw_audio = media_multi_audio_format::wildcard;
}


SoundPlayNode::~SoundPlayNode()
{
	CALLED();
	Quit();
}


bool
SoundPlayNode::IsPlaying()
{
	return RunState() == B_STARTED;
}


bigtime_t
SoundPlayNode::CurrentTime()
{
	int frameRate = (int)fOutput.format.u.raw_audio.frame_rate;
	return frameRate == 0 ? 0
		: bigtime_t((1000000LL * fFramesSent) / frameRate);
}


media_multi_audio_format
SoundPlayNode::Format() const
{
	return fOutput.format.u.raw_audio;
}


// #pragma mark - implementation of BMediaNode


BMediaAddOn*
SoundPlayNode::AddOn(int32* _internalID) const
{
	CALLED();
	// This only gets called if we are in an add-on.
	return NULL;
}


void
SoundPlayNode::Preroll()
{
	CALLED();
	// TODO: Performance opportunity
	BMediaNode::Preroll();
}


status_t
SoundPlayNode::HandleMessage(int32 message, const void* data, size_t size)
{
	CALLED();
	return B_ERROR;
}


void
SoundPlayNode::NodeRegistered()
{
	CALLED();

	if (fInitStatus != B_OK) {
		ReportError(B_NODE_IN_DISTRESS);
		return;
	}

	SetPriority(B_URGENT_PRIORITY);

	fOutput.format.type = B_MEDIA_RAW_AUDIO;
	fOutput.format.u.raw_audio = media_multi_audio_format::wildcard;
	fOutput.destination = media_destination::null;
	fOutput.source.port = ControlPort();
	fOutput.source.id = 0;
	fOutput.node = Node();
	strcpy(fOutput.name, Name());

	Run();
}


status_t
SoundPlayNode::RequestCompleted(const media_request_info& info)
{
	CALLED();
	return B_OK;
}


void
SoundPlayNode::SetTimeSource(BTimeSource* timeSource)
{
	CALLED();
	BMediaNode::SetTimeSource(timeSource);
}


void
SoundPlayNode::SetRunMode(run_mode mode)
{
	TRACE("SoundPlayNode::SetRunMode mode:%i\n", mode);
	BMediaNode::SetRunMode(mode);
}


// #pragma mark - implementation for BBufferProducer


status_t
SoundPlayNode::FormatSuggestionRequested(media_type type, int32 /*quality*/,
	media_format* format)
{
	// FormatSuggestionRequested() is not necessarily part of the format
	// negotiation process; it's simply an interrogation -- the caller wants
	// to see what the node's preferred data format is, given a suggestion by
	// the caller.
	CALLED();

	// a wildcard type is okay; but we only support raw audio
	if (type != B_MEDIA_RAW_AUDIO && type != B_MEDIA_UNKNOWN_TYPE)
		return B_MEDIA_BAD_FORMAT;

	// this is the format we'll be returning (our preferred format)
	format->type = B_MEDIA_RAW_AUDIO;
	format->u.raw_audio = media_multi_audio_format::wildcard;

	return B_OK;
}


status_t
SoundPlayNode::FormatProposal(const media_source& output, media_format* format)
{
	// FormatProposal() is the first stage in the BMediaRoster::Connect()
	// process. We hand out a suggested format, with wildcards for any
	// variations we support.
	CALLED();

	// is this a proposal for our one output?
	if (output != fOutput.source) {
		TRACE("SoundPlayNode::FormatProposal returning B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// if wildcard, change it to raw audio
	if (format->type == B_MEDIA_UNKNOWN_TYPE)
		format->type = B_MEDIA_RAW_AUDIO;

	// if not raw audio, we can't support it
	if (format->type != B_MEDIA_RAW_AUDIO) {
		TRACE("SoundPlayNode::FormatProposal returning B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}

#if DEBUG >0
	char buf[100];
	string_for_format(*format, buf, sizeof(buf));
	TRACE("SoundPlayNode::FormatProposal: format %s\n", buf);
#endif

	return B_OK;
}


status_t
SoundPlayNode::FormatChangeRequested(const media_source& source,
	const media_destination& destination, media_format* _format,
	int32* /* deprecated */)
{
	CALLED();

	// we don't support any other formats, so we just reject any format changes.
	return B_ERROR;
}


status_t
SoundPlayNode::GetNextOutput(int32* cookie, media_output* _output)
{
	CALLED();

	if (*cookie == 0) {
		*_output = fOutput;
		*cookie += 1;
		return B_OK;
	} else {
		return B_BAD_INDEX;
	}
}


status_t
SoundPlayNode::DisposeOutputCookie(int32 cookie)
{
	CALLED();
	// do nothing because we don't use the cookie for anything special
	return B_OK;
}


status_t
SoundPlayNode::SetBufferGroup(const media_source& forSource,
	BBufferGroup* newGroup)
{
	CALLED();

	// is this our output?
	if (forSource != fOutput.source) {
		TRACE("SoundPlayNode::SetBufferGroup returning B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// Are we being passed the buffer group we're already using?
	if (newGroup == fBufferGroup)
		return B_OK;

	// Ahh, someone wants us to use a different buffer group. At this point we
	// delete the one we are using and use the specified one instead.
	// If the specified group is NULL, we need to recreate one ourselves, and
	// use *that*. Note that if we're caching a BBuffer that we requested
	// earlier, we have to Recycle() that buffer *before* deleting the buffer
	// group, otherwise we'll deadlock waiting for that buffer to be recycled!
	delete fBufferGroup;
		// waits for all buffers to recycle

	if (newGroup != NULL) {
		// we were given a valid group; just use that one from now on
		fBufferGroup = newGroup;
		return B_OK;
	}

	// we were passed a NULL group pointer; that means we construct
	// our own buffer group to use from now on
	return AllocateBuffers();
}


status_t
SoundPlayNode::GetLatency(bigtime_t* _latency)
{
	CALLED();

	// report our *total* latency:  internal plus downstream plus scheduling
	*_latency = EventLatency() + SchedulingLatency();
	return B_OK;
}


status_t
SoundPlayNode::PrepareToConnect(const media_source& what,
	const media_destination& where, media_format* format,
	media_source* _source, char* _name)
{
	// PrepareToConnect() is the second stage of format negotiations that
	// happens inside BMediaRoster::Connect(). At this point, the consumer's
	// AcceptFormat() method has been called, and that node has potentially
	// changed the proposed format. It may also have left wildcards in the
	// format. PrepareToConnect() *must* fully specialize the format before
	// returning!
	CALLED();

	// is this our output?
	if (what != fOutput.source)	{
		TRACE("SoundPlayNode::PrepareToConnect returning "
			"B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// are we already connected?
	if (fOutput.destination != media_destination::null)
		return B_MEDIA_ALREADY_CONNECTED;

	// the format may not yet be fully specialized (the consumer might have
	// passed back some wildcards). Finish specializing it now, and return an
	// error if we don't support the requested format.

#if DEBUG > 0
	char buf[100];
	string_for_format(*format, buf, sizeof(buf));
	TRACE("SoundPlayNode::PrepareToConnect: input format %s\n", buf);
#endif

	// if not raw audio, we can't support it
	if (format->type != B_MEDIA_UNKNOWN_TYPE
		&& format->type != B_MEDIA_RAW_AUDIO) {
		TRACE("SoundPlayNode::PrepareToConnect: non raw format, returning "
			"B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}

	// the haiku mixer might have a hint
	// for us, so check for it
	#define FORMAT_USER_DATA_TYPE 		0x7294a8f3
	#define FORMAT_USER_DATA_MAGIC_1	0xc84173bd
	#define FORMAT_USER_DATA_MAGIC_2	0x4af62b7d
	uint32 channel_count = 0;
	float frame_rate = 0;
	if (format->user_data_type == FORMAT_USER_DATA_TYPE
		&& *(uint32 *)&format->user_data[0] == FORMAT_USER_DATA_MAGIC_1
		&& *(uint32 *)&format->user_data[44] == FORMAT_USER_DATA_MAGIC_2) {
		channel_count = *(uint32 *)&format->user_data[4];
		frame_rate = *(float *)&format->user_data[20];
		TRACE("SoundPlayNode::PrepareToConnect: found mixer info: "
			"channel_count %" B_PRId32 " , frame_rate %.1f\n", channel_count, frame_rate);
	}

	media_format default_format;
	default_format.type = B_MEDIA_RAW_AUDIO;
	default_format.u.raw_audio.frame_rate = frame_rate > 0 ? frame_rate : 44100;
	default_format.u.raw_audio.channel_count = channel_count > 0
		? channel_count : 2;
	default_format.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	default_format.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
	default_format.u.raw_audio.buffer_size = 0;
	format->SpecializeTo(&default_format);

	if (format->u.raw_audio.buffer_size == 0) {
		format->u.raw_audio.buffer_size
			= BMediaRoster::Roster()->AudioBufferSizeFor(
				format->u.raw_audio.channel_count, format->u.raw_audio.format,
				format->u.raw_audio.frame_rate);
	}

#if DEBUG > 0
	string_for_format(*format, buf, sizeof(buf));
	TRACE("SoundPlayNode::PrepareToConnect: output format %s\n", buf);
#endif

	// Now reserve the connection, and return information about it
	fOutput.destination = where;
	fOutput.format = *format;
	*_source = fOutput.source;
	strcpy(_name, Name());
	return B_OK;
}


void
SoundPlayNode::Connect(status_t error, const media_source& source,
	const media_destination& destination, const media_format& format,
	char* name)
{
	CALLED();

	// is this our output?
	if (source != fOutput.source) {
		TRACE("SoundPlayNode::Connect returning\n");
		return;
	}

	// If something earlier failed, Connect() might still be called, but with
	// a non-zero error code.  When that happens we simply unreserve the
	// connection and do nothing else.
	if (error) {
		fOutput.destination = media_destination::null;
		fOutput.format.type = B_MEDIA_RAW_AUDIO;
		fOutput.format.u.raw_audio = media_multi_audio_format::wildcard;
		return;
	}

	// Okay, the connection has been confirmed.  Record the destination and
	// format that we agreed on, and report our connection name again.
	fOutput.destination = destination;
	fOutput.format = format;
	strcpy(name, Name());

	// Now that we're connected, we can determine our downstream latency.
	// Do so, then make sure we get our events early enough.
	media_node_id id;
	FindLatencyFor(fOutput.destination, &fLatency, &id);
	TRACE("SoundPlayNode::Connect: downstream latency = %" B_PRId64 "\n",
		fLatency);

	// reset our buffer duration, etc. to avoid later calculations
	bigtime_t duration = ((fOutput.format.u.raw_audio.buffer_size * 1000000LL)
		/ ((fOutput.format.u.raw_audio.format
				& media_raw_audio_format::B_AUDIO_SIZE_MASK)
			* fOutput.format.u.raw_audio.channel_count))
		/ (int32)fOutput.format.u.raw_audio.frame_rate;
	SetBufferDuration(duration);
	TRACE("SoundPlayNode::Connect: buffer duration is %" B_PRId64 "\n",
		duration);

	fInternalLatency = (3 * BufferDuration()) / 4;
	TRACE("SoundPlayNode::Connect: using %" B_PRId64 " as internal latency\n",
		fInternalLatency);
	SetEventLatency(fLatency + fInternalLatency);

	// Set up the buffer group for our connection, as long as nobody handed us
	// a buffer group (via SetBufferGroup()) prior to this.
	// That can happen, for example, if the consumer calls SetOutputBuffersFor()
	// on us from within its Connected() method.
	if (!fBufferGroup)
		AllocateBuffers();
}


void
SoundPlayNode::Disconnect(const media_source& what,
	const media_destination& where)
{
	CALLED();

	// is this our output?
	if (what != fOutput.source) {
		TRACE("SoundPlayNode::Disconnect returning\n");
		return;
	}

	// Make sure that our connection is the one being disconnected
	if (where == fOutput.destination && what == fOutput.source) {
		fOutput.destination = media_destination::null;
		fOutput.format.type = B_MEDIA_RAW_AUDIO;
		fOutput.format.u.raw_audio = media_multi_audio_format::wildcard;
		delete fBufferGroup;
		fBufferGroup = NULL;
	} else {
		fprintf(stderr, "\tDisconnect() called with wrong source/destination "
			"(%" B_PRId32 "/%" B_PRId32 "), ours is (%" B_PRId32 "/%" B_PRId32
			")\n", what.id, where.id, fOutput.source.id,
			fOutput.destination.id);
	}
}


void
SoundPlayNode::LateNoticeReceived(const media_source& what, bigtime_t howMuch,
	bigtime_t performanceTime)
{
	CALLED();

	TRACE("SoundPlayNode::LateNoticeReceived, %" B_PRId64 " too late at %"
		B_PRId64 "\n", howMuch, performanceTime);

	// is this our output?
	if (what != fOutput.source) {
		TRACE("SoundPlayNode::LateNoticeReceived returning\n");
		return;
	}

	if (RunMode() != B_DROP_DATA) {
		// We're late, and our run mode dictates that we try to produce buffers
		// earlier in order to catch up.  This argues that the downstream nodes are
		// not properly reporting their latency, but there's not much we can do about
		// that at the moment, so we try to start producing buffers earlier to
		// compensate.

		fInternalLatency += howMuch;

		if (fInternalLatency > 30000)	// avoid getting a too high latency
			fInternalLatency = 30000;

		SetEventLatency(fLatency + fInternalLatency);
		TRACE("SoundPlayNode::LateNoticeReceived: increasing latency to %"
			B_PRId64 "\n", fLatency + fInternalLatency);
	} else {
		// The other run modes dictate various strategies for sacrificing data quality
		// in the interests of timely data delivery.  The way *we* do this is to skip
		// a buffer, which catches us up in time by one buffer duration.

		size_t nFrames = fOutput.format.u.raw_audio.buffer_size
			/ ((fOutput.format.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK)
			* fOutput.format.u.raw_audio.channel_count);

		fFramesSent += nFrames;

		TRACE("SoundPlayNode::LateNoticeReceived: skipping a buffer to try to catch up\n");
	}
}


void
SoundPlayNode::EnableOutput(const media_source& what, bool enabled,
	int32* /* deprecated */)
{
	CALLED();

	// If I had more than one output, I'd have to walk my list of output
	// records to see which one matched the given source, and then
	// enable/disable that one.
	// But this node only has one output, so I just make sure the given source
	// matches, then set the enable state accordingly.

	// is this our output?
	if (what != fOutput.source) {
		fprintf(stderr, "SoundPlayNode::EnableOutput returning\n");
		return;
	}

	fOutputEnabled = enabled;
}


void
SoundPlayNode::AdditionalBufferRequested(const media_source& source,
	media_buffer_id previousBuffer, bigtime_t previousTime,
	const media_seek_tag* previousTag)
{
	CALLED();
	// we don't support offline mode
	return;
}


void
SoundPlayNode::LatencyChanged(const media_source& source,
	const media_destination& destination, bigtime_t newLatency, uint32 flags)
{
	CALLED();

	TRACE("SoundPlayNode::LatencyChanged: new_latency %" B_PRId64 "\n",
		newLatency);

	// something downstream changed latency, so we need to start producing
	// buffers earlier (or later) than we were previously.  Make sure that the
	// connection that changed is ours, and adjust to the new downstream
	// latency if so.
	if (source == fOutput.source && destination == fOutput.destination) {
		fLatency = newLatency;
		SetEventLatency(fLatency + fInternalLatency);
	} else {
		TRACE("SoundPlayNode::LatencyChanged: ignored\n");
	}
}


// #pragma mark - implementation for BMediaEventLooper


void
SoundPlayNode::HandleEvent(const media_timed_event* event, bigtime_t lateness,
	bool realTimeEvent)
{
	CALLED();
	switch (event->type) {
		case BTimedEventQueue::B_START:
			HandleStart(event,lateness,realTimeEvent);
			break;
		case BTimedEventQueue::B_SEEK:
			HandleSeek(event,lateness,realTimeEvent);
			break;
		case BTimedEventQueue::B_WARP:
			HandleWarp(event,lateness,realTimeEvent);
			break;
		case BTimedEventQueue::B_STOP:
			HandleStop(event,lateness,realTimeEvent);
			break;
		case BTimedEventQueue::B_HANDLE_BUFFER:
			// we don't get any buffers
			break;
		case SEND_NEW_BUFFER_EVENT:
			if (RunState() == BMediaEventLooper::B_STARTED)
				SendNewBuffer(event, lateness, realTimeEvent);
			break;
		case BTimedEventQueue::B_DATA_STATUS:
			HandleDataStatus(event,lateness,realTimeEvent);
			break;
		case BTimedEventQueue::B_PARAMETER:
			HandleParameter(event,lateness,realTimeEvent);
			break;
		default:
			fprintf(stderr," unknown event type: %" B_PRId32 "\n", event->type);
			break;
	}
}


// #pragma mark - protected methods


// how should we handle late buffers?  drop them?
// notify the producer?
status_t
SoundPlayNode::SendNewBuffer(const media_timed_event* event,
	bigtime_t lateness, bool realTimeEvent)
{
	CALLED();
	// TRACE("latency = %12Ld, event = %12Ld, sched = %5Ld, arrive at %12Ld, now %12Ld, current lateness %12Ld\n", EventLatency() + SchedulingLatency(), EventLatency(), SchedulingLatency(), event->event_time, TimeSource()->Now(), lateness);

	// make sure we're both started *and* connected before delivering a buffer
	if (RunState() != BMediaEventLooper::B_STARTED
		|| fOutput.destination == media_destination::null)
		return B_OK;

	// The event->event_time is the time at which the buffer we are preparing
	// here should arrive at it's destination. The MediaEventLooper should have
	// scheduled us early enough (based on EventLatency() and the
	// SchedulingLatency()) to make this possible.
	// lateness is independent of EventLatency()!

	if (lateness > (BufferDuration() / 3) ) {
		TRACE("SoundPlayNode::SendNewBuffer, event scheduled much too late, "
			"lateness is %" B_PRId64 "\n", lateness);
	}

	// skip buffer creation if output not enabled
	if (fOutputEnabled) {

		// Get the next buffer of data
		BBuffer* buffer = FillNextBuffer(event->event_time);

		if (buffer) {

			// If we are ready way too early, decrase internal latency
/*
			bigtime_t how_early = event->event_time - TimeSource()->Now() - fLatency - fInternalLatency;
			if (how_early > 5000) {

				TRACE("SoundPlayNode::SendNewBuffer, event scheduled too early, how_early is %Ld\n", how_early);

				if (fTooEarlyCount++ == 5) {
					fInternalLatency -= how_early;
					if (fInternalLatency < 500)
						fInternalLatency = 500;
					TRACE("SoundPlayNode::SendNewBuffer setting internal latency to %Ld\n", fInternalLatency);
					SetEventLatency(fLatency + fInternalLatency);
					fTooEarlyCount = 0;
				}
			}
*/
			// send the buffer downstream if and only if output is enabled
			if (SendBuffer(buffer, fOutput.source, fOutput.destination)
					!= B_OK) {
				// we need to recycle the buffer
				// if the call to SendBuffer() fails
				TRACE("SoundPlayNode::SendNewBuffer: Buffer sending "
					"failed\n");
				buffer->Recycle();
			}
		}
	}

	// track how much media we've delivered so far
	size_t nFrames = fOutput.format.u.raw_audio.buffer_size
		/ ((fOutput.format.u.raw_audio.format
			& media_raw_audio_format::B_AUDIO_SIZE_MASK)
		* fOutput.format.u.raw_audio.channel_count);
	fFramesSent += nFrames;

	// The buffer is on its way; now schedule the next one to go
	// nextEvent is the time at which the buffer should arrive at it's
	// destination
	bigtime_t nextEvent = fStartTime + bigtime_t((1000000LL * fFramesSent)
		/ (int32)fOutput.format.u.raw_audio.frame_rate);
	media_timed_event nextBufferEvent(nextEvent, SEND_NEW_BUFFER_EVENT);
	EventQueue()->AddEvent(nextBufferEvent);

	return B_OK;
}


status_t
SoundPlayNode::HandleDataStatus(const media_timed_event* event,
	bigtime_t lateness, bool realTimeEvent)
{
	TRACE("SoundPlayNode::HandleDataStatus status: %" B_PRId32 ", lateness: %"
		B_PRId64 "\n", event->data, lateness);

	switch (event->data) {
		case B_DATA_NOT_AVAILABLE:
			break;
		case B_DATA_AVAILABLE:
			break;
		case B_PRODUCER_STOPPED:
			break;
		default:
			break;
	}
	return B_OK;
}


status_t
SoundPlayNode::HandleStart(const media_timed_event* event, bigtime_t lateness,
	bool realTimeEvent)
{
	CALLED();
	// don't do anything if we're already running
	if (RunState() != B_STARTED) {
		// We want to start sending buffers now, so we set up the buffer-sending
		// bookkeeping and fire off the first "produce a buffer" event.

		fFramesSent = 0;
		fStartTime = event->event_time;
		media_timed_event firstBufferEvent(event->event_time,
			SEND_NEW_BUFFER_EVENT);

		// Alternatively, we could call HandleEvent() directly with this event,
		// to avoid a trip through the event queue, like this:
		//
		//		this->HandleEvent(&firstBufferEvent, 0, false);
		//
		EventQueue()->AddEvent(firstBufferEvent);
	}
	return B_OK;
}


status_t
SoundPlayNode::HandleSeek(const media_timed_event* event, bigtime_t lateness,
	bool realTimeEvent)
{
	CALLED();
	TRACE("SoundPlayNode::HandleSeek(t=%" B_PRId64 ", d=%" B_PRId32 ", bd=%"
		B_PRId64 ")\n", event->event_time, event->data, event->bigdata);
	return B_OK;
}


status_t
SoundPlayNode::HandleWarp(const media_timed_event* event, bigtime_t lateness,
	bool realTimeEvent)
{
	CALLED();
	return B_OK;
}


status_t
SoundPlayNode::HandleStop(const media_timed_event* event, bigtime_t lateness,
	bool realTimeEvent)
{
	CALLED();
	// flush the queue so downstreamers don't get any more
	EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true,
		SEND_NEW_BUFFER_EVENT);

	return B_OK;
}


status_t
SoundPlayNode::HandleParameter(const media_timed_event* event,
	bigtime_t lateness, bool realTimeEvent)
{
	CALLED();
	return B_OK;
}


status_t
SoundPlayNode::AllocateBuffers()
{
	CALLED();

	// allocate enough buffers to span our downstream latency, plus one
	size_t size = fOutput.format.u.raw_audio.buffer_size;
	int32 count = int32(fLatency / BufferDuration() + 1 + 1);

	TRACE("SoundPlayNode::AllocateBuffers: latency = %" B_PRId64 ", buffer "
		"duration = %" B_PRId64 ", count %" B_PRId32 "\n", fLatency,
		BufferDuration(), count);

	if (count < 3)
		count = 3;

	TRACE("SoundPlayNode::AllocateBuffers: creating group of %" B_PRId32
		" buffers, size = %" B_PRIuSIZE "\n", count, size);

	fBufferGroup = new BBufferGroup(size, count);
	if (fBufferGroup->InitCheck() != B_OK) {
		ERROR("SoundPlayNode::AllocateBuffers: BufferGroup::InitCheck() "
			"failed\n");
	}

	return fBufferGroup->InitCheck();
}


BBuffer*
SoundPlayNode::FillNextBuffer(bigtime_t eventTime)
{
	CALLED();

	// get a buffer from our buffer group
	BBuffer* buffer = fBufferGroup->RequestBuffer(
		fOutput.format.u.raw_audio.buffer_size, BufferDuration() / 2);

	// If we fail to get a buffer (for example, if the request times out), we
	// skip this buffer and go on to the next, to avoid locking up the control
	// thread
	if (buffer == NULL) {
		ERROR("SoundPlayNode::FillNextBuffer: RequestBuffer failed\n");
		return NULL;
	}

	if (fPlayer->HasData()) {
		fPlayer->PlayBuffer(buffer->Data(),
			fOutput.format.u.raw_audio.buffer_size, fOutput.format.u.raw_audio);
	} else
		memset(buffer->Data(), 0, fOutput.format.u.raw_audio.buffer_size);

	// fill in the buffer header
	media_header* header = buffer->Header();
	header->type = B_MEDIA_RAW_AUDIO;
	header->size_used = fOutput.format.u.raw_audio.buffer_size;
	header->time_source = TimeSource()->ID();
	header->start_time = eventTime;

	return buffer;
}


}	// namespace BPrivate
