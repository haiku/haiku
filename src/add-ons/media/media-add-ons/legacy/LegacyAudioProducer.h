#ifndef _LEGACY_AUDIO_PRODUCER_H
#define _LEGACY_AUDIO_PRODUCER_H

#include <kernel/OS.h>
#include <media/BufferProducer.h>
#include <media/Controllable.h>
#include <media/MediaAddOn.h>
#include <media/MediaDefs.h>
#include <media/MediaEventLooper.h>
#include <media/MediaNode.h>
#include <support/Locker.h>

class LegacyAudioProducer : public virtual BMediaEventLooper,
							public virtual BBufferProducer,
							public virtual BControllable
{
	public:
							LegacyAudioProducer( BMediaAddOn *addon, const char *name, int32 internal_id );
		virtual				~LegacyAudioProducer();
		virtual	status_t	InitCheck() const { return fInitStatus; }

	/* BMediaNode */
	public:
		virtual port_id		ControlPort() const;
		virtual	BMediaAddOn	*AddOn( int32 *internal_id ) const;
		virtual	status_t	HandleMessage( int32 message, const void *data, size_t size );

	protected:
		virtual	void 		Preroll() {}
		virtual void		SetTimeSource( BTimeSource *time_source );
		virtual status_t	RequestCompleted( const media_request_info &info );

	/* BMediaEventLooper */
	protected:
		virtual	void 		NodeRegistered();
		virtual void		Start( bigtime_t performance_time );
		virtual void		Stop( bigtime_t performance_time, bool immediate );
		virtual void		Seek( bigtime_t media_time, bigtime_t performance_time );
		virtual void		TimeWarp( bigtime_t at_real_time, bigtime_t to_performance_time );
		virtual status_t	AddTimer( bigtime_t at_performance_time, int32 cookie );
		virtual void		SetRunMode( run_mode mode );
		virtual void		HandleEvent( const media_timed_event *event, bigtime_t lateness, bool realTimeEvent = false );
		virtual void		CleanUpEvent( const media_timed_event *event );
		virtual bigtime_t	OfflineTime();
		virtual void		ControlLoop();
		virtual status_t	DeleteHook( BMediaNode *node );

	/* BBufferProducer */									
	protected:
		virtual	status_t	FormatSuggestionRequested( media_type type, int32 quality, media_format *format );
		virtual	status_t 	FormatProposal( const media_source &output, media_format *format );
		virtual	status_t	FormatChangeRequested( const media_source &source, const media_destination &destination,
												   media_format *io_format, int32 *_deprecated_ );
		virtual	status_t 	GetNextOutput( int32 *cookie, media_output *out_output );
		virtual	status_t	DisposeOutputCookie( int32 cookie );
		virtual	status_t	SetBufferGroup( const media_source &for_source, BBufferGroup *group );
		virtual	status_t 	VideoClippingChanged( const media_source &for_source, int16 num_shorts, int16 *clip_data,
							const media_video_display_info &display, int32 *_deprecated_ )
								{ return B_ERROR; }
		virtual	status_t	GetLatency( bigtime_t *out_latency );
		virtual	status_t	PrepareToConnect( const media_source &what, const media_destination &where, media_format *format,
											  media_source *out_source, char *out_name);
		virtual	void		Connect( status_t error, const media_source &source, const media_destination &destination,
									 const media_format &format, char *io_name );
		virtual	void 		Disconnect( const media_source &what, const media_destination &where );
		virtual	void 		LateNoticeReceived( const media_source &what, bigtime_t how_much, bigtime_t performance_time );
		virtual	void 		EnableOutput( const media_source &what, bool enabled, int32 *_deprecated_ );
		virtual	status_t	SetPlayRate( int32 numer,int32 denom );
		virtual	void 		AdditionalBufferRequested( const media_source & source, media_buffer_id prev_buffer,
													   bigtime_t prev_time, const media_seek_tag *prev_tag );
		virtual	void		LatencyChanged( const media_source &source, const media_destination &destination,
											bigtime_t new_latency, uint32 flags );

	/* BControllable */									
	protected:
		virtual status_t	GetParameterValue( int32 id, bigtime_t *last_change, void *value, size_t *size );
		virtual void		SetParameterValue( int32 id, bigtime_t when, const void *value, size_t size );
		virtual status_t	StartControlPanel( BMessenger *out_messenger )
								{ return B_ERROR; }

	/* state */
	private:
		void				HandleStart( bigtime_t performance_time );
		void				HandleStop();
		void				HandleTimeWarp( bigtime_t performance_time );
		void				HandleSeek( bigtime_t performance_time );

		status_t			fInitStatus;

		int32				fInternalID;
		BMediaAddOn			*fAddOn;

		BLocker				fLock;
		BBufferGroup		*fBufferGroup;

		thread_id			fThread;
		sem_id				fFrameSync;

		static	int32		_run_thread_( void *data );
		int32				RunThread();

#if 0
		/* The remaining variables should be declared volatile, but they
		 * are not here to improve the legibility of the sample code. */
		uint32				fFrame;
		uint32				fFrameBase;
		bigtime_t			fPerformanceTimeBase;
		bigtime_t			fProcessingLatency;
		media_output		fOutput;
		media_raw_video_format	fConnectedFormat;
		bool				fRunning;
		bool				fConnected;
		bool				fEnabled;

		enum				{ P_COLOR };
		uint32				fColor;
		bigtime_t			fLastColorChange;
#endif
};

#endif
