/*
 * FireWire DV media addon for Haiku
 *
 * Copyright (c) 2008, JiSheng Zhang (jszhang3@mail.ustc.edu.cn)
 * Distributed under the terms of the MIT License.
 *
 * Based on DVB media addon 
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 */


#include "FireWireDVNode.h"

#include <fcntl.h>
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

#include <Autolock.h>
#include <Buffer.h>
#include <BufferGroup.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <MediaRoster.h>
#include <ParameterWeb.h>
#include <Path.h>
#include <TimeSource.h>
#include <String.h>

#include "FireWireDVNode.h"
#include "FireWireCard.h"
#include "debug.h"

#define REVISION		"unknown"
#define VERSION			"1.0"
#define BUILD	 		__DATE__ " "__TIME__

// debugging
#ifdef TRACE
#	undef TRACE
#endif
//#define TRACE_FIREWIRE_NODE
#ifdef TRACE_FIREWIRE_NODE
#	define TRACE(x...)		printf(x)
#else
#	define TRACE(x...)
#endif

#define RETURN_IF_ERROR(expr) { status_t e = (expr); if (e != B_OK) return e; }

#define M_REFRESH_PARAMETER_WEB 		(BTimedEventQueue::B_USER_EVENT + 1)

FireWireDVNode::FireWireDVNode(BMediaAddOn* addon, const char* name, 
	int32 internal_id, FireWireCard* card)
	: BMediaNode(name),
	BBufferProducer(B_MEDIA_ENCODED_VIDEO),	
	BControllable(),
	BMediaEventLooper(),
	fOutputEnabledEncVideo(false),
	fCard(card),
	fCaptureThreadsActive(false),
	fThreadIdCardReader(-1),
	fTerminateThreads(false),
	fBufferGroupEncVideo(0),
	fCaptureActive(false)
{
	CALLED();

	AddNodeKind(B_PHYSICAL_INPUT);
//	AddNodeKind(B_PHYSICAL_OUTPUT);

	fInternalID = internal_id;
	fAddOn = addon;
	
	fInitStatus = B_OK;
	
	fDefaultFormatEncVideo.type = B_MEDIA_ENCODED_VIDEO;
}


FireWireDVNode::~FireWireDVNode()
{
	CALLED();
	
	StopCapture();
	fWeb = NULL;	
}


/* BMediaNode */
BMediaAddOn*
FireWireDVNode::AddOn(int32* internal_id) const
{
	if (internal_id)
		*internal_id = fInternalID;
	return fAddOn;
}


status_t 
FireWireDVNode::HandleMessage(int32 message, const void* data, size_t size)
{
	return B_ERROR;
}


void 
FireWireDVNode::Preroll()
{
	/* This hook may be called before the node is started to give the hardware
	 * a chance to start. */
}


void
FireWireDVNode::SetTimeSource(BTimeSource* time_source)
{
	CALLED();
}


void
FireWireDVNode::SetRunMode(run_mode mode)
{
	CALLED();
	TRACE("FireWireDVNode::SetRunMode(%d)\n", mode);
}


/* BMediaEventLooper */
void 
FireWireDVNode::NodeRegistered()
{
	CALLED();

	SetPriority(B_REAL_TIME_PRIORITY);
	Run();

	fOutputEncVideo.node = Node();
	fOutputEncVideo.source.port = ControlPort();
	fOutputEncVideo.source.id = 0;
	fOutputEncVideo.destination = media_destination::null;
	fOutputEncVideo.format = fDefaultFormatEncVideo;
	strcpy(fOutputEncVideo.name, "encoded video");	
	
	RefreshParameterWeb();
}


void 
FireWireDVNode::HandleEvent(const media_timed_event* event,
	bigtime_t lateness, bool realTimeEvent)
{

	switch(event->type)
	{
		case M_REFRESH_PARAMETER_WEB:
			RefreshParameterWeb();
			break;
		case BTimedEventQueue::B_START:
			HandleStart(event->event_time);
			break;
		case BTimedEventQueue::B_STOP:
			HandleStop();
			break;
		case BTimedEventQueue::B_WARP:
			HandleTimeWarp(event->bigdata);
			break;
		case BTimedEventQueue::B_SEEK:
			HandleSeek(event->bigdata);
			break;
		case BTimedEventQueue::B_HANDLE_BUFFER:
		case BTimedEventQueue::B_DATA_STATUS:
		case BTimedEventQueue::B_PARAMETER:
		default:
			TRACE("FireWireDVNode::HandleEvent: Unhandled event -- %lx\n", event->type);
			break;
	}
}


