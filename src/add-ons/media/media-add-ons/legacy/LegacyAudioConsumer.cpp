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

#include "driver_io.h"
#include "LegacyAudioConsumer.h"


LegacyAudioConsumer::LegacyAudioConsumer( BMediaAddOn *addon, const char *name, int32 internal_id )
	: BMediaNode( name ), BBufferConsumer( B_MEDIA_RAW_AUDIO )
{
	mInitStatus = B_NO_INIT;

	mAddOn = addon;

	mId = internal_id;

	mBuffers = NULL;

	mThread = -1;

	mProcessingLatency = 0LL;

	mRunning	= false;
	mConnected	= false;

	io_buf1 = NULL;

	AddNodeKind( B_PHYSICAL_OUTPUT );

	sprintf( device_name, "/dev/audio/old/%s", name );

	//open the device driver for output
	fd = open( device_name, O_WRONLY );

	if ( fd == 0 ) {
		return;
	}

	mBuffer_size = 4096;

	io_buf1 = static_cast<audio_buffer_header *>(
				malloc( 2 * ( sizeof( audio_buffer_header ) + mBuffer_size ))
				);

	if ( io_buf1 == NULL ) {
		close( fd );
		return;
	}

	io_buf2 = reinterpret_cast<audio_buffer_header *>(
				reinterpret_cast<char *>(const_cast<audio_buffer_header *>( io_buf1 ))
				+ sizeof( audio_buffer_header )
				+ mBuffer_size);

	io_buf1->reserved_1 = sizeof( audio_buffer_header ) + mBuffer_size;
	io_buf2->reserved_1 = sizeof( audio_buffer_header ) + mBuffer_size;

	mInitStatus = B_OK;
	return;
}


LegacyAudioConsumer::~LegacyAudioConsumer()
{
	if ( mInitStatus != B_OK ) {
		return;
	}

	/* Clean up */
	if ( mConnected ) {
		Disconnected( mInput.source, mInput.destination );
	}

	if ( mRunning ) {
		HandleStop();
	}

	if ( io_buf1 != NULL ) {
		free( static_cast<void *>(const_cast<audio_buffer_header *>(io_buf1)) );
	}

	if ( fd != 0 ) {
		close( fd );
	}
}


/* BMediaNode */

BMediaAddOn *
LegacyAudioConsumer::AddOn( int32 *internal_id ) const
{
	if ( internal_id ) {
		*internal_id = mId;
	}

	return mAddOn;
}


void
LegacyAudioConsumer::NodeRegistered()
{
	mInput.destination.port = ControlPort();
	mInput.destination.id   = 0;

	mInput.source = media_source::null;

	mInput.format.type = B_MEDIA_RAW_AUDIO;
	mInput.format.u.raw_audio.frame_rate    = 44100.0;
	mInput.format.u.raw_audio.format        = media_raw_audio_format::B_AUDIO_SHORT;
	mInput.format.u.raw_audio.channel_count = 2;
	mInput.format.u.raw_audio.byte_order    = B_MEDIA_HOST_ENDIAN;
	mInput.format.u.raw_audio.buffer_size   = mBuffer_size;

	Run();
}


status_t 
LegacyAudioConsumer::HandleMessage( int32 message, const void *data, size_t size )
{
	status_t err = B_OK;

	if ( BBufferConsumer::HandleMessage( message, data, size ) != B_OK ) {
		if ( ( err = BMediaNode::HandleMessage( message, data, size ) ) != B_OK ) {
			BMediaNode::HandleBadMessage( message, data, size );
		}
	}

	return err;
}


/* BBufferConsumer */

status_t
LegacyAudioConsumer::AcceptFormat( const media_destination &dest, media_format *format )
{
	if ( format->type != B_MEDIA_RAW_AUDIO ) {
		return B_MEDIA_BAD_FORMAT;
	}

	format->u.raw_audio.frame_rate    = 44100.0;
	format->u.raw_audio.format        = media_raw_audio_format::B_AUDIO_SHORT;
	format->u.raw_audio.channel_count = 2;
	format->u.raw_audio.byte_order    = B_MEDIA_HOST_ENDIAN;
	format->u.raw_audio.buffer_size   = mBuffer_size;

	return B_OK;
}


status_t
LegacyAudioConsumer::GetNextInput( int32 *cookie, media_input *out_input )
{
	if ( *cookie == 0 ) {
		memcpy( out_input, &mInput , sizeof( media_input ) );
	}

	return B_BAD_INDEX;
}


void
LegacyAudioConsumer::BufferReceived( BBuffer *buffer )
{
	if ( ! mRunning ) {

		buffer->Recycle();

	} else {

		media_timed_event event( buffer->Header()->start_time,
		                         BTimedEventQueue::B_HANDLE_BUFFER,
		                         buffer,
		                         BTimedEventQueue::B_RECYCLE_BUFFER );

		EventQueue()->AddEvent( event );

	}
}


void
LegacyAudioConsumer::ProducerDataStatus( const media_destination &for_whom, int32 status,
										 bigtime_t at_media_time )
{
}


status_t
LegacyAudioConsumer::GetLatencyFor( const media_destination &for_whom, bigtime_t *out_latency,
									media_node_id *out_timesource )
{
	*out_latency = mBuffer_size * 10000 / 4 / 441;
	return B_OK;
}


