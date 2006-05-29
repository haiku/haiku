/*
 * This file is a part of BeOS USBVision driver project.
 * Copyright (c) 2003 by Siarzuk Zharski <imker@gmx.li>
 *
 * This file may be used under the terms of the BSD License
 *
 * Skeletal part of this code was inherired from original BeOS sample code,
 * that is distributed under the terms of the Be Sample Code License.
 * 
 */

#include <fcntl.h>
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

#include <media/Buffer.h>
#include <media/BufferGroup.h>
#include <media/ParameterWeb.h>
#include <media/TimeSource.h>

#include <support/Autolock.h>
#include <support/Debug.h>

#define TOUCH(x) ((void)(x))

#define PRINTF(a,b) \
		do { \
			if (a < 2) { \
				printf("VideoProducer::"); \
				printf b; \
			} \
		} while (0)

#include "Producer.h"

#define FIELD_RATE 30.f

const float kUSBBandWidthMin = 0.5f;
const float kUSBBandWidthMax = 7.5f;
const float kUSBBandWidthStep = 0.5f;

const float kVScreenOffsetMin = 0.f;
const float kVScreenOffsetMax = 4.f;
const float kVScreenOffsetStep = 1.f;
const float kHScreenOffsetMin = 0.f;
const float kHScreenOffsetMax = 4.f;
const float kHScreenOffsetStep = 1.f;

const float kEffectMin = 0.f;
const float kEffectMax = 31.f;
const float kEffectStep = 1.f;

const float kBrightnessMin = kEffectMin;
const float kBrightnessMax = kEffectMax;
const float kBrightnessStep = kEffectStep;

const float kContrastMin = kEffectMin;
const float kContrastMax = kEffectMax;
const float kContrastStep = kEffectStep;

const float kHueMin = kEffectMin;
const float kHueMax = kEffectMax;
const float kHueStep = kEffectStep;

const float kSaturationMin = kEffectMin;
const float kSaturationMax = kEffectMax;
const float kSaturationStep = kEffectStep;

const uint32        kDefChannel = 0;
//const VideoInput    kDefVideoInput = P_VI_TUNER;
const uint32        kDefAudioInput = 0;
const uint32        kDefBrightness = 20;
const uint32        kDefContrast = 22;
const uint32        kDefSaturation = 15;
const uint32        kDefHue = 20;
const uint32        kDefCaptureSize = 0;
const uint32        kDefCaptureFormat = 0;
//const VideoStandard kDefStandard = P_VF_PAL_BDGHI;
const float         kDefBandwidth = 7.0;
const uint32        kDefLocale = 0;
const uint32        kDefVertOffset = 1;
const uint32        kDefHorzOffset = 2;

int32 VideoProducer::fInstances = 0;

VideoProducer::VideoProducer(
		BMediaAddOn *addon, const char *name, int32 internal_id)
  :	BMediaNode(name),
	BMediaEventLooper(),
	BBufferProducer(B_MEDIA_RAW_VIDEO),
	BControllable()
{
	status_t err;

	fInitStatus = B_NO_INIT;

	/* Only allow one instance of the node to exist at any time */
	if (atomic_add(&fInstances, 1) != 0)
		return;

	fInternalID = internal_id;
	fAddOn = addon;

	fBufferGroup = NULL;

	fThread = -1;
	fFrameSync = -1;
	fProcessingLatency = 0LL;

	fRunning = false;
	fConnected = false;
	fEnabled = false;

	fOutput.destination = media_destination::null;

	AddNodeKind(B_PHYSICAL_INPUT);

	fInitStatus = B_OK;

    //debugger("op-op");
    
    status_t status = Locale::TunerLocale::LoadLocales(fLocales);    
    
    printf("LoadLocales:%08x\n", status);

	fChannel = kDefChannel;
    fVideoInput = P_VI_TUNER; 
	fAudioInput = kDefAudioInput;
    fBrightness = kDefBrightness;
	fContrast = kDefContrast; 
	fSaturation = kDefSaturation; 
	fHue = kDefHue; 
    fCaptureSize = kDefCaptureSize; 
    fCaptureFormat = kDefCaptureFormat; 
    fStandard = P_VF_PAL_BDGHI; 
	fLocale = kDefLocale; 
    fBandwidth = kDefBandwidth; 
    fVertOffset = kDefVertOffset; 
	fHorzOffset = kDefHorzOffset; 
    fColor = B_HOST_TO_LENDIAN_INT32(0x00ff0000);

	fLastChannelChange = 
	fLastVideoInputChange = 
	fLastAudioInputChange = 
	fLastBrightnessChange = 
	fLastContrastChange = 
	fLastSaturationChange = 
	fLastHueChange = 
	fLastCaptureSizeChange = 
	fLastCaptureFormatChange = 
	fLastStandardChange = 
	fLastLocaleChange = 
	fLastBandwidthChange = 
	fLastVertOffsetChange = 
	fLastHorzOffsetChange = 
	fLastColorChange = system_time();

	return;
}

