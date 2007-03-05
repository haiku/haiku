/******************************************************************************
/
/	File:			RadeonProducer.cpp
/
/	Description:	ATI Radeon Video Producer media node.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#include <fcntl.h>
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>
#include <scheduler.h>

#include <media/Buffer.h>
#include <media/BufferGroup.h>
#include <media/ParameterWeb.h>
#include <media/TimeSource.h>

#include <support/Autolock.h>
#include <support/Debug.h>

#include <app/Message.h>

#include "RadeonAddOn.h"
#include "VideoIn.h"

#define DPRINT(args)	{ PRINT(("\x1b[0;30;35m")); PRINT(args); PRINT(("\x1b[0;30;47m")); }

#define TOUCH(x) ((void)(x))

#define PRINTF(a,b) \
		do { \
			if (a < 2) { \
				printf("CRadeonProducer::"); \
				printf b; \
			} \
		} while (0)

#include "RadeonProducer.h"

// convert Be video standard to video-in standard;
// Be forgot some standards and define code 7 and 8 to be MPEG1/2, i.e.
// didn't leave any space for enhancements, so I chose to use 101 and up;
// this way, we get a scattered list of video standards, needing special
// functions to convert to scattered Be-code to the compact video-in-code
video_in_standard BeToVideoInStandard( int32 be_standard )
{
	
	DPRINT(("BeToVideoInStandard %d \n", be_standard));
	switch( be_standard ) {
	case 1:	return C_VIDEO_IN_NTSC;
	case 2:	return C_VIDEO_IN_NTSC_JAPAN;
	case 3:	return C_VIDEO_IN_PAL_BDGHI;
	case 4:	return C_VIDEO_IN_PAL_M;
	case 5:	return C_VIDEO_IN_PAL_N;
	case 6:	return C_VIDEO_IN_SECAM;
	case 101:	return C_VIDEO_IN_NTSC_443;
	case 102:	return C_VIDEO_IN_PAL_60;
	case 103:	return C_VIDEO_IN_PAL_NC;
	default:	return C_VIDEO_IN_NTSC;
	}
}

int32 VideoInStandardToBe( video_in_standard standard )
{
	DPRINT(("VideoInStandardToBe %d \n", standard));
	switch( standard ) {
		case C_VIDEO_IN_NTSC:		return 1;
		case C_VIDEO_IN_NTSC_JAPAN:	return 2;
		case C_VIDEO_IN_PAL_BDGHI:	return 3;
		case C_VIDEO_IN_PAL_M:		return 4;
		case C_VIDEO_IN_PAL_N:		return 5;
		case C_VIDEO_IN_SECAM:		return 6;
		case C_VIDEO_IN_NTSC_443:	return 101;
		case C_VIDEO_IN_PAL_60:		return 102;
		case C_VIDEO_IN_PAL_NC:		return 103;
		default: return 1;
	}
}

status_t CRadeonProducer::FindInt32( 
	BMessage *config, EOptions option, int32 min_value, int32 max_value, 
	int32 default_value, int32 *value )
{
	char name[5];
	status_t res;
	
	*value = default_value;
	
	*(int32 *)name = option;
	name[4] = 0;
	
	res = config->FindInt32( name, value );
	if( res == B_NAME_NOT_FOUND )
		return B_OK;
	
	if( res != B_OK )
		return res;
		
	*value = MAX( *value, min_value );
	*value = MIN( *value, max_value );
	return B_OK;
}

CRadeonProducer::CRadeonProducer(
		CRadeonAddOn *addon, const char *name, const char *device_name, int32 internal_id,
		BMessage *config )
  :	BMediaNode(name),
	BMediaEventLooper(),
	BBufferProducer(B_MEDIA_RAW_VIDEO),
	BControllable(),
	fVideoIn( device_name )
{
	DPRINT(("CRadeonProducer::CRadeonProducer()\n"));
	
	fInitStatus = B_NO_INIT;

	fInternalID = internal_id;
	fAddOn = addon;

	fBufferGroup = NULL;
	//fUsedBufferGroup = NULL;
	
	fProcessingLatency = 0LL;

	//fConnected = false;
	fEnabled = true;

	AddNodeKind(B_PHYSICAL_INPUT);
	
	if( fVideoIn.InitCheck() == B_OK )
		fInitStatus = B_OK;
		
	fSource = ((fVideoIn.Capabilities() & C_VIDEO_IN_HAS_TUNER) != 0 ? C_VIDEO_IN_TUNER : C_VIDEO_IN_COMPOSITE);
	fStandard = C_VIDEO_IN_NTSC;
	fMode = C_VIDEO_IN_WEAVE;
	fFormat = B_RGB32;
	fResolution = 4;
	fTuner = 25;
	fBrightness = 0;
	fContrast = 0;
	fSaturation = 0;
	fHue = 0;
	fSharpness = 0;

	if( config != NULL ) {	
		status_t res;
		int32 standard;
		
		if( (res = FindInt32( config, P_SOURCE, 0, C_VIDEO_IN_SOURCE_MAX, 
			 (fVideoIn.Capabilities() & C_VIDEO_IN_HAS_TUNER) != 0 ? C_VIDEO_IN_TUNER : C_VIDEO_IN_COMPOSITE, 
			 &fSource )) != B_OK ||
			(res = FindInt32( config, P_STANDARD, 0, C_VIDEO_IN_STANDARD_MAX,
				C_VIDEO_IN_NTSC, &standard )) != B_OK ||
			(res = FindInt32( config, P_MODE, 0, C_VIDEO_IN_CAPTURE_MODE_MAX,
				C_VIDEO_IN_FIELD, &fMode )) != B_OK ||
			(res = FindInt32( config, P_FORMAT, -2147483647L-1, 2147483647L,
				B_RGB16, &fFormat )) != B_OK ||
			(res = FindInt32( config, P_RESOLUTION, 0, C_RESOLUTION_MAX,
				4, &fResolution )) != B_OK ||
			(res = FindInt32( config, P_TUNER, 0, C_CHANNEL_MAX,
				25, &fTuner )) != B_OK ||
			(res = FindInt32( config, P_BRIGHTNESS, -100, +100,
				0, &fBrightness )) != B_OK ||
			(res = FindInt32( config, P_CONTRAST, 0, 100,
				0, &fContrast )) != B_OK ||
			(res = FindInt32( config, P_SATURATION, -100, +100,
				0, &fSaturation )) != B_OK ||
			(res = FindInt32( config, P_HUE, -90, +90,
				0, &fHue )) != B_OK ||
			(res = FindInt32( config, P_SHARPNESS, 0, 15,
				0, &fSharpness )) != B_OK )
		{
			DPRINT(("Corrupted settings (%s)\n", strerror( res )));
		}
		
		// standard is stored as internal code (which has no "holes" in its numbering);
		// time to convert it
		// if this value comes from our setup web is it not already linear?
		fStandard = VideoInStandardToBe( (video_in_standard)standard );
		
		// if there is no tuner, force composite input
		if( (fVideoIn.Capabilities() & C_VIDEO_IN_HAS_TUNER) == 0 )
			fSource = C_VIDEO_IN_COMPOSITE;
		
		// format ids are scattered, so we must verify them manually	
		switch( fFormat ) {
		case B_YCbCr422:
		case B_GRAY8:
		case B_RGB15:
		case B_RGB16:
		case B_RGB32:
			break;
		default:
			fFormat = B_RGB16;
		}
	}
	
	fSourceLastChange = 
	fStandardLastChange = 
	fModeLastChange = 
	fFormatLastChange =
	fResolutionLastChange = 
	fTunerLastChange =
	fBrightnessLastChange = 
	fContrastLastChange = 
	fSaturationLastChange =
	fHueLastChange =
	fSharpnessLastChange = system_time();

	fOutput.destination = media_destination::null;
	strcpy(fOutput.name, Name());	
	
	// we provide interlaced raw video in any format
	fOutput.format.type = B_MEDIA_RAW_VIDEO;
	fOutput.format.u.raw_video = media_raw_video_format::wildcard;
}


void CRadeonProducer::setupWeb()
{
	///////////
	/* Set up the parameter web */
	
	// in "kind" value of parameters is "stampTV-compatible", i.e.
	// if not defined, we use the name used in stampTV
	BParameterWeb *web = new BParameterWeb();
	BParameterGroup *controls = web->MakeGroup("Controls");
	BParameterGroup *options = web->MakeGroup("Video");
	/*BParameterGroup *audio = web->MakeGroup("Audio");*/
	
	BParameterGroup *controls1 = controls->MakeGroup("Controls1");
	BParameterGroup *controls2 = controls->MakeGroup("Controls2");
	BParameterGroup *controls3 = controls->MakeGroup("Controls3");

	BParameterGroup *options1 = options->MakeGroup("Options1");
	BParameterGroup *options2 = options->MakeGroup("Options2");

	/*BParameterGroup *audio1 = audio->MakeGroup("Audio1");
	BParameterGroup *audio2 = audio->MakeGroup("Audio2");*/

	
	// Controls
	if ((fVideoIn.Capabilities() & C_VIDEO_IN_HAS_TUNER) != 0) {
		// Controls.Channel
		BDiscreteParameter *tuner = controls1->MakeDiscreteParameter(
			P_TUNER, B_MEDIA_NO_TYPE, "Channel:", B_TUNER_CHANNEL);
	
		for (int channel = 0; channel <= 125; channel++) {
			char buffer[32];
			sprintf(buffer, "%d", channel);
			tuner->AddItem(channel, buffer);
		}
	}

	// Controls.Source
	BDiscreteParameter *source = controls1->MakeDiscreteParameter(
		P_SOURCE, B_MEDIA_RAW_VIDEO, "Video Input:", "Video Input:");

	if ((fVideoIn.Capabilities() & C_VIDEO_IN_HAS_TUNER) != 0)
		source->AddItem(C_VIDEO_IN_TUNER, "Tuner");
	if ((fVideoIn.Capabilities() & C_VIDEO_IN_HAS_COMPOSITE) != 0)
		source->AddItem(C_VIDEO_IN_COMPOSITE, "Composite");
	if ((fVideoIn.Capabilities() & C_VIDEO_IN_HAS_SVIDEO) != 0)
		source->AddItem(C_VIDEO_IN_SVIDEO, "SVideo");

	// TODO:
	BDiscreteParameter *source2 = controls1->MakeDiscreteParameter(
		P_AUDIO_SOURCE, B_MEDIA_RAW_VIDEO, "Audio Input:", "Audio Input:");
	if ((fVideoIn.Capabilities() & C_VIDEO_IN_HAS_TUNER) != 0)
		source2->AddItem(C_VIDEO_IN_TUNER, "Tuner");
