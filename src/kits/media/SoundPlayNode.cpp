/***********************************************************************
 * AUTHOR: Marcus Overhagen, Jérôme Duval
 *   FILE: SoundPlayNode.cpp
 *  DESCR: This is the BBufferProducer, used internally by BSoundPlayer
 *         This belongs into a private namespace, but isn't for 
 *         compatibility reasons.
 ***********************************************************************/

#include <TimeSource.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "SoundPlayNode.h"
#include "debug.h"

#define DPRINTF 1

#if DPRINTF
	#undef DPRINTF
	#define DPRINTF printf
#else
	#undef DPRINTF
	#define DPRINTF if (1) {} else printf
#endif

#define SEND_NEW_BUFFER_EVENT (BTimedEventQueue::B_USER_EVENT + 1)

_SoundPlayNode::_SoundPlayNode(const char *name, const media_multi_audio_format *format, BSoundPlayer *player) : 
	BMediaNode(name),
	BBufferProducer(B_MEDIA_RAW_AUDIO),
	BMediaEventLooper(),
	mPlayer(player),
	mInitCheckStatus(B_OK),
	mOutputEnabled(true),
	mBufferGroup(NULL),	
	mFramesSent(0),
	mTooEarlyCount(0)
{
	CALLED();
	mFormat.type = B_MEDIA_RAW_AUDIO;
	mFormat.u.raw_audio = *format;
	
	DPRINTF("Format Info:\n");
	DPRINTF("  frame_rate:     %.1f (%ld)\n", mFormat.u.raw_audio.frame_rate, (int32)mFormat.u.raw_audio.frame_rate);
	DPRINTF("  channel_count:  %ld\n",mFormat.u.raw_audio.channel_count);
	DPRINTF("  byte_order:     %ld (",mFormat.u.raw_audio.byte_order);
	switch (mFormat.u.raw_audio.byte_order) {
		case B_MEDIA_BIG_ENDIAN: DPRINTF("B_MEDIA_BIG_ENDIAN)\n"); break;
		case B_MEDIA_LITTLE_ENDIAN: DPRINTF("B_MEDIA_LITTLE_ENDIAN)\n"); break;
		default: DPRINTF("unknown)\n"); break;
	}
	DPRINTF("  buffer_size:    %ld\n",mFormat.u.raw_audio.buffer_size);
	DPRINTF("  format:         %ld (",mFormat.u.raw_audio.format);
	switch (mFormat.u.raw_audio.format) {
		case media_raw_audio_format::B_AUDIO_FLOAT: DPRINTF("B_AUDIO_FLOAT)\n"); break;
		case media_raw_audio_format::B_AUDIO_SHORT: DPRINTF("B_AUDIO_SHORT)\n"); break;
		case media_raw_audio_format::B_AUDIO_INT: DPRINTF("B_AUDIO_INT)\n"); break;
		case media_raw_audio_format::B_AUDIO_CHAR: DPRINTF("B_AUDIO_CHAR)\n"); break;
		case media_raw_audio_format::B_AUDIO_UCHAR: DPRINTF("B_AUDIO_UCHAR)\n"); break;
		default: DPRINTF("unknown)\n"); break;
	}
}


_SoundPlayNode::~_SoundPlayNode()
{
	CALLED();
	Quit();
}

bool
_SoundPlayNode::IsPlaying()
{
	return RunState() == B_STARTED;
}

bigtime_t
_SoundPlayNode::Latency()
{
	return EventLatency();
}


media_multi_audio_format 
_SoundPlayNode::Format() const
{
	return mFormat.u.raw_audio;
}

// -------------------------------------------------------- //
// implementation of BMediaNode
// -------------------------------------------------------- //

BMediaAddOn * _SoundPlayNode::AddOn(int32 * internal_id) const
{
	CALLED();
	// BeBook says this only gets called if we were in an add-on.
	return NULL;
}

void _SoundPlayNode::Preroll(void)
{
	CALLED();
	// XXX:Performance opportunity
	BMediaNode::Preroll();
}