VideoProducer::~VideoProducer()
{
	if (fInitStatus == B_OK) {
		/* Clean up after ourselves, in case the application didn't make us
		 * do so. */
		if (fConnected)
			Disconnect(fOutput.source, fOutput.destination);
		if (fRunning)
			HandleStop();
	}

	atomic_add(&fInstances, -1);

    Locale::TunerLocale::ReleaseLocales(fLocales);    
}

/* BMediaNode */

port_id
VideoProducer::ControlPort() const
{
	return BMediaNode::ControlPort();
}

BMediaAddOn *
VideoProducer::AddOn(int32 *internal_id) const
{
	if (internal_id)
		*internal_id = fInternalID;
	return fAddOn;
}

status_t 
VideoProducer::HandleMessage(int32 message, const void *data, size_t size)
{
	return B_ERROR;
}

void 
VideoProducer::Preroll()
{
	/* This hook may be called before the node is started to give the hardware
	 * a chance to start. */
}

void
VideoProducer::SetTimeSource(BTimeSource *time_source)
{
	/* Tell frame generation thread to recalculate delay value */
	release_sem(fFrameSync);
}

status_t
VideoProducer::RequestCompleted(const media_request_info &info)
{
	return BMediaNode::RequestCompleted(info);
}

/* BMediaEventLooper */

void 
VideoProducer::NodeRegistered()
{
	if (fInitStatus != B_OK) {
		ReportError(B_NODE_IN_DISTRESS);
		return;
	}

	/* After this call, the BControllable owns the BParameterWeb object and
	 * will delete it for you */
	SetParameterWeb(CreateParameterWeb());

	fOutput.node = Node();
	fOutput.source.port = ControlPort();
	fOutput.source.id = 0;
	fOutput.destination = media_destination::null;
	strcpy(fOutput.name, Name());	

	/* Tailor these for the output of your device */
	fOutput.format.type = B_MEDIA_RAW_VIDEO;
	fOutput.format.u.raw_video = media_raw_video_format::wildcard;
	fOutput.format.u.raw_video.interlace = 1;
	fOutput.format.u.raw_video.display.format = B_RGB32;

	/* Start the BMediaEventLooper control loop running */
	Run();
}