/*	if ((fVideoIn.Capabilities() & C_VIDEO_IN_HAS_COMPOSITE) != 0)
		source2->AddItem(C_VIDEO_IN_COMPOSITE, "Composite");
	if ((fVideoIn.Capabilities() & C_VIDEO_IN_HAS_SVIDEO) != 0)
		source2->AddItem(C_VIDEO_IN_SVIDEO, "SVideo");
*/	
	
	// Controls.Brightness/Contrast/Saturation/Hue
	controls2->MakeContinuousParameter(P_BRIGHTNESS, B_MEDIA_RAW_VIDEO,"Brightness", "BRIGHTNESS", "", -100, 100, 1);
	controls2->MakeContinuousParameter(P_CONTRAST, B_MEDIA_RAW_VIDEO, "Contrast", "CONTRAST", "", 0, 100, 1);
	controls2->MakeContinuousParameter(P_SHARPNESS, B_MEDIA_RAW_VIDEO, "Sharpness", B_LEVEL, "dB", 0, 15, 1);

	controls3->MakeContinuousParameter(P_SATURATION, B_MEDIA_RAW_VIDEO, "Saturation", "SATURATION", "", -100, 100, 1);
	controls3->MakeContinuousParameter(P_HUE, B_MEDIA_RAW_VIDEO, "Hue", B_LEVEL, "°", -90, 90, 1);


	// Options.Resolution
	BDiscreteParameter *resolution = options1->MakeDiscreteParameter(
		P_RESOLUTION, B_MEDIA_RAW_VIDEO, "Default Image Size:", B_RESOLUTION);
	
	resolution->AddItem(6, "768x576");
	resolution->AddItem(5, "720x576");
	resolution->AddItem(4, "720x480");
	resolution->AddItem(0, "640x480");
	resolution->AddItem(3, "480x360");
	resolution->AddItem(1, "320x240");
	resolution->AddItem(2, "160x120");

	// Options.Format
	BDiscreteParameter *format = options1->MakeDiscreteParameter(
		P_FORMAT, B_MEDIA_RAW_VIDEO, "Default Colors:", B_COLOR_SPACE);

	format->AddItem(B_YCbCr422, "YCbCr422 (fastest)");
	format->AddItem(B_GRAY8, "8 Bits/Pixel (gray)");
	format->AddItem(B_RGB15, "15 Bits/Pixel");
	format->AddItem(B_RGB16, "16 Bits/Pixel");
	format->AddItem(B_RGB32, "32 Bits/Pixel");

	// Options.Standard
	BDiscreteParameter *standard = options2->MakeDiscreteParameter(
		P_STANDARD, B_MEDIA_RAW_VIDEO, "Video Format:", B_VIDEO_FORMAT);

	standard->AddItem(1, "NTSC");
	standard->AddItem(2, "NTSC Japan");
	standard->AddItem(101, "NTSC 443");
	standard->AddItem(4, "PAL M");
	standard->AddItem(3, "PAL BDGHI");
	standard->AddItem(5, "PAL N");
	standard->AddItem(102, "PAL 60");
	standard->AddItem(103, "PAL NC");
	standard->AddItem(6, "SECAM");
	
	// Options.Mode
	BDiscreteParameter *mode = options2->MakeDiscreteParameter(
		P_MODE, B_MEDIA_RAW_VIDEO, "Video Interlace:", B_GENERIC);
	
	mode->AddItem(C_VIDEO_IN_FIELD, "Field");
	mode->AddItem(C_VIDEO_IN_BOB, "Bob");
	mode->AddItem(C_VIDEO_IN_WEAVE, "Weave");
	
	
	// TODO:
	/*
	BDiscreteParameter *standard2 = audio1->MakeDiscreteParameter(
		P_AUDIO_FORMAT, B_MEDIA_RAW_VIDEO, "Audio Format:", B_VIDEO_FORMAT);
	standard2->AddItem(0, "Stereo");
	standard2->AddItem(1, "Mono");
	standard2->AddItem(2, "NICAM");

	BDiscreteParameter *audioSource = audio1->MakeDiscreteParameter(
		P_AUDIO_FORMAT, B_MEDIA_RAW_VIDEO, "Audio Source:", B_VIDEO_FORMAT);
	audioSource->AddItem(0, "FM");
	audioSource->AddItem(1, "Stereo");
	audioSource->AddItem(2, "SCART");
	audioSource->AddItem(3, "Language A");
	audioSource->AddItem(4, "Language B");

	BDiscreteParameter *matrix= audio2->MakeDiscreteParameter(
		P_AUDIO_FORMAT, B_MEDIA_RAW_VIDEO, "Audio Matrix:", B_VIDEO_FORMAT);
	matrix->AddItem(0, "Sound A");
	matrix->AddItem(1, "Sound B");
	matrix->AddItem(2, "Stereo");
	matrix->AddItem(3, "Mono");*/
	
	/* After this call, the BControllable owns the BParameterWeb object and
	 * will delete it for you */
	SetParameterWeb(web);
	/////////
}

CRadeonProducer::~CRadeonProducer()
{
	DPRINT(("CRadeonProducer::~CRadeonProducer()\n"));

	if (fInitStatus == B_OK) {
		/* Clean up after ourselves, in case the application didn't make us
		 * do so. */
		/*if (fConnected)
			Disconnect(fOutput.source, fOutput.destination);*/

		HandleStop();
	}
	
	delete fBufferGroup;
	fBufferGroup = NULL;
	
	BMessage settings;
	
	GetConfiguration( &settings );
	
	fAddOn->UnregisterNode( this, &settings );
	
	Quit();
}

/* BMediaNode */

port_id
CRadeonProducer::ControlPort() const
{
	return BMediaNode::ControlPort();
}

BMediaAddOn *
CRadeonProducer::AddOn(int32 *internal_id) const
{
	if (internal_id)
		*internal_id = fInternalID;
	return fAddOn;
}

