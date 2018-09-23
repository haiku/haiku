/*	Copyright (c) 1998-99, Be Incorporated, All Rights Reserved.
 *	Distributed under the terms of the Be Sample Code license.
 *
 *	Copyright (c) 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>,
 *	Copyright (c) 2000-2008, Stephan Aßmus <superstippi@gmx.de>,
 *	All Rights Reserved. Distributed under the terms of the MIT license.
 */


#include "VideoProducer.h"

#include <stdio.h>
#include <string.h>

#include <Autolock.h>
#include <Buffer.h>
#include <BufferGroup.h>
#include <TimeSource.h>

#include "NodeManager.h"
#include "VideoSupplier.h"


// debugging
//#define TRACE_VIDEO_PRODUCER
#ifdef TRACE_VIDEO_PRODUCER
# define	TRACE(x...) printf("VideoProducer::"); printf(x)
# define	FUNCTION(x...) TRACE(x)
# define	ERROR(x...) fprintf(stderr, "VideoProducer::"); fprintf(stderr, x)
#else
# define	TRACE(x...)
# define	FUNCTION(x...)
# define	ERROR(x...) fprintf(stderr, "VideoProducer::"); fprintf(stderr, x)
#endif


#define BUFFER_COUNT 3

#define TOUCH(x) ((void)(x))


VideoProducer::VideoProducer(BMediaAddOn* addon, const char* name,
		int32 internalId, NodeManager* manager, VideoSupplier* supplier)
	: BMediaNode(name),
	  BMediaEventLooper(),
	  BBufferProducer(B_MEDIA_RAW_VIDEO),
	  fInitStatus(B_NO_INIT),
	  fInternalID(internalId),
	  fAddOn(addon),
	  fBufferGroup(NULL),
	  fUsedBufferGroup(NULL),
	  fThread(-1),
	  fFrameSync(-1),
	  fFrame(0),
	  fFrameBase(0),
	  fPerformanceTimeBase(0),
	  fBufferLatency(0),
	  fRunning(false),
	  fConnected(false),
	  fEnabled(false),
	  fManager(manager),
	  fSupplier(supplier)
{
	fOutput.destination = media_destination::null;
	fInitStatus = B_OK;
}


VideoProducer::~VideoProducer()
{
	if (fInitStatus == B_OK) {
		// Clean up after ourselves, in case the application didn't make us
		// do so.
		if (fConnected)
			Disconnect(fOutput.source, fOutput.destination);
		if (fRunning)
			_HandleStop();
	}
	Quit();
}


BMediaAddOn*
VideoProducer::AddOn(int32* _internalId) const
{
	if (_internalId)
		*_internalId = fInternalID;
	return fAddOn;
}


status_t
VideoProducer::HandleMessage(int32 message, const void* data, size_t size)
{
	return B_ERROR;
}


void
VideoProducer::SetTimeSource(BTimeSource* timeSource)
{
	// Tell frame generation thread to recalculate delay value
	release_sem(fFrameSync);
}


void
VideoProducer::NodeRegistered()
{
	if (fInitStatus != B_OK) {
		ReportError(B_NODE_IN_DISTRESS);
		return;
	}

	fOutput.node = Node();
	fOutput.source.port = ControlPort();
	fOutput.source.id = 0;
	fOutput.destination = media_destination::null;
	strcpy(fOutput.name, Name());

	// fill with wild cards at this point in time
	fOutput.format.type = B_MEDIA_RAW_VIDEO;
	fOutput.format.u.raw_video = media_raw_video_format::wildcard;
	fOutput.format.u.raw_video.interlace = 1;
	fOutput.format.u.raw_video.display.format = B_NO_COLOR_SPACE;
	fOutput.format.u.raw_video.display.bytes_per_row = 0;
	fOutput.format.u.raw_video.display.line_width = 0;
	fOutput.format.u.raw_video.display.line_count = 0;

	// start the BMediaEventLooper control loop running
	Run();
}


void
VideoProducer::Start(bigtime_t performanceTime)
{
	// notify the manager in case we were started from the outside world
//	fManager->StartPlaying();

	BMediaEventLooper::Start(performanceTime);
}