BParameterWeb  *VideoProducer::CreateParameterWeb()
{
	/* Set up the parameter web */
	BParameterWeb *web = new BParameterWeb();

	BParameterGroup *controls = web->MakeGroup("Controls");
	BParameterGroup *group = controls->MakeGroup("Main Controls");
	BDiscreteParameter *parameter = group->MakeDiscreteParameter(
			P_CHANNEL, B_MEDIA_NO_TYPE, "Channel:", B_GENERIC);
	for(int i = 0; i < fLocales[fLocale]->ChannelsCount(); i ++){
	  parameter->AddItem(i, (fLocales[fLocale]->GetChannel(i)).Name().c_str());
	}
	
	parameter = group->MakeDiscreteParameter(
			            P_VIDEO_INPUT, B_MEDIA_NO_TYPE, "Video Input:", B_INPUT_MUX);
	parameter->AddItem(P_VI_TUNER, "Tuner");
	parameter->AddItem(P_VI_COMPOSITE, "Composite");
	parameter->AddItem(P_VI_SVIDEO, "SVideo");

	parameter = group->MakeDiscreteParameter(
			P_AUDIO_INPUT, B_MEDIA_NO_TYPE, "Audio Input:", B_INPUT_MUX);
	parameter->AddItem(1, "TODO");
	parameter->AddItem(2, "TODO");
	parameter->AddItem(3, "TODO");

	group = controls->MakeGroup("Effect Controls");
	group->MakeContinuousParameter(
			       P_BRIGHTNESS, B_MEDIA_NO_TYPE, "Brightness", B_LEVEL,
			         "", kBrightnessMin, kBrightnessMax, kBrightnessStep);

	group->MakeContinuousParameter(
			       P_CONTRAST, B_MEDIA_NO_TYPE, "Contrast", B_LEVEL,
			         "", kContrastMin, kContrastMax, kContrastStep);

	group = controls->MakeGroup("Adv. Effect Controls");
	group->MakeContinuousParameter(
			       P_SATURATION, B_MEDIA_NO_TYPE, "Saturation", B_LEVEL,
			         "", kSaturationMin, kSaturationMax, kSaturationStep);

	group->MakeContinuousParameter(
			       P_HUE, B_MEDIA_NO_TYPE, "Hue", B_LEVEL,
			         "", kHueMin, kHueMax, kHueStep);

    BParameterGroup *options = web->MakeGroup("Options");
	group = options->MakeGroup("Capture Option");
	parameter = group->MakeDiscreteParameter(
			P_CAPTURE_SIZE, B_MEDIA_NO_TYPE, "Capture Size:", B_RESOLUTION);
	parameter->AddItem(1, "TODO");
	parameter->AddItem(2, "TODO");

	parameter = group->MakeDiscreteParameter(
			P_CAPTURE_FORMAT, B_MEDIA_NO_TYPE, "Capture Format:", B_COLOR_SPACE);
	parameter->AddItem(1, "TODO");
	parameter->AddItem(2, "TODO");

	BParameterGroup *hardware = web->MakeGroup("Hardware Setup");
	group = hardware->MakeGroup("Format");
	parameter = group->MakeDiscreteParameter(
			P_STANDART, B_MEDIA_NO_TYPE, "Video Format:", B_VIDEO_FORMAT);
	parameter->AddItem(P_VF_NTSC_M, "NTSC M");
	parameter->AddItem(P_VF_NTSC_MJ, "NTSC MJ");
	parameter->AddItem(P_VF_PAL_BDGHI, "PAL BDGHI");
	parameter->AddItem(P_VF_PAL_M, "PAL M");
	parameter->AddItem(P_VF_PAL_N, "PAL N");
	parameter->AddItem(P_VF_SECAM, "SECAM");

	parameter = group->MakeDiscreteParameter(
			P_LOCALE, B_MEDIA_NO_TYPE, "Tuner Locale:", B_GENERIC);
	for(int i = 0; i < fLocales.size(); i++)
	  parameter->AddItem(i, fLocales[i]->Name().c_str());
	
  group = hardware->MakeGroup("Format");
	
	group->MakeContinuousParameter(
			       P_BANDWIDTH, B_MEDIA_NO_TYPE, "Max.Bandwidth", B_LEVEL,
			         "", kUSBBandWidthMin, kUSBBandWidthMax, kUSBBandWidthStep);

	group = hardware->MakeGroup("Offsets");
	
	group->MakeContinuousParameter(
			       P_VERT_OFFSET, B_MEDIA_NO_TYPE, "Vertical Offset", B_LEVEL,
			         "", kVScreenOffsetMin, kVScreenOffsetMax, kVScreenOffsetStep);

	group->MakeContinuousParameter(
			       P_HORZ_OFFSET, B_MEDIA_NO_TYPE, "Horizontal Offset", B_LEVEL,
			         "", kHScreenOffsetMin, kHScreenOffsetMax, kHScreenOffsetStep);

	BParameterGroup *main = web->MakeGroup(Name());
	BDiscreteParameter *state = main->MakeDiscreteParameter(
			P_COLOR, B_MEDIA_RAW_VIDEO, "Color", "Color");
	state->AddItem(B_HOST_TO_LENDIAN_INT32(0x00ff0000), "Red");
	state->AddItem(B_HOST_TO_LENDIAN_INT32(0x0000ff00), "Green");
	state->AddItem(B_HOST_TO_LENDIAN_INT32(0x000000ff), "Blue");

    return web; 
}

void
VideoProducer::Start(bigtime_t performance_time)
{
	BMediaEventLooper::Start(performance_time);
}

void
VideoProducer::Stop(bigtime_t performance_time, bool immediate)
{
	BMediaEventLooper::Stop(performance_time, immediate);
}

void
VideoProducer::Seek(bigtime_t media_time, bigtime_t performance_time)
{
	BMediaEventLooper::Seek(media_time, performance_time);
}

void
VideoProducer::TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time)
{
	BMediaEventLooper::TimeWarp(at_real_time, to_performance_time);
}

status_t
VideoProducer::AddTimer(bigtime_t at_performance_time, int32 cookie)
{
	return BMediaEventLooper::AddTimer(at_performance_time, cookie);
}

void
VideoProducer::SetRunMode(run_mode mode)
{
	BMediaEventLooper::SetRunMode(mode);
}

void 
VideoProducer::HandleEvent(const media_timed_event *event,
		bigtime_t lateness, bool realTimeEvent)
{
	TOUCH(lateness); TOUCH(realTimeEvent);

	switch(event->type)
	{
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
		case BTimedEventQueue::B_PARAMETER:
			HandleParameter(event->data);
			break;
		case BTimedEventQueue::B_HANDLE_BUFFER:
		case BTimedEventQueue::B_DATA_STATUS:
		default:
			PRINTF(-1, ("HandleEvent: Unhandled event -- %lx\n", event->type));
			break;
	}
}