status_t 
CRadeonProducer::HandleMessage(int32 message, const void *data, size_t size)
{
	//DPRINT(("CRadeonProducer::HandleMessage()\n"));
	
	switch( message ) {
		case C_GET_CONFIGURATION: {
			const configuration_msg *request = (const configuration_msg *)data;
			BMessage msg;
			configuration_msg_reply *reply;
			size_t reply_size, config_size;
			status_t res;
			
			if( size < sizeof( configuration_msg ))
				return B_ERROR;
			
			res = GetConfiguration( &msg );
						
			config_size = msg.FlattenedSize();
			reply_size = sizeof( *reply ) + config_size;
			reply = (configuration_msg_reply *)malloc( reply_size );
			if( reply == NULL )
				return B_NO_MEMORY;
		
			reply->res = res;
			reply->config_size = config_size;
			msg.Flatten( &reply->config, config_size );
			
			write_port_etc( request->reply_port, C_GET_CONFIGURATION_REPLY, 
				reply, reply_size, B_TIMEOUT, 0 );
			
			free( reply );
			return B_OK;
		}
		default:
			return B_ERROR;
	//	return BControllable::HandleMessage(message, data, size);
	}
}

void 
CRadeonProducer::Preroll()
{
	/* This hook may be called before the node is started to give the hardware
	 * a chance to start. */
	DPRINT(("CRadeonProducer::Preroll()\n"));
}

void
CRadeonProducer::SetTimeSource(BTimeSource *time_source)
{
	DPRINT(("CRadeonProducer::SetTimeSource()\n"));

	/* Tell frame generation thread to recalculate delay value */
	//release_sem(fFrameSync);
}

status_t
CRadeonProducer::RequestCompleted(const media_request_info &info)
{
	DPRINT(("CRadeonProducer::RequestCompleted()\n"));

	return BMediaNode::RequestCompleted(info);
}

/* BMediaEventLooper */

void 
CRadeonProducer::NodeRegistered()
{
	DPRINT(("CRadeonProducer::NodeRegistered()\n"));

	if (fInitStatus != B_OK) {
		ReportError(B_NODE_IN_DISTRESS);
		return;
	}

	setupWeb();
//!!//

	fOutput.node = Node();
	fOutput.source.port = ControlPort();
	fOutput.source.id = 0;

	/* Tailor these for the output of your device */
	/********
	fOutput.format.type = B_MEDIA_RAW_VIDEO;
	fOutput.format.u.raw_video = media_raw_video_format::wildcard;
//	fOutput.format.u.raw_video.interlace = 1;
	fOutput.format.u.raw_video.display.format = B_RGB32;
	********/
	
	// up to 60 frames (NTSC); jitter less then half a frame; processing time
	// depends on whether colour space conversion is required, let's say half 
	// a frame in worst case
	SetPriority( suggest_thread_priority( B_VIDEO_RECORDING, 60, 8000, 8000 ));
	
	/* Start the BMediaEventLooper control loop running */
	Run();
}

void
CRadeonProducer::Start(bigtime_t performance_time)
{
	DPRINT(("CRadeonProducer::Start()\n"));

	BMediaEventLooper::Start(performance_time);
}

void
CRadeonProducer::Stop(bigtime_t performance_time, bool immediate)
{
	DPRINT(("CRadeonProducer::Stop()\n"));

	BMediaEventLooper::Stop(performance_time, immediate);
}

void
CRadeonProducer::Seek(bigtime_t media_time, bigtime_t performance_time)
{
	DPRINT(("CRadeonProducer::Seek()\n"));

	BMediaEventLooper::Seek(media_time, performance_time);
}

void
CRadeonProducer::TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time)
{
	DPRINT(("CRadeonProducer::TimeWarp()\n"));

	BMediaEventLooper::TimeWarp(at_real_time, to_performance_time);
}

status_t
CRadeonProducer::AddTimer(bigtime_t at_performance_time, int32 cookie)
{
	DPRINT(("CRadeonProducer::AddTimer()\n"));

	return BMediaEventLooper::AddTimer(at_performance_time, cookie);
}

void
CRadeonProducer::SetRunMode(run_mode mode)
{
	DPRINT(("CRadeonProducer::SetRunMode()\n"));

	BMediaEventLooper::SetRunMode(mode);
}

void 
CRadeonProducer::HandleEvent(const media_timed_event *event,
		bigtime_t lateness, bool realTimeEvent)
{
	//DPRINT(("CRadeonProducer::HandleEvent()\n"));

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
		case BTimedEventQueue::B_HANDLE_BUFFER:
		case BTimedEventQueue::B_DATA_STATUS:
		case BTimedEventQueue::B_PARAMETER:
		default:
			PRINTF(-1, ("HandleEvent: Unhandled event -- %lx\n", event->type));
			break;
		case BTimedEventQueue::B_HARDWARE:
			HandleHardware();
			break;
	}
	
	//DPRINT(("CRadeonProducer::HandleEvent() done\n"));
}

void 
CRadeonProducer::CleanUpEvent(const media_timed_event *event)
{
	DPRINT(("CRadeonProducer::CleanUpEvent()\n"));

	BMediaEventLooper::CleanUpEvent(event);
}

bigtime_t
CRadeonProducer::OfflineTime()
{
	return BMediaEventLooper::OfflineTime();
}

void
CRadeonProducer::ControlLoop()
{
	BMediaEventLooper::ControlLoop();
}

status_t
CRadeonProducer::DeleteHook(BMediaNode * node)
{
	DPRINT(("CRadeonProducer::DeleteHook()\n"));

	return BMediaEventLooper::DeleteHook(node);
}



/* BBufferProducer */

// choose capture mode according to format and update format according to that
status_t
CRadeonProducer::verifySetMode( media_format *format )
{
	float frame_rate = fVideoIn.getFrameRate( 
		BeToVideoInStandard( fStandard )) / 1000.0f;
	
	if( format->u.raw_video.interlace == media_raw_video_format::wildcard.interlace ) {
		if( format->u.raw_video.field_rate == media_raw_video_format::wildcard.field_rate ) {
			format->u.raw_video.interlace = fMode == C_VIDEO_IN_BOB ? 2 : 1;
			format->u.raw_video.field_rate = frame_rate * format->u.raw_video.interlace;
		} else {
			if( format->u.raw_video.field_rate == frame_rate )
				format->u.raw_video.interlace = 1;
			else if( format->u.raw_video.field_rate == frame_rate * 2 )
				format->u.raw_video.interlace = 2;
			else {
				DPRINT(( "Unsupported field rate for active TV standard (%f)\n",
					format->u.raw_video.field_rate ));
				return B_MEDIA_BAD_FORMAT;
			}
		}
		
	} else if( format->u.raw_video.interlace == 1 ) {
		if( format->u.raw_video.field_rate == media_raw_video_format::wildcard.field_rate )
			format->u.raw_video.field_rate = frame_rate;
		else {
			// don't compare directly - there are rounding errors
			if( fabs(format->u.raw_video.field_rate - frame_rate) > 0.001 ) {
				DPRINT(( "Wrong field rate for active TV standard (%f) in progressive mode (expected %f)\n",
					format->u.raw_video.field_rate - 29.976,
					frame_rate - 29.976 ));
				return B_MEDIA_BAD_FORMAT;
			}
		}
		
	} else if( format->u.raw_video.interlace == 2 ) {
		if( format->u.raw_video.field_rate == media_raw_video_format::wildcard.field_rate )
			format->u.raw_video.field_rate = frame_rate * 2;
		else {
			if( fabs(format->u.raw_video.field_rate - frame_rate * 2) > 0.001 ) {
				DPRINT(( "Wrong field rate for active TV standard (%f) in interlace mode\n",
					format->u.raw_video.field_rate ));
				return B_MEDIA_BAD_FORMAT;
			}
		}
		
	} else {
		DPRINT(( "Invalid interlace mode (%d)\n", format->u.raw_video.interlace ));
		return B_MEDIA_BAD_FORMAT;
	}

	return B_OK;
}