void
VideoProducer::Stop(bigtime_t performanceTime, bool immediate)
{
	// notify the manager in case we were stopped from the outside world
//	fManager->StopPlaying();

	BMediaEventLooper::Stop(performanceTime, immediate);
}


void
VideoProducer::Seek(bigtime_t media_time, bigtime_t performanceTime)
{
	BMediaEventLooper::Seek(media_time, performanceTime);
}


void
VideoProducer::HandleEvent(const media_timed_event* event,
		bigtime_t lateness, bool realTimeEvent)
{
	TOUCH(lateness); TOUCH(realTimeEvent);

	switch (event->type) {
		case BTimedEventQueue::B_START:
			_HandleStart(event->event_time);
			break;
		case BTimedEventQueue::B_STOP:
		{
			EventQueue()->FlushEvents(event->event_time,
				BTimedEventQueue::B_ALWAYS,
				true, BTimedEventQueue::B_HANDLE_BUFFER);
			_HandleStop();
			break;
		}
		case BTimedEventQueue::B_WARP:
			_HandleTimeWarp(event->bigdata);
			break;
		case BTimedEventQueue::B_SEEK:
			_HandleSeek(event->bigdata);
			break;
		case BTimedEventQueue::B_HANDLE_BUFFER:
		case BTimedEventQueue::B_DATA_STATUS:
		case BTimedEventQueue::B_PARAMETER:
		default:
			TRACE("HandleEvent: Unhandled event -- %lx\n", event->type);
			break;
	}
}


status_t
VideoProducer::DeleteHook(BMediaNode* node)
{
	return BMediaEventLooper::DeleteHook(node);
}


status_t
VideoProducer::FormatSuggestionRequested(media_type type, int32 quality,
	media_format* _format)
{
	FUNCTION("FormatSuggestionRequested\n");

	if (type != B_MEDIA_ENCODED_VIDEO)
		return B_MEDIA_BAD_FORMAT;

	TOUCH(quality);

	*_format = fOutput.format;
	return B_OK;
}


status_t
VideoProducer::FormatProposal(const media_source& output, media_format* format)
{
	#ifdef TRACE_VIDEO_PRODUCER
		char string[256];
		string_for_format(*format, string, 256);
		FUNCTION("FormatProposal(%s)\n", string);
	#endif

	if (!format)
		return B_BAD_VALUE;

	if (output != fOutput.source)
		return B_MEDIA_BAD_SOURCE;

	status_t ret = format_is_compatible(*format, fOutput.format) ?
		B_OK : B_MEDIA_BAD_FORMAT;
	if (ret != B_OK) {
		ERROR("FormatProposal() error: %s\n", strerror(ret));
		char string[512];
		string_for_format(*format, string, sizeof(string));
		ERROR("  requested: %s\n", string);
		string_for_format(fOutput.format, string, sizeof(string));
		ERROR("  output:    %s\n", string);
	}

	// change any wild cards to specific values

	return ret;

}


status_t
VideoProducer::FormatChangeRequested(const media_source& source,
	const media_destination& destination, media_format* ioFormat,
	int32 *_deprecated_)
{
	TOUCH(destination); TOUCH(ioFormat); TOUCH(_deprecated_);

	if (source != fOutput.source)
		return B_MEDIA_BAD_SOURCE;

	return B_ERROR;
}


status_t
VideoProducer::GetNextOutput(int32* cookie, media_output* outOutput)
{
	if (!outOutput)
		return B_BAD_VALUE;

	if ((*cookie) != 0)
		return B_BAD_INDEX;

	*outOutput = fOutput;
	(*cookie)++;

	return B_OK;
}


status_t
VideoProducer::DisposeOutputCookie(int32 cookie)
{
	TOUCH(cookie);

	return B_OK;
}


status_t
VideoProducer::SetBufferGroup(const media_source& forSource,
	BBufferGroup *group)
{
	if (forSource != fOutput.source)
		return B_MEDIA_BAD_SOURCE;

	TRACE("VideoProducer::SetBufferGroup() - using buffer group of "
		"consumer.\n");
	fUsedBufferGroup = group;

	return B_OK;
}