void 
VideoProducer::CleanUpEvent(const media_timed_event *event)
{
	BMediaEventLooper::CleanUpEvent(event);
}

bigtime_t
VideoProducer::OfflineTime()
{
	return BMediaEventLooper::OfflineTime();
}

void
VideoProducer::ControlLoop()
{
	BMediaEventLooper::ControlLoop();
}

status_t
VideoProducer::DeleteHook(BMediaNode * node)
{
	return BMediaEventLooper::DeleteHook(node);
}

/* BBufferProducer */

status_t 
VideoProducer::FormatSuggestionRequested(
		media_type type, int32 quality, media_format *format)
{
	if (type != B_MEDIA_ENCODED_VIDEO)
		return B_MEDIA_BAD_FORMAT;

	TOUCH(quality);

	*format = fOutput.format;
	return B_OK;
}

status_t 
VideoProducer::FormatProposal(const media_source &output, media_format *format)
{
	status_t err;

	if (!format)
		return B_BAD_VALUE;

	if (output != fOutput.source)
		return B_MEDIA_BAD_SOURCE;

	err = format_is_compatible(*format, fOutput.format) ?
			B_OK : B_MEDIA_BAD_FORMAT;
	*format = fOutput.format;
	return err;
		
}

status_t 
VideoProducer::FormatChangeRequested(const media_source &source,
		const media_destination &destination, media_format *io_format,
		int32 *_deprecated_)
{
	TOUCH(destination); TOUCH(io_format); TOUCH(_deprecated_);
	if (source != fOutput.source)
		return B_MEDIA_BAD_SOURCE;
		
	return B_ERROR;	
}