status_t
LegacyAudioConsumer::Connected( const media_source &producer, const media_destination &where,
							    const media_format &with_format, media_input *out_input )
{
	mInput.source = producer;
	//mInput.format = with_format;
	mInput.node   = Node();
	strcpy( mInput.name, "Legacy Audio Consumer" );

	mBuffers = new BBufferGroup;

	uint32 dummy_data = 0;
	int32  change_tag = 1;

	BBufferConsumer::SetOutputBuffersFor( producer, mDest, mBuffers, static_cast<void *>( &dummy_data ),
										  &change_tag, true );

	mConnected = true;
	return B_OK;
}


void
LegacyAudioConsumer::Disconnected( const media_source &producer, const media_destination &where )
{
	if ( mConnected ) {
		delete mBuffers;
		mInput.source = media_source::null;
		mConnected = false;
	}
}

status_t
LegacyAudioConsumer::FormatChanged( const media_source &producer, const media_destination &consumer, 
									int32 change_tag, const media_format &format )
{
	return B_OK;
}


/* BMediaEventLooper */

void 
LegacyAudioConsumer::HandleEvent( const media_timed_event *event, bigtime_t lateness, bool realTimeEvent )
{
	switch(event->type) {
		case BTimedEventQueue::B_START:
			HandleStart( event->event_time );
			break;

		case BTimedEventQueue::B_STOP:
			HandleStop();
			break;

		case BTimedEventQueue::B_WARP:
			HandleTimeWarp( event->bigdata );
			break;

		case BTimedEventQueue::B_SEEK:
			HandleSeek( event->bigdata );
			break;

		case BTimedEventQueue::B_HANDLE_BUFFER:
			HandleBuffer( static_cast<BBuffer *>( event->pointer ) );
			break;

		case BTimedEventQueue::B_DATA_STATUS:
		case BTimedEventQueue::B_PARAMETER:
		default:
			break; //HandleEvent: Unhandled event
	}
}


/* LegacyAudioConsumer */

void
LegacyAudioConsumer::HandleStart( bigtime_t performance_time )
{
	if ( mRunning ) {
		return;
	}

	mPerformanceTimeBase = performance_time;

	//allocate buffer available semaphore
	mBuffer_avail = create_sem( 0, "legacy audio out buffer avail" );

	if ( mBuffer_avail < B_OK ) {
		goto init_err1;
	}

	//allocate buffer free semaphore
	mBuffer_free = create_sem( 1, "legacy audio out buffer free" );
	
	if ( mBuffer_free < B_OK ) {
		goto init_err2;
	}

	//allocate output completion semaphore
	mBuffer_waitIO = create_sem( 1, "legacy audio out waitIO" );

	if ( mBuffer_waitIO < B_OK ) {
		goto init_err3;
	}

	//tell the driver about the playback completion semaphore
	DRIVER_SET_PLAYBACK_COMPLETION_SEM( &mBuffer_waitIO, 0 );

	mThread = spawn_thread( _run_thread_, "legacy audio output", B_REAL_TIME_PRIORITY, this );

	if ( mThread < B_OK ) {
		goto init_err4;
	}

	io_buf = io_buf1;

	resume_thread( mThread );

	mRunning = true;
	return;

init_err4:
	delete_sem( mBuffer_waitIO );

init_err3:
	delete_sem( mBuffer_free );

init_err2:
	delete_sem( mBuffer_avail );

init_err1:
	return;
}


void
LegacyAudioConsumer::HandleStop()
{
	if ( ! mRunning ) {
		return;
	}

	delete_sem( mBuffer_avail );
	delete_sem( mBuffer_free );
	delete_sem( mBuffer_waitIO );
	wait_for_thread( mThread, &mThread );

	mRunning = false;
}


void
LegacyAudioConsumer::HandleTimeWarp( bigtime_t performance_time )
{
	mPerformanceTimeBase = performance_time;
}


void
LegacyAudioConsumer::HandleSeek( bigtime_t performance_time )
{
	mPerformanceTimeBase = performance_time;
}


void
LegacyAudioConsumer::HandleBuffer( BBuffer *buffer )
{
	audio_buffer_header *buf;

	//wait for free buffer
	acquire_sem( mBuffer_free );

	//avoid buffer currently in use
	buf = const_cast<audio_buffer_header *>( ( io_buf == io_buf2 ) ? io_buf1 : io_buf2 );

	//prepare buffer
	memcpy( reinterpret_cast<char *>( buf ) + sizeof( audio_buffer_header ), buffer->Data(), buffer->SizeUsed() );

	//tell the io thread a new buffer is ready
	release_sem( mBuffer_avail );
}


int32
LegacyAudioConsumer::_run_thread_( void *data )
{
	return static_cast<LegacyAudioConsumer *>( data )->RunThread();
}


int32
LegacyAudioConsumer::RunThread()
{
	while ( 1 ) {
	
		//wait for next buffer
		if ( acquire_sem( mBuffer_avail ) == B_BAD_SEM_ID ) {
			return B_OK;
		}

		//send buffer
		DRIVER_WRITE_BUFFER( io_buf, 0 );

		//wait for IO
		if ( acquire_sem( mBuffer_waitIO ) == B_BAD_SEM_ID ) {
			return B_OK;
		}
		
		io_buf = ( io_buf == io_buf2 ) ? io_buf1 : io_buf2;

		//mark buffer free
		release_sem( mBuffer_free );
	}

	return B_OK;
}
