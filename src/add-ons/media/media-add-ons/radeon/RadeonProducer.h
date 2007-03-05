/******************************************************************************
/
/	File:			RadeonProducer.h
/
/	Description:	ATI Radeon Video Producer media node.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#ifndef __RADEON_PRODUCER_H__
#define __RADEON_PRODUCER_H__

#include <kernel/OS.h>
#include <media/BufferProducer.h>
#include <media/Controllable.h>
#include <media/MediaDefs.h>
#include <media/MediaEventLooper.h>
#include <media/MediaNode.h>
#include <support/Locker.h>
#include "VideoIn.h"

class CRadeonAddOn;

class CRadeonProducer :
	public virtual BMediaEventLooper,
	public virtual BBufferProducer,
	public virtual BControllable
{
public:
	CRadeonProducer(CRadeonAddOn *addon, const char *name, const char *device_name, 
					int32 internal_id, BMessage *config );
	virtual			~CRadeonProducer();

	void			setupWeb();
	virtual status_t	InitCheck() const { return fInitStatus; }

/* BMediaNode */
public:
	virtual port_id		ControlPort() const;
	virtual BMediaAddOn *AddOn(int32 * internal_id) const;
	virtual status_t	HandleMessage(int32 message, const void *data,
					size_t size);
protected:	
	virtual void		Preroll();
	virtual void		SetTimeSource(BTimeSource * time_source);
	virtual status_t	RequestCompleted(const media_request_info & info);

/* BMediaEventLooper */
protected:
	virtual void 		NodeRegistered();
	virtual void		Start(bigtime_t performance_time);
	virtual void		Stop(bigtime_t performance_time, bool immediate);
	virtual void		Seek(bigtime_t media_time, bigtime_t performance_time);
	virtual void		TimeWarp(bigtime_t at_real_time,
								bigtime_t to_performance_time);
	virtual status_t	AddTimer(bigtime_t at_performance_time, int32 cookie);
	virtual void		SetRunMode(run_mode mode);
	virtual void		HandleEvent(const media_timed_event *event,
								bigtime_t lateness, bool realTimeEvent = false);
	virtual void		CleanUpEvent(const media_timed_event *event);
	virtual bigtime_t	OfflineTime();
	virtual void		ControlLoop();
	virtual status_t	DeleteHook(BMediaNode * node);

/* BBufferProducer */									
protected:
	virtual	status_t	FormatSuggestionRequested(media_type type, int32 quality,
								media_format * format);
	virtual	status_t 	FormatProposal(const media_source &output,
								media_format *format);
	virtual	status_t	FormatChangeRequested(const media_source &source,
								const media_destination &destination,
								media_format *io_format, int32 *_deprecated_);
	virtual	status_t 	GetNextOutput(int32 * cookie, media_output * out_output);
	virtual	status_t	DisposeOutputCookie(int32 cookie);
	virtual	status_t	SetBufferGroup(const media_source &for_source,
								BBufferGroup * group);
	virtual	status_t 	VideoClippingChanged(const media_source &for_source,
								int16 num_shorts, int16 *clip_data,
								const media_video_display_info &display,
								int32 * _deprecated_);
	virtual	status_t	GetLatency(bigtime_t * out_latency);
	virtual	status_t	PrepareToConnect(const media_source &what,
								const media_destination &where,
								media_format *format,
								media_source *out_source, char *out_name);
	virtual	void		Connect(status_t error, const media_source &source,
								const media_destination &destination,
								const media_format & format, char *io_name);
	virtual	void 		Disconnect(const media_source & what,
								const media_destination & where);
	virtual	void 		LateNoticeReceived(const media_source & what,
								bigtime_t how_much, bigtime_t performance_time);
	virtual	void 		EnableOutput(const media_source & what, bool enabled,
								int32 * _deprecated_);
	virtual	status_t	SetPlayRate(int32 numer,int32 denom);
	virtual	void 		AdditionalBufferRequested(const media_source & source,
								media_buffer_id prev_buffer, bigtime_t prev_time,
								const media_seek_tag * prev_tag);
	virtual	void		LatencyChanged(const media_source & source,
								const media_destination & destination,
								bigtime_t new_latency, uint32 flags);

/* BControllable */									
protected:
	virtual status_t	GetParameterValue(int32 id, bigtime_t *last_change,
								void *value, size_t *size);
	virtual void		SetParameterValue(int32 id, bigtime_t when,
								const void *value, size_t size);
	virtual status_t	StartControlPanel(BMessenger *out_messenger);

