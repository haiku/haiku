/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */


#include <fcntl.h>
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

#include <MediaRoster.h>
#include <Buffer.h>
#include <BufferGroup.h>
#include <ParameterWeb.h>
#include <TimeSource.h>
#include <String.h>
#include <Autolock.h>
#include <Debug.h>

#include <Directory.h>
#include <Entry.h>
#include <Path.h>

#include "MediaFormat.h"
#include "Packet.h"
#include "PacketQueue.h"
#include "pes.h"
#include "media_header_ex.h"
#include "config.h"

//#define DUMP_VIDEO
//#define DUMP_AUDIO
//#define DUMP_RAW_AUDIO


#include "DVBMediaNode.h"

#define ENABLE_TRACE
//#define ENABLE_TRACE_TIMING

#undef TRACE

#ifdef ENABLE_TRACE
	#define TRACE printf
#else
	#define TRACE(a...)
#endif

#ifdef ENABLE_TRACE_TIMING
	#define TRACE_TIMING printf
#else
	#define TRACE_TIMING(a...)
#endif
		
#define RETURN_IF_ERROR(expr) { status_t e = (expr); if (e != B_OK) return e; }

#define ID_RAW_VIDEO	0
#define ID_RAW_AUDIO	1
#define ID_ENC_VIDEO	2
#define ID_ENC_AUDIO	3
#define ID_TS			4

// Timeouts for requesting buffers, if the system is busy,
// the output buffer queue is full, requesting a buffer will
// timeout, and we need to drop the current data
#define VIDEO_BUFFER_REQUEST_TIMEOUT	20000	
#define AUDIO_BUFFER_REQUEST_TIMEOUT	10000

// DVB data arrives early and with a timestamp, this is used to validate
// that the timestamp is correct and we don't get stuck
#define VIDEO_MAX_EARLY					3000000	// up to 3 seconds too early
#define VIDEO_MAX_LATE					50000	// no more than 50 ms too late
#define AUDIO_MAX_EARLY					3000000	// up to 3 seconds too early
#define AUDIO_MAX_LATE					50000	// no more than 50 ms too late

#define PROCESSING_LATENCY				1500	// assumed latency for sending the buffer

#define STOP_CAPTURE_WHILE_TUNING		1

#define MPEG2_VIDEO_DECODER_PATH		"/boot/home/config/add-ons/media/dvb/video_decoder_addon"
#define MPEG2_AUDIO_DECODER_PATH		"/boot/home/config/add-ons/media/dvb/audio_decoder_addon"
#define AC3_AUDIO_DECODER_PATH			"/boot/home/config/add-ons/media/dvb/ac3_decoder_addon"
#define DEINTERLACE_FILTER_PATH			"/boot/home/config/add-ons/media/dvb/deinterlace_filter_addon"


#define M_REFRESH_PARAMETER_WEB 		(BTimedEventQueue::B_USER_EVENT + 1)


DVBMediaNode::DVBMediaNode(
	BMediaAddOn *addon, const char *name,
	int32 internal_id, DVBCard *card)
 :	BMediaNode(name)
 ,	BBufferProducer(B_MEDIA_RAW_VIDEO)
 ,	BControllable()
 ,	BMediaEventLooper()
 ,	fStopDisabled(false)
 ,	fOutputEnabledRawVideo(false)
 ,	fOutputEnabledRawAudio(false)
 ,	fOutputEnabledEncVideo(false)
 ,	fOutputEnabledEncAudio(false)
 ,	fOutputEnabledTS(false)
 ,	fCardDataQueue(new PacketQueue(6))
 ,	fRawVideoQueue(new PacketQueue(56))
 ,	fRawAudioQueue(new PacketQueue(56))
 ,	fEncVideoQueue(new PacketQueue(56))
 ,	fEncAudioQueue(new PacketQueue(56))
 ,	fMpegTsQueue(new PacketQueue(16))
 ,	fCard(card)
 ,	fCaptureThreadsActive(false)
 ,	fThreadIdCardReader(-1)
 ,	fThreadIdMpegDemux(-1)
 ,	fThreadIdRawAudio(-1)
 ,	fThreadIdRawVideo(-1)
 ,	fThreadIdEncAudio(-1)
 ,	fThreadIdEncVideo(-1)
 ,	fThreadIdMpegTS(-1)
 ,	fTerminateThreads(false)
 ,	fDemux(new TransportStreamDemux(fRawVideoQueue, fRawAudioQueue,
 		fEncVideoQueue, fEncAudioQueue, fMpegTsQueue))
 ,	fBufferGroupRawVideo(0)
 ,	fBufferGroupRawAudio(0)
 ,	fInterfaceType(DVB_TYPE_UNKNOWN)
 ,	fAudioPid(-1)
 ,	fVideoPid(-1)
 ,	fPcrPid(-1)
 ,	fTuningSuccess(false)
 ,	fCaptureActive(false)
 ,	fVideoDelaySem(create_sem(0, "video delay sem"))
 ,	fAudioDelaySem(create_sem(0, "audio delay sem"))
 ,	fSelectedState(-1)
 ,	fSelectedRegion(-1)
 ,	fSelectedChannel(-1)
 ,	fSelectedAudio(-1)
 ,	fStateList(new StringList)
 ,	fRegionList(new StringList)
 ,	fChannelList(new StringList)
 ,	fAudioList(new StringList)
 ,	fVideoDecoder(0)
 ,	fAudioDecoder(0)
 ,	fCurrentVideoPacket(0)
 ,	fCurrentAudioPacket(0)
{
	TRACE("DVBMediaNode::DVBMediaNode\n");

	AddNodeKind(B_PHYSICAL_INPUT);

	fInternalID = internal_id;
	fAddOn = addon;
	
	fInitStatus = B_OK;
	
	InitDefaultFormats();

	// in the beginning, the required formats are the same as the defaults
	fRequiredFormatRawVideo = fDefaultFormatRawVideo;
	fRequiredFormatRawAudio = fDefaultFormatRawAudio;
	fRequiredFormatEncVideo = fDefaultFormatEncVideo;
	fRequiredFormatEncAudio = fDefaultFormatEncAudio;
	fRequiredFormatTS = fDefaultFormatTS;

	// start the BMediaEventLooper control loop running
	Run();

	TRACE("current RunMode = %d\n", RunMode());

#ifdef DUMP_VIDEO
	fVideoFile = open("/boot/home/dvb-video.mpg", O_RDWR | O_CREAT | O_TRUNC);
#endif
#ifdef DUMP_AUDIO
	fAudioFile = open("/boot/home/dvb-audio.mpg", O_RDWR | O_CREAT | O_TRUNC);
#endif
#ifdef DUMP_RAW_AUDIO
	fRawAudioFile = open("/boot/home/dvb-audio.raw", O_RDWR | O_CREAT | O_TRUNC);
#endif
}


DVBMediaNode::~DVBMediaNode()
{
	TRACE("DVBMediaNode::~DVBMediaNode\n");
	
	StopCapture();
	
	delete_sem(fVideoDelaySem);
	delete_sem(fAudioDelaySem);

	// fCard is owned by the media addon
	delete fCardDataQueue;
	delete fRawVideoQueue;
	delete fRawAudioQueue;
	delete fEncVideoQueue;
	delete fEncAudioQueue;
	delete fMpegTsQueue;

	delete fDemux;

	delete fBufferGroupRawVideo;
	delete fBufferGroupRawAudio;

	delete fStateList;
	delete fRegionList;
	delete fChannelList;
	delete fAudioList;
	
#ifdef DUMP_VIDEO
	close(fVideoFile);
#endif
#ifdef DUMP_AUDIO
	close(fAudioFile);
#endif
#ifdef DUMP_RAW_AUDIO
	close(fRawAudioFile);
#endif

}


/* BMediaNode */


BMediaAddOn *
DVBMediaNode::AddOn(int32 *internal_id) const
{
	if (internal_id)
		*internal_id = fInternalID;
	return fAddOn;
}


status_t 
DVBMediaNode::HandleMessage(int32 message, const void *data, size_t size)
{
	return B_ERROR;
}


void 
DVBMediaNode::Preroll()
{
	/* This hook may be called before the node is started to give the hardware
	 * a chance to start. */
}


void
DVBMediaNode::SetTimeSource(BTimeSource *time_source)
{
	TRACE("DVBMediaNode::SetTimeSource\n");
	//printf("current RunMode = %d\n", RunMode());
	//printf("_m_recordDelay = %Ld\n", _m_recordDelay);
}


void
DVBMediaNode::SetRunMode(run_mode mode)
{
	TRACE("DVBMediaNode::SetRunMode: %d\n", mode);
	TRACE("current RunMode = %d\n", RunMode());
	//printf("_m_recordDelay = %Ld\n", _m_recordDelay);
}


/* BMediaEventLooper */


void 
DVBMediaNode::NodeRegistered()
{
	TRACE("DVBMediaNode::NodeRegistered\n");

	fOutputRawVideo.node = Node();
	fOutputRawVideo.source.port = ControlPort();
	fOutputRawVideo.source.id = ID_RAW_VIDEO;
	fOutputRawVideo.destination = media_destination::null;
	fOutputRawVideo.format = fDefaultFormatRawVideo;
	strcpy(fOutputRawVideo.name, SourceDefaultName(fOutputRawVideo.source));	

	fOutputRawAudio.node = Node();
	fOutputRawAudio.source.port = ControlPort();
	fOutputRawAudio.source.id = ID_RAW_AUDIO;
	fOutputRawAudio.destination = media_destination::null;
	fOutputRawAudio.format = fDefaultFormatRawAudio;
	strcpy(fOutputRawAudio.name, SourceDefaultName(fOutputRawAudio.source));	

	fOutputEncVideo.node = Node();
	fOutputEncVideo.source.port = ControlPort();
	fOutputEncVideo.source.id = ID_ENC_VIDEO;
	fOutputEncVideo.destination = media_destination::null;
	fOutputEncVideo.format = fDefaultFormatEncVideo;
	strcpy(fOutputEncVideo.name, SourceDefaultName(fOutputEncVideo.source));	

	fOutputEncAudio.node = Node();
	fOutputEncAudio.source.port = ControlPort();
	fOutputEncAudio.source.id = ID_ENC_AUDIO;
	fOutputEncAudio.destination = media_destination::null;
	fOutputEncAudio.format = fDefaultFormatEncAudio;
	strcpy(fOutputEncAudio.name, SourceDefaultName(fOutputEncAudio.source));	

	fOutputTS.node = Node();
	fOutputTS.source.port = ControlPort();
	fOutputTS.source.id = ID_TS;
	fOutputTS.destination = media_destination::null;
	fOutputTS.format = fDefaultFormatTS;
	strcpy(fOutputTS.name, SourceDefaultName(fOutputTS.source));	

	fCard->GetCardType(&fInterfaceType);

	// set control thread priority	
	SetPriority(110);

	LoadSettings();
	
	RefreshParameterWeb();

	// this nodes operates in recording mode, so set it (will be done asynchronously)
	BMediaRoster::Roster()->SetRunModeNode(Node(), B_RECORDING);
	// as it's a notification hook, calling this doesn't work: SetRunMode(B_RECORDING);

	//printf("RunMode = %d\n", RunMode());
	//printf("_m_recordDelay = %Ld\n", _m_recordDelay);
}