status_t 
VideoProducer::GetNextOutput(int32 *cookie, media_output *out_output)
{
	if (!out_output)
		return B_BAD_VALUE;

	if ((*cookie) != 0)
		return B_BAD_INDEX;
	
	*out_output = fOutput;
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
VideoProducer::SetBufferGroup(const media_source &for_source,
		BBufferGroup *group)
{
	TOUCH(for_source); TOUCH(group);

	return B_ERROR;
}

status_t 
VideoProducer::VideoClippingChanged(const media_source &for_source,
		int16 num_shorts, int16 *clip_data,
		const media_video_display_info &display, int32 *_deprecated_)
{
	TOUCH(for_source); TOUCH(num_shorts); TOUCH(clip_data);
	TOUCH(display); TOUCH(_deprecated_);

	return B_ERROR;
}

status_t 
VideoProducer::GetLatency(bigtime_t *out_latency)
{
	*out_latency = EventLatency() + SchedulingLatency();
	return B_OK;
}

status_t 
VideoProducer::PrepareToConnect(const media_source &source,
		const media_destination &destination, media_format *format,
		media_source *out_source, char *out_name)
{
	status_t err;

	PRINTF(1, ("PrepareToConnect() %ldx%ld\n", \
			format->u.raw_video.display.line_width, \
			format->u.raw_video.display.line_count));

	if (fConnected) {
		PRINTF(0, ("PrepareToConnect: Already connected\n"));
		return EALREADY;
	}

	if (source != fOutput.source)
		return B_MEDIA_BAD_SOURCE;
	
	if (fOutput.destination != media_destination::null)
		return B_MEDIA_ALREADY_CONNECTED;

	/* The format parameter comes in with the suggested format, and may be
	 * specialized as desired by the node */
	if (!format_is_compatible(*format, fOutput.format)) {
		*format = fOutput.format;
		return B_MEDIA_BAD_FORMAT;
	}

	if (format->u.raw_video.display.line_width == 0)
		format->u.raw_video.display.line_width = 320;
	if (format->u.raw_video.display.line_count == 0)
		format->u.raw_video.display.line_count = 240;
	if (format->u.raw_video.field_rate == 0)
		format->u.raw_video.field_rate = 29.97f;

	*out_source = fOutput.source;
	strcpy(out_name, fOutput.name);

	fOutput.destination = destination;

	return B_OK;
}

void 
VideoProducer::Connect(status_t error, const media_source &source,
		const media_destination &destination, const media_format &format,
		char *io_name)
{
	PRINTF(1, ("Connect() %ldx%ld\n", \
			format.u.raw_video.display.line_width, \
			format.u.raw_video.display.line_count));

	if (fConnected) {
		PRINTF(0, ("Connect: Already connected\n"));
		return;
	}

	if (	(source != fOutput.source) || (error < B_OK) ||
			!const_cast<media_format *>(&format)->Matches(&fOutput.format)) {
		PRINTF(1, ("Connect: Connect error\n"));
		return;
	}

	fOutput.destination = destination;
	strcpy(io_name, fOutput.name);

	if (fOutput.format.u.raw_video.field_rate != 0.0f) {
		fPerformanceTimeBase = fPerformanceTimeBase +
				(bigtime_t)
					((fFrame - fFrameBase) *
					(1000000 / fOutput.format.u.raw_video.field_rate));
		fFrameBase = fFrame;
	}
	
	fConnectedFormat = format.u.raw_video;

	/* get the latency */
	bigtime_t latency = 0;
	media_node_id tsID = 0;
	FindLatencyFor(fOutput.destination, &latency, &tsID);
	#define NODE_LATENCY 1000
	SetEventLatency(latency + NODE_LATENCY);

	uint32 *buffer, *p, f = 3;
	p = buffer = (uint32 *)malloc(4 * fConnectedFormat.display.line_count *
			fConnectedFormat.display.line_width);
	if (!buffer) {
		PRINTF(0, ("Connect: Out of memory\n"));
		return;
	}
	bigtime_t now = system_time();
	for (int y=0;y<fConnectedFormat.display.line_count;y++)
		for (int x=0;x<fConnectedFormat.display.line_width;x++)
			*(p++) = ((((x+y)^0^x)+f) & 0xff) * (0x01010101 & fColor);
	fProcessingLatency = system_time() - now;
	free(buffer);

	/* Create the buffer group */
	fBufferGroup = new BBufferGroup(4 * fConnectedFormat.display.line_width *
			fConnectedFormat.display.line_count, 8);
	if (fBufferGroup->InitCheck() < B_OK) {
		delete fBufferGroup;
		fBufferGroup = NULL;
		return;
	}

	fConnected = true;
	fEnabled = true;

	/* Tell frame generation thread to recalculate delay value */
	release_sem(fFrameSync);
}

void 
VideoProducer::Disconnect(const media_source &source,
		const media_destination &destination)
{
	PRINTF(1, ("Disconnect()\n"));

	if (!fConnected) {
		PRINTF(0, ("Disconnect: Not connected\n"));
		return;
	}

	if ((source != fOutput.source) || (destination != fOutput.destination)) {
		PRINTF(0, ("Disconnect: Bad source and/or destination\n"));
		return;
	}

	fEnabled = false;
	fOutput.destination = media_destination::null;

	fLock.Lock();
		delete fBufferGroup;
		fBufferGroup = NULL;
	fLock.Unlock();

	fConnected = false;
}

void 
VideoProducer::LateNoticeReceived(const media_source &source,
		bigtime_t how_much, bigtime_t performance_time)
{
	TOUCH(source); TOUCH(how_much); TOUCH(performance_time);
}

void 
VideoProducer::EnableOutput(const media_source &source, bool enabled,
		int32 *_deprecated_)
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
VideoProducer::AdditionalBufferRequested(const media_source &source,
		media_buffer_id prev_buffer, bigtime_t prev_time,
		const media_seek_tag *prev_tag)
{
	TOUCH(source); TOUCH(prev_buffer); TOUCH(prev_time); TOUCH(prev_tag);
}

void 
VideoProducer::LatencyChanged(const media_source &source,
		const media_destination &destination, bigtime_t new_latency,
		uint32 flags)
{
	TOUCH(source); TOUCH(destination); TOUCH(new_latency); TOUCH(flags);
}

/* BControllable */									

status_t 
VideoProducer::GetParameterValue(
	int32 id, bigtime_t *last_change, void *value, size_t *size)
{
  switch(id){
  case P_CHANNEL:
    	*last_change = fLastChannelChange;
    	*size = sizeof(uint32);
	    *((uint32*)value) = fChannel;
      break;
		case P_VIDEO_INPUT:
    	*last_change = fLastVideoInputChange;
    	*size = sizeof(uint32);
	    *((uint32*)value) = fVideoInput;
      break;
    case P_AUDIO_INPUT:
    	*last_change = fLastAudioInputChange;
    	*size = sizeof(uint32);
	    *((uint32*)value) = fAudioInput;
      break;
    case P_BRIGHTNESS:
    	*last_change = fLastBrightnessChange;
    	*size = sizeof(float);
	    *((float *)value) = fBrightness;
      break;
    case P_CONTRAST:
    	*last_change = fLastContrastChange;
    	*size = sizeof(float);
	    *((float*)value) = fContrast;
      break;
    case P_SATURATION:
    	*last_change = fLastSaturationChange;
    	*size = sizeof(float);
	    *((float*)value) = fSaturation;
      break;
    case P_HUE:
    	*last_change = fLastHueChange;
    	*size = sizeof(float);
	    *((float*)value) = fHue;
      break;
    case P_CAPTURE_SIZE:
    	*last_change = fLastCaptureSizeChange;
    	*size = sizeof(uint32);
	    *((uint32*)value) = fCaptureSize;
      break;
    case P_CAPTURE_FORMAT:
    	*last_change = fLastCaptureFormatChange;
    	*size = sizeof(uint32);
	    *((uint32*)value) = fCaptureFormat;
      break;
    case P_STANDART:
    	*last_change = fLastStandardChange;
    	*size = sizeof(uint32);
	    *((uint32*)value) = fStandard;
      break;
    case P_BANDWIDTH:
    	*last_change = fLastBandwidthChange;
    	*size = sizeof(float);
	    *((float *)value) = fBandwidth;
      break;
    case P_LOCALE:
    	*last_change = fLastLocaleChange;
    	*size = sizeof(uint32);
	    *((uint32*)value) = fLocale;
      break;
    case P_VERT_OFFSET:
    	*last_change = fLastVertOffsetChange;
    	*size = sizeof(float);
	    *((float*)value) = fVertOffset;
      break;
    case P_HORZ_OFFSET:
    	*last_change = fLastHorzOffsetChange;
    	*size = sizeof(float);
	    *((float*)value) = fHorzOffset;
      break;
    case P_COLOR:
	    *last_change = fLastColorChange;
	    *size = sizeof(uint32);
	    *((uint32 *)value) = fColor;
      break;
    default:
      return B_BAD_VALUE;
  }
	return B_OK;
}

void
VideoProducer::SetParameterValue(
	int32 id, bigtime_t when, const void *value, size_t size)
{
  switch(id){
        case P_CHANNEL:
    if (*(uint32 *)value != fChannel){
	      fChannel = *(uint32 *)value;
	      fLastChannelChange = when;
	      BroadcastNewParameterValue(
            fLastChannelChange, P_CHANNEL, &fChannel, sizeof(fChannel));
      }
      break;
		case P_VIDEO_INPUT:
      if (*(uint32 *)value != fVideoInput){
	      fVideoInput = *(uint32 *)value;
	      fLastVideoInputChange = when;
	      BroadcastNewParameterValue(
            fLastVideoInputChange, P_VIDEO_INPUT, &fVideoInput, sizeof(fVideoInput));
      }
      break;
    case P_AUDIO_INPUT:
      if (*(uint32 *)value != fAudioInput){
	      fAudioInput = *(uint32 *)value;
	      fLastAudioInputChange = when;
	      BroadcastNewParameterValue(
            fLastAudioInputChange, P_AUDIO_INPUT, &fAudioInput, sizeof(fAudioInput));
      }
      break;
    case P_BRIGHTNESS:
      if (*(float *)value != fBrightness){
	      fBrightness = *(float *)value;
	      fLastBrightnessChange = when;
	      BroadcastNewParameterValue(
            fLastBrightnessChange, P_BRIGHTNESS, &fBrightness, sizeof(fBrightness));
      }
      break;
    case P_CONTRAST:
      if (*(float *)value != fContrast){
	      fContrast = *(float *)value;
	      fLastContrastChange = when;
	      BroadcastNewParameterValue(
            fLastContrastChange, P_CONTRAST, &fContrast, sizeof(fContrast));
      }
      break;
    case P_SATURATION:
      if (*(float *)value != fSaturation){
	      fSaturation = *(float *)value;
	      fLastSaturationChange = when;
	      BroadcastNewParameterValue(
            fLastSaturationChange, P_SATURATION, &fSaturation, sizeof(fSaturation));
      }
      break;
    case P_HUE:
      if (*(float *)value != fHue){
	      fHue = *(float *)value;
	      fLastHueChange = when;
	      BroadcastNewParameterValue(
            fLastHueChange, P_HUE, &fHue, sizeof(fHue));
      }
      break;
    case P_CAPTURE_SIZE:
      if (*(uint32 *)value != fCaptureSize){
	      fCaptureSize = *(uint32*)value;
	      fLastCaptureSizeChange = when;
	      BroadcastNewParameterValue(
            fLastCaptureSizeChange, P_CAPTURE_SIZE, &fCaptureSize, sizeof(fCaptureSize));
      }
      break;
    case P_CAPTURE_FORMAT:
      if (*(uint32 *)value != fCaptureFormat){
	      fCaptureFormat = *(uint32*)value;
	      fLastCaptureFormatChange = when;
	      BroadcastNewParameterValue(
            fLastCaptureFormatChange, P_CAPTURE_FORMAT, &fCaptureFormat, sizeof(fCaptureFormat));
      }
      break;
    case P_STANDART:
      if (*(uint32 *)value != fStandard){
	      fStandard = *(uint32*)value;
	      fLastStandardChange = when;
	      BroadcastNewParameterValue(
            fLastStandardChange, P_STANDART, &fStandard, sizeof(fStandard));
      }
      break;
    case P_BANDWIDTH:
      if (*(float *)value != fBandwidth){
	      fBandwidth = *(float *)value;
	      fLastBandwidthChange = when;
	      BroadcastNewParameterValue(
            fLastBandwidthChange, P_BANDWIDTH, &fBandwidth, sizeof(fBandwidth));
      }
      break;
    case P_LOCALE:
      if (*(uint32 *)value != fLocale){
	      fLocale = *(uint32 *)value;
	      fLastLocaleChange = when;
	      BroadcastNewParameterValue(
            fLastLocaleChange, P_LOCALE, &fLocale, sizeof(fLocale));
      }
      break;
    case P_VERT_OFFSET:
      if (*(float*)value != fVertOffset){
	      fVertOffset = *(float *)value;
	      fLastVertOffsetChange = when;
	      BroadcastNewParameterValue(
            fLastVertOffsetChange, P_VERT_OFFSET, &fVertOffset, sizeof(fVertOffset));
      }
      break;
    case P_HORZ_OFFSET:
      if (*(float*)value != fHorzOffset){
	      fHorzOffset = *(float *)value;
	      fLastHorzOffsetChange = when;
	      BroadcastNewParameterValue(
            fLastHorzOffsetChange, P_HORZ_OFFSET, &fHorzOffset, sizeof(fHorzOffset));
      }
      break;
    case P_COLOR:
      if (*(int32 *)value != fColor){
	      fColor = *(uint32 *)value;
	      fLastColorChange = when;
	      BroadcastNewParameterValue(
            fLastColorChange, P_COLOR, &fColor, sizeof(fColor));
      }
      break;
  }
  
  EventQueue()->AddEvent(media_timed_event(when,
        BTimedEventQueue::B_PARAMETER, NULL,
        BTimedEventQueue::B_NO_CLEANUP, id, 0, NULL, 0));

}

status_t
VideoProducer::StartControlPanel(BMessenger *out_messenger)
{
	return BControllable::StartControlPanel(out_messenger);
}

/* VideoProducer */

void
VideoProducer::HandleStart(bigtime_t performance_time)
{
	/* Start producing frames, even if the output hasn't been connected yet. */

	PRINTF(1, ("HandleStart(%Ld)\n", performance_time));

	if (fRunning) {
		PRINTF(-1, ("HandleStart: Node already started\n"));
		return;
	}

	fFrame = 0;
	fFrameBase = 0;
	fPerformanceTimeBase = performance_time;

	fFrameSync = create_sem(0, "frame synchronization");
	if (fFrameSync < B_OK)
		goto err1;

	fThread = spawn_thread(_frame_generator_, "frame generator",
			B_NORMAL_PRIORITY, this);
	if (fThread < B_OK)
		goto err2;

	resume_thread(fThread);

	fRunning = true;
	return;

err2:
	delete_sem(fFrameSync);
err1:
	return;
}

void
VideoProducer::HandleStop(void)
{
	PRINTF(1, ("HandleStop()\n"));

	if (!fRunning) {
		PRINTF(-1, ("HandleStop: Node isn't running\n"));
		return;
	}

	delete_sem(fFrameSync);
	wait_for_thread(fThread, &fThread);

	fRunning = false;
}

void
VideoProducer::HandleTimeWarp(bigtime_t performance_time)
{
	fPerformanceTimeBase = performance_time;
	fFrameBase = fFrame;

	/* Tell frame generation thread to recalculate delay value */
	release_sem(fFrameSync);
}

void
VideoProducer::HandleSeek(bigtime_t performance_time)
{
	fPerformanceTimeBase = performance_time;
	fFrameBase = fFrame;

	/* Tell frame generation thread to recalculate delay value */
	release_sem(fFrameSync);
}

void
VideoProducer::HandleParameter(uint32 parameter)
{
  switch(parameter){
  case P_CHANNEL:
    break;
  case P_VIDEO_INPUT:
    break;
  case P_AUDIO_INPUT:
    break;
  case P_BRIGHTNESS:
    break;
  case P_CONTRAST:
    break;
  case P_SATURATION:
    break;
  case P_HUE:
    break;
  case P_CAPTURE_SIZE:
    break;
  case P_CAPTURE_FORMAT:
    break;
  case P_STANDART:
    break;
  case P_BANDWIDTH:
    break;
  case P_LOCALE:
    {
      //debugger("stop");
      BParameterWeb *web = Web();
      int nCount = web->CountParameters();
      for(int idx = 0; idx < nCount; idx++){
        BParameter *parameter = web->ParameterAt(idx);
        if(parameter && parameter->Type() == BParameter::B_DISCRETE_PARAMETER &&
                         parameter->ID() == P_CHANNEL){
          BDiscreteParameter * d_parameter = dynamic_cast<BDiscreteParameter *>(parameter);
          d_parameter->MakeEmpty();
          for(int i = 0; i < fLocales[fLocale]->ChannelsCount(); i ++){
	        d_parameter->AddItem(i, (fLocales[fLocale]->GetChannel(i)).Name().c_str());
	      }
	      /*status_t st = BroadcastChangedParameter(P_CHANNEL);
          if(st != B_OK)
            debugger("kk");*/
          break;
        }
      }
    }
    break;
  case P_VERT_OFFSET:
    break;
  case P_HORZ_OFFSET:
    break;
  }
}

/* The following functions form the thread that generates frames. You should
 * replace this with the code that interfaces to your hardware. */
int32 
VideoProducer::FrameGenerator()
{
	bigtime_t wait_until = system_time();

	while (1) {
		status_t err = acquire_sem_etc(fFrameSync, 1, B_ABSOLUTE_TIMEOUT,
				wait_until);

		/* The only acceptable responses are B_OK and B_TIMED_OUT. Everything
		 * else means the thread should quit. Deleting the semaphore, as in
		 * VideoProducer::HandleStop(), will trigger this behavior. */
		if ((err != B_OK) && (err != B_TIMED_OUT))
			break;

		fFrame++;

		/* Recalculate the time until the thread should wake up to begin
		 * processing the next frame. Subtract fProcessingLatency so that
		 * the frame is sent in time. */
		wait_until = TimeSource()->RealTimeFor(fPerformanceTimeBase +
				(bigtime_t)
						((fFrame - fFrameBase) *
						(1000000 / fConnectedFormat.field_rate)), 0) -
				fProcessingLatency;

		/* Drop frame if it's at least a frame late */
		if (wait_until < system_time())
			continue;

		/* If the semaphore was acquired successfully, it means something
		 * changed the timing information (see VideoProducer::Connect()) and
		 * so the thread should go back to sleep until the newly-calculated
		 * wait_until time. */
		if (err == B_OK)
			continue;

		/* Send buffers only if the node is running and the output has been
		 * enabled */
		if (!fRunning || !fEnabled)
			continue;

		BAutolock _(fLock);

		/* Fetch a buffer from the buffer group */
		BBuffer *buffer = fBufferGroup->RequestBuffer(
						4 * fConnectedFormat.display.line_width *
						fConnectedFormat.display.line_count, 0LL);
		if (!buffer)
			continue;

		/* Fill out the details about this buffer. */
		media_header *h = buffer->Header();
		h->type = B_MEDIA_RAW_VIDEO;
		h->time_source = TimeSource()->ID();
		h->size_used = 4 * fConnectedFormat.display.line_width *
						fConnectedFormat.display.line_count;
		/* For a buffer originating from a device, you might want to calculate
		 * this based on the PerformanceTimeFor the time your buffer arrived at
		 * the hardware (plus any applicable adjustments). */
		h->start_time = fPerformanceTimeBase +
						(bigtime_t)
							((fFrame - fFrameBase) *
							(1000000 / fConnectedFormat.field_rate));
		h->file_pos = 0;
		h->orig_size = 0;
		h->data_offset = 0;
		h->u.raw_video.field_gamma = 1.0;
		h->u.raw_video.field_sequence = fFrame;
		h->u.raw_video.field_number = 0;
		h->u.raw_video.pulldown_number = 0;
		h->u.raw_video.first_active_line = 1;
		h->u.raw_video.line_count = fConnectedFormat.display.line_count;

		/* Fill in a pattern */
		uint32 *p = (uint32 *)buffer->Data();
		for (int y=0;y<fConnectedFormat.display.line_count;y++)
			for (int x=0;x<fConnectedFormat.display.line_width;x++)
				*(p++) = ((((x+y)^0^x)+fFrame) & 0xff) * (0x01010101 & fColor);

		/* Send the buffer on down to the consumer */
		if (SendBuffer(buffer, fOutput.destination) < B_OK) {
			PRINTF(-1, ("FrameGenerator: Error sending buffer\n"));
			/* If there is a problem sending the buffer, return it to its
			 * buffer group. */
			buffer->Recycle();
		}
	}

	return B_OK;
}

int32
VideoProducer::_frame_generator_(void *data)
{
	return ((VideoProducer *)data)->FrameGenerator();
}