status_t
VideoProducer::VideoClippingChanged(const media_source& forSource,
	int16 numShorts, int16* clipData, const media_video_display_info& display,
	int32* _deprecated_)
{
	TOUCH(forSource); TOUCH(numShorts); TOUCH(clipData);
	TOUCH(display); TOUCH(_deprecated_);

	return B_ERROR;
}


status_t
VideoProducer::GetLatency(bigtime_t* _latency)
{
	if (!_latency)
		return B_BAD_VALUE;

	*_latency = EventLatency() + SchedulingLatency();

	return B_OK;
}


status_t
VideoProducer::PrepareToConnect(const media_source& source,
	const media_destination& destination, media_format* format,
	media_source* outSource, char* outName)
{
	FUNCTION("PrepareToConnect() %ldx%ld\n",
		format->u.raw_video.display.line_width,
		format->u.raw_video.display.line_count);

	if (fConnected) {
		ERROR("PrepareToConnect() - already connected!\n");
		return B_MEDIA_ALREADY_CONNECTED;
	}

	if (source != fOutput.source)
		return B_MEDIA_BAD_SOURCE;

	if (fOutput.destination != media_destination::null) {
		ERROR("PrepareToConnect() - destination != null.\n");
		return B_MEDIA_ALREADY_CONNECTED;
	}

	// The format parameter comes in with the suggested format, and may be
	// specialized as desired by the node
	if (!format_is_compatible(*format, fOutput.format)) {
		ERROR("PrepareToConnect() - incompatible format.\n");
		*format = fOutput.format;
		return B_MEDIA_BAD_FORMAT;
	}

	if (format->u.raw_video.display.line_width == 0)
		format->u.raw_video.display.line_width = 384;
	if (format->u.raw_video.display.line_count == 0)
		format->u.raw_video.display.line_count = 288;
	if (format->u.raw_video.field_rate == 0)
		format->u.raw_video.field_rate = 25.0;
	if (format->u.raw_video.display.bytes_per_row == 0)
		format->u.raw_video.display.bytes_per_row = format->u.raw_video.display.line_width * 4;

	*outSource = fOutput.source;
	strcpy(outName, fOutput.name);

	return B_OK;
}


void
VideoProducer::Connect(status_t error, const media_source& source,
	const media_destination& destination, const media_format& format,
	char* _name)
{
	FUNCTION("Connect() %ldx%ld\n",
		format.u.raw_video.display.line_width,
		format.u.raw_video.display.line_count);

	if (fConnected) {
		ERROR("Connect() - already connected.\n");
		return;
	}

	if (source != fOutput.source) {
		ERROR("Connect() - wrong source.\n");
		return;
	}
	if (error != B_OK) {
		ERROR("Connect() - consumer error: %s\n", strerror(error));
		return;
	}
	if (!const_cast<media_format*>(&format)->Matches(&fOutput.format)) {
		ERROR("Connect() - format mismatch.\n");
		return;
	}

	fOutput.destination = destination;
	strcpy(_name, fOutput.name);
	fConnectedFormat = format.u.raw_video;
	fBufferDuration = 20000;

	if (fConnectedFormat.field_rate != 0.0f) {
		fPerformanceTimeBase = fPerformanceTimeBase
			+ (bigtime_t)((fFrame - fFrameBase)
				* 1000000LL / fConnectedFormat.field_rate);
		fFrameBase = fFrame;
		fBufferDuration = bigtime_t(1000000LL / fConnectedFormat.field_rate);
	}

	if (fConnectedFormat.display.bytes_per_row == 0) {
		ERROR("Connect() - connected format still has BPR wildcard!\n");
		fConnectedFormat.display.bytes_per_row
			= 4 * fConnectedFormat.display.line_width;
	}

	// Create the buffer group
	if (fUsedBufferGroup == NULL) {
		fBufferGroup = new BBufferGroup(fConnectedFormat.display.bytes_per_row
			* fConnectedFormat.display.line_count, BUFFER_COUNT);
		status_t err = fBufferGroup->InitCheck();
		if (err < B_OK) {
			delete fBufferGroup;
			fBufferGroup = NULL;
			ERROR("Connect() - buffer group error: %s\n", strerror(err));
			return;
		}
		fUsedBufferGroup = fBufferGroup;
	}

	// get the latency
	fBufferLatency = (BUFFER_COUNT - 1) * fBufferDuration;

	int32 bufferCount;
	if (fUsedBufferGroup->CountBuffers(&bufferCount) == B_OK) {
		// recompute the latency
		fBufferLatency = (bufferCount - 1) * fBufferDuration;
	}

	bigtime_t latency = 0;
	media_node_id tsID = 0;
	FindLatencyFor(fOutput.destination, &latency, &tsID);
	SetEventLatency(latency + fBufferLatency);

	fConnected = true;
	fEnabled = true;

	// Tell frame generation thread to recalculate delay value
	release_sem(fFrameSync);
}