void
DVBMediaNode::Stop(bigtime_t performance_time, bool immediate)
{
	if (fStopDisabled)
		return;
	else
		BMediaEventLooper::Stop(performance_time, immediate);
}


void 
DVBMediaNode::HandleEvent(const media_timed_event *event,
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
			TRACE("DVBMediaNode::HandleEvent: Unhandled event -- %lx\n", event->type);
			break;
	}
}


/* BBufferProducer */


status_t 
DVBMediaNode::FormatChangeRequested(const media_source &source,
		const media_destination &destination, media_format *io_format,
		int32 *_deprecated_)
{
	TRACE("DVBMediaNode::FormatChangeRequested denied: %s\n", SourceDefaultName(source));
	return B_ERROR;	
}


status_t 
DVBMediaNode::GetNextOutput(int32 *cookie, media_output *out_output)
{
	switch (*cookie) {
		case 0: *out_output = fOutputRawVideo;	break;
		case 1: *out_output = fOutputRawAudio;	break;
		case 2: *out_output = fOutputEncVideo;	break;
		case 3: *out_output = fOutputEncAudio;	break;
		case 4: *out_output = fOutputTS;		break;
		default:								return B_BAD_INDEX;
	}

	(*cookie) += 1;
	return B_OK;
}


status_t 
DVBMediaNode::DisposeOutputCookie(int32 cookie)
{
	return B_OK;
}


status_t 
DVBMediaNode::SetBufferGroup(const media_source &source, BBufferGroup *group)
{
	TRACE("DVBMediaNode::SetBufferGroup denied: %s\n", SourceDefaultName(source));
	return B_ERROR;
}


status_t 
DVBMediaNode::VideoClippingChanged(const media_source &for_source,
		int16 num_shorts, int16 *clip_data,
		const media_video_display_info &display, int32 *_deprecated_)
{
	return B_ERROR;
}


status_t 
DVBMediaNode::GetLatency(bigtime_t *out_latency)
{
	if (B_OK != BBufferProducer::GetLatency(out_latency))
		*out_latency = 50000;
		
	printf("DVBMediaNode::GetLatency: %Ld\n", *out_latency);
	*out_latency += PROCESSING_LATENCY;
	return B_OK;
}


status_t 
DVBMediaNode::FormatSuggestionRequested(
		media_type type, int32 quality, media_format *format)
{
	TRACE("DVBMediaNode::FormatSuggestionRequested\n");
	
	switch (type) {
		case B_MEDIA_RAW_VIDEO: 	*format = fDefaultFormatRawVideo; break;
		case B_MEDIA_RAW_AUDIO: 	*format = fDefaultFormatRawAudio; break;
		case B_MEDIA_ENCODED_VIDEO:	*format = fDefaultFormatEncVideo; break;
		case B_MEDIA_ENCODED_AUDIO:	*format = fDefaultFormatEncAudio; break;
		case B_MEDIA_MULTISTREAM:	*format = fDefaultFormatTS;		  break;
		default: TRACE("Bad type!\n");								  return B_MEDIA_BAD_FORMAT;
	}

	#ifdef DEBUG
		TRACE("suggested format: ");
		PrintFormat(*format);
	#endif

	return B_OK;
}


status_t 
DVBMediaNode::FormatProposal(const media_source &source, media_format *format)
{
	TRACE("DVBMediaNode::FormatProposal: %s\n", SourceDefaultName(source));
	
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
	
	if (source.port != ControlPort())
		goto _bad_source;

	#ifdef DEBUG
		TRACE("proposed format: ");
		PrintFormat(*format);
		TRACE("required format: ");
		switch (source.id) {
			case ID_RAW_VIDEO: PrintFormat(fRequiredFormatRawVideo); break;
			case ID_RAW_AUDIO: PrintFormat(fRequiredFormatRawAudio); break;
			case ID_ENC_VIDEO: PrintFormat(fRequiredFormatEncVideo); break;
			case ID_ENC_AUDIO: PrintFormat(fRequiredFormatEncAudio); break;
			case ID_TS:        PrintFormat(fRequiredFormatTS); break;
		}
	#endif

	switch (source.id) {
		case ID_RAW_VIDEO:
			// check if destination still available
			if (fOutputRawVideo.destination != media_destination::null)
				goto _bad_source;
			// set requirements and check if compatible
			color_space c;
			c = format->u.raw_video.display.format;
			format->SpecializeTo(&fRequiredFormatRawVideo);
			format->u.raw_video.display.format = c;
//			if (!format->Matches(&fRequiredFormatRawVideo))
//				goto _bad_format_1;
			if (!VerifyFormatRawVideo(format->u.raw_video))
				goto _bad_format_2;
			break;

		case ID_RAW_AUDIO:
			// check if destination still available
			if (fOutputRawAudio.destination != media_destination::null)
				goto _bad_source;
			// set requirements and check if compatible
			format->SpecializeTo(&fRequiredFormatRawAudio);
			if (!format->Matches(&fRequiredFormatRawAudio))
				goto _bad_format_1;
			if (!VerifyFormatRawAudio(format->u.raw_audio))
				goto _bad_format_2;
			break;

		case ID_ENC_VIDEO:
			// check if destination still available
			if (fOutputEncVideo.destination != media_destination::null)
				goto _bad_source;
			// set requirements and check if compatible
			format->SpecializeTo(&fRequiredFormatEncVideo);
			if (!format->Matches(&fRequiredFormatEncVideo))
				goto _bad_format_1;
			break;

		case ID_ENC_AUDIO:
			// check if destination still available
			if (fOutputEncAudio.destination != media_destination::null)
				goto _bad_source;
			// set requirements and check if compatible
			format->SpecializeTo(&fRequiredFormatEncAudio);
			if (!format->Matches(&fRequiredFormatEncAudio))
				goto _bad_format_1;
			break;

		case ID_TS:
			// check if destination still available
			if (fOutputTS.destination != media_destination::null)
				goto _bad_source;
			// set requirements and check if compatible
			format->SpecializeTo(&fRequiredFormatTS);
			if (!format->Matches(&fRequiredFormatTS))
				goto _bad_format_1;
			break;

		default:
			goto _bad_source;
	}

	#ifdef DEBUG
		TRACE("final format: ");
		PrintFormat(*format);
	#endif
	
	return B_OK;

_bad_source:
	TRACE("Error: bad source!\n");
	return B_MEDIA_BAD_SOURCE;

_bad_format_1:
	TRACE("Error, bad format (1): ");
	goto _bad_format;

_bad_format_2:
	TRACE("Error, bad format (2): ");
	goto _bad_format;

_bad_format:
	#ifdef DEBUG
		PrintFormat(*format);
	#endif
	return B_MEDIA_BAD_FORMAT;
}


status_t 
DVBMediaNode::PrepareToConnect(const media_source &source,
		const media_destination &destination, media_format *format,
		media_source *out_source, char *out_name)
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

	TRACE("DVBMediaNode::PrepareToConnect: %s\n", SourceDefaultName(source));

	#ifdef DEBUG
		TRACE("connecting format: ");
		PrintFormat(*format);
		TRACE("required format: ");
		switch (source.id) {
			case ID_RAW_VIDEO: PrintFormat(fRequiredFormatRawVideo); break;
			case ID_RAW_AUDIO: PrintFormat(fRequiredFormatRawAudio); break;
			case ID_ENC_VIDEO: PrintFormat(fRequiredFormatEncVideo); break;
			case ID_ENC_AUDIO: PrintFormat(fRequiredFormatEncAudio); break;
			case ID_TS:        PrintFormat(fRequiredFormatTS); break;
		}
	#endif
	
	// is the source valid?
	if (source.port != ControlPort())
		goto _bad_source;

	// 1) check if the output is still available,
	// 2) specialize and verify the format
	switch (source.id) {
		case ID_RAW_VIDEO:
			if (fOutputRawVideo.destination != media_destination::null)
				goto _already_connected;
			SpecializeFormatRawVideo(&format->u.raw_video);
//			if (!format->Matches(&fRequiredFormatRawVideo))
//				goto _bad_format;
			if (!VerifyFormatRawVideo(format->u.raw_video))
				goto _bad_format;
			break;

		case ID_RAW_AUDIO:
			if (fOutputRawAudio.destination != media_destination::null)
				goto _already_connected;
			SpecializeFormatRawAudio(&format->u.raw_audio);
			if (!format->Matches(&fRequiredFormatRawAudio))
				goto _bad_format;
			if (!VerifyFormatRawAudio(format->u.raw_audio))
				goto _bad_format;
			break;

		case ID_ENC_VIDEO:
			if (fOutputEncVideo.destination != media_destination::null)
				goto _already_connected;
			SpecializeFormatEncVideo(&format->u.encoded_video);
			if (!format->Matches(&fRequiredFormatEncVideo))
				goto _bad_format;
			break;

		case ID_ENC_AUDIO:
			if (fOutputEncAudio.destination != media_destination::null)
				goto _already_connected;
			SpecializeFormatEncAudio(&format->u.encoded_audio);
			if (!format->Matches(&fRequiredFormatRawVideo))
				goto _bad_format;
			break;

		case ID_TS:
			if (fOutputTS.destination != media_destination::null)
				goto _already_connected;
			SpecializeFormatTS(&format->u.multistream);
			if (!format->Matches(&fRequiredFormatTS))
				goto _bad_format;
			break;

		default:
			goto _bad_source;
	}

	#ifdef DEBUG
		TRACE("final format: ");
		PrintFormat(*format);
	#endif

	// reserve the connection by setting destination
	// set the output's format to the new format
	SetOutput(source, destination, *format);

	// set source and suggest a name
	*out_source = source;
	strcpy(out_name, SourceDefaultName(source));
	
	return B_OK;

_bad_source:
	TRACE("Error: bad source!\n");
	return B_MEDIA_BAD_SOURCE;

_bad_format:
	#ifdef DEBUG
		TRACE("Error, bad format: ");
		PrintFormat(*format);
	#endif
	return B_MEDIA_BAD_FORMAT;

_already_connected:
	TRACE("Error: already connected!\n");
	return B_MEDIA_ALREADY_CONNECTED;
}