/*
	Map BeOS capture mode to internal capture mode.
*/
int32
CRadeonProducer::extractCaptureMode( const media_format *format )
{
	// if application requests interlace, it always gets BOB;
	// if is requests non-interlace, it may get WEAVE or FIELD -
	// if the user selected one of them, we are fine; else,
	// we always choose WEAVE (could choose FIELD as well, make
	// it dependant on resolution, but the more magic the more problems)
	if( format->u.raw_video.interlace == 2 )
		return C_VIDEO_IN_BOB;
	else if( fMode == C_VIDEO_IN_BOB )
		return C_VIDEO_IN_WEAVE;
	else
		return fMode;
}

// check pixel aspect of format and set it if it's wildcarded
status_t
CRadeonProducer::verifySetPixelAspect( media_format *format )
{
	// for simplicity, we always assume 1:1 aspect
	if( format->u.raw_video.pixel_width_aspect != media_raw_video_format::wildcard.pixel_width_aspect ||
		format->u.raw_video.pixel_height_aspect != media_raw_video_format::wildcard.pixel_height_aspect )
	{
		if( format->u.raw_video.pixel_width_aspect !=
			format->u.raw_video.pixel_height_aspect ) 
		{
			DPRINT(( "Unsupported pixel aspect (%d:%d)\n", 
				format->u.raw_video.pixel_width_aspect,
				format->u.raw_video.pixel_height_aspect ));
			return B_MEDIA_BAD_FORMAT;
		}
	} else {
		format->u.raw_video.pixel_width_aspect = 1;
		format->u.raw_video.pixel_height_aspect = 1;
	}
	
	return B_OK;
	
#if 0
	// we assume 1:2 for interlaced and 1:1 for deinterlaced video
	// (this is not really true as it depends on TV standard and
	// resolution, but it should be enough for start)
	if( format->u.raw_video.pixel_width_aspect != media_raw_video_format::wildcard.pixel_width_aspect ||
		format->u.raw_video.pixel_height_aspect != media_raw_video_format::wildcard.pixel_height_aspect )
	{
		double ratio = mode == C_VIDEO_IN_WEAVE ? 1 : 0.5;
		
		if( (float)format->u.raw_video.pixel_width_aspect / 
			format->u.raw_video.pixel_height_aspect != ratio ) 
		{
			DPRINT(( "Unsupported pixel aspect (%d:%d)\n", 
				format->u.raw_video.pixel_width_aspect,
				format->u.raw_video.pixel_height_aspect ));
			return B_MEDIA_BAD_FORMAT;
		}
	} else {
		format->u.raw_video.pixel_width_aspect = 1;
		format->u.raw_video.pixel_height_aspect = 
			mode == C_VIDEO_IN_WEAVE ? 1 : 2;
	}
	
	return B_OK;
#endif
}

// verify active range defined as format
status_t
CRadeonProducer::verifyActiveRange( media_format *format )
{
	CRadeonRect active_rect;
	
	fVideoIn.getActiveRange( BeToVideoInStandard( fStandard ), active_rect );

	if( format->u.raw_video.first_active != media_raw_video_format::wildcard.first_active ) {
		if( (int32)format->u.raw_video.first_active < 0 ) {
			DPRINT(( "Unsupported first_active (%d)\n", format->u.raw_video.first_active ));
			return B_MEDIA_BAD_FORMAT;
		}
	}

	// don't care about last_active much - some programs set it to number of
	// captured lines, which is really something different 
	// (I have the feeling, noone really knows how to use this value properly)
	if( format->u.raw_video.last_active != media_raw_video_format::wildcard.last_active ) {
		if( format->u.raw_video.last_active >= (uint32)active_rect.Height() ) {
			DPRINT(( "Unsupported last_active (%d)\n", format->u.raw_video.last_active ));
			return B_MEDIA_BAD_FORMAT;
		}
	}
	
	return B_OK;
}


// set active range in format if yet undefined
void
CRadeonProducer::setActiveRange( media_format *format )
{
	CRadeonRect active_rect;
	
	fVideoIn.getActiveRange( BeToVideoInStandard( fStandard ), active_rect );

	if( format->u.raw_video.first_active == media_raw_video_format::wildcard.first_active )
		format->u.raw_video.first_active = 0;
	
	if( format->u.raw_video.last_active == media_raw_video_format::wildcard.last_active )
		format->u.raw_video.last_active = (uint32)active_rect.Height() - 1;
}


// verify requested orientation
status_t
CRadeonProducer::verifyOrientation( media_format *format )
{
	if( format->u.raw_video.orientation != media_raw_video_format::wildcard.orientation ) {
		if( format->u.raw_video.orientation != B_VIDEO_TOP_LEFT_RIGHT ) {
			DPRINT(( "Unsupported orientation (%d)\n", format->u.raw_video.orientation ));
			return B_MEDIA_BAD_FORMAT;
		}
	}
	
	return B_OK;
}


// set image orientation if yet undefined
void
CRadeonProducer::setOrientation( media_format *format )
{
	if( format->u.raw_video.orientation == media_raw_video_format::wildcard.orientation )
		format->u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
}


// verify requested pixel format
status_t
CRadeonProducer::verifyPixelFormat( media_format *format )
{
	if(	format->u.raw_video.display.format !=
		media_raw_video_format::wildcard.display.format )
	{
		switch( format->u.raw_video.display.format ) {
		case B_RGB32:
		case B_RGB16:
		case B_RGB15:
		case B_YCbCr422:
		case B_GRAY8:
			break;
			
		default:
			DPRINT(("Unsupported colour space (%x)\n", 
				format->u.raw_video.display.format ));
			return B_MEDIA_BAD_FORMAT;
		}
	}
	
	return B_OK;
}


// set pixel format to user-defined default if not set yet
void
CRadeonProducer::setPixelFormat( media_format *format )
{
	if(	format->u.raw_video.display.format ==
		media_raw_video_format::wildcard.display.format )
		format->u.raw_video.display.format = (color_space)fFormat;
}

/*
	Verify video size and set it if undefined.
*/
status_t
CRadeonProducer::verifySetSize( 
	media_format *format, int32 mode, bool set_bytes_per_row )
{
	CRadeonRect active_rect;
	
	fVideoIn.getActiveRange( BeToVideoInStandard( fStandard ), active_rect );
	
	// width and height must both be defined, else we define it ourself,
	// i.e. if the application leaves one of them wildcarded, we
	// set both
	if( format->u.raw_video.display.line_width !=
		media_raw_video_format::wildcard.display.line_width &&
		format->u.raw_video.display.line_count !=
		media_raw_video_format::wildcard.display.line_count )
	{
		uint32 max_height = active_rect.Height();
		
		if( mode != C_VIDEO_IN_WEAVE )
			max_height /= 2;
			
		if( format->u.raw_video.display.line_width > (uint32)active_rect.Width() ||
			format->u.raw_video.display.line_count > max_height )
		{
			DPRINT(("Requested image size is too high (%dx%d)\n", 
				format->u.raw_video.display.line_width,
				format->u.raw_video.display.line_count));
			return B_MEDIA_BAD_FORMAT;
		}
		
		// our format converters do up to 8 pixels at a time (grey8);
		// to be absolutely sure we don't get trouble there, refuse
		// any width that is not a multiple of 8
		
		if( (format->u.raw_video.display.line_width & 7) != 0 ) {
			DPRINT(( "Request image width is not multiple of 8 (%d)\n",
				format->u.raw_video.display.line_width ));
			return B_MEDIA_BAD_FORMAT;
		}
		
	} else {
		switch (fResolution) {
		case 0:
			format->u.raw_video.display.line_width = 640;
			format->u.raw_video.display.line_count = 480;
			break;
		case 3:
			format->u.raw_video.display.line_width = 480;
			format->u.raw_video.display.line_count = 360;
			break;
		case 4:
			format->u.raw_video.display.line_width = 720;
			format->u.raw_video.display.line_count = 480;
			break;
		case 5:
			format->u.raw_video.display.line_width = 720;
			format->u.raw_video.display.line_count = 576;
			break;
		case 6:
			format->u.raw_video.display.line_width = 768;
			format->u.raw_video.display.line_count = 576;
			break;
		case 1:
			format->u.raw_video.display.line_width = 320;
			format->u.raw_video.display.line_count = 240;
			break;
		case 2:
			format->u.raw_video.display.line_width = 160;
			format->u.raw_video.display.line_count = 120;
			break;
		}
		
		if( format->u.raw_video.display.line_width > (uint32)active_rect.Width() )
			format->u.raw_video.display.line_width = (uint32)active_rect.Width() & ~7;
			
		if( format->u.raw_video.display.line_count > (uint32)active_rect.Height() )
			format->u.raw_video.display.line_count = (uint32)active_rect.Height();
		
		// BOB and FIELD modes provide only field, which has half height
		if( mode != C_VIDEO_IN_WEAVE ) {
			if( format->u.raw_video.display.line_count > (uint32)active_rect.Height() / 2 )
				format->u.raw_video.display.line_count = (uint32)active_rect.Height() / 2;
		}
	}

	if( format->u.raw_video.display.format != media_raw_video_format::wildcard.display.format ) {
		uint32 bytes_per_row;
		
		switch( format->u.raw_video.display.format ) {
		case B_RGB32:
			bytes_per_row = 
				format->u.raw_video.display.line_width * 4;
			break;
			
		case B_RGB15:
		case B_RGB16:
		case B_YCbCr422:
			bytes_per_row = 
				format->u.raw_video.display.line_width * 2;
			break;
			
		default:
			bytes_per_row = 
				format->u.raw_video.display.line_width;
		}
		
		if( format->u.raw_video.display.bytes_per_row != 
			media_raw_video_format::wildcard.display.bytes_per_row )
		{
			if( format->u.raw_video.display.bytes_per_row < bytes_per_row ) {
				DPRINT(( "Requested bytes per row are too small",
					format->u.raw_video.display.bytes_per_row ));
				return B_MEDIA_BAD_FORMAT;
			}
		} else if( set_bytes_per_row )
			format->u.raw_video.display.bytes_per_row = bytes_per_row;
	}

	
	return B_OK;
}