status_t _SoundPlayNode::HandleMessage(int32 message, const void * data, size_t size)
{
	CALLED();
	return B_ERROR;
}

void _SoundPlayNode::NodeRegistered(void)
{
	CALLED();
	
	if (mInitCheckStatus != B_OK) {
		ReportError(B_NODE_IN_DISTRESS);
		return;
	}
	
	SetPriority(B_URGENT_PRIORITY);
	
	mOutput.format = mFormat;
	mOutput.destination = media_destination::null;
	mOutput.source.port = ControlPort();
	mOutput.source.id = 0;
	mOutput.node = Node();
	strcpy(mOutput.name, Name());
	
	Run();	
}

status_t _SoundPlayNode::RequestCompleted(const media_request_info &info)
{
	CALLED();
	return B_OK;
}

void _SoundPlayNode::SetTimeSource(BTimeSource *timeSource)
{
	CALLED();
	BMediaNode::SetTimeSource(timeSource);
}

void
_SoundPlayNode::SetRunMode(run_mode mode)
{
	TRACE("_SoundPlayNode::SetRunMode mode:%i\n", mode);
	BMediaNode::SetRunMode(mode);
}

// -------------------------------------------------------- //
// implementation for BBufferProducer
// -------------------------------------------------------- //

status_t 
_SoundPlayNode::FormatSuggestionRequested(media_type type, int32 /*quality*/, media_format* format)
{
	// FormatSuggestionRequested() is not necessarily part of the format negotiation
	// process; it's simply an interrogation -- the caller wants to see what the node's
	// preferred data format is, given a suggestion by the caller.
	CALLED();

	// a wildcard type is okay; but we only support raw audio
	if (type != B_MEDIA_RAW_AUDIO && type != B_MEDIA_UNKNOWN_TYPE)
		return B_MEDIA_BAD_FORMAT;

	// this is the format we'll be returning (our preferred format)
	*format = mFormat;

	return B_OK;
}