void 
DVBMediaNode::Connect(status_t error, const media_source &source,
		const media_destination &destination, const media_format &format,
		char *io_name)
{
	/* The connection process:
	 *                BBufferProducer::FormatProposal
	 *                BBufferConsumer::AcceptFormat
	 *                BBufferProducer::PrepareToConnect
	 *                BBufferConsumer::Connected
	 * we are here => BBufferProducer::Connect
	 */

	TRACE("DVBMediaNode::Connect: %s\n", SourceDefaultName(source));

	if (error != B_OK) {
		TRACE("Error during connecting\n");
		// if an error occured, unreserve the connection
		ResetOutput(source);
		return;
	}

	#ifdef DEBUG
		TRACE("connected format: ");
		PrintFormat(format);
	#endif

	// Since the destination is allowed to be changed by the
	// consumer, the one we got in PrepareToConnect() is no
	// longer correct, and must be updated here.
	SetOutput(source, destination, format);

	// Set output as connected
	switch (source.id) {
		case ID_RAW_VIDEO:	fOutputEnabledRawVideo = true; break;
		case ID_RAW_AUDIO:	fOutputEnabledRawAudio = true; break;
		case ID_ENC_VIDEO:	fOutputEnabledEncVideo = true; break;
		case ID_ENC_AUDIO:	fOutputEnabledEncAudio = true; break;
		case ID_TS:			fOutputEnabledTS = true; break;
		default:			break;
	}

	// if the connection has no name, we set it now
	if (strlen(io_name) == 0)
		strcpy(io_name, SourceDefaultName(source));

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
DVBMediaNode::Disconnect(const media_source &source,
		const media_destination &destination)
{
	TRACE("DVBMediaNode::Disconnect: %s\n", SourceDefaultName(source));
	
	// unreserve the connection
	ResetOutput(source);

	// Set output to disconnected
	switch (source.id) {
		case ID_RAW_VIDEO:	fOutputEnabledRawVideo = false; break;
		case ID_RAW_AUDIO:	fOutputEnabledRawAudio = false; break;
		case ID_ENC_VIDEO:	fOutputEnabledEncVideo = false; break;
		case ID_ENC_AUDIO:	fOutputEnabledEncAudio = false; break;
		case ID_TS:			fOutputEnabledTS = false; break;
		default:			break;
	}
}


void 
DVBMediaNode::LateNoticeReceived(const media_source &source,
		bigtime_t how_much, bigtime_t performance_time)
{
	TRACE("DVBMediaNode::LateNoticeReceived %Ld late at %Ld\n", how_much, performance_time);
}


void 
DVBMediaNode::EnableOutput(const media_source &source, bool enabled,
		int32 *_deprecated_)
{
	TRACE("DVBMediaNode::EnableOutput id = %ld, enabled = %d\n", source.id, enabled);
	/* not used yet
	switch (source.id) {
		case ID_RAW_VIDEO:	fOutputEnabledRawVideo = enabled; break;
		case ID_RAW_AUDIO:	fOutputEnabledRawAudio = enabled; break;
		case ID_ENC_VIDEO:	fOutputEnabledEncVideo = enabled; break;
		case ID_ENC_AUDIO:	fOutputEnabledEncAudio = enabled; break;
		case ID_TS:			fOutputEnabledTS = enabled; break;
		default:			break;
	}
	*/
}


status_t 
DVBMediaNode::SetPlayRate(int32 numer, int32 denom)
{

	return B_ERROR;
}


void 
DVBMediaNode::AdditionalBufferRequested(const media_source &source,
		media_buffer_id prev_buffer, bigtime_t prev_time,
		const media_seek_tag *prev_tag)
{
	TRACE("DVBMediaNode::AdditionalBufferRequested: %s\n", SourceDefaultName(source));
}


void 
DVBMediaNode::LatencyChanged(const media_source &source,
		const media_destination &destination, bigtime_t new_latency,
		uint32 flags)
{
	TRACE("DVBMediaNode::LatencyChanged to %Ld\n", new_latency);
}

/* DVBMediaNode */


void
DVBMediaNode::HandleTimeWarp(bigtime_t performance_time)
{
	TRACE("DVBMediaNode::HandleTimeWarp at %Ld\n", performance_time);
}


void
DVBMediaNode::HandleSeek(bigtime_t performance_time)
{
	TRACE("DVBMediaNode::HandleSeek at %Ld\n", performance_time);
}


void
DVBMediaNode::InitDefaultFormats()
{
	// 720 * 576
	fDefaultFormatRawVideo.type = B_MEDIA_RAW_VIDEO;
	fDefaultFormatRawVideo.u.raw_video.display.format = B_RGB32;
	fDefaultFormatRawVideo.u.raw_video.display.line_width = 720;
	fDefaultFormatRawVideo.u.raw_video.display.line_count = 576;
	fDefaultFormatRawVideo.u.raw_video.last_active = fDefaultFormatRawVideo.u.raw_video.display.line_count - 1;
	fDefaultFormatRawVideo.u.raw_video.display.bytes_per_row = fDefaultFormatRawVideo.u.raw_video.display.line_width * 4;
	fDefaultFormatRawVideo.u.raw_video.field_rate = 0; // wildcard
	fDefaultFormatRawVideo.u.raw_video.interlace = 1;
	fDefaultFormatRawVideo.u.raw_video.first_active = 0;
	fDefaultFormatRawVideo.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
	fDefaultFormatRawVideo.u.raw_video.pixel_width_aspect = 1;
	fDefaultFormatRawVideo.u.raw_video.pixel_height_aspect = 1;
	fDefaultFormatRawVideo.u.raw_video.display.pixel_offset = 0;
	fDefaultFormatRawVideo.u.raw_video.display.line_offset = 0;
	fDefaultFormatRawVideo.u.raw_video.display.flags = 0;

	fDefaultFormatRawAudio.type = B_MEDIA_RAW_AUDIO;
	fDefaultFormatRawAudio.u.raw_audio.frame_rate = 48000;
	fDefaultFormatRawAudio.u.raw_audio.channel_count = 2;
//  XXX broken in Haiku...
//	fDefaultFormatRawAudio.u.raw_audio.format = 0; // wildcard
	fDefaultFormatRawAudio.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
//  when set to 0, haiku mixer has problems when diung a format change
//  set to short and debug the buffer_size problem first!
	fDefaultFormatRawAudio.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
//	fDefaultFormatRawAudio.u.raw_audio.buffer_size = 0; // wildcard
//	fDefaultFormatRawAudio.u.raw_audio.buffer_size = 0x1200;
//	fDefaultFormatRawAudio.u.raw_audio.buffer_size = 0x1000;
	fDefaultFormatRawAudio.u.raw_audio.buffer_size = 32768;
//	fDefaultFormatRawAudio.u.raw_audio.buffer_size = 333 * 8;
//	fDefaultFormatRawAudio.u.raw_audio.buffer_size = 512;
//  when set to anything different from 32768 haiku mixer has problems

	fDefaultFormatEncVideo.type = B_MEDIA_ENCODED_VIDEO;
	fDefaultFormatEncAudio.type = B_MEDIA_ENCODED_AUDIO;
	fDefaultFormatTS.type = B_MEDIA_MULTISTREAM;
}


bool
DVBMediaNode::VerifyFormatRawVideo(const media_raw_video_format &format)
{
	if (format.display.format != 0 &&
		format.display.format != B_RGB32 &&
		format.display.format != B_YCbCr422)
		return false;
		
	return true;
}


bool
DVBMediaNode::VerifyFormatRawAudio(const media_multi_audio_format &format)
{
	if (format.format != 0 && 
		format.format != media_raw_audio_format::B_AUDIO_FLOAT &&
		format.format != media_raw_audio_format::B_AUDIO_SHORT)
		return false;

	return true;
}


void
DVBMediaNode::SpecializeFormatRawVideo(media_raw_video_format *format)
{
	// Here we need to specialize *all* remaining wildcard
	// fields that the consumer didn't set
	if (format->field_rate == 0.0)
		format->field_rate = 25;
		
	// XXX a lot is missing here...
}


void
DVBMediaNode::SpecializeFormatRawAudio(media_multi_audio_format *format)
{
	// Here we need to specialize *all* remaining wildcard
	// fields that the consumer didn't set
	if (format->format == 0)
		format->format = media_raw_audio_format::B_AUDIO_SHORT;

	if (format->buffer_size == 0)
		format->buffer_size = 333 * 8;

	// XXX a lot is missing here...
}


void
DVBMediaNode::SpecializeFormatEncVideo(media_encoded_video_format *format)
{
	// Here we need to specialize *all* remaining wildcard
	// fields that the consumer didn't set
}


void
DVBMediaNode::SpecializeFormatEncAudio(media_encoded_audio_format *format)
{
	// Here we need to specialize *all* remaining wildcard
	// fields that the consumer didn't set
}


void
DVBMediaNode::SpecializeFormatTS(media_multistream_format *format)
{
	// Here we need to specialize *all* remaining wildcard
	// fields that the consumer didn't set
}


status_t
DVBMediaNode::SetOutput(const media_source &source, 
						const media_destination &destination, 
						const media_format &format)
{
	switch (source.id) {
		case ID_RAW_VIDEO:
			fOutputRawVideo.destination = destination;
			fOutputRawVideo.format = format;
			break;

		case ID_RAW_AUDIO:
			fOutputRawAudio.destination = destination;
			fOutputRawAudio.format = format;
			break;

		case ID_ENC_VIDEO:
			fOutputEncVideo.destination = destination;
			fOutputEncVideo.format = format;
			break;

		case ID_ENC_AUDIO:
			fOutputEncAudio.destination = destination;
			fOutputEncAudio.format = format;
			break;

		case ID_TS:
			fOutputTS.destination = destination;
			fOutputTS.format = format;
			break;

		default:
			return B_MEDIA_BAD_SOURCE;
	}
	return B_OK;
}


status_t
DVBMediaNode::ResetOutput(const media_source &source)
{
	switch (source.id) {
		case ID_RAW_VIDEO:
			fOutputRawVideo.destination = media_destination::null;
			fOutputRawVideo.format = fDefaultFormatRawVideo;
			break;

		case ID_RAW_AUDIO:
			fOutputRawAudio.destination = media_destination::null;
			fOutputRawAudio.format = fDefaultFormatRawAudio;
			break;
				
		case ID_ENC_VIDEO:
			fOutputEncVideo.destination = media_destination::null;
			fOutputEncVideo.format = fDefaultFormatEncVideo;
			break;
				
		case ID_ENC_AUDIO:
			fOutputEncAudio.destination = media_destination::null;
			fOutputEncAudio.format = fDefaultFormatEncAudio;
			break;
				
		case ID_TS:
			fOutputTS.destination = media_destination::null;
			fOutputTS.format = fDefaultFormatTS;
			break;
				
		default:
			return B_MEDIA_BAD_SOURCE;
	}
	return B_OK;
}


const char *
DVBMediaNode::SourceDefaultName(const media_source &source)
{
	switch (source.id) {
		case ID_RAW_VIDEO:	return "raw video";
		case ID_RAW_AUDIO:	return "raw audio";
		case ID_ENC_VIDEO:	return "encoded video";
		case ID_ENC_AUDIO:	return "encoded audio";
		case ID_TS:			return "MPEG2 TS";
		default:			return NULL;
	}
}


void
DVBMediaNode::HandleStart(bigtime_t performance_time)
{
	TRACE("DVBMediaNode::HandleStart\n");
	StartCapture();
}


void
DVBMediaNode::HandleStop(void)
{
	TRACE("DVBMediaNode::HandleStop\n");
	StopCapture();
}	


status_t
DVBMediaNode::Tune()
{
	TRACE("DVBMediaNode::Tune enter\n");

	TRACE("state %d, region %d, channel %d, audio %d\n",
		fSelectedState, fSelectedRegion, fSelectedChannel, fSelectedAudio);

	status_t err;
	bool needs_tuning = false;

	if (fSelectedChannel < 0 || fSelectedAudio < 0) {
		printf("DVBMediaNode::Tune: invalid tuning info\n");
		StopCapture();
		err = B_ERROR;
		goto end;
//		return B_ERROR;
	}
	
	const char *desc;
	
	dvb_tuning_parameters_t new_params;
	int new_video_pid;
	int new_audio_pid;
	int new_pcr_pid;

	desc = fChannelList->ItemAt(fSelectedChannel);
	err = ExtractTuningParams(desc, fSelectedAudio, &new_params, &new_video_pid, &new_audio_pid, &new_pcr_pid);

	if (err != B_OK) {
		printf("DVBMediaNode::Tune: getting tuning info failed\n");
		StopCapture();
		err = B_ERROR;
		goto end;
//		return B_ERROR;
	}
/*	
	if (fTuningParam.frequency == new_params.frequency) {
		printf("DVBMediaNode::Tune: frequency not changed\n");
		fVideoPid = new_video_pid;
		fAudioPid = new_audio_pid;
		fPcrPid = new_pcr_pid;
		fDemux->SetPIDs(fVideoPid, fAudioPid, fPcrPid);
		return B_OK;
	}
*/	
	switch (fInterfaceType) {
		case DVB_TYPE_DVB_T:
			needs_tuning = (fTuningParam.u.dvb_t.frequency != new_params.u.dvb_t.frequency) || !fTuningSuccess;
			needs_tuning = true;
			break;
		case DVB_TYPE_DVB_S:
			printf("needs_tuning = %d, forcing tuning for DVB-S\n", needs_tuning);
			needs_tuning = true;
			break;
		default:
			needs_tuning = true;
			break;
	}
		
	fTuningParam = new_params;
	fVideoPid = new_video_pid;
	fAudioPid = new_audio_pid;
	fPcrPid = new_pcr_pid;
	
	if (needs_tuning) {
printf("about to stop capture 1\n");
#if STOP_CAPTURE_WHILE_TUNING
printf("about to stop capture 2\n");
		err = StopCapture();
		if (err) {
			printf("Tune: StopCapture failed\n");
			goto end;
		}
#endif
	} else {
#if STOP_CAPTURE_WHILE_TUNING
		StopCaptureThreads();
#endif
	}
	
	if (needs_tuning) {
		err = fCard->SetTuningParameter(fTuningParam);
		fTuningSuccess = err == B_OK;
	}

	fDemux->SetPIDs(fVideoPid, fAudioPid, fPcrPid);
	
	if (needs_tuning) {
		if (fTuningSuccess) {
			fCard->GetTuningParameter(&fTuningParam);
			err = StartCapture();
		}
	} else {
#if STOP_CAPTURE_WHILE_TUNING
		StartCaptureThreads();
#endif
	}
	
end:
	EventQueue()->AddEvent(media_timed_event(0, M_REFRESH_PARAMETER_WEB));
//	RefreshParameterWeb();

	TRACE("DVBMediaNode::Tune finished\n");
	return err;
}


status_t
DVBMediaNode::StartCapture()
{
	TRACE("DVBMediaNode::StartCapture\n");

	if (fCaptureActive)
		return B_OK;
		
	RETURN_IF_ERROR(StopCaptureThreads());
	
	if (!fTuningSuccess) {
		RETURN_IF_ERROR(Tune());
	}
	
	RETURN_IF_ERROR(StartCaptureThreads());
	
	fCard->CaptureStart();

	fCaptureActive = true;

	RefreshParameterWeb();

	return B_OK;	
}


status_t
DVBMediaNode::StopCapture()
{
	TRACE("DVBMediaNode::StopCapture\n");
	if (!fCaptureActive)
		return B_OK;

	StopCaptureThreads();
	
	fCard->CaptureStop();

	fCaptureActive = false;
	return B_OK;
}


status_t
DVBMediaNode::StartCaptureThreads()
{
	TRACE("DVBMediaNode::StartCaptureThreads\n");

	if (fCaptureThreadsActive)
		return B_OK;

	fTerminateThreads = false;

	fThreadIdCardReader = spawn_thread(_card_reader_thread_, "DVB card reader", 120, this);
	fThreadIdMpegDemux = spawn_thread(_mpeg_demux_thread_, "DVB MPEG demux", 110, this);
	fThreadIdEncAudio = spawn_thread(_enc_audio_thread_, "DVB audio streaming", 110, this);
	fThreadIdEncVideo = spawn_thread(_enc_video_thread_, "DVB video streaming", 110, this);
	fThreadIdMpegTS = spawn_thread(_mpeg_ts_thread_, "DVB MPEG TS streaming", 110, this);
	fThreadIdRawAudio = spawn_thread(_raw_audio_thread_, "DVB audio decode", 100, this);
	fThreadIdRawVideo = spawn_thread(_raw_video_thread_, "DVB video decode", 85, this);
	resume_thread(fThreadIdCardReader);
	resume_thread(fThreadIdMpegDemux);
	resume_thread(fThreadIdEncAudio);
	resume_thread(fThreadIdEncVideo);
	resume_thread(fThreadIdMpegTS);
	resume_thread(fThreadIdRawAudio);
	resume_thread(fThreadIdRawVideo);
	
	fCaptureThreadsActive = true;
	return B_OK;
}


status_t
DVBMediaNode::StopCaptureThreads()
{
	TRACE("DVBMediaNode::StopCaptureThreads\n");
	
	if (!fCaptureThreadsActive)
		return B_OK;
	
	fTerminateThreads = true;

	fCardDataQueue->Terminate();
	fEncVideoQueue->Terminate();
	fEncAudioQueue->Terminate();
	fMpegTsQueue->Terminate();
	fRawVideoQueue->Terminate();
	fRawAudioQueue->Terminate();
	
	status_t dummy; // NULL as parameter does not work
	wait_for_thread(fThreadIdCardReader, &dummy);
	wait_for_thread(fThreadIdMpegDemux, &dummy);
	wait_for_thread(fThreadIdEncAudio, &dummy);
	wait_for_thread(fThreadIdEncVideo, &dummy);
	wait_for_thread(fThreadIdMpegTS, &dummy);
	wait_for_thread(fThreadIdRawAudio, &dummy);
	wait_for_thread(fThreadIdRawVideo, &dummy);

	fCardDataQueue->Restart();
	fEncVideoQueue->Restart();
	fEncAudioQueue->Restart();
	fMpegTsQueue->Restart();
	fRawVideoQueue->Restart();
	fRawAudioQueue->Restart();
	
	fCaptureThreadsActive = false;
	return B_OK;
}


int32
DVBMediaNode::_card_reader_thread_(void *arg)
{
	static_cast<DVBMediaNode *>(arg)->card_reader_thread();
	return 0;
}


int32
DVBMediaNode::_mpeg_demux_thread_(void *arg)
{
	static_cast<DVBMediaNode *>(arg)->mpeg_demux_thread();
	return 0;
}


int32
DVBMediaNode::_raw_audio_thread_(void *arg)
{
	static_cast<DVBMediaNode *>(arg)->raw_audio_thread();
	return 0;
}


int32
DVBMediaNode::_raw_video_thread_(void *arg)
{
	static_cast<DVBMediaNode *>(arg)->raw_video_thread();
	return 0;
}


int32
DVBMediaNode::_enc_audio_thread_(void *arg)
{
	static_cast<DVBMediaNode *>(arg)->enc_audio_thread();
	return 0;
}


int32
DVBMediaNode::_enc_video_thread_(void *arg)
{
	static_cast<DVBMediaNode *>(arg)->enc_video_thread();
	return 0;
}


int32
DVBMediaNode::_mpeg_ts_thread_(void *arg)
{
	static_cast<DVBMediaNode *>(arg)->mpeg_ts_thread();
	return 0;
}


void
DVBMediaNode::card_reader_thread()
{
	while (!fTerminateThreads) {
		void *data;
		size_t size;
		status_t err;
		bigtime_t end_time;
		err = fCard->Capture(&data, &size, &end_time);
		if (err != B_OK) {
			TRACE("fCard->Capture failed, error %lx (%s)\n", err, strerror(err));
			continue;
		}

//		TRACE("captured %ld bytes\n", size);

		Packet *packet = new Packet(data, size, end_time);
		
		err = fCardDataQueue->Insert(packet);
		if (err != B_OK) {
			delete packet;
			TRACE("fCardDataQueue->Insert failed, error %lx\n", err);
			continue;
		}
	}
}


void
DVBMediaNode::mpeg_demux_thread()
{
	while (!fTerminateThreads) {
		status_t err;
		Packet *packet;
		err = fCardDataQueue->Remove(&packet);
		if (err != B_OK) {
			TRACE("fCardDataQueue->Remove failed, error %lx\n", err);
			continue;
		}
		
		// packet->TimeStamp() is the end time of the capture
		fDemux->AddData(packet);
	}
}


void
DVBMediaNode::mpeg_ts_thread()
{
	while (!fTerminateThreads) {
		status_t err;
		Packet *packet;
		err = fMpegTsQueue->Remove(&packet);
		if (err != B_OK) {
			TRACE("fMpegTsQueue->Remove failed, error %lx\n", err);
			continue;
		}

//		TRACE("mpeg ts   packet, size %6ld, start_time %14Ld\n", packet->Size(), packet->TimeStamp());

		delete packet;
	}
}


void
DVBMediaNode::enc_audio_thread()
{
	while (!fTerminateThreads) {
		status_t err;
		Packet *packet;
		err = fEncAudioQueue->Remove(&packet);
		if (err != B_OK) {
			TRACE("fEncAudioQueue->Remove failed, error %lx\n", err);
			continue;
		}
//		TRACE("enc audio packet, size %6ld, start_time %14Ld\n", packet->Size(), packet->TimeStamp());

#ifdef DUMP_AUDIO
		const uint8 *data;
		size_t size;
		if (B_OK != pes_extract(packet->Data(), packet->Size(), &data, &size)) {
			TRACE("audio pes_extract failed\n");
			delete packet;
			return;
		}
		lock.Lock();
		write(fAudioFile, data, size);
		lock.Unlock();
#endif

		if (!fOutputEnabledEncAudio) {
			delete packet;
			continue;
		}

		// send encoded audio buffer


		delete packet;
	}
}


void
DVBMediaNode::enc_video_thread()
{
	while (!fTerminateThreads) {
		status_t err;
		Packet *packet;
		err = fEncVideoQueue->Remove(&packet);
		if (err != B_OK) {
			TRACE("fEncVideoQueue->Remove failed, error %lx\n", err);
			continue;
		}

//		TRACE("enc video packet, size %6ld, start_time %14Ld\n", packet->Size(), packet->TimeStamp());


#ifdef DUMP_VIDEO
		const uint8 *data;
		size_t size;
		if (B_OK != pes_extract(packet->Data(), packet->Size(), &data, &size)) {
			TRACE("video pes_extract failed\n");
			delete packet;
			return;
		}
		lock.Lock();
		write(fVideoFile, data, size);
		lock.Unlock();
#endif

		if (!fOutputEnabledEncVideo) {
			delete packet;
			continue;
		}

		// send encoded video buffer

		delete packet;
	}
}


void
DVBMediaNode::raw_audio_thread()
{
	media_format format;
	status_t err;
	err = GetStreamFormat(fRawAudioQueue, &format);
	if (err) {
		printf("fAudioDecoder init error %s\n", strerror(err));
		return;
	}
	
	// create decoder interface

	fAudioDecoder = new MediaStreamDecoder(&_GetNextAudioChunk, this);
	
	err = fAudioDecoder->SetInputFormat(format);
	if (err) {
		printf("fAudioDecoder SetInputFormat error %s\n", strerror(err));
		return;
	}
	
	TRACE("requested audio decoder format: ");
	PrintFormat(fOutputRawAudio);
	
	media_format fmt = fOutputRawAudio.format;
	err = fAudioDecoder->SetOutputFormat(&fmt);
	if (err) {
		printf("fAudioDecoder SetOutputFormat error %s\n", strerror(err));
		return;
	}

	TRACE("final audio decoder format: ");
	PrintFormat(fmt);
		
	// change format of connection
	if (format_is_compatible(fmt, fOutputRawAudio.format)) {
		printf("audio formats are compatible\n");
		fOutputRawAudio.format = fmt;
	} else {
		printf("audio formats NOT compatible\n");
		lock.Lock();
		err = ChangeFormat(fOutputRawAudio.source,
						   fOutputRawAudio.destination,
						   &fmt);
		lock.Unlock();
		printf("format change result %lx (%s)\n", err, strerror(err));
		PrintFormat(fmt);
		fOutputRawAudio.format = fmt;
		if (err)
			return;
	}
	
	// decode data and send buffers
	
	delete fBufferGroupRawAudio;
	fBufferGroupRawAudio = new BBufferGroup(fOutputRawAudio.format.u.raw_audio.buffer_size * 3, 25);

	while (!fTerminateThreads) {
		int64 frameCount;
		media_header mh;
		media_header_ex *mhe = (media_header_ex *)&mh;
		
		if (!fOutputEnabledRawAudio) {
			fRawAudioQueue->Flush(40000);
			continue;
		}

		BBuffer* buf;
		buf = fBufferGroupRawAudio->RequestBuffer(fOutputRawAudio.format.u.raw_audio.buffer_size, AUDIO_BUFFER_REQUEST_TIMEOUT);
		if (!buf) {
			TRACE("audio: request buffer timout\n");
			continue;
		}

		err = fAudioDecoder->Decode(buf->Data(), &frameCount, &mh, NULL);
		if (err) {
			buf->Recycle();
			printf("fAudioDecoder Decode error %s\n", strerror(err));
			continue;
		}

#ifdef DUMP_RAW_AUDIO
		lock.Lock();
		write(fRawAudioFile, buf->Data(), mhe->size_used);
		lock.Unlock();
#endif

		if (	fOutputRawAudio.format.u.raw_audio.buffer_size     != mhe->size_used
		 	||	int(fOutputRawAudio.format.u.raw_audio.frame_rate) != mhe->u.raw_audio.frame_rate
			||	fOutputRawAudio.format.u.raw_audio.channel_count   != mhe->u.raw_audio.channel_count
		 ) {
			TRACE("audio: decode format change: changed buffer_size from %ld to %ld\n", fOutputRawAudio.format.u.raw_audio.buffer_size, mhe->size_used);
			TRACE("audio: decode format change: changed channel_count from %ld to %ld\n", fOutputRawAudio.format.u.raw_audio.channel_count, mhe->u.raw_audio.channel_count);
			TRACE("audio: decode format change: changed frame_rate from %.0f to %.0f\n", fOutputRawAudio.format.u.raw_audio.frame_rate, mhe->u.raw_audio.frame_rate);
			fOutputRawAudio.format.u.raw_audio.buffer_size   = mhe->size_used; 
			fOutputRawAudio.format.u.raw_audio.frame_rate    = mhe->u.raw_audio.frame_rate;
			fOutputRawAudio.format.u.raw_audio.channel_count = mhe->u.raw_audio.channel_count;
			lock.Lock();
			err = ChangeFormat(fOutputRawAudio.source,
							   fOutputRawAudio.destination,
							   &fOutputRawAudio.format);
			lock.Unlock();
			printf("format change result %lx (%s)\n", err, strerror(err));
			PrintFormat(fOutputRawAudio.format);
			if (err) {
				buf->Recycle();
				return; // we are dead!
			}
		}
	
		bigtime_t ts_perf_time;
		bigtime_t ts_sys_time;
		bigtime_t ts_offset;
		bigtime_t aud_time;
		bigtime_t start_time;

		// calculate start time of audio data

		fDemux->TimesourceInfo(&ts_perf_time, &ts_sys_time);
		ts_offset = ts_sys_time - ts_perf_time;
		aud_time = mhe->start_time;	// measured in PCR time base
		start_time = TimeSource()->PerformanceTimeFor(aud_time + ts_offset);

		// calculate delay and wait

		bigtime_t delay;
		delay = start_time - TimeSource()->Now();
		TRACE_TIMING("audio delay is %Ld\n", delay);
		if (delay < -AUDIO_MAX_LATE) {
			printf("audio: decoded packet is %Ldms too late, dropped\n", -delay / 1000);
			buf->Recycle();
			continue;
		}
		if (delay < 0) {
//			printf("audio: decoded packet is %Ldms too late\n", -delay / 1000);
		}
		if (delay > AUDIO_MAX_EARLY) {
			printf("audio: decoded packet is %Ldms too early, dropped\n", delay / 1000);
			buf->Recycle();
			continue;
		}
		if (delay > 0) {
//			printf("audio: decoded packet is %Ldms too early\n", delay / 1000);
		}
		delay -= PROCESSING_LATENCY;
		if (delay > 0) {
			if (acquire_sem_etc(fAudioDelaySem, 1, B_RELATIVE_TIMEOUT, delay) != B_TIMED_OUT) {
				printf("audio: delay sem not timed out, dropped packet\n");
				buf->Recycle();
				continue;
			}
		}

		TRACE_TIMING("audio playback delay %Ld\n", start_time - TimeSource()->Now());

		media_header* hdr;
		hdr = buf->Header();
//		*hdr = mh;								// copy header from decoded frame
		hdr->type = B_MEDIA_RAW_AUDIO;
		hdr->size_used = mhe->size_used;
		hdr->time_source = TimeSource()->ID();	// set time source id
		hdr->start_time = start_time;			// set start time
		lock.Lock();
		if (SendBuffer(buf, fOutputRawAudio.source, fOutputRawAudio.destination)
				!= B_OK) {
			TRACE("audio: sending buffer failed\n");
			buf->Recycle();
		} 
		lock.Unlock();
	}

	delete fCurrentAudioPacket;
	fCurrentAudioPacket = 0;
}


void
DVBMediaNode::raw_video_thread()
{
	media_format format;
	status_t err;
	err = GetStreamFormat(fRawVideoQueue, &format);
	if (err) {
		printf("fVideoDecoder init error %s\n", strerror(err));
		return;
	}
	
	// create decoder interface

	fVideoDecoder = new MediaStreamDecoder(&_GetNextVideoChunk, this);
	
	err = fVideoDecoder->SetInputFormat(format);
	if (err) {
		printf("fVideoDecoder SetInputFormat error %s\n", strerror(err));
		return;
	}
	
	TRACE("requested video decoder format: ");
	PrintFormat(fOutputRawVideo);
	
	media_format fmt = fOutputRawVideo.format;
	err = fVideoDecoder->SetOutputFormat(&fmt);
	if (err) {
		printf("fVideoDecoder SetOutputFormat error %s\n", strerror(err));
		return;
	}

	TRACE("final video decoder format: ");
	PrintFormat(fmt);
		
	// change format of connection
	if (format_is_compatible(fmt, fOutputRawVideo.format)) {
		printf("video formats are compatible\n");
		fOutputRawVideo.format = fmt;
	} else {
		printf("video formats NOT compatible\n");
		lock.Lock();
		err = ChangeFormat(fOutputRawVideo.source,
						   fOutputRawVideo.destination,
						   &fmt);
		lock.Unlock();
		printf("format change result %lx (%s)\n", err, strerror(err));
		PrintFormat(fmt);
		fOutputRawVideo.format = fmt;
		if (err) {
			printf("video format change failed\n");
			return;
		}
	}
	
	// decode data and send buffers

	uint32 video_buffer_size_max = 720 * 576 * 4;
	uint32 video_buffer_size
		= fOutputRawVideo.format.u.raw_video.display.line_count
			* fOutputRawVideo.format.u.raw_video.display.bytes_per_row;
	
	delete fBufferGroupRawVideo;
	fBufferGroupRawVideo = new BBufferGroup(video_buffer_size_max, 4);

	while (!fTerminateThreads) {
		int64 frameCount;
		media_header mh;
		media_header_ex *mhe = (media_header_ex *)&mh;

		if (!fOutputEnabledRawVideo) {
			fRawVideoQueue->Flush(40000);
			continue;
		}

		// fetch a new buffer (always of maximum size, as the stream may change)

		BBuffer* buf;
		buf = fBufferGroupRawVideo->RequestBuffer(video_buffer_size_max,
			VIDEO_BUFFER_REQUEST_TIMEOUT);
		if (!buf) {
			TRACE("video: request buffer timout\n");
			continue;
		}
		
		// decode one video frame into buffer

		err = fVideoDecoder->Decode(buf->Data(), &frameCount, &mh, NULL);
		if (err) {
			buf->Recycle();
			printf("fVideoDecoder Decode error %s\n", strerror(err));
			continue;
		}

		// check if the format of the stream has changed
		
		if (	mhe->u.raw_video.display_line_width  != fOutputRawVideo.format.u.raw_video.display.line_width
			||	mhe->u.raw_video.display_line_count  != fOutputRawVideo.format.u.raw_video.display.line_count
			||	mhe->u.raw_video.bytes_per_row       != fOutputRawVideo.format.u.raw_video.display.bytes_per_row
			||	mhe->u.raw_video.pixel_width_aspect  != fOutputRawVideo.format.u.raw_video.pixel_width_aspect
			||	mhe->u.raw_video.pixel_height_aspect != fOutputRawVideo.format.u.raw_video.pixel_height_aspect
			||  mhe->size_used						 != video_buffer_size)
		{
			printf("video format changed:\n");
			printf(" line_width %ld => %ld\n", fOutputRawVideo.format.u.raw_video.display.line_width, mhe->u.raw_video.display_line_width);
			printf(" line_count %ld => %ld\n", fOutputRawVideo.format.u.raw_video.display.line_count, mhe->u.raw_video.display_line_count);
			printf(" bytes_per_row %ld => %ld\n", fOutputRawVideo.format.u.raw_video.display.bytes_per_row, mhe->u.raw_video.bytes_per_row);
			printf(" pixel_width_aspect %d => %d\n", fOutputRawVideo.format.u.raw_video.pixel_width_aspect, mhe->u.raw_video.pixel_width_aspect);
			printf(" pixel_height_aspect %d => %d\n", fOutputRawVideo.format.u.raw_video.pixel_width_aspect, mhe->u.raw_video.pixel_height_aspect);
			printf(" pixel_height_aspect %d => %d\n", fOutputRawVideo.format.u.raw_video.pixel_width_aspect, mhe->u.raw_video.pixel_height_aspect);
			printf(" video_buffer_size %ld => %ld\n", video_buffer_size, mhe->size_used);

			// recalculate video buffer size
//			video_buffer_size = mhe->size_used;
			video_buffer_size = fOutputRawVideo.format.u.raw_video.display.line_count * fOutputRawVideo.format.u.raw_video.display.bytes_per_row;

			// perform a video format change
			
			fOutputRawVideo.format.u.raw_video.display.line_width    = mhe->u.raw_video.display_line_width;
			fOutputRawVideo.format.u.raw_video.display.line_count    = mhe->u.raw_video.display_line_count;
			fOutputRawVideo.format.u.raw_video.display.bytes_per_row = mhe->u.raw_video.bytes_per_row;
			fOutputRawVideo.format.u.raw_video.pixel_width_aspect    = mhe->u.raw_video.pixel_width_aspect;
			fOutputRawVideo.format.u.raw_video.pixel_width_aspect    = mhe->u.raw_video.pixel_height_aspect;
			fOutputRawVideo.format.u.raw_video.last_active           = mhe->u.raw_video.display_line_count - 1;
			lock.Lock();
			err = ChangeFormat(fOutputRawVideo.source,
							   fOutputRawVideo.destination,
							   &fOutputRawVideo.format);
			lock.Unlock();
			printf("format change result %lx (%s)\n", err, strerror(err));
			PrintFormat(fOutputRawVideo.format);
			if (err) {
				buf->Recycle();
				printf("video format change failed\n");
				return; // we are dead
			}
		}
		
		// calculate start time for video

		bigtime_t ts_perf_time;
		bigtime_t ts_sys_time;
		bigtime_t ts_offset;
		bigtime_t pic_time;
		bigtime_t start_time;

		fDemux->TimesourceInfo(&ts_perf_time, &ts_sys_time);
		ts_offset = ts_sys_time - ts_perf_time;
		pic_time = mhe->start_time;	// measured in PCR time base
		start_time = TimeSource()->PerformanceTimeFor(pic_time + ts_offset);

		// calculate delay and wait

		bigtime_t delay;
		delay = start_time - TimeSource()->Now();
		TRACE_TIMING("video delay %Ld\n", delay);
		if (delay < -VIDEO_MAX_LATE) {
			printf("video: decoded packet is %Ldms too late, dropped\n", -delay / 1000);
			buf->Recycle();
			continue;
		}
		if (delay > VIDEO_MAX_EARLY) {
			printf("video: decoded packet is %Ldms too early, dropped\n", delay / 1000);
			buf->Recycle();
			continue;
		}
		delay -= PROCESSING_LATENCY;
		if (delay > 0) {
			if (acquire_sem_etc(fVideoDelaySem, 1, B_RELATIVE_TIMEOUT, delay) != B_TIMED_OUT) {
				printf("video: delay sem not timed out, dropped packet\n");
				buf->Recycle();
				continue;
			}
		}

		TRACE_TIMING("video playback delay %Ld\n", start_time - TimeSource()->Now());

		media_header* hdr;
		hdr = buf->Header();
//		*hdr = mh;								// copy header from decoded frame
		hdr->type = B_MEDIA_RAW_VIDEO;
		hdr->size_used = video_buffer_size;
		hdr->time_source = TimeSource()->ID();	// set time source id
		hdr->start_time = start_time;			// set start time
		lock.Lock();
		if (SendBuffer(buf, fOutputRawVideo.source, fOutputRawVideo.destination)
				!= B_OK) {
			TRACE("video: sending buffer failed\n");
			buf->Recycle();
		} 
		lock.Unlock();
	
	}

	delete fCurrentVideoPacket;
	fCurrentVideoPacket = 0;
}


inline status_t
DVBMediaNode::GetNextVideoChunk(const void **chunkData, size_t *chunkLen, media_header *mh)
{
//	TRACE("DVBMediaNode::GetNextVideoChunk\n");

	delete fCurrentVideoPacket;
	
	status_t err;
	err = fRawVideoQueue->Remove(&fCurrentVideoPacket);
	if (err != B_OK) {
		TRACE("fRawVideoQueue->Remove failed, error %lx\n", err);
		fCurrentVideoPacket = 0;
		return B_ERROR;
	}

	const uint8 *data;
	size_t size;
			
	if (B_OK != pes_extract(fCurrentVideoPacket->Data(), fCurrentVideoPacket->Size(), &data, &size)) {
		TRACE("video pes_extract failed\n");
		return B_ERROR;
	}
	
	*chunkData = data;
	*chunkLen = size;
	// measured in PCR time base
	mh->start_time = fCurrentVideoPacket->TimeStamp();

	return B_OK;
}


inline status_t
DVBMediaNode::GetNextAudioChunk(const void **chunkData, size_t *chunkLen, media_header *mh)
{
//	TRACE("DVBMediaNode::GetNextAudioChunk\n");

	delete fCurrentAudioPacket;
	
	status_t err;
	err = fRawAudioQueue->Remove(&fCurrentAudioPacket);
	if (err != B_OK) {
		TRACE("fRawAudioQueue->Remove failed, error %lx\n", err);
		fCurrentAudioPacket = 0;
		return B_ERROR;
	}

	const uint8 *data;
	size_t size;
			
	if (B_OK != pes_extract(fCurrentAudioPacket->Data(), fCurrentAudioPacket->Size(), &data, &size)) {
		TRACE("audio pes_extract failed\n");
		return B_ERROR;
	}
	
	*chunkData = data;
	*chunkLen = size;
	// measured in PCR time base
	mh->start_time = fCurrentAudioPacket->TimeStamp();

//	printf("GetNextAudioChunk: done start_time %Ld\n", mh->start_time);

	return B_OK;
}


status_t
DVBMediaNode::_GetNextVideoChunk(const void **chunkData, size_t *chunkLen, media_header *mh, void *cookie)
{
	return static_cast<DVBMediaNode *>(cookie)->GetNextVideoChunk(chunkData, chunkLen, mh);
}


status_t
DVBMediaNode::_GetNextAudioChunk(const void **chunkData, size_t *chunkLen, media_header *mh, void *cookie)
{
	return static_cast<DVBMediaNode *>(cookie)->GetNextAudioChunk(chunkData, chunkLen, mh);
}


status_t
DVBMediaNode::GetStreamFormat(PacketQueue *queue, media_format *format)
{
	status_t status;
	Packet *packet;
	const uint8 *data;
	size_t size;
	int stream_id;
	
	// get copy of the first packet from queue, and determine format

	if ((status = queue->Peek(&packet)) != B_OK) {
		TRACE("queue->Peek failed, error %lx\n", status);
		return status;
	}

	if ((status = pes_extract(packet->Data(), packet->Size(), &data, &size)) != B_OK) {
		TRACE("pes_extract failed\n");
		goto done;
	}

	if ((status = pes_stream_id(packet->Data(), packet->Size(), &stream_id)) != B_OK) {
		TRACE("pes_stream_id failed\n");
		goto done;
	}

	if ((status = GetHeaderFormat(format, data, size, stream_id)) != B_OK) {
		TRACE("GetHeaderFormat failed, error %lx\n", status);
		goto done;
	}

done:
	delete packet;
	return status;
}


enum {
	ID_STATE	= 11,
	ID_REGION	= 12,
	ID_CHANNEL	= 13,
	ID_AUDIO	= 14,
};


void
DVBMediaNode::RefreshParameterWeb()
{
	TRACE("DVBMediaNode::RefreshParameterWeb enter\n");
	fWeb = CreateParameterWeb();
	SetParameterWeb(fWeb);
	TRACE("DVBMediaNode::RefreshParameterWeb finished\n");
}


void
DVBMediaNode::SetAboutInfo(BParameterGroup *about)
{
	about->MakeNullParameter(0, B_MEDIA_NO_TYPE, "DVB media_addon info:", B_GENERIC);
	about->MakeNullParameter(0, B_MEDIA_NO_TYPE, "Version " VERSION, B_GENERIC);
	about->MakeNullParameter(0, B_MEDIA_NO_TYPE, "Revision " REVISION, B_GENERIC);
	about->MakeNullParameter(0, B_MEDIA_NO_TYPE, "Build " BUILD, B_GENERIC);

	about->MakeNullParameter(0, B_MEDIA_NO_TYPE, "", B_GENERIC);
	about->MakeNullParameter(0, B_MEDIA_NO_TYPE, "Driver info:", B_GENERIC);

	dvb_type_t type;
	char name[200];
	char info[200];
	
	fCard->GetCardType(&type);
	fCard->GetCardInfo(name, sizeof(name), info, sizeof(info));

	about->MakeNullParameter(0, B_MEDIA_NO_TYPE, name, B_GENERIC);
	about->MakeNullParameter(0, B_MEDIA_NO_TYPE, info, B_GENERIC);
}


BParameterWeb *
DVBMediaNode::CreateParameterWeb()
{
	/* Set up the parameter web */
	BParameterWeb *web = new BParameterWeb();

	char n[200], i[200];
	fCard->GetCardInfo(n, sizeof(n), i, sizeof(i));
	
	BString name;
	name << Name() << " - " << i;
	
	BParameterGroup *main = web->MakeGroup(name.String());
	
	BParameterGroup *ctrl = main->MakeGroup("Channel Selection");
	ctrl->MakeNullParameter(0, B_MEDIA_NO_TYPE, ctrl->Name(), B_GENERIC);

	BParameterGroup *pref = main->MakeGroup("Preferences");
	pref->MakeNullParameter(0, B_MEDIA_NO_TYPE, pref->Name(), B_GENERIC);

	BDiscreteParameter *state = pref->MakeDiscreteParameter(
			ID_STATE, B_MEDIA_RAW_VIDEO, "State", B_GENERIC);

	BDiscreteParameter *region = pref->MakeDiscreteParameter(
			ID_REGION, B_MEDIA_RAW_VIDEO, "Region", B_GENERIC);

	BDiscreteParameter *chan = ctrl->MakeDiscreteParameter(
			ID_CHANNEL, B_MEDIA_RAW_VIDEO, "Channel", /* B_TUNER_CHANNEL */ B_GENERIC);

	BDiscreteParameter *aud = ctrl->MakeDiscreteParameter(
			ID_AUDIO, B_MEDIA_RAW_VIDEO, "Audio", B_GENERIC);

	AddStateItems(state);
	AddRegionItems(region);
	AddChannelItems(chan);
	AddAudioItems(aud);


	if (!fTuningSuccess || !fCaptureActive) {
		BParameterGroup *info = main->MakeGroup("Info");
		info->MakeNullParameter(0, B_MEDIA_NO_TYPE, info->Name(), B_GENERIC);
		BParameterGroup *about = main->MakeGroup("About");
		about->MakeNullParameter(0, B_MEDIA_NO_TYPE, about->Name(), B_GENERIC);
		SetAboutInfo(about);
//		info->MakeNullParameter(0, B_MEDIA_NO_TYPE, fCaptureActive ? "Tuning failed" : "Node stopped", B_GENERIC);
		info->MakeNullParameter(0, B_MEDIA_NO_TYPE, "Node is stopped", B_GENERIC);
		info->MakeNullParameter(0, B_MEDIA_NO_TYPE, "or tuning failed.", B_GENERIC);
		return web;
	}

	BParameterGroup *info1 = main->MakeGroup("Info");
	info1->MakeNullParameter(0, B_MEDIA_NO_TYPE, info1->Name(), B_GENERIC);
	BParameterGroup *info2 = main->MakeGroup("Info");
	info2->MakeNullParameter(0, B_MEDIA_NO_TYPE, info2->Name(), B_GENERIC);
	BParameterGroup *about = main->MakeGroup("About");
	about->MakeNullParameter(0, B_MEDIA_NO_TYPE, about->Name(), B_GENERIC);
	SetAboutInfo(about);
	
	BString sInterfaceType = "Interface Type: ";
	BString sFrequency = "Frequency: ";
	BString sAudioPid = "Audio PID: ";
	BString sVideoPid = "Video PID: ";
	BString sPcrPid = "PCR PID: ";
	BString sInversion = "Inversion: ";
	BString sBandwidth = "Bandwith: ";
	BString sModulation = "Modulation: ";
	BString sHierarchy = "Hierarchy: ";
	BString sCodeRateHP = "Code Rate HP: ";
	BString sCodeRateLP = "Code Rate LP: ";
	BString sTransmissionMode = "Transmission Mode: ";
	BString sGuardInterval = "Guard Interval: ";
	
	BString sSymbolRate = "Symbol Rate: ";
	BString sPolarity = "Polarity: ";
	BString sAgcInversion = "AGC Inversion: ";

	switch (fInterfaceType) {
		case DVB_TYPE_DVB_C: sInterfaceType << "DVB-C"; break;
		case DVB_TYPE_DVB_H: sInterfaceType << "DVB-H"; break;
		case DVB_TYPE_DVB_S: sInterfaceType << "DVB-S"; break;
		case DVB_TYPE_DVB_T: sInterfaceType << "DVB-T"; break;
		default: sInterfaceType << "unknown"; break;
	}

	sAudioPid << fAudioPid;
	sVideoPid << fVideoPid;
	sPcrPid << fPcrPid;
	
	if (fInterfaceType == DVB_TYPE_DVB_T) {

		sFrequency << fTuningParam.u.dvb_t.frequency / 1000000 << " MHz";

		switch (fTuningParam.u.dvb_t.inversion) {
			case DVB_INVERSION_AUTO: sInversion << "auto"; break;
			case DVB_INVERSION_ON: sInversion << "on"; break;
			case DVB_INVERSION_OFF: sInversion << "off"; break;
			default: sInversion << "unknown"; break;
		}

		switch (fTuningParam.u.dvb_t.bandwidth) {
			case DVB_BANDWIDTH_AUTO: sBandwidth << "auto"; break;
			case DVB_BANDWIDTH_6_MHZ: sBandwidth << "6 MHz"; break;
			case DVB_BANDWIDTH_7_MHZ: sBandwidth << "7 MHz"; break;
			case DVB_BANDWIDTH_8_MHZ: sBandwidth << "8 MHz"; break;
			default: sBandwidth << "unknown"; break;
		}

		switch (fTuningParam.u.dvb_t.modulation) {
			case DVB_MODULATION_AUTO: sModulation << "auto"; break;
			case DVB_MODULATION_QPSK: sModulation << "QPSK"; break;
			case DVB_MODULATION_16_QAM: sModulation << "16 QAM"; break;
			case DVB_MODULATION_32_QAM: sModulation << "32 QAM"; break;
			case DVB_MODULATION_64_QAM: sModulation << "64 QAM"; break;
			case DVB_MODULATION_128_QAM: sModulation << "128 QAM"; break;
			case DVB_MODULATION_256_QAM: sModulation << "256 QAM"; break;
			default: sModulation << "unknown"; break;
		}
		
		switch (fTuningParam.u.dvb_t.hierarchy) {
			case DVB_HIERARCHY_AUTO: sHierarchy << "auto"; break;
			case DVB_HIERARCHY_NONE: sHierarchy << "none"; break;
			case DVB_HIERARCHY_1: sHierarchy << "1"; break;
			case DVB_HIERARCHY_2: sHierarchy << "2"; break;
			case DVB_HIERARCHY_4: sHierarchy << "4"; break;
			default: sHierarchy << "unknown"; break;
		}
		
		switch (fTuningParam.u.dvb_t.code_rate_hp) {
			case DVB_FEC_AUTO: sCodeRateHP << "auto"; break;
			case DVB_FEC_NONE: sCodeRateHP << "none"; break;
			case DVB_FEC_1_2: sCodeRateHP << "FEC 1/2"; break;
			case DVB_FEC_2_3: sCodeRateHP << "FEC 2/3"; break;
			case DVB_FEC_3_4: sCodeRateHP << "FEC 3/4"; break;
			case DVB_FEC_4_5: sCodeRateHP << "FEC 4/5"; break;
			case DVB_FEC_5_6: sCodeRateHP << "FEC 5/6"; break;
			case DVB_FEC_6_7: sCodeRateHP << "FEC 6/7"; break;
			case DVB_FEC_7_8: sCodeRateHP << "FEC 7/8"; break;
			case DVB_FEC_8_9: sCodeRateHP << "FEC 8/9"; break;
			default: sCodeRateHP << "unknown"; break;
		}

		switch (fTuningParam.u.dvb_t.code_rate_lp) {
			case DVB_FEC_AUTO: sCodeRateLP << "auto"; break;
			case DVB_FEC_NONE: sCodeRateLP << "none"; break;
			case DVB_FEC_1_2: sCodeRateLP << "FEC 1/2"; break;
			case DVB_FEC_2_3: sCodeRateLP << "FEC 2/3"; break;
			case DVB_FEC_3_4: sCodeRateLP << "FEC 3/4"; break;
			case DVB_FEC_4_5: sCodeRateLP << "FEC 4/5"; break;
			case DVB_FEC_5_6: sCodeRateLP << "FEC 5/6"; break;
			case DVB_FEC_6_7: sCodeRateLP << "FEC 6/7"; break;
			case DVB_FEC_7_8: sCodeRateLP << "FEC 7/8"; break;
			case DVB_FEC_8_9: sCodeRateLP << "FEC 8/9"; break;
			default: sCodeRateLP << "unknown"; break;
		}

		switch (fTuningParam.u.dvb_t.transmission_mode) {
			case DVB_TRANSMISSION_MODE_AUTO: sTransmissionMode << "auto"; break;
			case DVB_TRANSMISSION_MODE_2K: sTransmissionMode << "2K"; break;
			case DVB_TRANSMISSION_MODE_4K: sTransmissionMode << "4K"; break;
			case DVB_TRANSMISSION_MODE_8K: sTransmissionMode << "8K"; break;
			default: sTransmissionMode << "unknown"; break;
		}

		switch (fTuningParam.u.dvb_t.guard_interval) {
			case DVB_GUARD_INTERVAL_AUTO: sGuardInterval << "auto"; break;
			case DVB_GUARD_INTERVAL_1_4: sGuardInterval << "1/4"; break;
			case DVB_GUARD_INTERVAL_1_8: sGuardInterval << "1/8"; break;
			case DVB_GUARD_INTERVAL_1_16: sGuardInterval << "1/16"; break;
			case DVB_GUARD_INTERVAL_1_32: sGuardInterval << "1/32"; break;
			default: sGuardInterval << "unknown"; break;
		}

		info1->MakeNullParameter(0, B_MEDIA_NO_TYPE, sInterfaceType.String(), B_GENERIC);
		info1->MakeNullParameter(0, B_MEDIA_NO_TYPE, sFrequency.String(), B_GENERIC);
		info1->MakeNullParameter(0, B_MEDIA_NO_TYPE, sBandwidth.String(), B_GENERIC);
		info1->MakeNullParameter(0, B_MEDIA_NO_TYPE, sVideoPid.String(), B_GENERIC);
		info1->MakeNullParameter(0, B_MEDIA_NO_TYPE, sAudioPid.String(), B_GENERIC);
		info1->MakeNullParameter(0, B_MEDIA_NO_TYPE, sPcrPid.String(), B_GENERIC);

		info2->MakeNullParameter(0, B_MEDIA_NO_TYPE, sModulation.String(), B_GENERIC);
		info2->MakeNullParameter(0, B_MEDIA_NO_TYPE, sTransmissionMode.String(), B_GENERIC);
		info2->MakeNullParameter(0, B_MEDIA_NO_TYPE, sGuardInterval.String(), B_GENERIC);
		info2->MakeNullParameter(0, B_MEDIA_NO_TYPE, sCodeRateHP.String(), B_GENERIC);
		info2->MakeNullParameter(0, B_MEDIA_NO_TYPE, sCodeRateLP.String(), B_GENERIC);
		info2->MakeNullParameter(0, B_MEDIA_NO_TYPE, sInversion.String(), B_GENERIC);
		info2->MakeNullParameter(0, B_MEDIA_NO_TYPE, sHierarchy.String(), B_GENERIC);
	}

	if (fInterfaceType == DVB_TYPE_DVB_S) {

		sFrequency << fTuningParam.u.dvb_s.frequency / 1000000 << " MHz";
		sSymbolRate << fTuningParam.u.dvb_s.symbolrate;

		switch (fTuningParam.u.dvb_s.inversion) {
			case DVB_INVERSION_AUTO: sInversion << "auto"; break;
			case DVB_INVERSION_ON: sInversion << "on"; break;
			case DVB_INVERSION_OFF: sInversion << "off"; break;
			default: sInversion << "unknown"; break;
		}
	
		switch (fTuningParam.u.dvb_s.polarity) {
			case DVB_POLARITY_VERTICAL: sPolarity << "vertical"; break;
			case DVB_POLARITY_HORIZONTAL: sPolarity << "horizontal"; break;
			default: sPolarity << "unknown"; break;
		}

		info1->MakeNullParameter(0, B_MEDIA_NO_TYPE, sInterfaceType.String(), B_GENERIC);
		info1->MakeNullParameter(0, B_MEDIA_NO_TYPE, sVideoPid.String(), B_GENERIC);
		info1->MakeNullParameter(0, B_MEDIA_NO_TYPE, sAudioPid.String(), B_GENERIC);
		info1->MakeNullParameter(0, B_MEDIA_NO_TYPE, sPcrPid.String(), B_GENERIC);

		info2->MakeNullParameter(0, B_MEDIA_NO_TYPE, sFrequency.String(), B_GENERIC);
		info2->MakeNullParameter(0, B_MEDIA_NO_TYPE, sPolarity.String(), B_GENERIC);
		info2->MakeNullParameter(0, B_MEDIA_NO_TYPE, sSymbolRate.String(), B_GENERIC);
		info2->MakeNullParameter(0, B_MEDIA_NO_TYPE, sInversion.String(), B_GENERIC);
		info2->MakeNullParameter(0, B_MEDIA_NO_TYPE, sAgcInversion.String(), B_GENERIC);
	}

	return web;
}


void
DVBMediaNode::LoadSettings()
{
	TRACE("DVBMediaNode::LoadSettings\n");
	RefreshStateList();
	fSelectedState = 0;
	RefreshRegionList();
	fSelectedRegion = 0;
	RefreshChannelList();
	fSelectedChannel = 0;
	RefreshAudioList();
	fSelectedAudio = 0;
}


void
DVBMediaNode::RefreshStateList()
{
	TRACE("DVBMediaNode::RefreshStateList\n");

	fStateList->MakeEmpty();
	fSelectedState = -1;

	const char *dir;
	switch (fInterfaceType) {
		case DVB_TYPE_DVB_C: dir = "/boot/home/config/settings/Media/dvb/dvb-c channels"; break;
		case DVB_TYPE_DVB_H: dir = "/boot/home/config/settings/Media/dvb/dvb-h channels"; break;
		case DVB_TYPE_DVB_S: dir = "/boot/home/config/settings/Media/dvb/dvb-s channels"; break;
		case DVB_TYPE_DVB_T: dir = "/boot/home/config/settings/Media/dvb/dvb-t channels"; break;
		default:
			printf("DVBMediaNode::RefreshStateList unknown interface type\n");
			return;
	}
	
	TRACE("loading channel lists from dir = %s\n", dir);
	
	BDirectory d(dir);
	BEntry e;
	BPath p;
	while (B_OK == d.GetNextEntry(&e, false)) {
		if (B_OK != e.GetPath(&p))
			continue;
		fStateList->AddItem(p.Path());
	}
	
	if (fStateList->ItemAt(0))
		fSelectedState = 0;
}


void
DVBMediaNode::RefreshRegionList()
{
	TRACE("DVBMediaNode::RefreshRegionList\n");

	fRegionList->MakeEmpty();
	fSelectedRegion = -1;

	const char *dir = fStateList->ItemAt(fSelectedState);
	if (!dir)
		return;

	BDirectory d(dir);
	BEntry e;
	BPath p;
	while (B_OK == d.GetNextEntry(&e, false)) {
		if (B_OK != e.GetPath(&p))
			continue;
		fRegionList->AddItem(p.Path());
	}

	if (fRegionList->ItemAt(0))
		fSelectedRegion = 0;
}


void
DVBMediaNode::RefreshChannelList()
{
	TRACE("DVBMediaNode::RefreshChannelList\n");

	fChannelList->MakeEmpty();
	fSelectedChannel = -1;

	const char *path = fRegionList->ItemAt(fSelectedRegion);
	if (!path)
		return;
		
	TRACE("opening channel list file = %s\n", path);
	
	FILE *f = fopen(path, "r");
	if (!f)
		return;

	char line[1024];
	while (fgets(line, sizeof(line), f)) {
		if (line[0] == ':') // skip comments
			continue;
		if (strchr(line, ':') == NULL)	// skip empty lines
			continue;
		fChannelList->AddItem(line);
	}

	fclose(f);	

	if (fChannelList->ItemAt(0))
		fSelectedChannel = 0;
}


void
DVBMediaNode::RefreshAudioList()
{
	TRACE("DVBMediaNode::RefreshAudioList\n");

	fAudioList->MakeEmpty();	
	fSelectedAudio = -1;
	
	fAudioList->AddItem("default"); // XXX test
	
	if (fAudioList->ItemAt(0))
		fSelectedAudio = 0;
}


void
DVBMediaNode::AddStateItems(BDiscreteParameter *param)
{
	TRACE("DVBMediaNode::AddStateItems\n");
	
	const char *str;
	for (int i = 0; (str = fStateList->ItemAt(i)); i++) {
		str = strrchr(str, '/');
		if (!str)
			continue;
		str++;
		param->AddItem(i, str);
	}
	if (param->CountItems() == 0)
		param->AddItem(-1, "none");
}


void
DVBMediaNode::AddRegionItems(BDiscreteParameter *param)
{
	TRACE("DVBMediaNode::AddRegionItems\n");

	const char *str;
	for (int i = 0; (str = fRegionList->ItemAt(i)); i++) {
		str = strrchr(str, '/');
		if (!str)
			continue;
		str++;
		param->AddItem(i, str);
	}
	if (param->CountItems() == 0)
		param->AddItem(-1, "none");
}


void
DVBMediaNode::AddChannelItems(BDiscreteParameter *param)
{
	TRACE("DVBMediaNode::AddChannelItems\n");

	const char *str;
	for (int i = 0; (str = fChannelList->ItemAt(i)); i++) {
		char name[256];
//		sscanf(str, "%s:", name);
		sscanf(str, "%[^:]", name);
		param->AddItem(i, name);

	}
	if (param->CountItems() == 0)
		param->AddItem(-1, "none");
}


void
DVBMediaNode::AddAudioItems(BDiscreteParameter *param)
{
	TRACE("DVBMediaNode::AddAudioItems\n");

	if (param->CountItems() == 0)
		param->AddItem(-1, "default");
}


status_t 
DVBMediaNode::GetParameterValue(int32 id, bigtime_t *last_change, void *value, size_t *size)
{
//	TRACE("DVBMediaNode::GetParameterValue, id 0x%lx\n", id);
	
	switch (id) {
		case ID_STATE:   *size = 4; *(int32 *)value = fSelectedState; break;
		case ID_REGION:  *size = 4; *(int32 *)value = fSelectedRegion; break;
		case ID_CHANNEL: *size = 4; *(int32 *)value = fSelectedChannel; break;
		case ID_AUDIO:   *size = 4; *(int32 *)value = fSelectedAudio; break;
		default: 		 return B_ERROR;
	}
	return B_OK;
}


void
DVBMediaNode::SetParameterValue(int32 id, bigtime_t when, const void *value, size_t size)
{
	TRACE("DVBMediaNode::SetParameterValue, id 0x%lx, size %ld, value 0x%lx\n", id, size, *(const int32 *)value);
	
	switch (id) {
		case ID_STATE:
			fSelectedState = *(const int32 *)value;
			StopCapture();
			RefreshRegionList();
			RefreshChannelList();
			RefreshAudioList();
			EventQueue()->AddEvent(media_timed_event(0, M_REFRESH_PARAMETER_WEB));
//			Tune();
			break;
			
		case ID_REGION:  
			fSelectedRegion = *(const int32 *)value;
			StopCapture();
			RefreshChannelList();
			RefreshAudioList();
			EventQueue()->AddEvent(media_timed_event(0, M_REFRESH_PARAMETER_WEB));
//			Tune();
			break;
			
		case ID_CHANNEL: 
			fSelectedChannel = *(const int32 *)value;
			RefreshAudioList();
//			EventQueue()->AddEvent(media_timed_event(0, M_REFRESH_PARAMETER_WEB));
			Tune();
			break;
			
		case ID_AUDIO:   
			fSelectedAudio = *(const int32 *)value;
			Tune();
			break;
			
		default:
			break;
	}
	TRACE("DVBMediaNode::SetParameterValue finished\n");
}


status_t
DVBMediaNode::ExtractTuningParams(const char *description, int audio_pid_index,
								  dvb_tuning_parameters_t *tuning_param, 
								  int *video_pid, int *audio_pid, int *pcr_pid)
{
	if (!description)
		return B_ERROR;

	printf("ExtractTuningParams: \"%s\"\n", description);
		
	char name[50];
	char freq[50];
	char para[100];
	char src[50];
	char srate[50];
	char vpid[50];
	char apid[50];
	char tpid[50];
	char ca[50];
	char sid[50];
	char nid[50];
	char tid[50];
	char rid[50];
	
	sscanf(description, " %[^:] : %[^:] : %[^:] : %[^:] : %[^:] : %[^:] : %[^:] : %[^:] : %[^:] : %[^:] : %[^:] : %[^:] : %[^:] ",
		name, freq, para, src, srate, vpid, apid, tpid, ca, sid, nid, tid, rid);
		
	char *cpid = strchr(vpid, '+');
	if (cpid) cpid++;
		
	int _vpid = strtol(vpid, 0, 0);
	int _apid = strtol(apid, 0, 0);
//	int _tpid = strtol(tpid, 0, 0);
	int _cpid = cpid ? strtol(cpid, 0, 0) : _vpid;
	int _srate = strtol(srate, 0, 0);
	int64 _freq = strtol(freq, 0, 0);
	while (_freq && _freq <= 1000000)
		_freq *= 1000;

	if (fInterfaceType == DVB_TYPE_DVB_S && _freq < 950000000) {
		TRACE("workaround activated: type is DVB_S and frequency < 950 MHz, multiplying by 1000!\n");
		_freq *= 1000;
	}


	*video_pid = _vpid;
	*audio_pid = _apid;
	*pcr_pid   = _cpid;

	TRACE("parsing result: params: '%s'\n", para);
	
	TRACE("parsing result: video pid %d\n", _vpid);
	TRACE("parsing result: audio pid %d\n", _apid);
	TRACE("parsing result: PCR pid %d\n", _cpid);
	TRACE("parsing result: symbol rate %d\n", _srate);
	TRACE("parsing result: Frequency %Ld Hz, %Ld MHz\n", _freq, _freq / 1000000);

	if (fInterfaceType == DVB_TYPE_DVB_T) {

		dvb_t_tuning_parameters_t *param = &tuning_param->u.dvb_t;

		TRACE("Interface is DVB-T\n");
		param->frequency = _freq;
		param->inversion = DVB_INVERSION_OFF;
		param->bandwidth = DVB_BANDWIDTH_8_MHZ;
		param->modulation = DVB_MODULATION_16_QAM;
		param->hierarchy = DVB_HIERARCHY_NONE;
		param->code_rate_hp = DVB_FEC_2_3;
		param->code_rate_lp = DVB_FEC_2_3;
		param->transmission_mode = DVB_TRANSMISSION_MODE_8K;
		param->guard_interval = DVB_GUARD_INTERVAL_1_4;
	}

	if (fInterfaceType == DVB_TYPE_DVB_S) {
		dvb_s_tuning_parameters_t *param = &tuning_param->u.dvb_s;

		TRACE("Interface is DVB-S\n");

		const char *pInv = strchr(para, 'I');
		if (!pInv)
			pInv = strchr(para, 'i');
		if (pInv && pInv[1] == '0') {
			TRACE("DVB_INVERSION_OFF\n");
			param->inversion = DVB_INVERSION_OFF;
		} else if (pInv && pInv[1] == '1') {
			TRACE("DVB_INVERSION_ON\n");
			param->inversion = DVB_INVERSION_ON;
		} else {
			TRACE("parse error, assuming DVB_INVERSION_OFF\n");
			param->inversion = DVB_INVERSION_OFF;
		}

		const char *pPolH = strchr(para, 'H');
		if (!pPolH)
			pPolH = strchr(para, 'h');
		const char *pPolV = strchr(para, 'V');
		if (!pPolV)
			pPolV = strchr(para, 'v');
		if (pPolH && !pPolV) {
			TRACE("DVB_POLARITY_HORIZONTAL\n");
			param->polarity = DVB_POLARITY_HORIZONTAL;
		} else if (!pPolH && pPolV) {
			TRACE("DVB_POLARITY_VERTICAL\n");
			param->polarity = DVB_POLARITY_VERTICAL;
		} else {
			TRACE("parse error, assuming DVB_POLARITY_HORIZONTAL\n");
			param->polarity = DVB_POLARITY_HORIZONTAL;
		}
		
		param->frequency = _freq;
		param->symbolrate = _srate;
	}
	
	return B_OK;
}