// verify "offset" parameters of format
status_t 
CRadeonProducer::verifyFormatOffsets( media_format *format )
{
	if( format->u.raw_video.display.pixel_offset !=
		media_raw_video_format::wildcard.display.pixel_offset &&
		format->u.raw_video.display.pixel_offset != 0 )
	{
		DPRINT(( "Requested pixel offset is not zero" ));
		return B_MEDIA_BAD_FORMAT;
	}
	
	if( format->u.raw_video.display.line_offset !=
		media_raw_video_format::wildcard.display.line_offset &&
		format->u.raw_video.display.line_offset != 0 )
	{
		DPRINT(( "Requested line offset is not zero" ));
		return B_MEDIA_BAD_FORMAT;
	}

	return B_OK;
}


// set "offset" parameters of format if not set yet
void
CRadeonProducer::setFormatOffsets( media_format *format )
{
	if( format->u.raw_video.display.pixel_offset ==
		media_raw_video_format::wildcard.display.pixel_offset )
		format->u.raw_video.display.pixel_offset = 0;
	
	if( format->u.raw_video.display.line_offset ==
		media_raw_video_format::wildcard.display.line_offset )
		format->u.raw_video.display.line_offset = 0;
}


// verify "flags" parameter of format
status_t
CRadeonProducer::verifyFormatFlags( media_format *format )
{
	if( format->u.raw_video.display.flags !=
		media_raw_video_format::wildcard.display.flags &&
		format->u.raw_video.display.flags != 0 )
	{
		DPRINT(( "Requested display flags are not zero" ));
		return B_MEDIA_BAD_FORMAT;
	}
	
	return B_OK;
}


// set "flags" parameter of format if not set yet
void
CRadeonProducer::setFormatFlags( media_format *format )
{
	if( format->u.raw_video.display.flags ==
		media_raw_video_format::wildcard.display.flags )
		format->u.raw_video.display.flags = 0;
}


/*
 *	Fill out all wildcards in a format descriptor.
 */
status_t
CRadeonProducer::finalizeFormat( media_format *format )
{
	if (format->type != B_MEDIA_RAW_VIDEO)
		return B_MEDIA_BAD_FORMAT;

	status_t res;

	res = verifySetMode( format );
	if( res != B_OK )
		return res;
		
	int32 mode = extractCaptureMode( format );
			
	res = verifyActiveRange( format );	
	if( res != B_OK )
		return res;
		
	setActiveRange( format );

	res = verifyOrientation( format );
	if( res != B_OK )
		return res;

	res = verifySetPixelAspect( format );
	if( res != B_OK )
		return res;

	res = verifyPixelFormat( format );
	if( res != B_OK )
		return res;
		
	setPixelFormat( format);

	res = verifySetSize( format, mode, true );
	if( res != B_OK )
		return res;	

	res = verifyFormatOffsets( format );
	if( res != B_OK )
		return res;
		
	setFormatOffsets( format );

	res = verifyFormatFlags( format );
	if( res != B_OK )
		return res;	

	setFormatFlags( format );
	return res;	
}


/*
 *	Someone has no idea what format we usually provide and asks us.
 *	
 *	It's not really clear whether we are allowed to return wildcards.
 */
status_t 
CRadeonProducer::FormatSuggestionRequested(
	media_type type, int32 quality, media_format *format)
{
	DPRINT(("CRadeonProducer::FormatSuggestionRequested()\n"));

	if (type != B_MEDIA_RAW_VIDEO)
		return B_MEDIA_BAD_FORMAT;

	TOUCH(quality);

	format->type = B_MEDIA_RAW_VIDEO;
	
	format->u.raw_video = media_raw_video_format::wildcard;
	
	finalizeFormat( format );

	return B_OK;
}


/* 
	Initial format proposal as part of a connection establishment.
	
	First, the application defines a format with many wildcards in it;
	this format is passed to us, so we can restrict it if necessary;
	we should leave as many wildcards as possible, because in the next
	step the consumer is asked, and he will not be happy if he has no choice left .
*/
status_t 
CRadeonProducer::FormatProposal(const media_source &output, media_format *format)
{
	char buffer[256];
	
	DPRINT(("CRadeonProducer::FormatProposal()\n"));

	if( format == NULL )
		return B_BAD_VALUE;

	if( output != fOutput.source )
		return B_MEDIA_BAD_SOURCE;

	string_for_format(*format, buffer, sizeof(buffer));
	
	DPRINT(("CRadeonProducer::FormatProposal() - in=%s\n", buffer));

	if( format->type == B_MEDIA_NO_TYPE ) {
		// if there is not even a type, set raw video 
		format->type = B_MEDIA_RAW_VIDEO;
		format->u.raw_video = media_raw_video_format::wildcard;
	}
	
	if (format->type != B_MEDIA_RAW_VIDEO)
		return B_MEDIA_BAD_FORMAT;

	status_t res;
	
	// first, choose capture mode, so we know the maximum video size	
	res = verifySetMode( format );
	if( res != B_OK )
		return res;
		
	int32 mode = extractCaptureMode( format );
			
	res = verifyActiveRange( format );	
	if( res != B_OK )
		return res;

	res = verifyOrientation( format );
	if( res != B_OK )
		return res;
		
	setOrientation( format );

	// simple aspect calculation: we always use 1:1, so setting this is easy
	res = verifySetPixelAspect( format );
	if( res != B_OK )
		return res;

	res = verifyPixelFormat( format );
	if( res != B_OK )
		return res;
		
	// if we don't set it, the consumer usually chooses a stupid format
	setPixelFormat( format );

	// verify size and set if if undefined;
	// do that now, else the consumer will set it (making the defaults
	// set via preferences useless)
	// leave bytes_per_lines untouched, though 
	// (we don't really care but the consumer may have some alignment restrictions)
	res = verifySetSize( format, mode, false );
	if( res != B_OK )
		return res;	

	res = verifyFormatOffsets( format );
	if( res != B_OK )
		return res;

	res = verifyFormatFlags( format );
	if( res != B_OK )
		return res;	
		
	string_for_format(*format, buffer, sizeof(buffer));

	DPRINT(("CRadeonProducer::FormatProposal() - out=%s\n", buffer));

	return B_OK;
}

status_t 
CRadeonProducer::FormatChangeRequested(const media_source &source,
		const media_destination &destination, media_format *io_format,
		int32 *_deprecated_)
{
	DPRINT(("CRadeonProducer::FormatChangeRequested()\n"));

	TOUCH(destination); TOUCH(io_format); TOUCH(_deprecated_);
	if (source != fOutput.source)
		return B_MEDIA_BAD_SOURCE;
		
	return B_ERROR;	
}