/* BBufferProducer */
status_t 
FireWireDVNode::FormatChangeRequested(const media_source& source,
	const media_destination& destination, media_format* io_format,
	int32* _deprecated_)
{
	CALLED();

	// we don't support any other formats, so we just reject any format changes.
	return B_ERROR;
}


status_t 
FireWireDVNode::GetNextOutput(int32* cookie, media_output* out_output)
{
	CALLED();
	
	if (*cookie == 0) {
		*out_output = fOutputEncVideo;
		*cookie += 1;
		return B_OK;
	} else {
		return B_BAD_INDEX;
	}
}


status_t 
FireWireDVNode::DisposeOutputCookie(int32 cookie)
{
	CALLED();
	// do nothing because we don't use the cookie for anything special
	return B_OK;
}


status_t 
FireWireDVNode::SetBufferGroup(const media_source& source, BBufferGroup* group)
{
	CALLED();
	return B_ERROR;
}


status_t 
FireWireDVNode::VideoClippingChanged(const media_source& for_source,
	int16 num_shorts, int16* clip_data,
	const media_video_display_info& display, int32* _deprecated_)
{
	CALLED();
	return B_ERROR;
}


status_t 
FireWireDVNode::GetLatency(bigtime_t* out_latency)
{
	CALLED();
	
	*out_latency = EventLatency() + SchedulingLatency();
	return B_OK;
}


status_t 
FireWireDVNode::FormatSuggestionRequested(
	media_type type, int32 quality, media_format* format)
{
	CALLED();

	if (format == NULL) {
		fprintf(stderr, "\tERROR - NULL format pointer passed in!\n");
		return B_BAD_VALUE;
	}

	// this is the format we'll be returning (our preferred format)
	*format = fDefaultFormatEncVideo;

	// a wildcard type is okay; we can specialize it
	if (type == B_MEDIA_UNKNOWN_TYPE)
		type = B_MEDIA_ENCODED_VIDEO;

	if (type != B_MEDIA_ENCODED_VIDEO)
		return B_MEDIA_BAD_FORMAT;

	return B_OK;
}