void
VideoProducer::Disconnect(const media_source& source,
	const media_destination& destination)
{
	FUNCTION("Disconnect()\n");

	if (!fConnected) {
		ERROR("Disconnect() - Not connected\n");
		return;
	}

	if ((source != fOutput.source) || (destination != fOutput.destination)) {
		ERROR("Disconnect() - Bad source and/or destination\n");
		return;
	}

	fEnabled = false;
	fOutput.destination = media_destination::null;

	if (fLock.Lock()) {
		// Always delete the buffer group, even if it is not ours.
		// (See BeBook::SetBufferGroup()).
		delete fUsedBufferGroup;
		if (fBufferGroup != fUsedBufferGroup)
			delete fBufferGroup;
		fUsedBufferGroup = NULL;
		fBufferGroup = NULL;
		fLock.Unlock();
	}

	fConnected = false;
	TRACE("Disconnect() done\n");
}


void
VideoProducer::LateNoticeReceived(const media_source &source,
		bigtime_t how_much, bigtime_t performanceTime)
{
	TOUCH(source); TOUCH(how_much); TOUCH(performanceTime);
	TRACE("Late!!!\n");
}


void
VideoProducer::EnableOutput(const media_source& source, bool enabled,
	int32* _deprecated_)
{
	TOUCH(_deprecated_);

	if (source != fOutput.source)
		return;

	fEnabled = enabled;
}


status_t
VideoProducer::SetPlayRate(int32 numer, int32 denom)
{
	TOUCH(numer); TOUCH(denom);

	return B_ERROR;
}


void
VideoProducer::AdditionalBufferRequested(const media_source& source,
	media_buffer_id prevBuffer, bigtime_t prevTime,
	const media_seek_tag* prevTag)
{
	TOUCH(source); TOUCH(prevBuffer); TOUCH(prevTime); TOUCH(prevTag);
}


void
VideoProducer::LatencyChanged(const media_source& source,
	const media_destination& destination,
	bigtime_t newLatency, uint32 flags)
{
	TOUCH(source); TOUCH(destination); TOUCH(newLatency); TOUCH(flags);
	TRACE("Latency changed!\n");
}


// #pragma mark -


void
VideoProducer::_HandleStart(bigtime_t performanceTime)
{
	// Start producing frames, even if the output hasn't been connected yet.
	TRACE("_HandleStart(%Ld)\n", performanceTime);

	if (fRunning) {
		TRACE("_HandleStart: Node already started\n");
		return;
	}

	fFrame = 0;
	fFrameBase = 0;
	fPerformanceTimeBase = performanceTime;

	fFrameSync = create_sem(0, "frame synchronization");
	if (fFrameSync < B_OK)
		return;

	fThread = spawn_thread(_FrameGeneratorThreadEntry, "frame generator",
		B_NORMAL_PRIORITY, this);
	if (fThread < B_OK) {
		delete_sem(fFrameSync);
		return;
	}

	resume_thread(fThread);
	fRunning = true;
	return;
}


void
VideoProducer::_HandleStop()
{
	TRACE("_HandleStop()\n");

	if (!fRunning) {
		TRACE("_HandleStop: Node isn't running\n");
		return;
	}

	delete_sem(fFrameSync);
	wait_for_thread(fThread, &fThread);

	fRunning = false;
}