status_t 
CRadeonProducer::GetNextOutput(int32 *cookie, media_output *out_output)
{
	DPRINT(("CRadeonProducer::GetNextOutput()\n"));

	if (!out_output)
		return B_BAD_VALUE;

	if ((*cookie) != 0)
		return B_BAD_INDEX;
	
	*out_output = fOutput;
	(*cookie)++;
	return B_OK;
}

status_t 
CRadeonProducer::DisposeOutputCookie(int32 cookie)
{
	DPRINT(("CRadeonProducer::DisposeOutputCookie()\n"));

	TOUCH(cookie);

	return B_OK;
}

status_t 
CRadeonProducer::SetBufferGroup(const media_source &for_source,
		BBufferGroup *group)
{
	DPRINT(("CRadeonProducer::SetBufferGroup()\n"));
	
	if( for_source != fOutput.source )
		return B_MEDIA_BAD_SOURCE;

	if( group != NULL ) {
		delete fBufferGroup;
		fBufferGroup = group;
	}
	
	return B_OK;
}

status_t 
CRadeonProducer::VideoClippingChanged(const media_source &for_source,
		int16 num_shorts, int16 *clip_data,
		const media_video_display_info &display, int32 *_deprecated_)
{
	DPRINT(("CRadeonProducer::VideoClippingChanged()\n"));

	TOUCH(for_source); TOUCH(num_shorts); TOUCH(clip_data);
	TOUCH(display); TOUCH(_deprecated_);

	return B_ERROR;
}

status_t 
CRadeonProducer::GetLatency(bigtime_t *out_latency)
{
	DPRINT(("CRadeonProducer::GetLatency()\n"));

	// delay is one frame for capturing, a scheduling latency, the
	// DMA copying, the format conversion and the output nodes latency;
	// scheduling, DMA copying and format conversion is summed up in
	// fProcessingLatency
	bigtime_t capture_latency = (bigtime_t)(1000000.0 / fOutput.format.u.raw_video.field_rate);
	
	// HACK: (see HandleHardware())
	// to be compatible to existing software, we write the ending time of
	// capture instead of the beginning time into buffers, thus we
	// have no capture delay
	capture_latency = 0;
	
	bigtime_t buffer_latency = fProcessingLatency;
	BBufferProducer::GetLatency( &buffer_latency );
	
	*out_latency = SchedulingLatency() + capture_latency + fProcessingLatency + 
		buffer_latency;
		
	DPRINT(("latency=%lld\n", *out_latency));
	
	return B_OK;
}


status_t 
CRadeonProducer::PrepareToConnect(const media_source &source,
		const media_destination &destination, media_format *format,
		media_source *out_source, char *out_name)
{
	DPRINT(("CRadeonProducer::PrepareToConnect()\n"));

	PRINTF(1, ("PrepareToConnect() %ldx%ld\n", \
			format->u.raw_video.display.line_width, \
			format->u.raw_video.display.line_count));

	if (source != fOutput.source) {
		DPRINT(("bad source\n"));
		return B_MEDIA_BAD_SOURCE;
	}
	
	if (fOutput.destination != media_destination::null) {
		DPRINT(("already connected\n"));
		return B_MEDIA_ALREADY_CONNECTED;
	}

	char buffer[256];	
	string_for_format(*format, buffer, sizeof(buffer));	
	DPRINT(("CRadeonProducer::PrepareToConnect() - in=%s\n", buffer));

	status_t res = finalizeFormat( format );
	if( res != B_OK )
		return res;

	*out_source = fOutput.source;
	strcpy(out_name, fOutput.name);
	
	string_for_format(*format, buffer, sizeof(buffer));
	DPRINT(("CRadeonProducer::PrepareToConnect() - out=%s\n", buffer));

	// reserve connection
	fOutput.destination = destination;
	
	DPRINT(("finished\n"));

	return B_OK;
}

void
CRadeonProducer::setDefaultBufferGroup()
{
	DPRINT(("CRadeonProducer::setDefaultBufferGroup()\n"));
	/*delete fBufferGroup;
	fBufferGroup = NULL;*/
	if( fBufferGroup != NULL ) {
		DPRINT(("Buffer already set\n"));
		return;
	}

	fBufferGroup = new BBufferGroup( 
		fOutput.format.u.raw_video.display.bytes_per_row *
		fOutput.format.u.raw_video.display.line_count, 
		3, B_ANY_ADDRESS, B_FULL_LOCK );
			
	if (fBufferGroup->InitCheck() < B_OK) {
		delete fBufferGroup;
		fBufferGroup = NULL;
		return;
	}
}

void
CRadeonProducer::startCapturing()
{
	if( RunState() != BMediaEventLooper::B_STARTED ||
		fOutput.destination == media_destination::null )
		return;

	fVideoIn.SetChannel(fTuner, C_VIDEO_IN_NTSC); // was hardcoded to NTSC
	fVideoIn.SetBrightness(fBrightness);
	fVideoIn.SetContrast(fContrast);
	fVideoIn.SetSaturation(fSaturation);
	fVideoIn.SetHue(fHue);
	fVideoIn.SetSharpness(fSharpness);
	
	fVideoIn.Start(video_in_source(fSource), BeToVideoInStandard( fStandard ),
		video_in_capture_mode(fCurMode),
		fOutput.format.u.raw_video.display.line_width,
		fOutput.format.u.raw_video.display.line_count);
	
	char *tmp_buffer;
	tmp_buffer = (char *)malloc( 
		fOutput.format.u.raw_video.display.bytes_per_row * 
		fOutput.format.u.raw_video.display.line_count );
		
	int field_sequence;
	short field_number;
	bigtime_t capture_time;
	
	// do a real capturing to prime everything
	fVideoIn.Capture(
		tmp_buffer != NULL ? fOutput.format.u.raw_video.display.format : B_NO_COLOR_SPACE,
		tmp_buffer,
		fOutput.format.u.raw_video.display.bytes_per_row * fOutput.format.u.raw_video.display.line_count,
		fOutput.format.u.raw_video.display.bytes_per_row,
		&field_sequence, &field_number, &capture_time );

	// capture some frames to be sure there are no pending buffers;
	// discard captured data to safe time (we want to catch up and not fall behind)
	for( int i = 0; i < 3; ++i ) {
		fVideoIn.Capture(
			B_NO_COLOR_SPACE,
			NULL,
			fOutput.format.u.raw_video.display.bytes_per_row * fOutput.format.u.raw_video.display.line_count,
			fOutput.format.u.raw_video.display.bytes_per_row,
			&field_sequence, &field_number, &capture_time );
			
		DPRINT(("Captured: %lld, current: %lld\n", capture_time, system_time() ));
	}
	
	// do a real capturing to see how long it takes until the
	// buffer is ready, i.e. including DMA and colour conversion
	fVideoIn.Capture(
		tmp_buffer != NULL ? fOutput.format.u.raw_video.display.format : B_NO_COLOR_SPACE,
		tmp_buffer,
		fOutput.format.u.raw_video.display.bytes_per_row * fOutput.format.u.raw_video.display.line_count,
		fOutput.format.u.raw_video.display.bytes_per_row,
		&field_sequence, &field_number, &capture_time );
		
	DPRINT(("Captured: %lld, current: %lld\n", capture_time, system_time() ));
		
	// now we know our internal latency
	fProcessingLatency = system_time() - capture_time;
	
	DPRINT(("Processing latency: %dµs\n", fProcessingLatency ));
	
	// store field sequence to start with zero
	// (capture-internal field sequence always counts up)
	fFieldSequenceBase = field_sequence;
	
	if( tmp_buffer != NULL )
		free(tmp_buffer);

	// tell latence MediaEventLooper so it can schedule events properly ahead
	bigtime_t total_latency;
	
	GetLatency( &total_latency );
	SetEventLatency( total_latency );

	// Create the buffer group
	setDefaultBufferGroup();
	
	//fUsedBufferGroup = fBufferGroup;
	
	// schedule a capture event after one field's time
	RealTimeQueue()->FlushEvents( 0, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HARDWARE );
	
	media_timed_event event(
		capture_time + 1000000 / fOutput.format.u.raw_video.field_rate,
		BTimedEventQueue::B_HARDWARE);
				
	RealTimeQueue()->AddEvent(event);
}