public:
	enum {
		C_GET_CONFIGURATION = BTimedEventQueue::B_USER_EVENT,
		C_GET_CONFIGURATION_REPLY
	};
	
	struct configuration_msg {
		port_id reply_port;
	};
	
	struct configuration_msg_reply {
		status_t 		res;
		size_t			config_size;
		char			config;
	};

/* state */
private:
	void				HandleStart(bigtime_t performance_time);
	void				HandleStop();
	void				HandleTimeWarp(bigtime_t performance_time);
	void				HandleSeek(bigtime_t performance_time);
	void				HandleHardware();
	
	// home-brewed extension
	status_t			GetConfiguration( BMessage *out );
	
	CVideoIn			fVideoIn;
	
	status_t			fInitStatus;

	int32				fInternalID;
	CRadeonAddOn		*fAddOn;

	BBufferGroup		*fBufferGroup;
	//BBufferGroup		*fUsedBufferGroup;
		
	static	int32				_frame_generator_(void *data);
	int32				FrameGenerator();

	/* The remaining variables should be declared volatile, but they
	 * are not here to improve the legibility of the sample code. */
	//uint32				fFrame;
	uint32				fFieldSequenceBase;
	//bigtime_t			fPerformanceTimeBase;
	bigtime_t			fProcessingLatency;
	media_output		fOutput;
	//media_raw_video_format	fConnectedFormat;
	//bool				fConnected;
	bool				fEnabled;

	// use fixed names as they are used in settings file
	enum EOptions		{ 
		P_SOURCE		= 'VSRC', 
		P_AUDIO_SOURCE	= 'ASRC',
		P_AUDIO_FORMAT	= 'AFMT',
		P_VOLUME		= 'VOL ',
		P_STANDARD		= 'TVST',
		P_MODE			= 'CMOD',
		P_FORMAT		= 'VFMT',
		P_RESOLUTION	= 'RES ',
		P_TUNER			= 'TUNR',
		P_BRIGHTNESS	= 'BRGT',
		P_CONTRAST		= 'CONT',
		P_SATURATION	= 'SATU',
		P_HUE			= 'HUE ',
		P_SHARPNESS		= 'SHRP' 
	};
	
	enum { C_RESOLUTION_MAX = 6 };
	enum { C_CHANNEL_MAX = 125 };
	
	int32				fSource;
	int32				fStandard;
	int32				fMode;
	int32				fCurMode;	// mode, overwritten by application
	int32				fFormat;
	int32				fResolution;
	int32				fTuner;		
	int32				fBrightness;
	int32				fContrast;
	int32				fSaturation;
	int32				fHue;
	int32				fSharpness;
	bigtime_t			fSourceLastChange;
	bigtime_t			fStandardLastChange;
	bigtime_t			fModeLastChange;
	bigtime_t			fFormatLastChange;
	bigtime_t			fResolutionLastChange;
	bigtime_t			fTunerLastChange;
	bigtime_t			fBrightnessLastChange; 
	bigtime_t			fContrastLastChange;
	bigtime_t			fSaturationLastChange;
	bigtime_t			fHueLastChange;
	bigtime_t			fSharpnessLastChange;

	status_t	AddInt32( 
		BMessage *msg, EOptions option, int32 value );
		
	status_t	FindInt32( 
		BMessage *config, EOptions option, int32 min_value, int32 max_value, 
		int32 default_value, int32 *value );

	// format negotiation helpers
	status_t	verifySetMode( media_format *format );
	int32		extractCaptureMode( const media_format *format );
	status_t 	verifySetPixelAspect( media_format *format );
	status_t 	verifyActiveRange( media_format *format );
	void		setActiveRange( media_format *format );
	status_t	verifyOrientation( media_format *format );
	void		setOrientation( media_format *format );
	status_t	verifyPixelFormat( media_format *format );
	void		setPixelFormat( media_format *format );
	status_t	verifySetSize( media_format *format, int32 mode, bool set_bytes_per_row );
	status_t 	verifyFormatOffsets( media_format *format );
	void		setFormatOffsets( media_format *format );
	status_t	verifyFormatFlags( media_format *format );
	void		setFormatFlags( media_format *format );
	
	status_t	finalizeFormat( media_format *format );
	
	void		startCapturing();
	void	 	captureField( bigtime_t *capture_time );
	void		setDefaultBufferGroup();
	void		instantFormatChange( media_format *new_format );
};

#endif