status_t 
FireWireDVNode::FormatProposal(const media_source& source, 
	media_format* format)
{
	CALLED();	
	/* The connection process:
	 * we are here => BBufferProducer::FormatProposal
	 *                BBufferConsumer::AcceptFormat
	 *                BBufferProducer::PrepareToConnect
	 *                BBufferConsumer::Connected
	 *                BBufferProducer::Connect
	 *
	 * What we need to do:
	 * - if the format contains a wildcard AND we have a requirement for that
	 *   field, set it to the value we need.
	 * - if a field has a value that is not wildcard and not supported by us,
	 *   we don't change it, and return B_MEDIA_BAD_FORMAT
	 * - after we are done, the format may still contain wildcards.
	 */
	
	if (source.port != ControlPort()) {
		fprintf(stderr, "FireWireDVNode::FormatProposal returning "
			"B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	media_type requestedType = format->type;
	*format = fDefaultFormatEncVideo;

	if (requestedType != B_MEDIA_UNKNOWN_TYPE
		&& requestedType != B_MEDIA_ENCODED_VIDEO) {
		fprintf(stderr, "FireWireDVNode::FormatProposal returning "
			"B_MEDIA_BAD_FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}
	
	// encoded video or wildcard type, either is okay by us
	return B_OK;
}


status_t 
FireWireDVNode::PrepareToConnect(const media_source& source,
	const media_destination& destination, media_format* format,
	media_source* out_source, char* out_name)
{
	/* The connection process:
	 *                BBufferProducer::FormatProposal
	 *                BBufferConsumer::AcceptFormat
	 * we are here => BBufferProducer::PrepareToConnect
	 *                BBufferConsumer::Connected
	 *                BBufferProducer::Connect
	 *
	 * At this point, the consumer's AcceptFormat() method has been called,
	 * and that node has potentially changed the proposed format. It may
	 * also have left wildcards in the format. PrepareToConnect()
	 * *must* fully specialize the format before returning!
	 */

	CALLED();
	// is the source valid?
	if (source.port != ControlPort() &&
			fCard->DetectRecvFn() != B_OK) {
		fprintf(stderr, "FireWireDVNode::PrepareToConnect returning "
			"B_MEDIA_BAD_SOURCE\n");
		return B_MEDIA_BAD_SOURCE;
	}

	// are we already connected?
	if (fOutputEncVideo.destination != media_destination::null)
		return B_MEDIA_ALREADY_CONNECTED;

	// the format may not yet be fully specialized (the consumer might have
	// passed back some wildcards).  Finish specializing it now, and return an
	// error if we don't support the requested format.
	if (format->type != B_MEDIA_RAW_AUDIO) {
		fprintf(stderr, "\tnon-raw-audio format?!\n");
		return B_MEDIA_BAD_FORMAT;
	}

	// reserve the connection by setting destination
	// set the output's format to the new format
	fOutputEncVideo.destination = destination;
	fOutputEncVideo.format = *format;

	// set source and suggest a name
	*out_source = source;
	strcpy(out_name, "encoded video");
	
	return B_OK;
}


void 
FireWireDVNode::Connect(status_t error, const media_source& source,
	const media_destination& destination, const media_format& format,
	char* io_name)
{
	/* The connection process:
	 *                BBufferProducer::FormatProposal
	 *                BBufferConsumer::AcceptFormat
	 *                BBufferProducer::PrepareToConnect
	 *                BBufferConsumer::Connected
	 * we are here => BBufferProducer::Connect
	 */

	CALLED();

	if (error != B_OK) {
		TRACE("Error during connecting\n");
		// if an error occured, unreserve the connection
		fOutputEncVideo.destination = media_destination::null;
		fOutputEncVideo.format = fDefaultFormatEncVideo;
		return;
	}

	// Since the destination is allowed to be changed by the
	// consumer, the one we got in PrepareToConnect() is no
	// longer correct, and must be updated here.
	fOutputEncVideo.destination = destination;
	fOutputEncVideo.format = format;

	// if the connection has no name, we set it now
	if (strlen(io_name) == 0)
		strcpy(io_name, "encoded video");

	#ifdef DEBUG
		bigtime_t latency;
		media_node_id ts;
		if (B_OK != FindLatencyFor(destination, &latency, &ts))
			TRACE("FindLatencyFor failed\n");
		else
			TRACE("downstream latency %Ld\n", latency);
	#endif
}


void 
FireWireDVNode::Disconnect(const media_source &source,
	const media_destination& destination)
{
	CALLED();

	// unreserve the connection
	fOutputEncVideo.destination = media_destination::null;
	fOutputEncVideo.format = fDefaultFormatEncVideo;
}


void 
FireWireDVNode::LateNoticeReceived(const media_source& source,
	bigtime_t how_much, bigtime_t performance_time)
{
	TRACE("FireWireDVNode::LateNoticeReceived %Ld late at %Ld\n", how_much, performance_time);
}


void 
FireWireDVNode::EnableOutput(const media_source& source, bool enabled,
	int32* _deprecated_)
{
	CALLED();
	fOutputEnabledEncVideo = enabled;
}


void 
FireWireDVNode::AdditionalBufferRequested(const media_source& source,
	media_buffer_id prev_buffer, bigtime_t prev_time,
	const media_seek_tag* prev_tag)
{
	CALLED();
	// we don't support offline mode
	return;
}


/* FireWireDVNode */
void
FireWireDVNode::HandleTimeWarp(bigtime_t performance_time)
{
	TRACE("FireWireDVNode::HandleTimeWarp at %Ld\n", performance_time);
}


void
FireWireDVNode::HandleSeek(bigtime_t performance_time)
{
	TRACE("FireWireDVNode::HandleSeek at %Ld\n", performance_time);
}


void
FireWireDVNode::HandleStart(bigtime_t performance_time)
{
	CALLED();
	StartCapture();
}


void
FireWireDVNode::HandleStop(void)
{
	CALLED();
	StopCapture();
}	


status_t
FireWireDVNode::StartCapture()
{
	CALLED();

	if (fCaptureActive)
		return B_OK;
		
	RETURN_IF_ERROR(StopCaptureThreads());
	
	RETURN_IF_ERROR(StartCaptureThreads());
	
	fCaptureActive = true;

	RefreshParameterWeb();

	return B_OK;	
}


status_t
FireWireDVNode::StopCapture()
{
	CALLED();
	if (!fCaptureActive)
		return B_OK;

	StopCaptureThreads();
	
	fCaptureActive = false;
	return B_OK;
}


status_t
FireWireDVNode::StartCaptureThreads()
{
	CALLED();

	if (fCaptureThreadsActive)
		return B_OK;

	fTerminateThreads = false;

	fThreadIdCardReader = spawn_thread(_card_reader_thread_, "FireWire DV reader", 120, this);
	resume_thread(fThreadIdCardReader);
	
	fCaptureThreadsActive = true;
	return B_OK;
}


status_t
FireWireDVNode::StopCaptureThreads()
{
	CALLED();
	
	if (!fCaptureThreadsActive)
		return B_OK;
	
	fTerminateThreads = true;
	
	status_t dummy; // NULL as parameter does not work
	wait_for_thread(fThreadIdCardReader, &dummy);
	
	fCaptureThreadsActive = false;
	return B_OK;
}


int32
FireWireDVNode::_card_reader_thread_(void* arg)
{
	static_cast<FireWireDVNode *>(arg)->card_reader_thread();
	return 0;
}


void
FireWireDVNode::card_reader_thread()
{
	status_t err;
	size_t rbufsize;
	int rcount;

	fCard->GetBufInfo(&rbufsize, &rcount);
	delete fBufferGroupEncVideo;
	fBufferGroupEncVideo = new BBufferGroup(rbufsize, rcount);
	while (!fTerminateThreads) {
		void *data, *end;
		ssize_t sizeUsed = fCard->Read(&data);
		if (sizeUsed < 0) {
			TRACE("FireWireDVNode::%s: %s\n", __FUNCTION__,
				strerror(sizeUsed));
			continue;
		}

		end = (char*)data + sizeUsed;

		while (data < end) {
			BBuffer* buf = fBufferGroupEncVideo->RequestBuffer(rbufsize, 10000);
			if (!buf) {
				TRACE("OutVideo: request buffer timout\n");
				continue;
			}
			
			err = fCard->Extract(buf->Data(), &data, &sizeUsed);
			if (err) {
				buf->Recycle();
				printf("OutVideo Extract error %s\n", strerror(err));
				continue;
			}
	
			media_header* hdr = buf->Header();
			hdr->type = B_MEDIA_ENCODED_VIDEO;
			hdr->size_used = sizeUsed;
			hdr->time_source = TimeSource()->ID();	// set time source id
			//what should the start_time be?
			hdr->start_time = TimeSource()->PerformanceTimeFor(system_time());

			fLock.Lock();
			if (SendBuffer(buf, fOutputEncVideo.source,
					fOutputEncVideo.destination) != B_OK) {
				TRACE("OutVideo: sending buffer failed\n");
				buf->Recycle();
			} 
			fLock.Unlock();
		}
		
	}
}


void
FireWireDVNode::RefreshParameterWeb()
{
	TRACE("FireWireDVNode::RefreshParameterWeb enter\n");
	fWeb = CreateParameterWeb();
	SetParameterWeb(fWeb);
	TRACE("FireWireDVNode::RefreshParameterWeb finished\n");
}


void
FireWireDVNode::SetAboutInfo(BParameterGroup* about)
{
	//May need more useful infomation?
	about->MakeNullParameter(0, B_MEDIA_NO_TYPE, "FireWireDV media_addon info:", B_GENERIC);
	about->MakeNullParameter(0, B_MEDIA_NO_TYPE, "Version " VERSION, B_GENERIC);
	about->MakeNullParameter(0, B_MEDIA_NO_TYPE, "Revision " REVISION, B_GENERIC);
	about->MakeNullParameter(0, B_MEDIA_NO_TYPE, "Build " BUILD, B_GENERIC);

	about->MakeNullParameter(0, B_MEDIA_NO_TYPE, "", B_GENERIC);
	about->MakeNullParameter(0, B_MEDIA_NO_TYPE, "Driver info:", B_GENERIC);
}


BParameterWeb *
FireWireDVNode::CreateParameterWeb()
{
	/* Set up the parameter web */
	BParameterWeb* web = new BParameterWeb();

	BString name;
	name << Name();
	
	BParameterGroup* main = web->MakeGroup(name.String());

	if (!fCaptureActive) {
		BParameterGroup* info = main->MakeGroup("Info");
		info->MakeNullParameter(0, B_MEDIA_NO_TYPE, info->Name(), B_GENERIC);
		BParameterGroup* about = main->MakeGroup("About");
		about->MakeNullParameter(0, B_MEDIA_NO_TYPE, about->Name(), B_GENERIC);
		SetAboutInfo(about);
		info->MakeNullParameter(0, B_MEDIA_NO_TYPE, "Node is stopped", B_GENERIC);
		info->MakeNullParameter(0, B_MEDIA_NO_TYPE, "or tuning failed.", B_GENERIC);
		return web;
	}

	BParameterGroup* about = main->MakeGroup("About");
	about->MakeNullParameter(0, B_MEDIA_NO_TYPE, about->Name(), B_GENERIC);
	SetAboutInfo(about);
	
	return web;
}


status_t 
FireWireDVNode::GetParameterValue(int32 id, bigtime_t* last_change, 
	void* value, size_t* size)
{
	TRACE("FireWireDVNode::GetParameterValue, id 0x%lx\n", id);
	//do we need Parameter for firewire dv?	
	return B_OK;
}


void
FireWireDVNode::SetParameterValue(int32 id, bigtime_t when, const void* value, 
	size_t size)
{
	TRACE("FireWireDVNode::SetParameterValue, id 0x%lx, size %ld, "
		"value 0x%lx\n", id, size, *(const int32*)value);
	//do we need parameter for firewire dv?
	TRACE("FireWireDVNode::SetParameterValue finished\n");
}