void 
CRadeonProducer::Connect(status_t error, const media_source &source,
		const media_destination &destination, const media_format &format,
		char *io_name)
{
	// we even get called if consumer reported error in AcceptFormat;
	// in this case, we must release the source already reserved by
	// PrepareToConnect
	if( error != B_OK ) {
		DPRINT(( "Connect: Consumer returned error (%s) - releasing source", 
			strerror( error )));
		fOutput.destination = media_destination::null;
		return;
	}
	
	if(	source != fOutput.source ) {
		DPRINT(( "Connect: Wrong source specified\n"));
		return;
	}
	
	fOutput.destination = destination;
	fOutput.format = format;
	fCurMode = extractCaptureMode( &format );

	char buffer[256];
	string_for_format(format, buffer, sizeof(buffer));

	DPRINT(("CRadeonProducer::Connect() - %s\n", buffer));

	strcpy(io_name, fOutput.name);

	startCapturing();
	
	DPRINT(("CRadeonProducer::Connect() done\n"));
}

void 
CRadeonProducer::Disconnect(const media_source &source,
		const media_destination &destination)
{
	DPRINT(("Disconnect()\n"));
	
	if( source != fOutput.source || destination != fOutput.destination ) {
		DPRINT(("Disconnect: Bad source and/or destination\n"));
		return;
	}

	fOutput.destination = media_destination::null;

	delete fBufferGroup;
	fBufferGroup = NULL;

	fVideoIn.Stop();
	
	RealTimeQueue()->FlushEvents( 0, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HARDWARE );
	
	// reset format to get rid of any connection left-overs
	fOutput.format.type = B_MEDIA_RAW_VIDEO;
	fOutput.format.u.raw_video = media_raw_video_format::wildcard;
	
	DPRINT(("Disconnect() done\n"));
}

void 
CRadeonProducer::LateNoticeReceived(const media_source &source,
		bigtime_t how_much, bigtime_t performance_time)
{
	DPRINT(("CRadeonProducer::LateNoticeReceived()\n"));

	TOUCH(source); TOUCH(how_much); TOUCH(performance_time);
}

void 
CRadeonProducer::EnableOutput(const media_source &source, bool enabled,
		int32 *_deprecated_)
{
	DPRINT(("CRadeonProducer::EnableOutput()\n"));

	TOUCH(_deprecated_);

	if (source != fOutput.source)
		return;

	fEnabled = enabled;
}

status_t 
CRadeonProducer::SetPlayRate(int32 numer, int32 denom)
{
	DPRINT(("CRadeonProducer::SetPlayRate()\n"));

	TOUCH(numer); TOUCH(denom);

	return B_ERROR;
}

void 
CRadeonProducer::AdditionalBufferRequested(const media_source &source,
		media_buffer_id prev_buffer, bigtime_t prev_time,
		const media_seek_tag *prev_tag)
{
	DPRINT(("CRadeonProducer::AdditionalBufferRequested()\n"));

	TOUCH(source); TOUCH(prev_buffer); TOUCH(prev_time); TOUCH(prev_tag);
}

void 
CRadeonProducer::LatencyChanged(const media_source &source,
		const media_destination &destination, bigtime_t new_latency,
		uint32 flags)
{
	DPRINT(("CRadeonProducer::LatencyChanged()\n"));
	BBufferProducer::LatencyChanged( source, destination, new_latency, flags );
}



/* BControllable */									

status_t 
CRadeonProducer::GetParameterValue(
	int32 id, bigtime_t *last_change, void *value, size_t *size)
{
	DPRINT(("CRadeonProducer::GetParameterValue(%d)\n", id));

	switch (id) {
	case P_SOURCE:
		*last_change = fSourceLastChange;
		*size = sizeof(fSource);
		*((uint32 *) value) = fSource;
		break;
	case P_STANDARD:
		*last_change = fStandardLastChange;
		*size = sizeof(fStandard);
		*((uint32 *) value) = fStandard;
		break;
	case P_MODE:
		*last_change = fModeLastChange;
		*size = sizeof(fMode);
		*((uint32 *) value) = fMode;
		break;
	case P_FORMAT:
		*last_change = fFormatLastChange;
		*size = sizeof(fFormat);
		*((uint32 *) value) = fFormat;
		break;
	case P_RESOLUTION:
		*last_change = fResolutionLastChange;
		*size = sizeof(fResolution);
		*((uint32 *) value) = fResolution;
		break;
	case P_TUNER:
		*last_change = fTunerLastChange;
		*size = sizeof(fTuner);
		*((uint32 *) value) = fTuner;
		break;
	case P_BRIGHTNESS:
		*last_change = fBrightnessLastChange;
		*size = sizeof(fBrightness);
		*((float *) value) = fBrightness;
		break;
	case P_CONTRAST:
		*last_change = fContrastLastChange;
		*size = sizeof(fContrast);
		*((float *) value) = fContrast;
		break;
	case P_SATURATION:
		*last_change = fSaturationLastChange;
		*size = sizeof(fSaturation);
		*((float *) value) = fSaturation;
		break;
	case P_HUE:
		*last_change = fHueLastChange;
		*size = sizeof(fHue);
		*((float *) value) = fHue;
		break;
	case P_SHARPNESS:
		*last_change = fSharpnessLastChange;
		*size = sizeof(fSharpness);
		*((float *) value) = fSharpness;
		break;
	default:
		DPRINT(("Unknown parameter\n"));
		return B_BAD_VALUE;
	}

	return B_OK;
}


/*
 *	Change video format instantly.
 *
 *	Used when user changes a settings that affect the video format.
 *	The new format must be the current format with some values 
 *	replaced with wildcards. Don't put too many wildcards:
 *	some settings (like video size) are normally chosen by the 
 *	application, so don't use this function as a secret override.
 */
void
CRadeonProducer::instantFormatChange( media_format *new_format )
{
	if( fOutput.destination == media_destination::null )
		return;
		
	if( finalizeFormat( new_format ) != B_OK ) {
		DPRINT(("Current format does not allow to change interlace mode on-the-fly\n"));
		return;
	}
			
	if( ChangeFormat( fOutput.source, fOutput.destination, new_format ) != B_OK ) {
		DPRINT(("Consumer does not allow to change interlace mode instantly\n"));
		return;
	}
	
	fOutput.format = *new_format;
	fCurMode = extractCaptureMode( new_format );

	fVideoIn.Stop();
	startCapturing();
}