status_t 
_SoundPlayNode::FormatProposal(const media_source& output, media_format* format)
{
	// FormatProposal() is the first stage in the BMediaRoster::Connect() process.  We hand
	// out a suggested format, with wildcards for any variations we support.
	CALLED();

	// is this a proposal for our one output?
	if (output != mOutput.source) {
		TRACE("_SoundPlayNode::FormatProposal returning B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// we only support floating-point raw audio, so we always return that, but we
	// supply an error code depending on whether we found the proposal acceptable.
	media_type requestedType = format->type;
	*format = mFormat;
	if ((requestedType != B_MEDIA_UNKNOWN_TYPE) && (requestedType != B_MEDIA_RAW_AUDIO)) {
		TRACE("_SoundPlayNode::FormatProposal returning B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}
	else
		return B_OK;		// raw audio or wildcard type, either is okay by us
}

status_t 
_SoundPlayNode::FormatChangeRequested(const media_source& source, const media_destination& destination, media_format* io_format, int32* _deprecated_)
{
	CALLED();

	// we don't support any other formats, so we just reject any format changes.
	return B_ERROR;
}

status_t 
_SoundPlayNode::GetNextOutput(int32* cookie, media_output* out_output)
{
	CALLED();

	if (*cookie == 0) {
		*out_output = mOutput;
		*cookie += 1;
		return B_OK;
	} else {
		return B_BAD_INDEX;
	}
}

status_t 
_SoundPlayNode::DisposeOutputCookie(int32 cookie)
{
	CALLED();
	// do nothing because we don't use the cookie for anything special
	return B_OK;
}

status_t 
_SoundPlayNode::SetBufferGroup(const media_source& for_source, BBufferGroup* newGroup)
{
	CALLED();

	// is this our output?
	if (for_source != mOutput.source) {
		TRACE("_SoundPlayNode::SetBufferGroup returning B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// Are we being passed the buffer group we're already using?
	if (newGroup == mBufferGroup)
		return B_OK;

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
		if (count < 3)
			count = 3;
		mBufferGroup = new BBufferGroup(size, count);
	}

	return B_OK;
}

status_t 
_SoundPlayNode::GetLatency(bigtime_t* out_latency)
{
	CALLED();
	
	// report our *total* latency:  internal plus downstream plus scheduling
	*out_latency = EventLatency() + SchedulingLatency();
	return B_OK;
}

status_t 
_SoundPlayNode::PrepareToConnect(const media_source& what, const media_destination& where, media_format* format, media_source* out_source, char* out_name)
{
	// PrepareToConnect() is the second stage of format negotiations that happens
	// inside BMediaRoster::Connect().  At this point, the consumer's AcceptFormat()
	// method has been called, and that node has potentially changed the proposed
	// format.  It may also have left wildcards in the format.  PrepareToConnect()
	// *must* fully specialize the format before returning!
	CALLED();

	// is this our output?
	if (what != mOutput.source)
	{
		TRACE("_SoundPlayNode::PrepareToConnect returning B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// are we already connected?
	if (mOutput.destination != media_destination::null)
		return B_MEDIA_ALREADY_CONNECTED;

	// the format may not yet be fully specialized (the consumer might have
	// passed back some wildcards).  Finish specializing it now, and return an
	// error if we don't support the requested format.
	if (format->type != B_MEDIA_RAW_AUDIO)
	{
		TRACE("\tnon-raw-audio format?!\n");
		return B_MEDIA_BAD_FORMAT;
	}
	// !!! validate all other fields except for buffer_size here, because the consumer might have
	// supplied different values from AcceptFormat()?

	// check the buffer size, which may still be wildcarded
	if (format->u.raw_audio.buffer_size == media_raw_audio_format::wildcard.buffer_size)
	{
		format->u.raw_audio.buffer_size = 2048;		// pick something comfortable to suggest
		TRACE("\tno buffer size provided, suggesting %lu\n", format->u.raw_audio.buffer_size);
	}
	else
	{
		TRACE("\tconsumer suggested buffer_size %lu\n", format->u.raw_audio.buffer_size);
	}

	// Now reserve the connection, and return information about it
	mOutput.destination = where;
	mOutput.format = *format;
	*out_source = mOutput.source;
	strcpy(out_name, Name());
	return B_OK;
}

void 
_SoundPlayNode::Connect(status_t error, const media_source& source, const media_destination& destination, const media_format& format, char* io_name)
{
	CALLED();
	
	// is this our output?
	if (source != mOutput.source)
	{
		TRACE("_SoundPlayNode::Connect returning\n");
		return;
	}
	
	// If something earlier failed, Connect() might still be called, but with a non-zero
	// error code.  When that happens we simply unreserve the connection and do
	// nothing else.
	if (error)
	{
		mOutput.destination = media_destination::null;
		mOutput.format = mFormat;
		return;
	}

	// Okay, the connection has been confirmed.  Record the destination and format
	// that we agreed on, and report our connection name again.
	mOutput.destination = destination;
	mOutput.format = format;
	strcpy(io_name, Name());

	// Now that we're connected, we can determine our downstream latency.
	// Do so, then make sure we get our events early enough.
	media_node_id id;
	FindLatencyFor(mOutput.destination, &mLatency, &id);
	TRACE("_SoundPlayNode::Connect: downstream latency = %Ld\n", mLatency);

	// reset our buffer duration, etc. to avoid later calculations
	bigtime_t duration = ((mOutput.format.u.raw_audio.buffer_size * 1000000LL)
		/ ((mOutput.format.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK) * mOutput.format.u.raw_audio.channel_count))
		/ (int32)mOutput.format.u.raw_audio.frame_rate;
	SetBufferDuration(duration);
	TRACE("_SoundPlayNode::Connect: buffer duration is %Ld\n", duration);

	mInternalLatency = (3 * BufferDuration()) / 4;
	TRACE("_SoundPlayNode::Connect: using %Ld as internal latency\n", mInternalLatency);
	SetEventLatency(mLatency + mInternalLatency);

	// Set up the buffer group for our connection, as long as nobody handed us a
	// buffer group (via SetBufferGroup()) prior to this.  That can happen, for example,
	// if the consumer calls SetOutputBuffersFor() on us from within its Connected()
	// method.
	if (!mBufferGroup) 
		AllocateBuffers();
}

void 
_SoundPlayNode::Disconnect(const media_source& what, const media_destination& where)
{
	CALLED();
	
	// is this our output?
	if (what != mOutput.source)
	{
		TRACE("_SoundPlayNode::Disconnect returning\n");
		return;
	}

	// Make sure that our connection is the one being disconnected
	if ((where == mOutput.destination) && (what == mOutput.source))
	{
		mOutput.destination = media_destination::null;
		mOutput.format = mFormat;
		delete mBufferGroup;
		mBufferGroup = NULL;
	}
	else
	{
		fprintf(stderr, "\tDisconnect() called with wrong source/destination (%ld/%ld), ours is (%ld/%ld)\n",
			what.id, where.id, mOutput.source.id, mOutput.destination.id);
	}
}

void 
_SoundPlayNode::LateNoticeReceived(const media_source& what, bigtime_t how_much, bigtime_t performance_time)
{
	CALLED();
	
	printf("_SoundPlayNode::LateNoticeReceived, %Ld too late at %Ld\n", how_much, performance_time);
	
	// is this our output?
	if (what != mOutput.source)
	{
		TRACE("_SoundPlayNode::LateNoticeReceived returning\n");
		return;
	}

	if (RunMode() != B_DROP_DATA)
	{
		// We're late, and our run mode dictates that we try to produce buffers
		// earlier in order to catch up.  This argues that the downstream nodes are
		// not properly reporting their latency, but there's not much we can do about
		// that at the moment, so we try to start producing buffers earlier to
		// compensate.
		
		mInternalLatency += how_much;

		if (mInternalLatency > 30000)	// avoid getting a too high latency
			mInternalLatency = 30000;

		SetEventLatency(mLatency + mInternalLatency);
		TRACE("_SoundPlayNode::LateNoticeReceived: increasing latency to %Ld\n", mLatency + mInternalLatency);
	}
	else
	{
		// The other run modes dictate various strategies for sacrificing data quality
		// in the interests of timely data delivery.  The way *we* do this is to skip
		// a buffer, which catches us up in time by one buffer duration.

		size_t nFrames = mOutput.format.u.raw_audio.buffer_size
			/ ((mOutput.format.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK)
			* mOutput.format.u.raw_audio.channel_count);

		mFramesSent += nFrames;

		TRACE("_SoundPlayNode::LateNoticeReceived: skipping a buffer to try to catch up\n");
	}
}

void 
_SoundPlayNode::EnableOutput(const media_source& what, bool enabled, int32* _deprecated_)
{
	CALLED();

	// If I had more than one output, I'd have to walk my list of output records to see
	// which one matched the given source, and then enable/disable that one.  But this
	// node only has one output, so I just make sure the given source matches, then set
	// the enable state accordingly.
	// is this our output?
	if (what != mOutput.source)
	{
		fprintf(stderr, "_SoundPlayNode::EnableOutput returning\n");
		return;
	}
	
	mOutputEnabled = enabled;
}

void 
_SoundPlayNode::AdditionalBufferRequested(const media_source& source, media_buffer_id prev_buffer, bigtime_t prev_time, const media_seek_tag* prev_tag)
{
	CALLED();
	// we don't support offline mode
	return;
}

void 
_SoundPlayNode::LatencyChanged(const media_source& source, const media_destination& destination, bigtime_t new_latency, uint32 flags)
{
	CALLED();
	
	printf("_SoundPlayNode::LatencyChanged: new_latency %Ld\n", new_latency);
	
	// something downstream changed latency, so we need to start producing
	// buffers earlier (or later) than we were previously.  Make sure that the
	// connection that changed is ours, and adjust to the new downstream
	// latency if so.
	if ((source == mOutput.source) && (destination == mOutput.destination))
	{
		mLatency = new_latency;
		SetEventLatency(mLatency + mInternalLatency);
	} else {
		printf("_SoundPlayNode::LatencyChanged: ignored\n");
	}
}

// -------------------------------------------------------- //
// implementation for BMediaEventLooper
// -------------------------------------------------------- //

void _SoundPlayNode::HandleEvent(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent = false)
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
			if (RunState() == BMediaEventLooper::B_STARTED) {
				SendNewBuffer(event, lateness, realTimeEvent);
			}
			break;
		case BTimedEventQueue::B_DATA_STATUS:
			HandleDataStatus(event,lateness,realTimeEvent);
			break;
		case BTimedEventQueue::B_PARAMETER:
			HandleParameter(event,lateness,realTimeEvent);
			break;
		default:
			fprintf(stderr,"  unknown event type: %li\n",event->type);
			break;
	}
}

// protected:

// how should we handle late buffers?  drop them?
// notify the producer?
status_t 
_SoundPlayNode::SendNewBuffer(const media_timed_event *event, bigtime_t lateness, bool realTimeEvent)
{
	CALLED();
	// printf("latency = %12Ld, event = %12Ld, sched = %5Ld, arrive at %12Ld, now %12Ld, current lateness %12Ld\n", EventLatency() + SchedulingLatency(), EventLatency(), SchedulingLatency(), event->event_time, TimeSource()->Now(), lateness);

	// make sure we're both started *and* connected before delivering a buffer
	if ((RunState() != BMediaEventLooper::B_STARTED) || (mOutput.destination == media_destination::null))
		return B_OK;
		
	// The event->event_time is the time at which the buffer we are preparing here should
	// arrive at it's destination. The MediaEventLooper should have scheduled us early enough
	// (based on EventLatency() and the SchedulingLatency()) to make this possible.
	// lateness is independent of EventLatency()!

	if (lateness > (BufferDuration() / 3) ) {
		printf("_SoundPlayNode::SendNewBuffer, event scheduled much too late, lateness is %Ld\n", lateness);
	}

	// skip buffer creation if output not enabled	
	if (mOutputEnabled) {

		// Get the next buffer of data
		BBuffer* buffer = FillNextBuffer(event->event_time);

		if (buffer) {

			// If we are ready way too early, decrase internal latency
/*
			bigtime_t how_early = event->event_time - TimeSource()->Now() - mLatency - mInternalLatency;
			if (how_early > 5000) {

				printf("_SoundPlayNode::SendNewBuffer, event scheduled too early, how_early is %Ld\n", how_early);

				if (mTooEarlyCount++ == 5) {
					mInternalLatency -= how_early;
					if (mInternalLatency < 500)
						mInternalLatency = 500;
					printf("_SoundPlayNode::SendNewBuffer setting internal latency to %Ld\n", mInternalLatency);
					SetEventLatency(mLatency + mInternalLatency);
					mTooEarlyCount = 0;
				}
			}
*/
			// send the buffer downstream if and only if output is enabled
			if (B_OK != SendBuffer(buffer, mOutput.destination)) {
				// we need to recycle the buffer
				// if the call to SendBuffer() fails
				printf("_SoundPlayNode::SendNewBuffer: Buffer sending failed\n");
				buffer->Recycle();
			}
		}
	}

	// track how much media we've delivered so far
	size_t nFrames = mOutput.format.u.raw_audio.buffer_size
		/ ((mOutput.format.u.raw_audio.format & media_raw_audio_format::B_AUDIO_SIZE_MASK)
		* mOutput.format.u.raw_audio.channel_count);
	mFramesSent += nFrames;

	// The buffer is on its way; now schedule the next one to go
	// nextEvent is the time at which the buffer should arrive at it's destination
	bigtime_t nextEvent = mStartTime + bigtime_t((1000000LL * mFramesSent) / (int32)mOutput.format.u.raw_audio.frame_rate);
	media_timed_event nextBufferEvent(nextEvent, SEND_NEW_BUFFER_EVENT);
	EventQueue()->AddEvent(nextBufferEvent);
	
	return B_OK;
}

status_t 
_SoundPlayNode::HandleDataStatus(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	TRACE("_SoundPlayNode::HandleDataStatus status: %li, lateness: %Li\n", event->data, lateness);
	switch(event->data) {
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
_SoundPlayNode::HandleStart(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	CALLED();
	// don't do anything if we're already running
	if (RunState() != B_STARTED)
	{
		// We want to start sending buffers now, so we set up the buffer-sending bookkeeping
		// and fire off the first "produce a buffer" event.
	
		mFramesSent = 0;	
		mStartTime = event->event_time;
		media_timed_event firstBufferEvent(event->event_time, SEND_NEW_BUFFER_EVENT);

		// Alternatively, we could call HandleEvent() directly with this event, to avoid a trip through
		// the event queue, like this:
		//
		//		this->HandleEvent(&firstBufferEvent, 0, false);
		//
		EventQueue()->AddEvent(firstBufferEvent);
	}
	return B_OK;
}

status_t 
_SoundPlayNode::HandleSeek(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	CALLED();
	DPRINTF("_SoundPlayNode::HandleSeek(t=%lld,d=%li,bd=%lld)\n",event->event_time,event->data,event->bigdata);
	return B_OK;
}
						
status_t 
_SoundPlayNode::HandleWarp(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	CALLED();
	return B_OK;
}

status_t 
_SoundPlayNode::HandleStop(
						const media_timed_event *event,
						bigtime_t lateness,
						bool realTimeEvent = false)
{
	CALLED();
	// flush the queue so downstreamers don't get any more
	EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true, SEND_NEW_BUFFER_EVENT);
	
	return B_OK;
}

status_t 
_SoundPlayNode::HandleParameter(
				const media_timed_event *event,
				bigtime_t lateness,
				bool realTimeEvent = false)
{
	CALLED();
	return B_OK;
}

void 
_SoundPlayNode::AllocateBuffers()
{
	CALLED();

	// allocate enough buffers to span our downstream latency, plus one
	size_t size = mOutput.format.u.raw_audio.buffer_size;
	int32 count = int32(mLatency / BufferDuration() + 1 + 1);

	DPRINTF("\tlatency = %Ld, buffer duration = %Ld, count %ld\n", mLatency, BufferDuration(), count);
	
	if (count < 3)
		count = 3;
	
	DPRINTF("\tcreating group of %ld buffers, size = %lu\n", count, size);
	mBufferGroup = new BBufferGroup(size, count);
}

BBuffer*
_SoundPlayNode::FillNextBuffer(bigtime_t event_time)
{
	CALLED();

	// get a buffer from our buffer group
	BBuffer* buf = mBufferGroup->RequestBuffer(mOutput.format.u.raw_audio.buffer_size, BufferDuration() / 2);

	// if we fail to get a buffer (for example, if the request times out), we skip this
	// buffer and go on to the next, to avoid locking up the control thread
	if (!buf) {
		ERROR("_SoundPlayNode::FillNextBuffer: RequestBuffer failed\n");
		return NULL;
	}
	
	memset(buf->Data(), 0, mOutput.format.u.raw_audio.buffer_size);
	if (mPlayer->HasData()) {
		mPlayer->PlayBuffer(buf->Data(), 
			mOutput.format.u.raw_audio.buffer_size, mOutput.format.u.raw_audio);
	}
	
	// fill in the buffer header
	media_header* hdr = buf->Header();
	hdr->type = B_MEDIA_RAW_AUDIO;
	hdr->size_used = mOutput.format.u.raw_audio.buffer_size;
	hdr->time_source = TimeSource()->ID();
	hdr->start_time = event_time;
	return buf;
}