void
VideoProducer::_HandleTimeWarp(bigtime_t performanceTime)
{
	fPerformanceTimeBase = performanceTime;
	fFrameBase = fFrame;

	// Tell frame generation thread to recalculate delay value
	release_sem(fFrameSync);
}


void
VideoProducer::_HandleSeek(bigtime_t performanceTime)
{
	fPerformanceTimeBase = performanceTime;
	fFrameBase = fFrame;

	// Tell frame generation thread to recalculate delay value
	release_sem(fFrameSync);
}


int32
VideoProducer::_FrameGeneratorThreadEntry(void* data)
{
	return ((VideoProducer*)data)->_FrameGeneratorThread();
}


int32
VideoProducer::_FrameGeneratorThread()
{
	bool forceSendingBuffer = true;
	int32 droppedFrames = 0;
	const int32 kMaxDroppedFrames = 15;
	bool running = true;
	while (running) {
		TRACE("_FrameGeneratorThread: loop: %Ld\n", fFrame);
		// lock the node manager
		status_t err = fManager->LockWithTimeout(10000);
		bool ignoreEvent = false;
		// Data to be retrieved from the node manager.
		bigtime_t performanceTime = 0;
		bigtime_t nextPerformanceTime = 0;
		bigtime_t waitUntil = 0;
		bigtime_t nextWaitUntil = 0;
		int32 playingDirection = 0;
		int64 playlistFrame = 0;
		switch (err) {
			case B_OK: {
				TRACE("_FrameGeneratorThread: node manager successfully "
					"locked\n");
				if (droppedFrames > 0)
					fManager->FrameDropped();
				// get the times for the current and the next frame
				performanceTime = fManager->TimeForFrame(fFrame);
				nextPerformanceTime = fManager->TimeForFrame(fFrame + 1);
				waitUntil = TimeSource()->RealTimeFor(fPerformanceTimeBase
					+ performanceTime, fBufferLatency);
				nextWaitUntil = TimeSource()->RealTimeFor(fPerformanceTimeBase
					+ nextPerformanceTime, fBufferLatency);
				// get playing direction and playlist frame for the current
				// frame
				bool newPlayingState;
				playlistFrame = fManager->PlaylistFrameAtFrame(fFrame,
					playingDirection, newPlayingState);
				TRACE("_FrameGeneratorThread: performance time: %Ld, "
					"playlist frame: %lld\n", performanceTime, playlistFrame);
				forceSendingBuffer |= newPlayingState;
				fManager->SetCurrentVideoTime(nextPerformanceTime);
				fManager->Unlock();
				break;
			}
			case B_TIMED_OUT:
				TRACE("_FrameGeneratorThread: Couldn't lock the node "
					"manager.\n");
				ignoreEvent = true;
				waitUntil = system_time() - 1;
				break;
			default:
				ERROR("_FrameGeneratorThread: Couldn't lock the node manager. "
					"Terminating video producer frame generator thread.\n");
				TRACE("_FrameGeneratorThread: frame generator thread done.\n");
				// do not access any member variables, since this could
				// also mean the Node has been deleted
				return B_OK;
		}

		TRACE("_FrameGeneratorThread: waiting (%Ld)...\n", waitUntil);
		// wait until...
		err = acquire_sem_etc(fFrameSync, 1, B_ABSOLUTE_TIMEOUT, waitUntil);
		// The only acceptable responses are B_OK and B_TIMED_OUT. Everything
		// else means the thread should quit. Deleting the semaphore, as in
		// VideoProducer::_HandleStop(), will trigger this behavior.
		switch (err) {
			case B_OK:
				TRACE("_FrameGeneratorThread: going back to sleep.\n");
				break;
			case B_TIMED_OUT:
				TRACE("_FrameGeneratorThread: timed out => event\n");
				// Catch the cases in which the node manager could not be
				// locked and we therefore have no valid data to work with,
				// or the producer is not running or enabled.
				if (ignoreEvent || !fRunning || !fEnabled) {
					TRACE("_FrameGeneratorThread: ignore event\n");
					// nothing to do
				} else if (!forceSendingBuffer
					&& nextWaitUntil < system_time() - fBufferLatency
					&& droppedFrames < kMaxDroppedFrames) {
					// Drop frame if it's at least a frame late.
					if (playingDirection > 0) {
						printf("VideoProducer: dropped frame (%" B_PRId64
							") (perf. time %" B_PRIdBIGTIME ")\n", fFrame,
							performanceTime);
					}
					// next frame
					droppedFrames++;
					fFrame++;
				} else if (playingDirection != 0 || forceSendingBuffer) {
					// Send buffers only, if playing, the node is running and
					// the output has been enabled
					TRACE("_FrameGeneratorThread: produce frame\n");
					BAutolock _(fLock);
					// Fetch a buffer from the buffer group
					fUsedBufferGroup->WaitForBuffers();
					BBuffer* buffer = fUsedBufferGroup->RequestBuffer(
						fConnectedFormat.display.bytes_per_row
						* fConnectedFormat.display.line_count, 0LL);
					if (buffer == NULL) {
						// Wait until a buffer becomes available again
						ERROR("_FrameGeneratorThread: no buffer!\n");
						break;
					}
					// Fill out the details about this buffer.
					media_header* h = buffer->Header();
					h->type = B_MEDIA_RAW_VIDEO;
					h->time_source = TimeSource()->ID();
					h->size_used = fConnectedFormat.display.bytes_per_row
						* fConnectedFormat.display.line_count;
					// For a buffer originating from a device, you might
					// want to calculate this based on the
					// PerformanceTimeFor the time your buffer arrived at
					// the hardware (plus any applicable adjustments).
					h->start_time = fPerformanceTimeBase + performanceTime;
					h->file_pos = 0;
					h->orig_size = 0;
					h->data_offset = 0;
					h->u.raw_video.field_gamma = 1.0;
					h->u.raw_video.field_sequence = fFrame;
					h->u.raw_video.field_number = 0;
					h->u.raw_video.pulldown_number = 0;
					h->u.raw_video.first_active_line = 1;
					h->u.raw_video.line_count
						= fConnectedFormat.display.line_count;
					// Fill in a frame
					TRACE("_FrameGeneratorThread: frame: %Ld, "
						"playlistFrame: %Ld\n", fFrame, playlistFrame);
					bool wasCached = false;
					err = fSupplier->FillBuffer(playlistFrame,
						buffer->Data(), fConnectedFormat, forceSendingBuffer,
						wasCached);
					if (err == B_TIMED_OUT) {
						// Don't send the buffer if there was insufficient
						// time for rendering, this will leave the last
						// valid frame on screen until we catch up, instead
						// of going black.
						wasCached = true;
						err = B_OK;
					}
					// clean the buffer if something went wrong
					if (err != B_OK && err != B_LAST_BUFFER_ERROR) {
						// TODO: should use "back value" according
						// to color space!
						memset(buffer->Data(), 0, h->size_used);
						err = B_OK;
					} else if (err == B_LAST_BUFFER_ERROR)
						running = false;
					// Send the buffer on down to the consumer
					if (wasCached || (err = SendBuffer(buffer, fOutput.source,
							fOutput.destination) != B_OK)) {
						// If there is a problem sending the buffer,
						// or if we don't send the buffer because its
						// contents are the same as the last one,
						// return it to its buffer group.
						buffer->Recycle();
						// we tell the supplier to delete
						// its caches if there was a problem sending
						// the buffer
						if (err != B_OK) {
							ERROR("_FrameGeneratorThread: Error "
								"sending buffer\n");
							fSupplier->DeleteCaches();
						}
					}
					// Only if everything went fine we clear the flag
					// that forces us to send a buffer even if not
					// playing.
					if (err == B_OK)
						forceSendingBuffer = false;
					// next frame
					fFrame++;
					droppedFrames = 0;
				} else {
					TRACE("_FrameGeneratorThread: not playing\n");
					// next frame
					fFrame++;
				}
				break;
			default:
				TRACE("_FrameGeneratorThread: Couldn't acquire semaphore. "
					"Error: %s\n", strerror(err));
				running = false;
				break;
		}
	}
	TRACE("_FrameGeneratorThread: frame generator thread done.\n");
	return B_OK;
}