void
CRadeonProducer::SetParameterValue(
	int32 id, bigtime_t when, const void *value, size_t size)
{
	DPRINT(("CRadeonProducer::SetParameterValue()\n"));

	if (!value || size != sizeof(uint32))
		return;
		
	switch (id) {
	case P_SOURCE:
		if (*((const int32 *) value) == fSource)
			return;
		fSource = *((const uint32 *) value);
		fSourceLastChange = when;
		
		// if there is no tuner, force composite input
		// (eXposer sets source manually to tuner, even if there is none)
		// if there is no tuner, it isn't in the list and can't be picked!
		//if( (fVideoIn.Capabilities() & C_VIDEO_IN_HAS_TUNER) == 0 )
		//	fSource = C_VIDEO_IN_COMPOSITE;
		
		break;
	case P_STANDARD: {
		if (*((const int32 *) value) == fStandard)
			return;

		fStandard = BeToVideoInStandard( *((const int32 *) value) );
		
		fStandardLastChange = when;

		media_format new_format = fOutput.format;
		
		new_format.u.raw_video.field_rate = media_raw_video_format::wildcard.field_rate;
		new_format.u.raw_video.interlace = media_raw_video_format::wildcard.interlace;
		new_format.u.raw_video.pixel_width_aspect = media_raw_video_format::wildcard.pixel_width_aspect;
		new_format.u.raw_video.pixel_height_aspect = media_raw_video_format::wildcard.pixel_height_aspect;
		new_format.u.raw_video.first_active = media_raw_video_format::wildcard.first_active;
		new_format.u.raw_video.last_active = media_raw_video_format::wildcard.last_active;

		instantFormatChange( &new_format );		
		break; }
	case P_MODE: {
		if (*((const int32 *) value) == fMode)
			return;
			
		fMode = *((const uint32 *) value);
		fModeLastChange = when;
			
		media_format new_format = fOutput.format;
		
		new_format.u.raw_video.field_rate = media_raw_video_format::wildcard.field_rate;
		new_format.u.raw_video.interlace = media_raw_video_format::wildcard.interlace;
		new_format.u.raw_video.pixel_width_aspect = media_raw_video_format::wildcard.pixel_width_aspect;
		new_format.u.raw_video.pixel_height_aspect = media_raw_video_format::wildcard.pixel_height_aspect;

		instantFormatChange( &new_format );		
		
		break; }
	case P_FORMAT: {
		if (*((const int32 *) value) == fFormat)
			return;
		fFormat = *((const uint32 *) value);
		fFormatLastChange = when;
		
		media_format new_format = fOutput.format;
		
		new_format.u.raw_video.display.format = media_raw_video_format::wildcard.display.format;
		new_format.u.raw_video.display.bytes_per_row = media_raw_video_format::wildcard.display.bytes_per_row;

		instantFormatChange( &new_format );		
		break; }
	case P_RESOLUTION:
		if (*((const int32 *) value) == fResolution)
			return;
		fResolution = *((const uint32 *) value);
		fResolutionLastChange = when;
		// no live update - see instantFormatChange()
		break;
	case P_TUNER:
		if (*((const int32 *) value) == fTuner)
			return;
		fTuner = *((const uint32 *) value);
		fTunerLastChange = when;
		fVideoIn.SetChannel(fTuner, C_VIDEO_IN_NTSC); // was hardcoded to NTSC
		break;
	case P_BRIGHTNESS:
		if (*((const float *) value) == fBrightness)
			return;
		fBrightness = (int32)*((const float *) value);
		fBrightnessLastChange = when;
		fVideoIn.SetBrightness(fBrightness);
		break;
	case P_CONTRAST:
		if (*((const float *) value) == fContrast)
			return;
		fContrast = (int32)*((const float *) value);
		fContrastLastChange = when;
		fVideoIn.SetContrast(fContrast);
		break;
	case P_SATURATION:
		if (*((const float *) value) == fSaturation)
			return;
		fSaturation = (int32)*((const float *) value);
		fSaturationLastChange = when;
		fVideoIn.SetSaturation(fSaturation);
		break;
	case P_HUE:
		if (*((const float *) value) == fHue)
			return;
		fHue = (int32)*((const float *) value);
		fHueLastChange = when;
		fVideoIn.SetHue(fHue);
		break;
	case P_SHARPNESS:
		if (*((const float *) value) == fSharpness)
			return;
		fSharpness = (int32)*((const float *) value);
		fSharpnessLastChange = when;
		fVideoIn.SetSharpness(fSharpness);
		break;
	default:
		return;
	}

	BroadcastNewParameterValue(when, id, const_cast<void *>(value), sizeof(uint32));
}

status_t
CRadeonProducer::StartControlPanel(BMessenger *out_messenger)
{
	return BControllable::StartControlPanel(out_messenger);
}

status_t CRadeonProducer::AddInt32( 
	BMessage *msg, EOptions option, int32 value )
{
	char name[5];
	
	*(int32 *)name = option;
	name[4] = 0;
	
	return msg->AddInt32( name, value );
}

status_t
CRadeonProducer::GetConfiguration( BMessage *out )
{
	status_t res;
	
	if( (res = AddInt32( out, P_SOURCE, fSource )) != B_OK ||
		(res = AddInt32( out, P_STANDARD, BeToVideoInStandard( fStandard ))) != B_OK ||
		(res = AddInt32( out, P_MODE, fMode )) != B_OK ||
		(res = AddInt32( out, P_FORMAT, fFormat )) != B_OK ||
		(res = AddInt32( out, P_RESOLUTION, fResolution )) != B_OK ||
		(res = AddInt32( out, P_TUNER, fTuner )) != B_OK ||
		(res = AddInt32( out, P_BRIGHTNESS, fBrightness )) != B_OK ||
		(res = AddInt32( out, P_CONTRAST, fContrast )) != B_OK ||
		(res = AddInt32( out, P_SATURATION, fSaturation )) != B_OK ||
		(res = AddInt32( out, P_HUE, fHue )) != B_OK ||
		(res = AddInt32( out, P_SHARPNESS, fSharpness )) != B_OK )
		return res;
		
	return B_OK;
}


/* VideoProducer */

void
CRadeonProducer::HandleStart(bigtime_t performance_time)
{
	/* Start producing frames, even if the output hasn't been connected yet. */
	DPRINT(("CRadeonProducer::HandleStart()\n"));
	
	if( RunState() != BMediaEventLooper::B_STOPPED ) {
		DPRINT(("already running\n"));
		return;
	}
	
	SetRunState( BMediaEventLooper::B_STARTED );
	
	startCapturing();
}

void
CRadeonProducer::HandleStop(void)
{
	DPRINT(("CRadeonProducer::HandleStop()\n"));

	fVideoIn.Stop();

	// discard pending capture event
	RealTimeQueue()->FlushEvents( 0, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HARDWARE );
}

void
CRadeonProducer::HandleTimeWarp(bigtime_t performance_time)
{
	DPRINT(("CRadeonProducer::HandleTimeWarp()\n"));
}

void
CRadeonProducer::HandleSeek(bigtime_t performance_time)
{
	DPRINT(("CRadeonProducer::HandleSeek()\n"));
}

void 
CRadeonProducer::captureField( bigtime_t *capture_time )
{
	*capture_time = system_time();
	
	// don't capture if output is disabled
	if (!fEnabled)
		return;
		
	BBuffer *buffer = fBufferGroup->RequestBuffer(
		fOutput.format.u.raw_video.display.bytes_per_row *
		fOutput.format.u.raw_video.display.line_count, 0LL);
		
	if (!buffer) {
		DPRINT(( "No buffer\n" ));
		return;
	}
		
	media_header *h = buffer->Header();
	h->type = B_MEDIA_RAW_VIDEO;
	h->time_source = TimeSource()->ID();
	h->size_used = fOutput.format.u.raw_video.display.bytes_per_row *
					fOutput.format.u.raw_video.display.line_count;
	h->file_pos = 0;
	h->orig_size = 0;
	h->data_offset = 0;
	h->u.raw_video.field_gamma = 1.0;
	h->u.raw_video.pulldown_number = 0;
	h->u.raw_video.first_active_line = fOutput.format.u.raw_video.first_active;
	h->u.raw_video.line_count = fOutput.format.u.raw_video.display.line_count;

	int field_sequence;
	short field_number;
	
	int dropped=fVideoIn.Capture(
		fOutput.format.u.raw_video.display.format,
		buffer->Data(),
		fOutput.format.u.raw_video.display.bytes_per_row * fOutput.format.u.raw_video.display.line_count,
		fOutput.format.u.raw_video.display.bytes_per_row,
		&field_sequence,
		&field_number,
		capture_time);
	
	// HACK: store end instead of start time
	// obviously, the _start_ time of a frame is always in the past by one
	// frame; unfortunately, programs like stamptv expect the start time to
	// be in the present, else they drop the frame; therefore, we write the
	// _end_ time into the start time field, and everyone is happy (though
	// the time is wrong)
	// this leads to many tweaks in the code!
	h->start_time = TimeSource()->PerformanceTimeFor( *capture_time );	
	
	h->u.raw_video.field_sequence = field_sequence - fFieldSequenceBase;
	h->u.raw_video.field_number = field_number;

	if (dropped > 1) {
		PRINT(("%d frames dropped\n", dropped-1));
	}
	
	if (SendBuffer(buffer, fOutput.destination) < B_OK) {
		PRINTF(-1, ("FrameGenerator: Error sending buffer\n"));
		buffer->Recycle();
	}
}

void
CRadeonProducer::HandleHardware()
{
	bigtime_t capture_time;
	
	//DPRINT(("Hi\n"));
	
	if( RunState() != BMediaEventLooper::B_STARTED )
		return;
		
	captureField( &capture_time );
		
	// generate next event after next field
	media_timed_event event(
		capture_time + 1000000 / fOutput.format.u.raw_video.field_rate,
		BTimedEventQueue::B_HARDWARE);
				
	RealTimeQueue()->AddEvent(event);
}
