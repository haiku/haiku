#ifndef _LEGACY_AUDIO_CONSUMER_H
#define _LEGACY_AUDIO_CONSUMER_H

#include <kernel/OS.h>
#include <media/BufferConsumer.h>
#include <media/Controllable.h>
#include <media/MediaEventLooper.h>
#include <media/MediaDefs.h>
#include <media/MediaAddOn.h>
#include <media/MediaNode.h>
#include <support/Locker.h>

struct audio_buffer_header;

class LegacyAudioConsumer :
	public virtual BMediaEventLooper,
	public virtual BBufferConsumer
{
	public:
							LegacyAudioConsumer( BMediaAddOn *addon, const char *name, int32 internal_id );
							~LegacyAudioConsumer();

		virtual	status_t	InitCheck() const
								{ return mInitStatus; }

	/* BMediaNode */
	public:
		virtual	BMediaAddOn	*AddOn( int32 *internal_id ) const;

	protected:	
		virtual void		NodeRegistered();
		virtual	status_t 	HandleMessage( int32 message,
										   const void *data,
										   size_t size );

	/* BMediaEventLooper */
	protected:
		virtual void		HandleEvent( const media_timed_event *event,
										 bigtime_t lateness,
										 bool realTimeEvent = false );


	/* BBufferConsumer */									
	public:
		virtual	status_t	AcceptFormat( const media_destination &dest,
										  media_format *format );

		virtual	status_t	GetNextInput( int32 *cookie,
		                                  media_input *out_input );

		virtual	void		DisposeInputCookie( int32 cookie ) {}

	protected:
		virtual	void		BufferReceived( BBuffer *buffer );

	private:
		virtual	void		ProducerDataStatus( const media_destination &for_whom,
												int32 status,
												bigtime_t at_performance_time );

		virtual	status_t	GetLatencyFor( const media_destination &for_whom,
										   bigtime_t *out_latency,
										   media_node_id *out_timesource );

		virtual	status_t 	Connected( const media_source &producer,
									   const media_destination &where,
									   const media_format &with_format,
									   media_input *out_input );

		virtual	void		Disconnected( const media_source &producer,
		                                  const media_destination &where );

		virtual	status_t	FormatChanged( const media_source &producer,
										   const media_destination &consumer, 
										   int32 change_tag,
										   const media_format &format );

	/* state */
	private:
		void				HandleStart( bigtime_t performance_time );
		void				HandleStop();
		void				HandleTimeWarp( bigtime_t performance_time );
		void				HandleSeek( bigtime_t performance_time );
		void				HandleBuffer( BBuffer *buffer );

		static int32		_run_thread_( void *data );
		int32				RunThread();

		status_t			mInitStatus;

		BMediaAddOn			*mAddOn;
		int32				mId;

		char				device_name[32];

		BBufferGroup		*mBuffers;
		thread_id			mThread;

		/* state variables */
		volatile bool		mRunning;
		volatile bool		mConnected;

		volatile bigtime_t	mPerformanceTimeBase;
		volatile bigtime_t	mProcessingLatency;

		media_input			mInput;
		media_destination	mDest;

		size_t				mBuffer_size;

		volatile audio_buffer_header *io_buf1;
		volatile audio_buffer_header *io_buf2;
		volatile void		*io_buf;

		int					fd; 			//file descriptor for hw driver

		sem_id				mBuffer_avail;
		sem_id				mBuffer_free;

		sem_id				mBuffer_waitIO;	//IO completion semaphore
};

#endif
